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
* **`Terrain Config` (`FTerrainGenConfig`)**:
  * **`Map Seed` (= 0)**: Mã hạt giống ngẫu nhiên (`0`: Sinh map ngẫu nhiên mới mỗi lần bấm; `> 0`, ví dụ `1337`: Tái tạo chính xác $100\%$ thế bản đồ cố định).
  * **`Base Height` (= 2)**: Độ cao mặt đất phẳng cơ bản (số tầng block nền).
  * **`Plateau Count` (= 2)**: Số lượng vùng cao nguyên đồi núi chính xuất hiện trên bản đồ.
  * **`Plateau Radius` (= 8)**: Bán kính mở rộng của mỗi ngọn đồi cao nguyên (theo số ô lưới).
  * **`Max Tiers` (= 3)**: Số bậc thang cao nguyên nhấp nhô xếp chồng lên nhau ($1 - 10$ tầng).
  * **`Tier Height` (= 2)**: Chiều cao của mỗi bậc thang (số block dựng đứng).
  * **`Cliff Jaggedness` (= 0.45)**: Độ lồi lõm so le răng cưa phong hóa tự nhiên của các cột vách đá (Hình 2).
  * **`Noise Scale` (= 0.12)**: Tần số uốn lượn hữu cơ của ranh giới giữa Cỏ, Đất và Đá (Hình 5).
  * **`Grass Ratio` (= 0.55)**: Tỷ lệ phủ Cỏ xanh tươi tốt màu mỡ trên bề mặt ($55\%$).
  * **`Rock Band Width` (= 0.08)**: Độ dày của dải Đá đệm tự nhiên uốn lượn nằm giữa Cỏ và Đất ($8\%$).
  * **`bHillTopAlwaysRock` (= true)**: Đỉnh cao nhất của cao nguyên luôn được phủ khối Đá xám (`Cliff`).
  * **`Max Climbable Height` (= 1)**: Độ chênh lệch chiều cao tối đa cho phép Hải ly leo trèo ($1$ block đi lại được, $\ge 2$ block bị chặn).
* **`Forest Config` (`FForestClusterConfig`)**:
  * **`Cluster Count` (= 3)**: Số lượng cụm rừng xuất hiện trên bản đồ.
  * **`Cluster Radius` (= 4)**: Bán kính lan tỏa của mỗi cụm rừng.
  * **`Center Density` (= 0.85)**: Mật độ phủ cây tại tâm cụm rừng ($85\%$).
  * **`Edge Density` (= 0.20)**: Mật độ phủ cây thưa dần khi ra đến rìa mép rừng ($20\%$).
  * **`Grass Tree Density` (= 0.85)**: Tỷ lệ phân cấp mọc cây dày đặc trên nền Cỏ xanh.
  * **`Dirt Tree Density` (= 0.25)**: Tỷ lệ phân cấp mọc cây thưa thớt trên nền Đất khô.
  * **`Rock Tree Density` (= 0.00)**: Tuyệt đối $0\%$ cây mọc trên nền Đá / Vách đá.

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

### 🌍 1. Phân Tầng & Phối Hợp Địa Hình Hữu Cơ Đa Khối (Organic Multi-Block Surface on Same Height)
- **Quy tắc Bề mặt trên cùng một tầng độ cao ($Z$)**:
  - Trên cùng một mặt phẳng độ cao, bề mặt được phối hợp tự nhiên giữa **3 loại khối**:
    1. **`Grass` (Cỏ xanh)**: Đại diện cho vùng đất màu mỡ, tươi tốt.
    2. **`Dirt` (Đất khô)**: Đại diện cho vùng đất khô cằn, hoang mạc.
    3. **`Cliff` (Đá)**: Xuất hiện ở các **vách đứng cao nguyên** và dải đệm tự nhiên uốn lượn nằm giữa Cỏ và Đất.
  - **Mối nối chuyển tiếp hữu cơ (Organic Noise Transitions)**: Ranh giới giữa Cỏ, Đất và Đá uốn lượn răng cưa tự nhiên theo hàm nhiễu hữu cơ (Perlin Noise), loại bỏ hoàn toàn các khối vuông vức $5 \times 5$ đơn điệu.
  - **Lớp Đất Dưới Sâu (Subsurface Layer)**: Toàn bộ phần thân đồi bên trong và tầng đáy sâu là khối `Dirt` hoặc `Cliff`.
