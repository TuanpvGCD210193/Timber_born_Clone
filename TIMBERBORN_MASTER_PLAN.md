# 🦫 TIMBERBORN CLONE (UE 5.6 C++) - MASTER ARCHITECTURE & SPECIFICATION PLAN v3.5

> **Mục tiêu Dự án**: Xây dựng phiên bản Prototype hoàn chỉnh của game mô phỏng xây dựng thành phố hải ly **Timberborn** trên nền tảng **Unreal Engine 5.6** hoàn toàn bằng **C++**, tuân thủ cấu trúc chuẩn **Public / Private**, toàn bộ thông số gameplay đều được thiết kế bằng `UPROPERTY` linh hoạt, không hardcode, hỗ trợ đầy đủ **Phân tầng Địa hình Tự nhiên (Cỏ, Đất, Đồi Đá)**, **Cụm Rừng Hữu cơ (Organic Forest Clusters)**, **Mạng Lưới Đường Đi & A* Pathfinding 3D** bằng **Instanced Static Mesh (ISM)**.

---

## 🎛️ 1. CẨM NANG THIẾT LẬP & GIẢI THÍCH THÔNG SỐ TRONG UNREAL EDITOR

### A. Nhóm **Rendering (Hiển thị & Cấu hình Mesh ISM)**
* **`Block Mesh Configs` (`TMap<ETimberBlockType, FTimberBlockMeshConfig>`)**: 
  - Bảng ánh xạ linh hoạt giữa *Loại khối C++* và *Mô hình 3D (Mesh)*.
  - **`Key`**: Loại khối (`Dirt`, `Grass`, `Cliff`, `TreeMature`, `TreeSapling`, `DirtPath`, v.v.).
  - **`Static Mesh`**: Mesh 3D hình học dùng để vẽ (Cube, Cylinder, Mesh cây).
  - **`Override Material`**: Vật liệu ghi đè màu sắc/texture (để trống sẽ dùng material mặc định của mesh).
  - **`Mesh Scale`**: Tỷ lệ phóng to/thu nhỏ (`1.0, 1.0, 1.0` tương ứng $100 \times 100 \times 100$ cm = 1 mét khối; `DirtPath`: `1.0, 1.0, 0.09`).
  - **`Mesh Offset`**: Độ lệch tọa độ nếu Pivot tâm của Mesh không nằm chính giữa đáy.
* **`Block ISMMap`**: Danh sách `UInstancedStaticMeshComponent` do C++ tự động tạo ngầm bên dưới (chỉ đọc, tối ưu hóa 1 Draw Call).

### B. Nhóm **Generation (Thông Số Sinh Địa Hình & Cụm Rừng)**
* **`Terrain Config` (`FTerrainGenConfig`)**:
  * **`Map Seed` (= 0)**: Mã hạt giống ngẫu nhiên (`0`: Sinh map ngẫu nhiên mới mỗi lần bấm; `> 0`, ví dụ `1337`: Tái tạo chính xác $100\%$ thế bản đồ cố định).
  * **`Base Height` (= 2)**: Độ cao mặt đất phẳng cơ bản (số tầng block nền).
  * **`Plateau Count` (= 2)**: Số lượng vùng cao nguyên đồi núi chính xuất hiện trên bản đồ.
  * **`Plateau Radius` (= 11)**: Bán kính mở rộng của mỗi ngọn đồi cao nguyên (theo số ô lưới).
  * **`Max Tiers` (= 3)**: Số bậc thang cao nguyên nhấp nhô xếp chồng lên nhau ($1 - 10$ tầng).
  * **`Tier Height` (= 2)**: Chiều cao của mỗi bậc thang (số block dựng đứng).
  * **`Cliff Jaggedness` (= 0.45)**: Độ lồi lõm so le răng cưa phong hóa tự nhiên của các cột vách đá.
  * **`Noise Scale` (= 0.045)**: Tần số uốn lượn hữu cơ tạo đại thung lũng Cỏ và Đất hoang rộng lớn liền mạch (triệt tiêu 100% chó đốm).
  * **`Grass Ratio` (= 0.50)**: Tỷ lệ phủ Cỏ xanh tươi tốt màu mỡ trên bề mặt ($50\%$).
  * **`Rock Band Width` (= 0.00)**: Không dùng viền ruy-băng; Khối Đá tự nhiên gán cho toàn bộ vách đứng cao nguyên và đỉnh đồi.
  * **`bHillTopAlwaysRock` (= true)**: Đỉnh cao nhất của cao nguyên luôn được phủ khối Đá xám (`Cliff`).
  * **`Max Climbable Height` (= 1)**: Độ chênh lệch chiều cao tối đa cho phép Hải ly leo trèo ($1$ block đi lại được, $\ge 2$ block bị chặn).
