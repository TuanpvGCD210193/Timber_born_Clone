# 🦫 TIMBERBORN CLONE (UE 5.6 C++) - MASTER ARCHITECTURE & SPECIFICATION PLAN v3.3

> **Mục tiêu Dự án**: Xây dựng phiên bản Prototype hoàn chỉnh của game mô phỏng xây dựng thành phố hải ly **Timberborn** trên nền tảng **Unreal Engine 5.6** hoàn toàn bằng **C++**, tuân thủ cấu trúc chuẩn **Public / Private**, toàn bộ thông số gameplay đều được thiết kế bằng `UPROPERTY` linh hoạt, không hardcode, hỗ trợ đầy đủ **Phân tầng Địa hình Tự nhiên (Cỏ, Đất, Đồi Đá)** và **Cụm Rừng Hữu cơ (Organic Forest Clusters)** bằng **Instanced Static Mesh (ISM)**.

---

## 🎛️ 1. CẨM NANG THIẾT LẬP & GIẢI THÍCH THÔNG SỐ TRONG UNREAL EDITOR

### A. Nhóm **Rendering (Hiển thị & Cấu hình Mesh ISM)**
* **`Block Mesh Configs` (`TMap<ETimberBlockType, FTimberBlockMeshConfig>`)**: 
  - Bảng ánh xạ linh hoạt giữa *Loại khối C++* và *Mô hình 3D (Mesh)*.
  - **`Key`**: Loại khối (`Dirt`, `Grass`, `Cliff`, `TreeMature`, `TreeSapling`, `DirtPath`, v.v.).
  - **`Static Mesh`**: Mesh 3D hình học dùng để vẽ (Cube, Cylinder, Mesh cây).
  - **`Override Material`**: Vật liệu ghi đè màu sắc/texture (để trống sẽ dùng Material gốc của Mesh).
  - **`Mesh Scale`**: Tỷ lệ phóng to/thu nhỏ (`1.0, 1.0, 1.0` tương ứng $100 \times 100 \times 100$ cm = 1 mét khối).
  - **`Mesh Offset`**: Độ lệch tọa độ nếu Pivot tâm của Mesh không nằm chính giữa đáy.
* **`Block ISMMap`**: Danh sách `UInstancedStaticMeshComponent` do C++ tự động tạo ngầm bên dưới (chỉ đọc, tối ưu hóa 1 Draw Call).

### B. Nhóm **Generation (Thông Số Sinh Địa Hình & Cụm Rừng)**
* **`Terrain Config` (Sinh Đồi Núi & Địa Hình Phân Tầng)**:
  * **`Base Height` (= 1)**: Độ cao mặt đất phẳng cơ bản (1 tầng block).
  * **`Hill Count` (= 2)**: Số lượng ngọn đồi/cao nguyên đá nhô lên khỏi mặt đất.
  * **`Hill Radius` (= 5)**: Bán kính mở rộng của mỗi ngọn đồi (theo số ô lưới).
  * **`Hill Height` (= 2)**: Chiều cao cộng thêm của mỗi ngọn đồi (số tầng block nhô cao).
* **`Forest Config` (Sinh Cụm Rừng Hữu Cơ)**:
  * **`Cluster Count` (= 3)**: Số lượng cụm rừng xuất hiện trên bản đồ.
  * **`Cluster Radius` (= 4)**: Bán kính lan tỏa của mỗi cụm rừng.
  * **`Center Density` (= 0.85)**: Mật độ phủ cây tại tâm cụm rừng ($85\%$).
  * **`Edge Density` (= 0.20)**: Mật độ phủ cây thưa dần khi ra đến rìa mép rừng ($20\%$).

### C. Hướng Dẫn Thao Tác Gán 4 Khối Chuẩn (`Dirt`, `Grass`, `Cliff`, `TreeMature`):
1. Tại **Block Mesh Configs**, bấm nút `(+)` để tạo đủ 4 phần tử.
2. Chọn `Key` lần lượt là: **`Dirt`**, **`Grass`**, **`Cliff`**, **`TreeMature`**.
3. Gán `Static Mesh`: Chọn khối `Cube` (hoặc `1M_Cube`) cho Dirt, Grass, Cliff; chọn `Cylinder`/`Cone` (hoặc Mesh Cây) cho TreeMature.
4. Gán `Override Material`:
   - `Dirt`: Material màu nâu/đất.
   - `Grass`: Material màu xanh lá cây.
   - `Cliff`: Material màu xám đá.
   - `TreeMature`: Material màu gỗ/lá cây.
5. Nhấn nút **`GenerateNaturalTerrainAndForests`** trong **Timber | Grid Actions** để chiêm ngưỡng bản đồ tức thì trong Viewport!