- **Tính chất Đi lại (`bIsWalkable`)**:
  - Các ô bề mặt `Grass` và `Dirt` là có thể đi lại (`bIsWalkable = true`).
  - Vách đá dựng đứng cao $> 1$ block sẽ chặn di chuyển (`bIsWalkable = false`).

---

### 🌲 2. Cụm Rừng Hữu Cơ (Organic Forest Clusters) & Tái Sinh Cây
- **Phân Bổ Tự Nhiên**:
  - Cây **chỉ mọc trên bề mặt Cỏ (`Grass`)**, tuyệt đối **KHÔNG mọc trên Vách Đá (`Cliff`)**.
  - Rừng sinh theo cụm với mật độ giảm dần từ tâm ra rìa (Radial Falloff).
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

- **Phase 1: Địa hình Grid, Rừng cây & Khối Cơ Bản [HOÀN THÀNH 100%]**
  - `Step 1.1`: [XONG] Khởi tạo Cấu trúc thư mục Public/Private và Content.
  - `Step 1.2`: [XONG] Core Grid Data (`FTimberCell`, `ETimberBlockType`, `ATimberGridManager`).
  - `Step 1.3`: Refactor Thuật Toán Sinh Địa Hình Đa Khối Hữu Cơ & Persistence:
    - `Step 1.3.1`: [XONG] Thuật toán Cao nguyên 2-3 Tầng Xếp Chồng & Vách Đá Lồi Lõm So Le + Map Seed.
    - `Step 1.3.2`: [XONG] Phân bổ Bề Mặt Hữu Cơ (Cỏ/Đất/Đá) + Đỉnh Đồi Là Khối Đá + Walkable (chênh 1 block).
    - `Step 1.3.3`: [XONG] Thuật toán Sinh Cây Phân Cấp Mật Độ (Dày trên Cỏ, Thưa trên Đất, 0% trên Đá).
    - `Step 1.3.4`: [XONG] Tính năng Lưu Bản Đồ (Save/Load Terrain Persistence) giữ nguyên địa hình khi Rebuild.
  - `Step 1.4`: [XONG] Quản lý Vòng đời Sinh trưởng của Cây (Stump $\rightarrow$ Sapling $\rightarrow$ Mature).
  - `Phase 1 Retrospective`: [XONG] Tổng kết mã nguồn, Review Kiến trúc & Đánh giá năng lực Phase 1.
- **Phase 2: Mạng Lưới Đường Đi & Thuật Toán A* Pathfinding [TIẾP THEO]**
  - `Step 2.1`: Cấu trúc Dữ liệu Đường Đi & Path Network Graph.
  - `Step 2.2`: Thuật toán A* Pathfinding 3D (Hỗ trợ chênh lệch độ cao $\le 1$ block).
  - `Step 2.3`: Hệ thống Kết Nối Đường Đi & Phạm Vi Tác Vụ (Work Range Overlay).
- **Phase 3: Hệ Thống 3 Công Trình & Cơ Chế Móng Xây Dựng (Construction Site)**
  - `Step 3.1`: Base Building Actor & Construction Site System.
  - `Step 3.2`: Triển khai 3 Công Trình Cốt Lõi (Nhà Kho District Center, Trại Đốn Gỗ Lumberjack, Nhà Dân Beaver Lodge).
  - `Step 3.3`: Hệ Thống Cung Ứng Vật Liệu Xây Dựng (Hauling & Build Progress).
  - `Step 3.4`: Công cụ In-Editor Brush (Click chuột vẽ / sửa địa hình & đặt công trình trực tiếp trong Viewport).
- **Phase 4: Hải Ly AI & Vòng Lặp Khai Thác Gỗ**
- **Phase 5: Camera RTS, Game Speed & UI HUD**