* **`Forest Config` (`FForestClusterConfig`)**:
  * **`Cluster Count` (= 3)**: Số lượng cụm rừng xuất hiện trên bản đồ.
  * **`Cluster Radius` (= 5)**: Bán kính lan tỏa của mỗi cụm rừng.
  * **`Center Density` (= 0.85)**: Mật độ phủ cây tại tâm cụm rừng ($85\%$).
  * **`Edge Density` (= 0.20)**: Mật độ phủ cây thưa dần khi ra đến rìa mép rừng ($20\%$).
  * **`Grass Tree Density` (= 0.90)**: Tỷ lệ phân cấp mọc cây dày đặc trên nền Cỏ xanh.
  * **`Dirt Tree Density` (= 0.25)**: Tỷ lệ phân cấp mọc cây thưa thớt trên nền Đất khô.
  * **`Rock Tree Density` (= 0.00)**: Tuyệt đối $0\%$ cây mọc trên nền Đá / Vách đá.

### C. Nhóm **District & Path Network (Đường Đi & Nhà Chính)**
* **`District Center Coord`**: Tọa độ ô tâm của Nhà Chính (mặc định: `X=10, Y=10, Z=2`).
* **`District Center Door Coord`**: Tọa độ ô cửa ra vào của Nhà Chính (mặc định: `X=10, Y=10, Z=2`), là gốc phát tỏa mạng lưới giao thông.
* **`DrawDebugDistrictNetwork`**: Nút bấm CallInEditor để hiển thị dải mạng lưới đường đi phát sáng (Magenta = Cửa, Cyan = Đường, Vàng = Liên kết) trong Viewport.

---

## 🌍 2. BẢNG THÔNG SỐ & CƠ CHẾ ĐỊA HÌNH & TÀI NGUYÊN (TERRAIN & RESOURCES)

### 🌍 1. Phân Tầng & Phối Hợp Địa Hình Hữu Cơ Đa Khối
- **Quy tắc Bề mặt**:
  1. `Grass` (Cỏ xanh): Đại diện cho vùng đất màu mỡ, tươi tốt.
  2. `Dirt` (Đất khô): Đại diện cho vùng đất khô cằn, hoang mạc.
  3. `Cliff` (Đá): Xuất hiện ở các **vách đứng cao nguyên** và đỉnh núi đá.
- **Tính chất Đi lại (`bIsWalkable`)**:
  - Các ô bề mặt `Grass`, `Dirt` và `DirtPath` là có thể đi lại.
  - Vách đá dựng đứng cao $> 1$ block sẽ chặn di chuyển (`bIsWalkable = false`).

### 🌲 2. Cụm Rừng Hữu Cơ & Vòng Đời Tự Động Của Cây
- **Vòng Đời Cây (Tree Lifecycle)**:
  1. `Stump / Regrowing`: Gốc cây sau khi đốn (Đếm ngược 10s).
  2. `Sapling`: Cây non mọc lại (Đếm ngược 15s).
  3. `Mature Tree`: Cây trưởng thành cho $+2$ Gỗ khi chặt.

### 🏛️ 3. Hệ Thống 3 Công Trình & Cơ Chế Móng Xây Dựng (Construction Site)
- **Quy tắc Đặt móng**: Chỉ được đặt trên mặt đất phẳng và trống.
- **Cơ chế Móng**: Hologram (Xanh/Đỏ) $\rightarrow$ Móng công trình $\rightarrow$ Hải ly vác Gỗ tới thi công hoàn thiện.

