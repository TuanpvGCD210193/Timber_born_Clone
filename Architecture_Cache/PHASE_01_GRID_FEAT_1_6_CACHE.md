# 🏔️ ARCHITECTURE CACHE & DEBUG TRAIL: PHASE 01 & FEAT 1.6
## MODULE: VOXEL GRID TERRAIN & HAND-CRAFTED MAP BAKING

---

## 🎯 A. MỤC TIÊU NGHIỆP VỤ (TARGET & BUSINESS OBJECTIVES)
1. **Procedural Grid Terrain (Phase 1)**: Tự động khởi tạo hệ thống ô lưới 3D (`FTimberCell`) kích thước tùy biến $N \times M \times Z$, phân tầng tự nhiên (Cỏ trên đỉnh, Đất ở sườn, Vách đá ở đáy/sườn dốc) bằng Perlin Noise và quy hoạch rừng cây.
2. **Hand-Crafted Map Baking (Feat 1.6)**: Cho phép Level Designer tự do dùng công cụ có sẵn của Unreal Engine Viewport (Grid Snapping = 100cm) kéo thả Vòm cầu đá, Vách thác nước, Cụm cây thủ công; sau đó bấm 1 nút Call-In-Editor để C++ quét, lượng tử hóa và nạp thẳng vào hệ thống ISM chung + Lưu bền vững vào map `.umap`.

---

## 🧮 B. THUẬT TOÁN & MÔ HÌNH TOÁN HỌC (ALGORITHMS & MATH MODELS)
1. **Lượng Tử Hóa Không Gian 3D (Spatial Quantization)**:
   $$\text{GridCoord.X} = \text{RoundToInt}\left(\frac{\text{WorldPos.X} - \text{Origin.X}}{\text{CellSize}}\right)$$
   $$\text{GridCoord.Y} = \text{RoundToInt}\left(\frac{\text{WorldPos.Y} - \text{Origin.Y}}{\text{CellSize}}\right)$$
   $$\text{GridCoord.Z} = \text{RoundToInt}\left(\frac{\text{WorldPos.Z} - \text{Origin.Z}}{\text{CellSize}}\right)$$
2. **Generic ISM Multiplexing**:
   - Sử dụng từ điển `BlockMeshConfigs` để ánh xạ giữa `ETimberBlockType` (Enum) $\leftrightarrow$ `UInstancedStaticMeshComponent` (GPU Instance Buffer).
   - Tối ưu 50.000 khối block về chỉ còn **1 - 4 Draw Calls** duy nhất trên toàn bộ thế giới.
3. **Cơ Chế Bền Vững Dữ Liệu (SaveGame Serialization)**:
   - Toàn bộ trạng thái ô lưới được đóng gói vào mảng 1D `SavedGridData: TArray<FTimberCell>` lưu trực tiếp vào Level Package.

---

## 📡 C. BẢN ĐỒ TƯƠNG TÁC & LỜI GỌI HÀM (CALL-GRAPH & DATA-FLOW)
```text
[ Unreal Editor Viewport ]
    │ (Level Designer kéo thả các StaticMeshActor: Cầu đá, Cây, Vách núi)
    ▼
[ Nút Bấm: BakeHandCraftedBlocksToGrid() ] (Details Panel)
    ├──► 1. UGameplayStatics::GetAllActorsOfClass(AStaticMeshActor)
    ├──► 2. Lặp từng Actor -> IdentifyBlockTypeFromMesh(Actor->GetStaticMesh())
    ├──► 3. WorldLocationToGridCoord(Actor->GetActorLocation()) -> Coord(X, Y, Z)
    ├──► 4. ATimberGridManager::SetBlock(Coord, BlockType, bWalkable)
    │         ├──► Nạp vào GridCells[CellIndex]
    │         └──► AddInstance vào ISM Component tương ứng
    ├──► 5. Actor->Destroy() (Dọn dẹp Actor rác rời rạc trên Viewport)
    └──► 6. ATimberGridManager::SaveTerrainData() (Lưu vĩnh viễn vào file .umap)
```

---

## 🔑 D. ĐIỂM NEO DỮ LIỆU & BIẾN CỐT LÕI (KEY STATE VARIABLES)
* `FTimberCell.BlockType`: Enum xác định loại ô (`Dirt`, `Grass`, `Cliff`, `TreeMature`, `DirtPath`...).
* `FTimberCell.bIsWalkable`: Quyết định Hải ly và A* Pathfinding có thể bước lên ô này hay không.
* `FTimberCell.InstanceIndex`: Chỉ số quản lý Instance tương ứng trong GPU Buffer của ISM Component.
* `SavedGridData`: Mảng sao lưu địa hình phục vụ việc hồi sinh bản đồ khi Rebuild/BeginPlay.

---

## ⏱️ E. NHẬT KÝ SỬA LỖI & TIẾN TRÌNH CẬP NHẬT (CHRONO DEBUG TRAIL)

| Thời Gian (YYYY-MM-DD HH:MM) | Mã Thao Tác / Fix | Hiện Tượng & Nguyên Nhân | Giải Pháp Kỹ Thuật Đã Áp Dụng |
| :--- | :--- | :--- | :--- |
| **2026-08-19 11:28** | `Feat 1.6.1 [NEW]` | Thiết kế quy trình Hand-Crafted Map Native không dùng Tool phức tạp | Khai báo `BakeHandCraftedBlocksToGrid`, `IdentifyBlockTypeFromMesh` trong `TimberGridManager.h` |
| **2026-08-19 11:34** | `Arch Cache [INIT]` | Cần nơi lưu vết thuật toán và Call-Graph để Debug tức thì trong 30s | Khởi tạo tài liệu Kiến trúc Cache Phân cấp chuẩn hóa theo Proposal 10 |
| **2026-08-19 11:44** | `Feat 1.6.2 [DONE]` | Cài đặt logic Quét Actor, Lượng tử hóa tọa độ, nạp ISM & Lưu map | Triển khai `IdentifyBlockTypeFromMesh` & `BakeHandCraftedBlocksToGrid` trong `TimberGridManager.cpp` |
| **2026-08-19 13:43** | `Feat 1.6.2b [ENHANCE]`| Nâng cấp Bake hỗ trợ Scale thể tích 3D & Cục tẩy xóa khối (`DeleteTag`)| Quét BoundingBox AABB theo Scale (X,Y,Z), gọi `ClearBlock` cho Tag Delete & `SetBlock` cho khối Đất/Đá/Cây |
| **2026-08-19 13:58** | `Fix 1.6.2.1 [FIX]` | Đổi tên trên Outliner không xóa mà biến thành Đất do `GetName` khác `GetActorLabel` | Bổ sung `GetActorLabel().Contains("Delete")` trong `TimberGridManager.cpp` để nhận diện 100% tên Outliner |
| **2026-08-19 14:16** | `Feat 1.6.2c [PERF]`| Xóa/Thêm dải khối lớn (60-1000 ô) bị khựng lag do gọi `ClearBlock` lặp lẻ | Nâng cấp cơ chế **Batch Memory Update + Single GPU Rebuild**, xử lý 1000 ô trong 0.001s (Zero Lag) |
| **2026-08-19 14:44** | `Fix 1.6.2.2 [FIX]` | Bake báo thành công nhưng không hiện Mesh trên Viewport do `RebuildISMComponents` thiếu Render Loop | Thay thế bằng `LoadTerrainData()` sau khi `SaveTerrainData()` để GPU sinh ngay 100% Instance ra Viewport |


