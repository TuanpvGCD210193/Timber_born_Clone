# 🪵 PHASE 05 - STEP 5.3: MASTER UI HUD & SETTLEMENT STATS CACHE

> **Tài liệu tham chiếu chuẩn hóa theo Proposal 10 & Proposal 13**: Lưu vết Kiến trúc Data-Flow, Call-Graph và Debug Trail phục vụ tra cứu tức thì trong 30 giây.

---

## 🎯 A. MỤC TIÊU NGHIỆP VỤ (TARGET & BUSINESS OBJECTIVE)
* **Vấn đề giải quyết**: Hiển thị bảng điều khiển trung tâm (Master HUD) phản ánh toàn cảnh khu định cư: Tổng số lượng Gỗ thời gian thực (Nhà Chính + Kho Lưu Trữ), Giới hạn sức chứa kho, Tổng dân số Hải ly, Số lượng có việc làm, và Thanh Menu xây dựng 5 nút ở đáy màn hình.
* **Nguyên tắc SOLID**: Tách riêng Class C++ `UTimberMasterHUDWidget : public UUserWidget` làm cầu nối Data Binding Model độc lập, PlayerController chỉ phát sự kiện (Event Dispatch) khi có thay đổi trạng thái.

---

## 📐 B. CẤU TRÚC SỰ KIỆN & DATA BINDING MODEL

### 1. `UTimberMasterHUDWidget : public UUserWidget`
* `OnResourceStocksUpdated(TotalWoodCount, MaxWoodCapacity)`: Blueprint Event cập nhật Textbox `Txt_TotalWood` (vd: `20 / 150`).
* `OnPopulationStatsUpdated(TotalBeavers, EmployedBeavers, MaxPopulationCapacity)`: Blueprint Event cập nhật Textbox `Txt_Population` (vd: `2 / 10`).
* `OnActiveToolChanged(ActiveBrushMode)`: Blueprint Event bật viền sáng Highlight cho nút công cụ đang chọn.

---

## 🔄 C. BẢN ĐỒ TƯƠNG TÁC & LỜI GỌI HÀM (CALL-GRAPH & DATA-FLOW)

```text
[ ATimberPlayerController (PlayerTick / Event Trigger) ]
                      │
                      ├──► Quét GetAllActorsOfClass(ATimberDistrictCenter) -> Cộng WoodStock & MaxCapacity
                      ├──► Quét GetAllActorsOfClass(ATimberStorage)        -> Cộng StoredWood & MaxCapacity
                      ├──► Quét GetAllActorsOfClass(ABeaverAgent)          -> Đếm Dân số & Nghề nghiệp
                      │
                      ▼
[ UTimberMasterHUDWidget::UpdateResourceDisplay(TotalWood, MaxCap) ]
                      │
                      ▼
[ Blueprint Graph: WBP_MasterHUD -> Cập nhật Textbox & Progress Bar thời gian thực! ]
```

---

## ⏱️ E. NHẬT KÝ SỬA LỖI & TIẾN TRÌNH CẬP NHẬT (CHRONO DEBUG TRAIL)

| Thời Gian (YYYY-MM-DD HH:MM) | Mã Thao Tác / Fix | Hiện Tượng & Nguyên Nhân | Giải Pháp Kỹ Thuật Đã Áp Dụng |
| :--- | :--- | :--- | :--- |
| **2026-08-20 14:26** | `Step 5.3.1 [INIT]` | Cần cầu nối Data Binding cho HUD tổng thể theo chuẩn SOLID | Khởi tạo Class `UTimberMasterHUDWidget` kế thừa `UUserWidget` trong `Public/UI/` |
| **2026-08-20 14:30** | `Step 5.3.2 [DONE]` | Tính toán tổng tài nguyên và quản lý vòng đời Master HUD | Cài đặt `UpdateMasterHUDStats`, quét Nhà chính/Kho/Hải ly và đẩy Event xuống HUD |