| Công trình | Kích thước Grid | Chi phí Gỗ (Mặc định) | Cơ chế hoạt động & Kết nối Đường đi |
| :--- | :--- | :--- | :--- |
| **1. Nhà Chính (District Center)** | $2 \times 2$ | $0$ (Đặt sẵn ban đầu) | Cấp Gỗ khởi đầu; sinh Hải ly; gốc phát tỏa mạng lưới Đường đi. |
| **2. Kho Lưu Trữ (Storage)** | $2 \times 2$ | 10 Gỗ | Chứa Gỗ khai thác. Tiếp giáp bất kỳ ô đường nào quanh chân. |
| **3. Flag Đốn Gỗ (Lumberjack Flag)** | $1 \times 1$ | 3 Gỗ | 1 slot công nhân quét cây trong bán kính. Bắt buộc nối đường về Nhà Chính. |
| **4. Đường Đi (Dirt Path)** | $1 \times 1$ | 0 Gỗ | Lát tức thì trên mặt đất, tăng $+50\%$ tốc độ di chuyển. |

### 🐾 4. Hệ Thống Hải Ly AI & A* Pathfinding 3D
- **Hình thể & Va chạm**: Overlap (đi xuyên qua nhau) để triệt tiêu kẹt đường.
- **Tìm đường**: Thuật toán A* 3D Grid với trọng số ưu tiên đường đi (Cost 1.0 vs 1.5).

### 🎮 5. Điều Khiển Camera (RTS Controls), Tốc Độ Game & UI
- **Camera Controls**: `WASD` di chuyển, `Q/E` xoay, cuộn chuột zoom.
- **Tốc Độ Game**: `Space` (Pause/Resume), `1, 2, 3` (1x, 2x, 3x).
- **Giao Diện HUD**: Hiển thị Gỗ & Dân số ở đỉnh, Menu xây dựng ở đáy màn hình.

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

## 🏆 4. TỔNG KẾT CHI TIẾT KỸ THUẬT, THUẬT TOÁN & GIẢI THÍCH HỌC THUẬT (ACADEMIC & TECHNICAL RETROSPECTIVE)

Mục này cung cấp tài liệu kỹ thuật chuyên sâu và các nguyên lý Khoa học Máy tính (Computer Science) đằng sau từng hệ thống, giúp các kỹ sư phát triển sau này có thể dễ dàng hiểu được bản chất thiết kế (Why & How).

---

### 🌟 A. TỔNG KẾT PHASE 1: ĐỊA HÌNH GRID 3D, RỪNG CÂY SINH THÁI & KHỐI CƠ BẢN (HOÀN THÀNH 100%)

#### 1. Không Gian Voxel 3D & Biểu Diễn Mảng Tuyến Tính 1D (Spatial Representation & Memory Locality)
* **Nguyên lý Khoa học Máy tính**:
  - Thay vì dùng mảng lồng nhau 3 chiều `Cell[X][Y][Z]` (vốn phân tán dữ liệu trên Heap và gây gián đoạn đường truyền bộ nhớ đệm Cache Miss), hệ thống sử dụng cấu trúc **1D Flat-Array Contiguous Buffer** gồm $102.400$ phần tử struct `FTimberCell`.
  - Công thức ánh xạ không gian Euclid 3D về chỉ số bộ nhớ tuyến tính 1D:
    $$\text{Index}(X, Y, Z) = X + Y \times \text{GridSizeX} + Z \times (\text{GridSizeX} \times \text{GridSizeY})$$
  - **Lợi ích**: Đạt hiệu năng truy xuất tuyệt đối $\mathcal{O}(1)$ với độ trễ xấp xỉ $0\text{ ms}$, tối ưu hóa 100% nguyên lý **Data Locality** trong kiến trúc CPU hiện đại.
* **Triết lý Data-Driven & Zero Hardcoding**:
  - Không tồn tại hằng số tĩnh (Magic Numbers) trong mã C++. Mọi tham số từ kích thước lưới, mật độ rừng, tỷ lệ biome đến thời gian sinh trưởng đều được công khai qua `UPROPERTY(EditAnywhere)` để kết nối trực tiếp với Unreal Editor Details Panel.

#### 2. Tối Ưu Hóa Render GPU Bằng Instanced Static Mesh (Draw Call Optimization)
* **Bản chất Vấn đề**: Nếu mỗi khối voxel $1\text{ m}^3$ là một Actor `AActor` riêng lẻ, một bản đồ 50.000 khối sẽ tạo ra 50.000 Draw Calls gửi tới GPU mỗi khung hình, làm CPU bị nghẽn cổ chai (CPU Bound) và tụt khung hình xuống $< 10\text{ FPS}$.
* **Giải pháp Kỹ thuật**:
  - Gom toàn bộ các khối cùng loại vào một `UInstancedStaticMeshComponent` (ISM).
  - Thuật toán gom cụm mảng biến đổi ma trận không gian **`TArray<FTransform>`** theo đợt (**Batching**) và nạp vào VRAM thông qua hàm `AddInstances(Transforms)` đúng $1$ lần duy nhất.
  - **Kết quả**: Gom toàn bộ 100.000 khối voxel xuống chỉ còn **1-2 Draw Calls**, duy trì tốc độ khung hình vững chắc $120+\text{ FPS}$.