---

## 🌍 2. BẢNG THÔNG SỐ & CƠ CHẾ ĐỊA HÌNH & TÀI NGUYÊN (TERRAIN & RESOURCES)

### 🌍 1. Phân Tầng Địa Hình Tự Nhiên (Natural Terrain Layering)
- **Quy tắc phân tầng**:
  - **Lớp Mặt Trên (Top Layer)**: Ô trên cùng tiếp xúc trực tiếp với không khí là **Cỏ (`Grass`)**.
  - **Lớp Đất Dưới Sâu (Subsurface Layer)**: Toàn bộ các khối nằm dưới mặt cỏ là **Đất (`Dirt`)**.
  - **Cao Nguyên & Đồi Núi (Hills & Plateaus)**: Các ngọn đồi/vách núi cao 1–3 tầng nhô lên khỏi mặt đất phẳng. Các vách đứng là **Đá (`Cliff/Rock`)**, mặt phẳng trên đỉnh đồi được phủ **Cỏ (`Grass`)**.
- **Tính chất Đi lại (`bIsWalkable`)**:
  - Các ô bề mặt `Grass` là có thể đi lại (`bIsWalkable = true`).
  - Hải ly có thể bước lên/xuống độ cao chênh lệch tối đa 1 block ($100$ cm).
  - Vách đá dựng đứng cao $> 1$ block sẽ chặn di chuyển cho đến khi xây cầu thang/bậc dốc.

---

### 🌲 2. Cụm Rừng Hữu Cơ (Organic Forest Clusters) & Tái Sinh Cây
- **Phân Bổ Tự Nhiên (Organic Density)**:
  - Cây **chỉ mọc trên bề mặt Cỏ (`Grass`)**, tuyệt đối không mọc trên Vách Đá (`Cliff`) hay Nước (`Water`).
  - Rừng sinh theo từng cụm: Mật độ dày đặc ở tâm cụm và thưa dần khi tiến ra rìa mép rừng (Gaussian / Radial Falloff).
- **Vòng Đời Cây (Tree Lifecycle)**:
  1. `Stump / Empty`: Gốc cây sau khi đốn (Đếm ngược thời gian hồi).
  2. `Sapling`: Cây non mọc lại.
  3. `Growing`: Kích thước scale lớn dần theo thời gian.
  4. `Mature Tree`: Cây trưởng thành cho gỗ.

---

### 🏛️ 3. Hệ Thống 3 Công Trình & Cơ Chế Móng Xây Dựng (Construction Site)
- **Quy tắc Đặt móng**: Chỉ được đặt trên mặt đất phẳng và trống (không có cây, không có vách đá dốc).
- **Cơ chế Móng (Construction Site)**:
  - Khi click đặt công trình: Hiện Hologram (Xanh: Hợp lệ / Đỏ: Bị vướng/Không phẳng) $\rightarrow$ Tạo móng công trình $\rightarrow$ Hải ly vác Gỗ tới thi công hoàn thiện.
- **Cấu hình Linh hoạt (`UPROPERTY`)**:
  - `WoodCost`, `BuildTimeSeconds`, `WorkerSlots`, `StartingWood` cấu hình trực tiếp trong Blueprint / Editor.

| Công trình | Kích thước Grid | Chi phí Gỗ (Mặc định) | Cơ chế hoạt động & Kết nối Đường đi |
| :--- | :--- | :--- | :--- |
| **1. Nhà Chính (District Center)** | $2 \times 2$ | $0$ (Đặt sẵn ban đầu) | Cấp sẵn Gỗ khởi đầu (`StartingWood`); sinh 5-10 Hải ly; là gốc phát tỏa mạng lưới Đường đi. |
| **2. Kho Lưu Trữ (Storage)** | $2 \times 2$ | Cấu hình trong BP (vd: 10 Gỗ) | Chứa Gỗ khai thác (`MaxCapacity` cấu hình được). Tiếp giáp bất kỳ ô đường nào quanh chân. |
| **3. Flag Đốn Gỗ (Lumberjack Flag)** | $1 \times 1$ | Cấu hình trong BP (vd: 3 Gỗ) | 1 slot công nhân quét cây trong bán kính (`WorkRadius`). Tiếp giáp đường đi. |
| **4. Đường Đi (Dirt Path)** | $1 \times 1$ | Cấu hình trong BP (vd: 0 hoặc 1 Gỗ) | Lát trên mặt đất, kết nối công trình về Nhà chính. |

---

