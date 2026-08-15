# 🪵 PROJECT LEARNING & ENGINEERING KNOWLEDGE BASE
## DỰ ÁN: TIMBERBORN CLONE (UNREAL ENGINE 5.6 C++)

> **Mục đích**: Lưu trữ tích lũy và có hệ thống toàn bộ các bài học kỹ thuật, quy tắc làm việc, kiến trúc mã nguồn của riêng dự án Timberborn Clone; đồng thời theo dõi, đánh giá sự tiến bộ và lộ trình phát triển năng lực C++ / Unreal Engine của Anh qua từng Phase.

---

## 🏛️ PHẦN 1: BỘ QUY TẮC KỸ THUẬT & TRIẾT LÝ THIẾT KẾ (DESIGN RULES)

### 1. Triết lý "Code Để Mở Rộng & 100% Editor Sync" (Design for Extensibility & Scalability)
- **Data-Driven & Dynamic Sync 100%**: Mọi thông số từ kích thước lưới (`GridSizeX/Y/Z`), tỷ lệ phủ bề mặt (`GrassRatio`, `RockBandWidth`), độ cao đồi (`MaxTiers`, `TierHeight`), giới hạn leo trèo (`MaxClimbableHeight`), đến gameplay (`WoodCost`, `BuildTime`) **bắt buộc $100\%$ là `UPROPERTY(EditAnywhere, BlueprintReadWrite)`**.
- **Tuyệt đối KHÔNG hardcode bất kỳ con số nào trong C++**: C++ luôn đọc trực tiếp giá trị thực tế từ Editor/Details Panel để khi Anh chỉnh bất kỳ thông số nào trên Editor, Engine sẽ phản ánh chính xác $100\%$.
- **Generic & Modular**: Thiết kế các hệ thống dưới dạng tổng quát, để khi thêm loại Block/Công trình mới chỉ cần khai báo trong Data/Editor mà không phải sửa logic cốt lõi.

### 2. Chuẩn Cấu Trúc Thư Mục Public / Private (UE5 C++ Conventions)
- Tách biệt hoàn toàn `Source/Timber_born_Clone/Public/<Module>/` (.h) và `Source/Timber_born_Clone/Private/<Module>/` (.cpp).
- 7 Modules chuẩn:
  1. `Grid/`: Ô lưới 3D, FTimberCell, Generic ISM Manager.
  2. `Environment/`: Phân tầng địa hình (Cỏ, Đất, Đá), Cụm rừng hữu cơ, Vòng đời cây.
  3. `Pathfinding/`: A* 3D Grid, Mạng lưới đường đi vô hạn về Nhà Chính.
  4. `Buildings/`: Base Building, Construction Site (Móng xây dựng), 3 Công trình (Nhà chính, Kho, Flag).
  5. `Beavers/`: Hải ly AI, Overlap va chạm, FSM công nhân.
  6. `Player/`: RTS Camera (WASD, Q/E, Zoom), Game Speed (Pause, 1x, 2x, 3x).
  7. `UI/`: HUD Widget (Gỗ, Hải ly, Menu xây dựng).

---

## ⚙️ PHẦN 2: QUY TRÌNH LÀM VIỆC & QUY ƯỚC ĐẶT TÊN (WORKFLOW & CONVENTIONS)

### 1. Chu trình Micro-Steps Chuẩn 3 Phần (Universal Tri-Partite Micro-Steps):
Mọi Step tính năng (`Step X.Y`) bắt buộc phải chia thành 3 phần rõ ràng:
- **`Step X.Y.1` (C++ Header & Architecture)**: Khai báo struct `USTRUCT`, enum `UENUM`, class `UCLASS`, biến `UPROPERTY(EditAnywhere)` và hàm.
- **`Step X.Y.2` (C++ Implementation Logic)**: Viết mã nguồn `.cpp`, thuật toán, FSM state machine và hàm xử lý tính toán.
- **`Step X.Y.3` (Hands-on UE Editor Setup & Visual Test)**: Hướng dẫn chi tiết thao tác trên Unreal Engine (Tạo Material, gán Mesh, cấu hình Details Panel, tạo Blueprint và test trên Viewport).
- **Quy tắc phê duyệt**: Trình bày giải pháp -> Hỏi ý kiến Anh ("chưa code nhé") -> Chờ Anh đồng ý ("bắt đầu Step X.Y.Z đi em") mới được phép code!