#### 3. Thuật Toán Sinh Địa Hình Tự Nhiên Thủ Tục (Procedural Terrain Generation & Noise Theory)
* **Toán học Perlin Noise & Đại Biome Liền Mạch (Triệt tiêu hiện tượng Chó Đốm)**:
  - Bản chất hàm nhiễu Perlin $N(x, y) \in [0, 1]$ là một hàm liên tục khả vi. Nếu áp dụng tần số cao ($f > 0.1$) trên một lưới nhỏ, hàm sẽ tạo ra các đường đồng mức khép kín đường kính nhỏ trông giống các "chó đốm" lốm đốm.
  - Chúng ta hạ tần số xuống mức thấp $\text{NoiseScale} = 0.045$, phân tách không gian bằng hàm ngưỡng (Thresholding Function):
    $$\text{Biome}(x, y) = \begin{cases} \text{Grass (Cỏ xanh màu mỡ)}, & \text{khi } N(x \cdot \text{scale}, y \cdot \text{scale}) \ge \text{GrassRatio} \\ \text{Dirt (Đất hoang khô cằn)}, & \text{ngược lại} \end{cases}$$
    Tạo ra các đại vùng thung lũng cỏ và sa mạc đất khô rộng lớn uốn lượn tự nhiên.
* **Lượng Tử Hóa Chiều Cao Tạo Cao Nguyên Đa Tầng (Heightmap Quantization)**:
  - Chiều cao địa hình được chia tầng thành các cao nguyên phẳng (Mesas/Plateaus) qua hàm lượng tử:
    $$\text{Tier}(x, y) = \min\left(\text{MaxTiers}, \left\lfloor \frac{H_{\text{raw}}(x, y)}{\text{TierHeight}} \right\rfloor\right)$$
  - Mặt trên của mỗi cao nguyên phẳng $100\%$ để phục vụ quy hoạch xây dựng, trong khi sườn dốc phong hóa răng cưa tự nhiên nhờ nhiễu vi phân `CliffJaggedness`.

#### 4. Máy Trạng Thái Hữu Hạn Vòng Đời Cây (Finite State Machine in Eco-Forest)
* **Mô hình Trạng thái Sinh học**:
  - Không sinh cây bằng Actor để tiết kiệm bộ nhớ; cây tồn tại dưới dạng trường dữ liệu `TreeStage` (`ETreeGrowthStage`) và `TreeGrowthTimer` bên trong `FTimberCell`.
  - Bộ điều phối trong `Tick(DeltaTime)` vận hành như một Máy trạng thái hữu hạn (**FSM**):
    $$\text{None} \xrightarrow{\text{Plant}} \text{Sapling} \xrightarrow{15\text{s}} \text{Mature} \xrightarrow{\text{Chop (+2 Wood)}} \text{Stump} \xrightarrow{10\text{s}} \text{Sapling}$$
  - Phân bổ bám rễ sinh thái: Mật độ xác suất mọc cây phụ thuộc chặt chẽ vào độ ẩm của Biome bên dưới ($90\%$ trên Cỏ, $25\%$ trên Đất, $0\%$ trên Vách đá).

#### 5. Cơ Chế Lưu Trữ & Phục Hồi Bản Đồ Bền Vững (Terrain Persistence Lifecycle)
* **Vòng đời Unreal Engine Serialization**:
  - Lưu trạng thái ô lưới vào `TArray<FTimberCell> SavedGridData` có gắn cờ `UPROPERTY(SaveGame)`. Dữ liệu này được đóng gói nhúng trực tiếp vào file bản đồ `.umap`.
  - **Thứ tự nạp dữ liệu (Lifecycle Order)**: Khi mở Editor hoặc Rebuild C++, `OnConstruction()` kích hoạt $\rightarrow$ Dọn dẹp Component cũ trước $\rightarrow$ Gán `GridCells = SavedGridData` $\rightarrow$ Tái tạo lại toàn bộ ISM Instances trong Viewport tức thì mà không cần bấm lại nút sinh bản đồ.

