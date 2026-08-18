# 🪵 CUMULATIVE LEARNING PROPOSALS & SYSTEM RULES LOG

> **Nguyên tắc cốt lõi (Strict Cumulative Append-Only Rule)**: Mọi đề xuất học tập, bài học kinh nghiệm và quy tắc được người dùng chỉ dạy **BẮT BUỘC** phải được ghi nối tiếp tích lũy liên tục theo số thứ tự (Proposal 1, 2, 3, 4...). **TUYỆT ĐỐI KHÔNG ĐƯỢC XÓA BỎ, GHI ĐÈ HOẶC RÚT GỌN NỘI DUNG CŨ** để đảm bảo hệ thống luôn ghi nhớ toàn bộ tiến trình học tập từ quá khứ đến tương lai!

---

## 📌 PROPOSAL 1: Chuẩn Hóa Đường Dẫn Include Đầy Đủ & Macro Export API (2026-08-15)
* **Vấn đề**: Việc dùng đường dẫn tương đối ngắn (ví dụ `#include "Grid/..."`) hoặc gõ sai macro export (`FTimber_born_Clone_API`) khiến UHT và Rider bị lỗi biên dịch và báo đỏ file.
* **Quy tắc bắt buộc**:
  - **100% file .h và .cpp** khi include header nội bộ project BẮT BUỘC dùng đường dẫn module đầy đủ:
    `#include "Timber_born_Clone/Public/<SubFolder>/<FileName>.h"`
  - Tên macro export DLL do UHT sinh ra luôn là **`TIMBER_BORN_CLONE_API`** (viết hoa toàn bộ).

---

## 📌 PROPOSAL 2: Quy Tắc Phân Chia Micro-Steps Chuẩn 3 Phần (2026-08-15)
* **Vấn đề**: Tránh dồn việc thiết lập visual/material về cuối Phase khiến người phát triển không nhìn thấy kết quả tức thì.
* **Quy tắc bắt buộc**: Mọi Step tính năng (`Step X.Y`) BẮT BUỘC chia thành 3 phần:
  - **`Step X.Y.1` (C++ Header & Architecture)**: Khai báo `USTRUCT`, `UENUM`, `UCLASS`, `UPROPERTY` và nguyên mẫu hàm.
  - **`Step X.Y.2` (C++ Implementation Logic)**: Viết mã nguồn `.cpp`, máy trạng thái FSM và thuật toán.
  - **`Step X.Y.3` (Hands-on UE Editor Setup & Visual Test)**: Hướng dẫn chi tiết tạo Material, Mesh, gán Details Panel, tạo Blueprint và test trực quan ngay trên Viewport!

---

## 📌 PROPOSAL 3: Quy Trình Khởi Tạo Dự Án & Kế Thừa Tri Thức Toàn Diện (2026-08-15)
* **Vấn đề**: Cần một quy trình onboarding chuẩn chỉ khi bắt đầu bất kỳ dự án game mới nào.
* **Quy tắc bắt buộc**:
  1. **Giai Đoạn 1 (Phỏng vấn & Khảo sát đến khi Hết Thắc Mắc)**: Trao đổi sâu về gameplay, kỹ thuật cho đến khi đạt mức Zero Ambiguity. Nếu có nhiều câu hỏi, chủ động phân loại theo nhóm: `Step` (tính năng chính), `Fix / Risk` (rủi ro / lỗi), `Feat` (tính năng mở rộng).
  2. **Giai Đoạn 2 (Thiết Lập Master Plan)**: Soạn thảo Master Plan chính thức sau khi thống nhất $100\%$ các thắc mắc.
  3. **Giai Đoạn 3 (Đóng Gói Learning Log)**: Tạo tài liệu `PROJECT_LEARNING_LOG_<TÊN_DỰ_ÁN>.md` để lưu trữ tri thức và kế thừa vĩnh viễn cho các dự án sau.

---

## 📌 PROPOSAL 4: Nguyên Tắc Tích Lũy Bất Biến "Chỉ Thêm Nối Tiếp - Không Ghi Đè" (2026-08-15)
* **Vấn đề**: Ghi đè file đề xuất/học tập làm mất các bài học trước đó, khiến AI chỉ nhớ cái gần nhất và quên các kinh nghiệm cũ.
* **Quy tắc bắt buộc**:
  - Mọi tài liệu tri thức (`PROJECT_LEARNING_LOG_...`) và tài liệu đề xuất (`learning_proposal.md`) **PHẢI HOẠT ĐỘNG THEO CƠ CHẾ APPEND-ONLY (CHỈ GHI THÊM NỐI TIẾP)**.
  - Tuyệt đối giữ nguyên toàn bộ các mục, bảng bài học, quy tắc cũ; mỗi bài học mới chỉ được phép bổ sung thêm vào cuối danh sách.

---

## 📌 PROPOSAL 5: Tính Linh Hoạt Của Số Lượng Micro-Steps Theo Độ Phức Tạp (Adaptive Micro-Steps Hierarchy) (2026-08-15)
* **Vấn đề**: Không phải tính năng nào cũng gói gọn trong đúng 3 micro-steps. Nếu bước Test hoặc cấu hình cần sâu sắc, việc gộp chung sẽ làm giảm tính chi tiết.
* **Quy tắc bắt buộc**:
  - Số lượng Micro-Steps **hoàn toàn co giãn linh hoạt** theo độ phức tạp của Step: Có thể là 3, 4 hoặc 5 micro-steps (ví dụ: `.1 C++ Header`, `.2 C++ Logic`, `.3 UE Setup`, `.4 Hands-on Viewport Testing / Debugger Tools`).
  - Khi người dùng yêu cầu tách riêng một Micro-Step chuyên biệt cho việc Testing hoặc cấu hình nâng cao, lập tức cập nhật vào Master Plan và cung cấp hướng dẫn chuyên sâu riêng biệt cho Micro-Step đó!

---

## 📌 PROPOSAL 6: Kiến Trúc Tương Tác Chuột Runtime & Giao Diện HUD Xây Dựng (2026-08-17)
* **Vấn đề**: Người chơi cần tương tác trực tiếp bằng chuột khi bấm Play như game chiến thuật thực tế (vừa click UI vừa click thế giới 3D).
* **Quy tắc bắt buộc**:
  1. **Game & UI Input Mode**: Bật `SetShowMouseCursor(true)` và `SetInputMode(FInputModeGameAndUI)` để chuột click mượt mà cả trên Widget UI và Viewport 3D.
  2. **Thuật Toán Kiểm Tra Móng Linh Hoạt ($N \times M$)**: Khi đặt công trình, thuật toán phải quét toàn bộ $N \times M$ ô của Footprint theo góc xoay $0^\circ/90^\circ/180^\circ/270^\circ$, đảm bảo $100\%$ các ô cùng cao độ $Z$ và không vướng cây/vật cản.
  3. **Hologram Hover Preview**: Luôn hiển thị bóng Hologram bám theo con trỏ chuột thời gian thực (Xanh = Hợp lệ, Đỏ = Không hợp lệ) để người chơi dễ dàng ngắm vị trí trước khi click đặt.
  4. **Công Cụ Phá Hủy Đa Năng (Demolish)**: Tích hợp xóa đường đi `DirtPath` và tháo dỡ công trình vào chung 1 công cụ tiện lợi.