### 2. End-of-Phase Retrospective & Code Review:
- Sau khi hoàn thành mỗi Phase (ví dụ Phase 1: Step 1.1 $\rightarrow$ Step 1.5):
  - Dừng lại tổng kết toàn bộ mã nguồn C++, kiến trúc và tối ưu hiệu năng (Draw Calls, Memory, Clean Code).
  - Thảo luận cùng Anh rút kinh nghiệm thực tế.
  - Cập nhật các bài học mới vào tài liệu Learning này mà **không xóa đi các bài học cũ**.

### 3. Quy Ước Đánh Số Sửa Lỗi (Fix Naming Hierarchy):
- Sử dụng tiền tố `Fix <Phase>.<Issue>` (ví dụ: `Fix 1.1`, `Fix 1.2`).
- Nếu phát sinh vấn đề phụ liên quan đến cùng một mục đang xử lý, **bắt buộc đánh số phân cấp con `Fix <Phase>.<Issue>.<SubIssue>`** (ví dụ: `Fix 1.2.1`, `Fix 1.2.2`), tuyệt đối không tự ý nhảy cóc sang số khác khi chưa hoàn tất phạm vi hiện tại.

### 4. Quy Tắc Đồng Bộ Tài Liệu Master Plan:
- **Vị trí 1 (Brain Artifact)**: `C:\Users\SSS_TUNG\.gemini\antigravity\brain\...\timberborn_master_plan.md` $\rightarrow$ Cập nhật liên tục theo từng Step và Feat.
- **Vị trí 2 (Thư mục Gốc Dự Án)**: `D:\UE Project\Timber_born_Clone\TIMBERBORN_MASTER_PLAN.md` $\rightarrow$ Cập nhật tổng thể khi hoàn thành mỗi Phase (Phase Milestone Synchronization).

### 5. Quy Ước Phân Loại & Đặt Tên (Step vs Fix vs Feat):
- **`Step <Phase>.<Number>`**: Các bước phát triển tính năng chính theo lộ trình gốc của Master Plan (ví dụ: `Step 1.1`, `Step 1.2`, `Step 1.3`).
- **`Fix <Phase>.<Issue>[.<SubIssue>]`**: Dành riêng cho sửa lỗi và tối ưu hóa hiệu năng (ví dụ: `Fix 1.1`, `Fix 1.2.1`). **Không lưu Fix vào Master Plan**.
- **`Feat <Phase>.<Step>.<Feature>`**: Các tính năng tiện ích làm thêm bổ trợ cho Step (ví dụ: `Feat 1.3.1`). **ĐƯỢC LƯU VÀO MASTER PLAN ngay bên dưới Step tương ứng**.

### 6. Quy Trình Khởi Tạo Dự Án & Kế Thừa Tri Thức Toàn Diện (Universal Project Lifecycle):
Áp dụng cho mọi dự án game từ nay về sau:
1. **Giai Đoạn 1 (Trao đổi & Khảo sát đến khi Hết Thắc Mắc)**:
   - Khi bắt đầu dự án mới, Em và Anh sẽ trao đổi, hỏi đáp chuyên sâu cho đến khi Em không còn bất kỳ thắc mắc nào (Zero Ambiguity).
   - Nếu có nhiều thắc mắc, Em phải chủ động chia nhỏ và phân nhóm theo: `Step` (tính năng chính), `Fix / Risk` (rủi ro / điểm nghẽn), `Feat` (tính năng mở rộng).
2. **Giai Đoạn 2 (Thiết Lập Master Plan Hoàn Chỉnh)**:
   - Sau khi thống nhất $100\%$ các câu hỏi, Em mới tạo tài liệu `Master Plan` chính thức cho dự án đó (gồm cẩm nang Editor, bảng thông số, cấu trúc thư mục, tổng kết học thuật và lộ trình Micro-Steps 3 phần).
