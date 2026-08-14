# 🦫 Timberborn Clone (Unreal Engine 5 C++)

Dự án phát triển nguyên mẫu game mô phỏng xây dựng thành phố hải ly phong cách **Timberborn** trên nền tảng **Unreal Engine 5** hoàn toàn bằng **C++**.

---

## 🎯 Mục Tiêu Cốt Lõi (Core Goals)

1. **Địa Hình Ô Lưới 3D (3D Grid Terrain)**:
   - Bản đồ dạng khối hộp tiêu chuẩn `1m x 1m x 1m` (`100x100x100 cm`).
   - Tối ưu hóa hiệu năng tối đa bằng `UInstancedStaticMeshComponent` (ISM) gom hàng ngàn block vào 1 Draw Call duy nhất.
   - Hỗ trợ công cụ thiết kế, sinh địa hình ngẫu nhiên và chỉnh sửa trực tiếp trong **Unreal Editor**.

2. **Hệ Thống Cây & Rừng Tái Sinh (Forest & Tree Regrowth)**:
   - Cây sinh ra theo từng **Cụm Rừng (Clusters)** tự nhiên.
   - Vòng đời cây sinh trưởng theo 3 giai đoạn: *Gốc cây sau chặt $\rightarrow$ Cây non $\rightarrow$ Cây trưởng thành* sẵn sàng khai thác.

3. **Hệ Thống Đường Đi (Path Network)**:
   - Lát đường (`Dirt Path`) trên bề mặt địa hình.
   - Kết nối toàn bộ công trình về **Nhà Chính (District Center)**. Công trình chỉ hoạt động khi có đường kết nối hợp lệ.

4. **Hệ Thống 3 Công Trình (Buildings)**:
   - 🏛️ **Nhà Chính (District Center)**: Điểm xuất phát của 5-10 Hải ly, phát tỏa mạng lưới đường đi.
   - 📦 **Kho Lưu Trữ (Warehouse / Storage)**: Nơi tiếp nhận và lưu trữ tài nguyên Gỗ.
   - 🚩 **Flag Đốn Gỗ (Lumberjack Flag)**: Công trình 1x1 phân công 1 thợ đốn gỗ quét cây trong bán kính để khai thác.

5. **Hệ Thống Hải Ly AI (Beaver Agent System)**:
   - Mesh hình học đơn giản (Low-poly/Capsule).
   - Thuật toán **A* Pathfinding trên lưới 3D** di chuyển mượt mà trên đường đi.
   - Tự động nhận việc: *Đi từ Flag $\rightarrow$ Chặt cây trưởng thành $\rightarrow$ Vác gỗ về Kho Lưu Trữ*.

---

## 🛠️ Công Nghệ Sử Dụng (Tech Stack)

- **Engine**: Unreal Engine 5 (UE 5.x)
- **Language**: C++ (Core Logic, Grid Management, A* Algorithm)
- **Rendering Optimization**: `UInstancedStaticMeshComponent` / HISM
- **Editor Tooling**: `CallInEditor` & Editor Utility Widgets

---

## 📋 Lộ Trình Phát Triển (Micro-Steps Roadmap)

- **Phase 1: Địa hình Grid, Cụm Rừng & Công cụ In-Editor**
  - `Step 1.1`: Core Grid Data (`FTimberCell`, `ATimberGridManager`).
  - `Step 1.2`: ISM Rendering cho Đất/Đá/Nước và Cụm Rừng.
  - `Step 1.3`: Quản lý Vòng đời Sinh trưởng của Cây.
  - `Step 1.4`: Công cụ In-Editor (Sinh map ngẫu nhiên + Click vẽ map trực tiếp trong Viewport).
- **Phase 2: Mạng Lưới Đường Đi & Thuật Toán A* Pathfinding**
  - `Step 2.1`: C++ 3D Grid A* Pathfinder.
  - `Step 2.2`: Hệ thống Đường đi & Kiểm tra kết nối về Nhà Chính.
- **Phase 3: Hệ Thống 3 Công Trình (District Center, Storage, Lumberjack Flag)**
  - `Step 3.1`: Base Building & Cơ chế Đặt công trình (Ghost Preview, Snap Grid).
  - `Step 3.2`: Lập trình chi tiết 3 loại công trình.
- **Phase 4: Hải Ly AI & Vòng Lặp Khai Thác Gỗ**
  - `Step 4.1`: Beaver Agent di chuyển bằng A*.
  - `Step 4.2`: Logic Chặt cây $\rightarrow$ Vác gỗ về Kho $\rightarrow$ Cập nhật tài nguyên.

---

> Chi tiết kỹ thuật đầy đủ được lưu tại Master Plan: `timberborn_master_plan.md`.
