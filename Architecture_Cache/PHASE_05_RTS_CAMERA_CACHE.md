# 🪵 PHASE 05 - STEP 5.1: RTS CAMERA CONTROLLER CACHE

> **Tài liệu tham chiếu chuẩn hóa theo Proposal 10 & Proposal 13**: Lưu vết Kiến trúc Data-Flow, Call-Graph và Debug Trail phục vụ tra cứu tức thì trong 30 giây.

---

## 🎯 A. MỤC TIÊU NGHIỆP VỤ (TARGET & BUSINESS OBJECTIVE)
* **Vấn đề giải quyết**: Cung cấp camera chiến thuật góc nhìn từ trên cao (Top-down RTS Isometric) bay lượn mượt mà khắp bản đồ Timberborn. Hỗ trợ Pan (WASD/Mouse edge), Orbit xoay quanh tâm (Q/E hoặc Chuột giữa), Zoom thu phóng mượt mà (Mouse Wheel), và Giới hạn biên bản đồ (Map Clamping).
* **Nguyên tắc SOLID**: Tách riêng Class `ATimberRTSCamera : public APawn` phụ trách toàn bộ hệ thống thị giác và điều hướng máy quay, hoàn toàn độc lập với UI HUD và Player Controller.

---

## 📐 B. CẤU TRÚC THÀNH PHẦN & BIẾN TRẠNG THÁI (DATA MODEL)

### 1. Thành Phần Cốt Lõi (Components):
* `RootScene` (`USceneComponent`): Gốc neo tọa độ trên mặt đất.
* `SpringArmComp` (`USpringArmComponent`): Cần cẩu camera điều khiển góc nghiêng Pitch ($45^\circ$), xoay Yaw và chiều dài Zoom.
* `CameraComp` (`UCameraComponent`): Máy quay thực tế.

### 2. Các Biến Cấu Hình & Nội Suy Mượt Mà (Smooth Interp):
* `TargetArmLength` / `MinZoomDistance` / `MaxZoomDistance` ($800\text{cm} \rightarrow 8000\text{cm}$).
* `TargetYaw` / `TargetPitch`: Góc xoay mục tiêu nội suy `FMath::FInterpTo`.
* `TargetLocation`: Tọa độ mục tiêu di chuyển `FMath::VInterpTo`.
* `PanSpeed` ($2500\text{cm/s}$), `OrbitSpeed` ($90^\circ/\text{s}$), `ZoomSpeed` ($500\text{cm/step}$).

---

## 🔄 C. BẢN ĐỒ TƯƠNG TÁC & LỜI GỌI HÀM (CALL-GRAPH & DATA-FLOW)

```text
[ Người Chơi Nhấn Phím / Cuộn Chuột ]
             │
             ├──► WASD / Edge Scroll ──► AddMovementInput (TargetLocation += Forward/Right * Speed * DeltaTime)
             │
             ├──► Q / E / Middle Drag ──► TargetYaw += OrbitDelta
             │
             └──► Mouse Wheel Up/Down ──► TargetArmLength = Clamp(TargetArmLength +/- ZoomStep)
                          │
                          ▼
[ Tick(DeltaTime): Nội suy mượt mà ]
├──► SetActorLocation(VInterpTo(Current, TargetLocation)) (Kẹp biên Clamping theo GridSize)
├──► SpringArm->SetRelativeRotation(RInterpTo(CurrentRot, TargetRot))
└──► SpringArm->TargetArmLength = FInterpTo(CurrentArm, TargetArmLength)
```

---

## ⏱️ E. NHẬT KÝ SỬA LỖI & TIẾN TRÌNH CẬP NHẬT (CHRONO DEBUG TRAIL)

| Thời Gian (YYYY-MM-DD HH:MM) | Mã Thao Tác / Fix | Hiện Tượng & Nguyên Nhân | Giải Pháp Kỹ Thuật Đã Áp Dụng |
| :--- | :--- | :--- | :--- |
| **2026-08-20 13:46** | `Step 5.1.1 [INIT]` | Khởi tạo hệ thống RTS Camera góc nhìn cao mượt mà theo chuẩn SOLID | Khởi tạo Class `ATimberRTSCamera` kế thừa `APawn` với SpringArm & CameraComp |
| **2026-08-20 13:54** | `Step 5.1.2 [DONE]` | Cài đặt logic Pan (WASD), Orbit (Q/E) và Smooth Zoom (MouseWheel) | Triển khai nội suy `VInterpTo`, `FInterpTo`, `RInterpTo` và Map Clamping trong `TimberRTSCamera.cpp` |
| **2026-08-20 17:02** | `Step 5.4 [DONE]` | Cần cơ chế nhấn giữ chuột phải kéo map mượt mà chuẩn Timberborn | Triển khai `AddPanDelta` và cơ chế Drag Threshold phân biệt Click nhanh vs Kéo map |
| **2026-08-21 08:26** | `Step 5.5 [DONE]` | Xoay góc nhìn tự do như Editor Viewport nhưng khóa cứng độ cao Z khi di chuyển | Triển khai `AddCameraRotationDelta` (Yaw/Pitch) và cô lập hoàn toàn biến đổi trục Z khi Pan |