---

### 🌟 B. TỔNG KẾT PHASE 2: MẠNG LƯỚI ĐƯỜNG ĐI & THUẬT TOÁN A* PATHFINDING 3D (HOÀN THÀNH 100%)

#### 1. Lý Thuyết Đồ Thị Không Gian Lưới Voxel (Voxel Graph Theory & Topological Adjacency)
* **Mô hình Hóa Đồ Thị Vô Hướng (Undirected Graph $G = (V, E)$)**:
  - Tập đỉnh $V$: Mỗi điểm đường `DirtPath` là một nút đồ thị `FTimberPathNode` lưu trong bảng băm $\mathcal{O}(1)$ `TMap<FIntVector, FTimberPathNode>`.
  - Tập cạnh $E$: Liên kết giữa 2 nút lân cận $(u, v)$ theo 4 hướng chính Đông, Tây, Nam, Bắc.
* **Ràng Buộc Chuyển Tiếp Chiều Cao (Vertical Stair Manifold Constraint)**:
  - Do địa hình voxel có chênh lệch độ cao theo từng tầng block, hai ô đường $(X_1, Y_1, Z_1)$ và $(X_2, Y_2, Z_2)$ kề nhau được coi là có cạnh nối $e \in E$ khi và chỉ khi:
    $$|\Delta X| + |\Delta Y| = 1 \quad \text{và} \quad |\Delta Z| = |Z_2 - Z_1| \le 1$$
  - Nếu $|\Delta Z| \ge 2$, cạnh kết nối bị triệt tiêu hoàn toàn (vách đứng chặn di chuyển).

#### 2. Thuật Toán A* Pathfinding 3D Với Hàm Trọng Số Tối Ưu (A* Search & Dynamic Edge Weighting)
* **Bản chất Thuật toán A***:
  - Là thuật toán tìm đường ngắn nhất tối ưu dựa trên hàm đánh giá tổng chi phí:
    $$f(n) = g(n) + h(n)$$
    Trong đó $g(n)$ là chi phí thực tế từ điểm xuất phát tới nút $n$, và $h(n)$ là hàm ước lượng khoảng cách (Heuristic) từ $n$ tới đích.
* **Hàm Heuristic 3D Hợp Lệ (Admissible Heuristic)**:
  - Để thuật toán luôn đảm bảo tìm ra đường ngắn nhất mà không đánh giá thừa (Never overestimate), chúng ta sử dụng khoảng cách Manhattan 2D kết hợp tỷ trọng độ cao Z:
    $$h(n) = (|X_n - X_{\text{target}}| + |Y_n - Y_{\text{target}}|) + 1.5 \times |Z_n - Z_{\text{target}}|$$
* **Hàm Trọng Số Chi Phí Thúc Đẩy Hành Vi AI (AI Road Preference Weighting)**:
  - Để hải ly tự nhiên ưu tiên chạy trên các con đường đất do người chơi xây dựng thay vì băng qua đồi cỏ, hàm chi phí từng bước $C(u, v)$ được thiết kế:
    $$C(u, v) = \begin{cases} 1.0 + 0.3 \times |\Delta Z|, & \text{nếu } v \text{ là ô đường DirtPath} \\ 1.5 + 0.3 \times |\Delta Z|, & \text{nếu } v \text{ là Cỏ/Đất tự nhiên} \end{cases}$$
  - **Kết quả**: Hải ly tự động chọn đi vòng trên đường đất để đạt vận tốc tối đa thay vì đi đường tắt gồ ghề!
* **Cấu Trúc Hàng Đợi Ưu Tiên Min-Heap (`OpenSet.HeapPush/HeapPop`)**:
  - Đạt độ phức tạp thuật toán tối ưu $\mathcal{O}(E \log V)$, xử lý tìm đường trong vài micro-giây mà không gây sụt giảm FPS.

#### 3. Thuật Toán BFS Kiểm Tra Kết Nối Mạng Lưới District (Reachability & Service Topology)
* **Thuật toán Duyệt Theo Chiều Rộng (Breadth-First Search - BFS)**:
  - Trong Timberborn, các công trình chỉ hoạt động khi có đường nối thông suốt về Nhà Chính (District Center).
  - Sử dụng thuật toán BFS khởi phát từ ô cửa của Nhà Chính `DistrictCenterDoorCoord`:
    - **`IsReachable(Start, Target, OutDist)`**: Duyệt đồ thị theo từng lớp sóng lan tỏa với độ phức tạp tuyến tính cực nhanh $\mathcal{O}(V + E)$, trả về trạng thái kết nối và số bước đường đi ngắn nhất.
    - **`GetAllReachableNodes(Root)`**: Thuật toán Flood Fill lấy toàn bộ cây khung giao thông đang hoạt động.

