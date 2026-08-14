# 🦫 Timberborn Clone (Unreal Engine 5.6 C++)

Dự án phát triển nguyên mẫu game mô phỏng xây dựng thành phố hải ly phong cách **Timberborn** trên nền tảng **Unreal Engine 5.6** hoàn toàn bằng **C++**.

---

## 🎯 Mục Tiêu & Cơ Chế Cốt Lõi (Core Goals)

1. **Địa Hình Phân Tầng Tự Nhiên (Natural Terrain Layering)**:
   - Bản đồ khối hộp tiêu chuẩn `1m x 1m x 1m` (`100x100x100 cm`).
   - Phân tầng chân thực: Bề mặt trên cùng là **Cỏ (`Grass`)**, lòng đất sâu là **Đất (`Dirt`)**, các cao nguyên/ngọn đồi nhô cao là **Đá (`Cliff/Rock`)**.
   - Tối ưu hóa tối đa bằng `UInstancedStaticMeshComponent` (ISM) gom hàng ngàn block vào 1 Draw Call duy nhất.

2. **Cụm Rừng Hữu Cơ & Tái Sinh (Organic Forest & Tree Regrowth)**:
   - Cây sinh theo từng **Cụm Rừng Hữu Cơ (Organic Clusters)** trên nền mặt cỏ, mật độ dày ở tâm và thưa dần ra mép.
   - Vòng đời cây sinh trưởng theo các giai đoạn: *Gốc cây sau chặt $\rightarrow$ Cây non $\rightarrow$ Lớn dần $\rightarrow$ Cây trưởng thành* sẵn sàng khai thác.

3. **Hệ Thống Móng Xây Dựng (Construction Site)**:
   - Khi đặt công trình sẽ hiển thị Hologram (Xanh lá: Hợp lệ / Đỏ: Không hợp lệ) và tạo móng; Hải ly sẽ vác gỗ tới và thi công hoàn thiện.
   - Toàn bộ thông số: `WoodCost`, `BuildTime`, `WorkerSlots`, `StartingWood` đều cấu hình linh hoạt bằng `UPROPERTY`, **không bị hardcode trong C++**.

4. **Mạng Lưới Đường Đi (Path Network)**:
   - Lát đường (`Dirt Path`) trên bề mặt địa hình.
   - Phạm vi vô hạn, kết nối toàn bộ công trình về **Nhà Chính (District Center)**. Công trình chỉ hoạt động khi có đường kết nối hợp lệ.

5. **Hệ Thống 3 Công Trình (Buildings)**:
   - 🏛️ **Nhà Chính (District Center)**: Điểm xuất phát của 5-10 Hải ly, cấp sẵn lượng gỗ khởi đầu, phát tỏa mạng lưới đường đi.
   - 📦 **Kho Lưu Trữ (Warehouse / Storage)**: Nơi tiếp nhận và lưu trữ tài nguyên Gỗ.
   - 🚩 **Flag Đốn Gỗ (Lumberjack Flag)**: Công trình 1x1 phân công 1 thợ đốn gỗ quét cây trong bán kính để khai thác.

6. **Hệ Thống Hải Ly AI (Beaver Agent System)**:
   - Mesh hình học đơn giản (Low-poly/Capsule). **Hải ly đi xuyên qua nhau (Overlap)** trên đường đi để tránh kẹt đường.
   - Thuật toán **A* Pathfinding trên lưới 3D** di chuyển mượt mà trên đường đi.
   - Tự động nhận việc: *Đi từ Flag $\rightarrow$ Chặt cây trưởng thành $\rightarrow$ Vác gỗ về Kho Lưu Trữ (hoặc xây dựng móng)*.

7. **Camera RTS, Game Speed & Giao Diện (HUD/UI)**:
   - Camera: `WASD` di chuyển, `Q/E` hoặc chuột giữa xoay, cuộn chuột để zoom.
   - Tốc độ Game: `Space` (Pause/Resume), `1, 2, 3` (Tốc độ 1x, 2x, 3x).
   - UI: Thanh hiển thị Gỗ & Hải ly ở đỉnh màn hình, menu 4 nút chọn xây dựng ở đáy màn hình.

---

## 🎛️ Hướng Dẫn Cấu Hình Trong Unreal Editor (Details Panel Setup)

1. Chọn Actor `TimberGridManager` trong Level.
2. Tại mục **Timber | Rendering $\rightarrow$ Block Mesh Configs**, thêm 4 phần tử:
   - `Dirt`: Static Mesh `Cube`, Material Nâu.
   - `Grass`: Static Mesh `Cube`, Material Xanh lá.
   - `Cliff`: Static Mesh `Cube`, Material Xám đá.
   - `TreeMature`: Static Mesh `Cylinder`/`Cone`/`Tree`, Material Cây.
3. Nhấn nút **GenerateNaturalTerrainAndForests** trong **Timber | Grid Actions** để sinh bản đồ tức thì.

---

## 🛠️ Công Nghệ & Kiến Trúc Mã Nguồn (Tech Stack)

- **Engine**: Unreal Engine 5.6 (UE 5.6.x)
- **Language**: C++ (Tuân thủ cấu trúc chuẩn **Public / Private**)
- **Rendering Optimization**: `UInstancedStaticMeshComponent` / HISM
- **Editor Tooling**: `CallInEditor` & Editor Utility

---

## 📋 Lộ Trình Phát Triển (Micro-Steps Roadmap)

- **Phase 1: Địa hình Grid, Rừng cây & Công cụ In-Editor**
  - `Step 1.1`: [XONG] Khởi tạo Cấu trúc thư mục Public/Private và Content.
  - `Step 1.2`: [XONG] Core Grid Data (`FTimberCell`, `ETimberBlockType`, `ATimberGridManager`).
  - `Step 1.3`: [XONG] Dynamic ISM Rendering cho Đất/Cỏ/Đá & Thuật toán Sinh Địa hình Phân tầng + Cụm Rừng Hữu cơ.
  - `Step 1.4`: Quản lý Vòng đời Sinh trưởng của Cây (Sapling $\rightarrow$ Mature).
  - `Step 1.5`: Công cụ In-Editor (Click chuột vẽ / sửa địa hình trực tiếp trong Viewport).
- **Phase 2: Mạng Lưới Đường Đi & Thuật Toán A* Pathfinding**
- **Phase 3: Hệ Thống 3 Công Trình & Cơ Chế Móng Xây Dựng (Construction Site)**
- **Phase 4: Hải Ly AI & Vòng Lặp Khai Thác Gỗ**
- **Phase 5: Camera RTS, Game Speed & UI HUD**

---

> Chi tiết kỹ thuật đầy đủ được lưu tại Master Plan: `timberborn_master_plan.md`.