### 🐾 4. Hệ Thống Hải Ly AI & A* Pathfinding
- **Hình thể & Va chạm**: Mesh đơn giản (Low-poly/Capsule). **Hải ly đi xuyên qua nhau (Overlap)** trên đường đi để tránh kẹt đường.
- **Hành vi (Behavior Loop)**:
  - `Idle`: Đứng nghỉ hoặc chờ việc khi hết cây xung quanh (tự động tiếp tục khi cây non lớn lên).
  - `Builder`: Vác gỗ từ kho đến công trình đang xây và gõ búa hoàn thiện.
  - `Lumberjack`: Đi từ Flag $\rightarrow$ Chặt cây trưởng thành $\rightarrow$ Vác gỗ về Kho lưu trữ.
- **Tìm đường**: Thuật toán **A* 3D Grid** chạy trên nền ô lưới.

---

### 🎮 5. Điều Khiển Camera (RTS Controls), Tốc Độ Game & UI
- **Camera Controls**: `WASD` di chuyển, `Q/E` hoặc chuột giữa xoay, cuộn chuột để zoom.
- **Tốc Độ Game**: `Space` (Pause/Resume), `1, 2, 3` (Tốc độ 1x, 2x, 3x).
- **Giao Diện HUD**: Thanh hiển thị Gỗ & Hải ly ở đỉnh màn hình, menu 4 nút chọn xây dựng ở đáy màn hình.

---

## 📂 3. CẤU TRÚC THƯ MỤC C++ CHUẨN (PUBLIC / PRIVATE)

```text
d:\UE Project\Timber_born_Clone\
│
├── 📁 Source\
│   └── 📁 Timber_born_Clone\
│       ├── 📁 Public\
│       │   ├── 📁 Grid\           (TimberGridTypes.h, TimberGridManager.h)
│       │   ├── 📁 Environment\    (ForestGenerator.h, TreeLifecycleManager.h)
│       │   ├── 📁 Pathfinding\    (TimberAStar.h, TimberPathGraph.h)
│       │   ├── 📁 Buildings\      (TimberBuildingBase.h, TimberDistrictCenter.h, TimberStorage.h, TimberLumberjackFlag.h)
│       │   ├── 📁 Beavers\        (BeaverAgent.h, BeaverTypes.h)
│       │   ├── 📁 Player\         (TimberPlayerController.h, TimberRTSCamera.h)
│       │   └── 📁 UI\             (TimberHUDWidget.h)
│       │
│       └── 📁 Private\
│           ├── 📁 Grid\           (TimberGridManager.cpp)
│           ├── 📁 Environment\    (ForestGenerator.cpp, TreeLifecycleManager.cpp)
│           ├── 📁 Pathfinding\    (TimberAStar.cpp, TimberPathGraph.cpp)
│           ├── 📁 Buildings\      (TimberBuildingBase.cpp, TimberDistrictCenter.cpp, TimberStorage.cpp, TimberLumberjackFlag.cpp)
│           ├── 📁 Beavers\        (BeaverAgent.cpp)
│           ├── 📁 Player\         (TimberPlayerController.cpp, TimberRTSCamera.cpp)
│           └── 📁 UI\             (TimberHUDWidget.cpp)
│
└── 📁 Content\
    ├── 📁 Blueprints\
    ├── 📁 Meshes\
    ├── 📁 Materials\
    ├── 📁 Maps\
    └── 📁 UI\
```

---

## 📋 4. LỘ TRÌNH TRIỂN KHAI MICRO-STEPS

- **Phase 1: Địa hình Grid, Rừng cây & Công cụ In-Editor**
  - `Step 1.1`: [XONG] Khởi tạo Cấu trúc thư mục Public/Private và Content.
  - `Step 1.2`: [XONG] Core Grid Data (`FTimberCell`, `ETimberBlockType`, `ATimberGridManager`).
  - `Step 1.3`: [XONG] Dynamic ISM Rendering cho Đất/Cỏ/Đá & Thuật toán Sinh Địa hình Phân tầng + Cụm Rừng Hữu cơ.
    - `Feat 1.3.1`: [ĐANG THỰC HIỆN] Thanh Tiến Độ Unreal Editor (`FScopedSlowTask`) & Visual Progress Log (Cập nhật tiến trình từng 20%).
  - `Step 1.4`: Quản lý Vòng đời Sinh trưởng của Cây (Sapling $\rightarrow$ Mature).
  - `Step 1.5`: Công cụ In-Editor (Click chuột vẽ / sửa địa hình trực tiếp trong Viewport).
- **Phase 2: Mạng Lưới Đường Đi & Thuật Toán A* Pathfinding**
- **Phase 3: Hệ Thống 3 Công Trình & Cơ Chế Móng Xây Dựng (Construction Site)**
- **Phase 4: Hải Ly AI & Vòng Lặp Khai Thác Gỗ**
- **Phase 5: Camera RTS, Game Speed & UI HUD**