#### 4. Hệ Thống Trực Quan Hóa Dữ Liệu Ngầm Trong Viewport (Debug Visual Feedback Loop)
* **Tầm quan trọng của Visual Debugger**:
  - Dữ liệu đồ thị và thuật toán A* chạy ngầm dưới RAM, mắt thường không nhìn thấy được.
  - Hệ thống cung cấp 2 bộ công cụ vẽ trực quan thời gian thực:
    1. **`DrawDebugPath`**: Vẽ dải đường line màu xanh lá cây dạ quang phát sáng thể hiện chính xác từng bước chân uốn lượn vượt đồi của thuật toán A*.
    2. **`DrawDebugDistrictNetwork`**: 
       - 🌸 **Hộp Magenta (Hồng)**: Vị trí Cửa Nhà Chính (`DistrictCenterDoorCoord`).
       - 🟦 **Hộp Cyan (Xanh lam)**: Các ô đường `DirtPath` đang kết nối hợp lệ.
       - 🟨 **Line Vàng Neon**: Các cạnh liên kết giữa các ô đường trong đồ thị.

---

## 📋 5. LỘ TRÌNH TRIỂN KHAI MICRO-STEPS (ROADMAP)

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
- **Phase 2: Mạng Lưới Đường Đi & Thuật Toán A* Pathfinding [HOÀN THÀNH 100%]**
  - `Step 2.1`: [XONG] Cấu trúc Dữ liệu Đường Đi & Path Network Graph.
  - `Step 2.2`: [XONG] Thuật toán A* Pathfinding 3D.
  - `Step 2.3`: [XONG] Hệ thống Kết Nối Đường Đi & Mạng Lưới Công Trình (District Reachability BFS).
  - `Step 2.4`: [XONG] Thiết Lập UE Editor, Tạo Mesh/Material Đường Đi & Visual Testing Trên Viewport.
  - `Phase 2 Retrospective`: [XONG] Tổng kết Mã nguồn, Đồ thị Mạng lưới & Thuật toán A* Pathfinding.
- **Phase 3: Hệ Thống 3 Công Trình & Cơ Chế Móng Xây Dựng (Construction Site) [TIẾP THEO]**
  - `Step 3.1`: Base Building Actor & Construction Site System.
  - `Step 3.2`: Triển khai 3 Công Trình Cốt Lõi (Nhà Kho District Center, Trại Đốn Gỗ Lumberjack, Nhà Dân Beaver Lodge).
  - `Step 3.3`: Hệ Thống Cung Ứng Vật Liệu Xây Dựng (Hauling & Build Progress).
  - `Step 3.4`: Công cụ In-Editor Brush (Click chuột vẽ / sửa địa hình & đặt công trình trực tiếp trong Viewport).
  - `Step 3.5`: [MỚI] Thiết Lập UE Editor, Mesh/Material 3 Công Trình & Testing Viewport.
- **Phase 4: Hải Ly AI & Vòng Lặp Khai Thác Gỗ**
  - `Step 4.1`: Beaver AI Controller & State Machine.
  - `Step 4.2`: Khai Thác Cây, Vận Chuyển Gỗ & Tích Trữ Vào Kho.
  - `Step 4.3`: Chu Trình Nghỉ Ngơi Tại Beaver Lodge.
  - `Step 4.4`: [MỚI] Thiết Lập UE Editor, Mesh Hải Ly, Animation & Testing Chu Trình Làm Việc.
- **Phase 5: Camera RTS, Game Speed & UI HUD**
  - `Step 5.1`: RTS Camera Controller (Pan, Orbit, Zoom).
  - `Step 5.2`: Game Speed Manager (Pause, 1x, 2x, 3x).
  - `Step 5.3`: UI HUD (Hiển thị tài nguyên Gỗ, Dân số, Nút xây dựng).
  - `Step 5.4`: [MỚI] Thiết Lập Toàn Diện UI HUD, Polish & Đóng Gói Hoàn Thiện Prototype Game.