3. **Giai Đoạn 3 (Đóng Gói Learning Log & Kế Thừa Kinh Nghiệm)**:
   - Mỗi dự án đều sở hữu tài liệu `PROJECT_LEARNING_LOG_<TÊN_DỰ_ÁN>.md` để lưu trữ tích lũy mọi bài học kỹ thuật, kinh nghiệm xương máu, kiến trúc chuẩn mực để các dự án sau này có thể kế thừa và học hỏi vĩnh viễn!

### 7. Quy Tắc Bất Biến Tích Lũy Chỉ Thêm Không Xóa (Strict Cumulative Append-Only Invariant):
- Mọi tài liệu tri thức (`PROJECT_LEARNING_LOG_...`) và tài liệu đề xuất (`learning_proposal.md`) **TUYỆT ĐỐI KHÔNG ĐƯỢC XÓA BỎ, GHI ĐÈ HOẶC RÚT GỌN NỘI DUNG CŨ**.
- Toàn bộ các bài học, quy tắc và đề xuất được đánh số thứ tự liên tục (Proposal 1, 2, 3, 4...) và chỉ được phép ghi thêm nối tiếp vào cuối file để hệ thống ghi nhớ vĩnh viễn toàn bộ tiến trình lịch sử!

---

## 💡 PHẦN 3: BÀI HỌC KỸ THUẬT ĐÃ TÍCH LŨY (LESSONS LEARNED LOG)

