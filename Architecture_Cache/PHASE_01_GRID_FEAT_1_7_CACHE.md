# 🪵 PHASE 01 - FEAT 1.7: MULTI-SNAPSHOT MAP PRESETS & DATA ASSET PERSISTENCE CACHE

> **Tài liệu tham chiếu chuẩn hóa theo Proposal 10 & Proposal 13**: Lưu vết Kiến trúc Data-Flow, Call-Graph và Debug Trail phục vụ tra cứu tức thì trong 30 giây.

---

## 🎯 A. MỤC TIÊU NGHIỆP VỤ (TARGET & BUSINESS OBJECTIVE)
* **Vấn đề giải quyết**: Tránh việc ghi đè hoặc mất dữ liệu bản đồ khi chỉnh sửa bằng tay. Cung cấp cơ chế đóng gói vĩnh viễn địa hình thành các File Data Asset (`.uasset`) độc lập trong Content Browser, cho phép Lưu và Hồi sinh nhiều bản đồ (Multi-Map Slots) chỉ bằng 1-Click.
* **Nguyên tắc SOLID**: Tách riêng Class `UTimberMapPreset : public UPrimaryDataAsset` phụ trách dữ liệu lưu trữ, `ATimberGridManager` chỉ đóng vai trò ủy quyền nạp và hiển thị.

---

## 📐 B. CẤU TRÚC DỮ LIỆU & CLASS DATA ASSET (DATA MODEL)

### 1. `UTimberMapPreset : public UPrimaryDataAsset`
* `MapName` (`FString`): Tên định danh của bản đồ.
* `GridSizeX, GridSizeY, GridSizeZ` (`int32`): Kích thước ô lưới 3D tương ứng.
* `CellSize` (`float`): Kích thước 1 ô voxel (100cm).
* `PresetCells` (`TArray<FTimberCell>`): Toàn bộ mảng dữ liệu Voxel của bản đồ đã được đóng gói.

---

## 🔄 C. BẢN ĐỒ TƯƠNG TÁC & LỜI GỌI HÀM (CALL-GRAPH & DATA-FLOW)

```text
[ Details Panel: ActiveMapPreset (DA_GrandCanyon_Map01) ]
                  │
                  ├──► 💾 Bấm "SaveToMapPreset()" ──► Copy GridCells sang ActiveMapPreset->PresetCells
                  │                                  └──► Modify() + MarkPackageDirty()
                  │
                  └──► 🔄 Bấm "LoadFromMapPreset()" ──► Copy ActiveMapPreset->PresetCells sang GridCells
                                                     └──► SaveTerrainData() -> LoadTerrainData() (GPU Batch Render 0.001s)
```

---

## ⏱️ E. NHẬT KÝ SỬA LỖI & TIẾN TRÌNH CẬP NHẬT (CHRONO DEBUG TRAIL)

| Thời Gian (YYYY-MM-DD HH:MM) | Mã Thao Tác / Fix | Hiện Tượng & Nguyên Nhân | Giải Pháp Kỹ Thuật Đã Áp Dụng |
| :--- | :--- | :--- | :--- |
| **2026-08-20 10:48** | `Feat 1.7.1 [INIT]` | Cần giải pháp lưu cứng bản đồ vĩnh viễn nhiều slot theo chuẩn SOLID | Khởi tạo Class `UTimberMapPreset` kế thừa `UPrimaryDataAsset` |
| **2026-08-20 11:00** | `Feat 1.7.2 [DONE]` | Kết nối ủy quyền Save/Load sang Data Asset và Batch Render GPU | Khai báo `ActiveMapPreset`, hàm `SaveToMapPreset` và `LoadFromMapPreset` trong `TimberGridManager` |
| **2026-08-20 11:27** | `Feat 1.7.3 [DONE]` | Kiểm thử thực tế lưu cứng map Grand Canyon vào `DA_GrandCanyon_Map01` | Tạo Data Asset, xuất thành công 180.000 ô và khôi phục 100% trong 0.001s |
| **2026-08-20 11:56** | `Feat 1.8.2 [DONE]` | Cần thuật toán rải Cỏ, Vách Đá và Cây tươi trên map hiện tại | Triển khai `PopulateForestAndBiomesOnExistingMap` giữ nguyên 100% độ cao |