| Ngày | Chủ đề | Bài học & Giải pháp kỹ thuật |
| :--- | :--- | :--- |
| **2026-08-15** | **Adaptive Micro-Steps Hierarchy** | Số lượng Micro-Steps linh hoạt theo độ phức tạp thực tế (3, 4 hoặc 5 steps). Tách riêng các Micro-Steps chuyên biệt cho phần Hands-on Testing & Visual Debugging khi cần hướng dẫn kiểm thử chi tiết. |
| **2026-08-15** | **Live In-Editor Component Sync (OnConstruction / PostEditChangeProperty)** | Khi thay đổi biến `UPROPERTY` trong Details Panel ở Editor, bắt buộc gọi hàm cập nhật visual (`UpdateVisuals()`) trong `OnConstruction()` và `PostEditChangeProperty()` để Actor lập tức biến đổi hình ảnh trên Viewport mà không cần nhấn Play! |
| **2026-08-15** | **Strict Cumulative Append-Only Invariant** | Mọi tài liệu Learning Log và Learning Proposals phải hoạt động theo cơ chế Append-Only $100\%$, tuyệt đối không ghi đè để bảo toàn vĩnh viễn toàn bộ kho tàng tri thức qua mọi phiên làm việc. |
| **2026-08-14** | **Persistence Lifecycle Ordering** | Khi phục hồi dữ liệu từ `SavedGridData`, bắt buộc phải gọi `ClearAllInstances()` và `RebuildISMComponents()` TRƯỚC, rồi mới gán `GridCells = SavedGridData;`. Tránh việc dữ liệu vừa nạp bị vòng lặp dọn dẹp trong `ClearAllInstances()` xóa mất. Đồng thời gắn tự động phục hồi trong `OnConstruction()` để Viewport Editor luôn hiển thị bản đồ sau Rebuild. |
| **2026-08-14** | **100% Dynamic Editor Sync (Zero Hardcode)** | Mọi thông số (chiều cao Z, tỷ lệ phân bổ Cỏ/Đất/Đá, bán kính đồi, độ chênh lệch leo trèo) bắt buộc liên kết $100\%$ qua `UPROPERTY(EditAnywhere)`. C++ đọc trực tiếp từ biến cấu hình để phản ánh ngay tức khắc khi đổi giá trị trên Details Panel. |
| **2026-08-14** | **Editor Slow Task & Progress Bar (FScopedSlowTask)** | Trong Unreal Editor, sử dụng `FScopedSlowTask` kết hợp `MakeDialog()` để hiển thị hộp thoại thanh tiến độ màu xanh native của Engine khi thực hiện các tác vụ tính toán lớn (như sinh bản đồ). |
| **2026-08-14** | **Slate UI Inspector Freeze (Large Array UPROPERTY)** | Mảng dữ liệu lớn (như 16.384 phần tử `GridCells`) nếu gắn `VisibleAnywhere` sẽ khiến giao diện Slate Details Panel cố vẽ hàng chục ngàn dòng widget làm đơ UI thread khi click Actor. **Giải pháp**: Ẩn khỏi Details Panel bằng `UPROPERTY(BlueprintReadOnly)` hoặc dùng Custom Detail Customization / Raw Heap Buffer. |
| **2026-08-14** | **Material bUsedWithInstancedStaticMeshes** | Khi gán Material cho ISM lần đầu, UE5 biên dịch ngầm Shader cho cờ ISM. Phải bấm **Save (Ctrl + S)** để Engine lưu vào DDC cache, kết thúc biên dịch ngầm và mượt mà $100\%$. |
| **2026-08-14** | **Batch ISM Rendering vs Single AddInstance** | Khi sinh bản đồ lớn, gom toàn bộ Transforms vào mảng `TArray<FTransform>` rồi gọi `ISM->AddInstances(Batch)` 1 lần duy nhất thay vì lặp `AddInstance()` 1000 lần, giúp tăng tốc độ gen gấp **100x** và triệt tiêu lag! |
| **2026-08-14** | **Solid Heightmap Grid Generation** | Tính toán mảng độ cao 2D (`TopHeightMap`) trước, sau đó đổ đầy dữ liệu từ đáy $Z=0$ lên đỉnh để triệt tiêu hoàn toàn các lỗ hổng/khe hở ở sườn đồi núi. |
| **2026-08-14** | **UHT UFUNCTION Overload Conflict** | Unreal Header Tool (UHT) không hỗ trợ Function Overloading có cùng tên khi gắn `UFUNCTION(...)` do cơ chế Reflection. **Giải pháp**: Đặt tên hàm tường minh cho kiểu Struct Vector (ví dụ: `GetCellIndexFromVector`, `IsValidGridCoord`) kết hợp `meta = (DisplayName = "...")`. |
| **2026-08-14** | **Instanced Static Mesh (ISM) Optimization** | Render hàng chục ngàn khối block bằng `UInstancedStaticMeshComponent` giúp gộp từ 50.000 Draw Calls xuống chỉ còn **1-2 Draw Calls**, giữ vững 60+ FPS. |
| **2026-08-14** | **ISM RemoveInstance Swap-Back Index** | Trong Unreal Engine, khi gọi `ISM->RemoveInstance(Index)`, Engine swap instance cuối cùng (`LastIndex`) về vị trí `Index` bị xóa. Phải cập nhật lại `InstanceIndex` của Cell tương ứng để tránh lệch index! |
| **2026-08-14** | **Editor Component Cleanup (GetComponents)** | Khi làm việc trong Editor, không nên chỉ Clear mảng TMap tham chiếu mà phải dùng `GetComponents<UInstancedStaticMeshComponent>()` để quét và `DestroyComponent()` trực tiếp từ Actor tránh bị kẹt Component rác trong Level! |
| **2026-08-14** | **UE_LOG Format String Sanitizer** | Trong UE5 C++, để in ký tự phần trăm `%` trong chuỗi `UE_LOG` / `FString::Printf`, bắt buộc phải viết `%%` (ví dụ `100%%`) để tránh lỗi compile static_assert C2338. |
| **2026-08-15** | **Per-Step Hands-on UE Editor Setup Loop** | Mỗi Step phát triển tính năng BẮT BUỘC bao gồm 2 phần liền kề: Code C++ cốt lõi $\rightarrow$ Hướng dẫn Hands-on thiết lập trực tiếp trên Unreal Engine Editor (Mesh, Material, BP, Viewport test). Không dồn cấu hình UE về cuối Phase để người phát triển luôn thấy ngay kết quả trực quan! |
| **2026-08-15** | **Full Module Include Path & Module API Invariant** | Mọi file header/cpp BẮT BUỘC dùng đường dẫn module đầy đủ `#include "Timber_born_Clone/Public/<SubFolder>/<File>.h"` và macro export DLL viết hoa chính xác `TIMBER_BORN_CLONE_API` để tránh lỗi UHT và Rider indexing. |
| **2026-08-15** | **Translucent Unlit Hologram Shader** | Khi làm shader Hologram xem trước vị trí đặt công trình (`Ghost_Valid` / `Ghost_Invalid`), chọn Material Blend Mode = `Translucent`, Shading Model = `Unlit`. Cắm `VectorParameter (RGB) * Multiplier (2.0 - 10.0)` vào `Emissive Color`, và Constant Float `0.45` vào `Opacity` để hiển thị trong suốt phát sáng rực rỡ không bị ảnh hưởng bởi bóng đổ! |
| **2026-08-15** | **Live In-Editor Sync (OnConstruction & PostEditChangeProperty)** | Để thay đổi biến trên Details Panel (như đổi trạng thái `Ghost` -> `Scaffold` -> `Completed`) lập tức cập nhật Mesh và Material trên Viewport mà không cần bấm Play, ghi đè hàm `OnConstruction(const FTransform&)` và `#if WITH_EDITOR PostEditChangeProperty()`. |
| **2026-08-15** | **Unified Details Category (Preventing Slate Button Displacement)** | Không dùng quá nhiều sub-category lồng nhau (`Category = "Timber|Building State"`, `Category = "Timber|Building Debug"`...) vì Slate Details Panel sẽ đẩy các nút `CallInEditor` xuống tít dưới đáy bên dưới mục LOD/Physics/Tags. Gom toàn bộ về chung `Category = "Timber"` kèm `meta = (DisplayName = "...")` để toàn bộ thuộc tính và nút bấm xuất hiện chung 1 khối trên đỉnh! |
| **2026-08-15** | **Two-Phase Construction Prerequisite (Wood First, Hammer Later)** | Cơ chế thi công móng chuẩn Timberborn: Bắt buộc phải vận chuyển đủ $100\%$ Gỗ yêu cầu (`CurrentWoodDelivered >= WoodCost`) thì mới cho phép thợ xây gõ búa tăng tiến độ (`AdvanceBuildProgress`). Khi đạt $100\%$ tiến độ, tự động gọi `SetBuildingState(Completed)` để giàn giáo biến hình thành nhà hoàn thiện! |

---

## 📈 PHẦN 4: THEO DÕI NĂNG LỰC & LỘ TRÌNH PHÁT TRIỂN CỦA ANH (DEVELOPER PROFILE)

- **Mục tiêu**: Nâng cao tư duy Lead Game Developer / Unreal Engine C++ Architect.
- **Kỹ năng đã thực hành xuất sắc**:
  - ✅ Tư duy kiến trúc mở rộng (Extensible & Data-Driven Design).
  - ✅ Nắm vững cấu trúc Source chuẩn Public/Private của Unreal Engine 5.
  - ✅ Nhận diện và xử lý vấn đề Reflection / Function Overload trong UHT.
  - ✅ Quy hoạch địa hình tự nhiên theo quy luật phân tầng thực tế.
  - ✅ Phân tích và giải quyết triệt để các vấn đề nghẽn hiệu năng ISM & Slate UI.
  - ✅ Thiết lập chuẩn mực phân loại rõ ràng giữa Step, Fix và Feat.
  - ✅ Quản lý Vòng đời Sinh trưởng của Cây (Tree Lifecycle FSM).
  - ✅ Thuật toán A* 3D Pathfinding & Đồ thị Mạng lưới Giao thông (Graph Theory & BFS Reachability).
  - ✅ Thiết kế Hệ thống Móng Công Trình (Construction Site Lifecycle: Ghost $\rightarrow$ Scaffold $\rightarrow$ Completed).
  - ✅ Lập trình 3 Công trình Cốt lõi (`DistrictCenter`, `Storage`, `LumberjackFlag`) chuẩn Gameplay Timberborn.
  - ✅ Xây dựng Hệ thống Cung ứng Vật liệu (`FHaulJob`) & Nút bấm CallInEditor Debugger trên Viewport.
- **Kỹ năng trọng tâm tiếp theo**:
  - 🎯 Công cụ In-Editor Brush: Click chuột kéo vẽ đường đi & click đặt móng nhà trực tiếp trên Viewport.
  - 🎯 Lập trình AI Hải Ly (Beaver AI FSM & Vòng lặp khai thác gỗ thực chiến).
  - 🎯 Hệ thống Điều khiển Camera RTS & Giao diện UMG HUD.
