// Copyright Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
#include "Timber_born_Clone/Public/Pathfinding/TimberPathGraph.h"
#include "TimberGridManager.generated.h"

/**
 * Actor quản lý toàn bộ hệ thống ô lưới 3D, dữ liệu địa hình, ISM Rendering và sinh cụm rừng
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberGridManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ATimberGridManager();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// GRID CONFIGURATION (CẤU HÌNH Ô LƯỚI)
	// ==========================================
	
	/** Số lượng ô theo trục X */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Grid Settings", meta = (ClampMin = "1", ClampMax = "256"))
	int32 GridSizeX = 32;

	/** Số lượng ô theo trục Y */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Grid Settings", meta = (ClampMin = "1", ClampMax = "256"))
	int32 GridSizeY = 32;

	/** Số lượng ô theo trục Z (Chiều cao) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Grid Settings", meta = (ClampMin = "1", ClampMax = "64"))
	int32 GridSizeZ = 16;

	/** Kích thước của 1 ô (cm). Mặc định 100.0f = 1 mét */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Grid Settings", meta = (ClampMin = "10.0"))
	float CellSize = 100.0f;

	/** Có tự động khởi tạo lưới khi load Actor hay không */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Grid Settings")
	bool bAutoInitializeOnBeginPlay = true;

	// ==========================================
	// RENDERING & ISM CONFIG (CẤU HÌNH ISM ĐỘNG)
	// ==========================================

	/** Bảng đăng ký Mesh & Material cho từng loại Block (Data-Driven, mở rộng tùy ý trong Editor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Rendering")
	TMap<ETimberBlockType, FTimberBlockMeshConfig> BlockMeshConfigs;

	/** Bảng lưu các Instanced Static Mesh Component tương ứng với từng loại Block (Quản lý ngầm) */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Timber|Rendering")
	TMap<ETimberBlockType, TObjectPtr<UInstancedStaticMeshComponent>> BlockISMMap;

	// ==========================================
	// PROCEDURAL GENERATION CONFIGS
	// ==========================================

	/** Cấu hình sinh địa hình đồi núi phân tầng */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Generation")
	FTerrainGenConfig TerrainConfig;

	/** Cấu hình sinh cụm rừng cây hữu cơ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Generation")
	FForestClusterConfig ForestConfig;

	// ==========================================
	// GRID DATA (MẢNG DỮ LIỆU)
	// ==========================================

	/** Mảng 1 chiều lưu trữ dữ liệu 3D đã được Flatten (Ẩn khỏi Details Panel để tối ưu Slate UI) */
	UPROPERTY(BlueprintReadOnly, Category = "Timber|Grid Data")
	TArray<FTimberCell> GridCells;

	// ==========================================
	// MATH & COORDINATE CONVERSION (HÀM TOÁN HỌC)
	// ==========================================

	/** Chuyển đổi tọa độ (X, Y, Z) sang Index 1 chiều */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math")
	FORCEINLINE int32 GetCellIndex(int32 X, int32 Y, int32 Z) const
	{
		return X + (Y * GridSizeX) + (Z * GridSizeX * GridSizeY);
	}

	/** Chuyển đổi FIntVector sang Index 1 chiều */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math", meta = (DisplayName = "Get Cell Index From Vector"))
	FORCEINLINE int32 GetCellIndexFromVector(const FIntVector& Coord) const
	{
		return GetCellIndex(Coord.X, Coord.Y, Coord.Z);
	}

	/** Kiểm tra tọa độ (X, Y, Z) có nằm trong phạm vi bản đồ không */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math")
	FORCEINLINE bool IsValidCoord(int32 X, int32 Y, int32 Z) const
	{
		return (X >= 0 && X < GridSizeX &&
				Y >= 0 && Y < GridSizeY &&
				Z >= 0 && Z < GridSizeZ);
	}

	/** Kiểm tra FIntVector có nằm trong phạm vi bản đồ không */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math", meta = (DisplayName = "Is Valid Grid Coord"))
	FORCEINLINE bool IsValidGridCoord(const FIntVector& Coord) const
	{
		return IsValidCoord(Coord.X, Coord.Y, Coord.Z);
	}

	/** Chuyển đổi từ Tọa độ Ô Lưới (GridCoord) sang Vị trí Thực tế trong Thế giới (WorldLocation) */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math")
	FVector GridCoordToWorldLocation(const FIntVector& Coord, bool bCenterOffset = true) const;

	/** Chuyển đổi từ Vị trí Thế giới (WorldLocation) sang Tọa độ Ô Lưới gần nhất (GridCoord) */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math")
	FIntVector WorldLocationToGridCoord(const FVector& WorldLocation) const;

	// ==========================================
	// GENERIC ISM OPERATIONS (THAO TÁC RENDER ISM)
	// ==========================================

	/** Tự động khởi tạo hoặc cập nhật các ISM Components dựa trên BlockMeshConfigs */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Rendering")
	void RebuildISMComponents();

	/** Lấy hoặc tạo mới ISM Component cho 1 loại Block */
	UInstancedStaticMeshComponent* GetOrCreateISMComponent(ETimberBlockType BlockType);

	/** Đặt 1 khối mới lên ô lưới và hiển thị Instance tương ứng */
	UFUNCTION(BlueprintCallable, Category = "Timber|Grid Actions")
	bool SetBlock(const FIntVector& Coord, ETimberBlockType NewType, bool bWalkable = false);

	/** Xóa khối và Instance tại tọa độ ô lưới */
	UFUNCTION(BlueprintCallable, Category = "Timber|Grid Actions")
	bool ClearBlock(const FIntVector& Coord);

	/** Xóa toàn bộ Instance của tất cả các ISM Components */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void ClearAllInstances();

	// ==========================================
	// TREE LIFECYCLE CONFIG & ACTIONS (STEP 1.4)
	// ==========================================

	/** Cấu hình thời gian sinh trưởng & lượng gỗ của cây */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Tree Lifecycle")
	FTreeGrowthConfig TreeConfig;

	/** Chặt 1 cây trưởng thành hoặc cây non -> biến thành Gốc cây (Stump) và thu được Gỗ */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tree Actions")
	bool ChopTree(const FIntVector& Coord, int32& OutWoodEarned);

	/** Trồng 1 mầm cây non (TreeSapling) mới lên ô lưới */
	UFUNCTION(BlueprintCallable, Category = "Timber|Tree Actions")
	bool PlantSapling(const FIntVector& Coord);

	/** Cập nhật bộ đếm thời gian sinh trưởng của các gốc cây và mầm cây */
	void AdvanceTreeGrowth(float DeltaTime);

	// ==========================================
	// PATH NETWORK CONFIG & ACTIONS (STEP 2.1)
	// ==========================================

	/** Đồ thị mạng lưới đường đi */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Timber|Pathfinding")
	UTimberPathGraph* PathGraph;

	/** Lát 1 ô đường đi (DirtPath) lên trên mặt đất và kết nối vào đồ thị đường */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Actions")
	bool BuildPath(const FIntVector& Coord);

	/** Phá dỡ 1 ô đường đi (DirtPath) và ngắt kết nối khỏi đồ thị đường */
	UFUNCTION(BlueprintCallable, Category = "Timber|Path Actions")
	bool RemovePath(const FIntVector& Coord);

	/** Kiểm tra tại ô lưới có đường đi không */
	UFUNCTION(BlueprintPure, Category = "Timber|Path Actions")
	bool HasPathAt(const FIntVector& Coord) const;

	/** Tìm đường đi ngắn nhất giữa 2 điểm Start và Target bằng thuật toán A* 3D */
	UFUNCTION(BlueprintCallable, Category = "Timber|Pathfinding")
	bool FindPath(const FIntVector& StartCoord, const FIntVector& TargetCoord, TArray<FIntVector>& OutPath, bool bRequirePathAtTarget = false) const;

	/** Vẽ đường line trực quan hiển thị lộ trình tìm được trong Viewport */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Pathfinding")
	void DrawDebugPath(const TArray<FIntVector>& Path, FColor LineColor = FColor::Green, float Duration = 10.0f);

	// ==========================================
	// DISTRICT NETWORK & BUILDING CONNECTIVITY (STEP 2.3)
	// ==========================================

	/** Lấy tọa độ ô cửa ra vào của Nhà Chính thực tế trong thế giới */
	UFUNCTION(BlueprintPure, Category = "Timber|District")
	FIntVector GetDistrictCenterDoorCoord() const;

	/**
	 * Kiểm tra xem một công trình (thông qua tọa độ ô cửa của nó) có kết nối đường đi về District Center không
	 * @param BuildingDoorCoord Tọa độ ô cửa công trình
	 * @param OutPathDistance Khoảng cách số bước đường đi từ District Center tới công trình
	 * @return true nếu công trình được kết nối thành công (sáng đèn hoạt động)
	 */
	UFUNCTION(BlueprintCallable, Category = "Timber|District")
	bool IsBuildingConnectedToDistrict(const FIntVector& BuildingDoorCoord, int32& OutPathDistance) const;

	/** Vẽ trực quan toàn bộ mạng lưới đường đi hợp lệ đang kết nối với District Center trong Viewport */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|District")
	void DrawDebugDistrictNetwork();

	// ==========================================
	// PROCEDURAL TERRAIN & FOREST ACTIONS
	// ==========================================

	/** Sinh bản đồ địa hình phân tầng (Dirt, Grass, Cliff) và các cụm rừng hữu cơ tự nhiên */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void GenerateNaturalTerrainAndForests();

	/**
	 * Phủ thông minh Biome Cỏ, Vách Đá và Rừng Cây Hữu Cơ lên KHUÔN MAP HIỆN TẠI:
	 * Bảo toàn 100% độ cao đồi núi bậc thang hiện tại, chỉ thay đổi bề mặt và cắm cây!
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions", meta = (DisplayName = "🌲 Populate Forest & Biomes on Existing Map"))
	void PopulateForestAndBiomesOnExistingMap();

	/** Lưu trữ bền vững dữ liệu ô lưới hiện tại vào Actor / Level Package */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void SaveTerrainData();

	/** Phục hồi và tái tạo lại địa hình từ dữ liệu đã lưu */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void LoadTerrainData();

	// ==========================================
	// LEVEL DESIGN & HAND-CRAFTED MAP BAKING (FEAT 1.6)
	// ==========================================

	/** Tag nhận diện tùy chọn (Nếu để trống thì sẽ tự động quét tất cả các StaticMesh khớp với BlockMeshConfigs) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Level Design", meta = (DisplayName = "Hand-Crafted Block Tag Filter"))
	FName HandCraftedTagFilter = NAME_None;

	/** Tự động xóa các StaticMeshActor rời rạc trên Level sau khi đã nướng (Bake) thành công vào ISM */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Level Design", meta = (DisplayName = "Auto Delete Actors After Baking"))
	bool bAutoDeleteHandPlacedActorsAfterBaking = true;

	/**
	 * Quét toàn bộ các StaticMeshActor được đặt thủ công trên Viewport,
	 * tự động lượng tử hóa tọa độ thế giới sang ô lưới (X, Y, Z),
	 * gộp vào hệ thống ISM chung và lưu vĩnh viễn vào Map dữ liệu.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Level Design", meta = (DisplayName = "🔨 Bake Hand-Crafted Blocks to Grid"))
	void BakeHandCraftedBlocksToGrid();

	/** Đối chiếu StaticMesh của Actor để nhận diện chính xác đó là loại Block nào (Cliff, Grass, Dirt, Tree...) */
	bool IdentifyBlockTypeFromMesh(UStaticMesh* Mesh, ETimberBlockType& OutType) const;

	// ==========================================
	// MULTI-SNAPSHOT MAP PRESETS (FEAT 1.7)
	// ==========================================

	/** File Data Asset Bản Đồ đang chọn để Lưu / Nạp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Map Presets", meta = (DisplayName = "Active Map Preset"))
	TObjectPtr<class UTimberMapPreset> ActiveMapPreset;

	/** Đóng gói và lưu cứng toàn bộ dữ liệu địa hình hiện tại vào File Data Asset đã chọn */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Map Presets", meta = (DisplayName = "💾 Save Current Map to Preset"))
	void SaveToMapPreset();

	/** Nạp dữ liệu từ File Data Asset và hồi sinh toàn bộ địa hình lên Viewport trong 0.001s */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Map Presets", meta = (DisplayName = "🔄 Load Map from Preset"))
	void LoadFromMapPreset();

	// ==========================================
	// SAVED DATA PERSISTENCE
	// ==========================================

	/** Dữ liệu lưu trữ bền vững của ô lưới (được serialize lưu vào file .umap) */
	UPROPERTY(SaveGame)
	TArray<FTimberCell> SavedGridData;

	// ==========================================
	// BUILDING REGISTRY & RUNTIME QUERIES (STEP 3.4)
	// ==========================================

	/** Tìm tọa độ của ô khối đặc (Solid Ground) cao nhất tại cột (X, Y) */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Math")
	bool GetTopSolidGridCoordAt(int32 X, int32 Y, FIntVector& OutTopCoord) const;

	/** Kiểm tra ô có phải là không gian trống sẵn sàng để đặt công trình lên trên không */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Validation")
	bool IsCellEmptyForBuilding(const FIntVector& Coord) const;

	/** Lấy công trình đang chiếm dụng tại tọa độ ô lưới (trả về nullptr nếu không có) */
	UFUNCTION(BlueprintPure, Category = "Timber|Buildings")
	ATimberBuildingBase* GetBuildingAt(const FIntVector& Coord) const;

	/** Đăng ký công trình mới vào hệ thống quản lý */
	UFUNCTION(BlueprintCallable, Category = "Timber|Buildings")
	void RegisterBuilding(ATimberBuildingBase* Building);

	/** Hủy đăng ký công trình khi bị tháo dỡ / phá hủy */
	UFUNCTION(BlueprintCallable, Category = "Timber|Buildings")
	void UnregisterBuilding(ATimberBuildingBase* Building);

	/** Cập nhật trạng thái kết nối đường về District Center cho tất cả các công trình trên bản đồ */
	UFUNCTION(BlueprintCallable, Category = "Timber|Buildings")
	void UpdateAllBuildingsConnectionStatus();

	/** Danh sách tất cả các công trình đang tồn tại trên bản đồ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	TArray<TObjectPtr<ATimberBuildingBase>> RegisteredBuildings;

	/** Sub-Class Chuyên trách Quản lý Xây dựng & Hàng đợi móng (Phase 4 - Step 4.3) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	TObjectPtr<class UTimberConstructionManager> ConstructionManager;

	/** Lấy con trỏ tới ConstructionManager */
	UFUNCTION(BlueprintPure, Category = "Timber|Buildings")
	class UTimberConstructionManager* GetConstructionManager() const { return ConstructionManager; }

	// ==========================================
	// GETTERS & DEBUG
	// ==========================================

	/** Lấy thông tin 1 ô theo tọa độ FIntVector (Trả về false nếu ngoài phạm vi) */
	UFUNCTION(BlueprintPure, Category = "Timber|Grid Data")
	bool GetCell(const FIntVector& Coord, FTimberCell& OutCell) const;

	/** Đặt loại khối và trạng thái Walkable cho 1 ô (không đụng ISM) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Grid Data")
	bool SetCellType(const FIntVector& Coord, ETimberBlockType NewType, bool bWalkable = false);

	/** Khởi tạo lại toàn bộ mảng ô lưới rỗng */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void InitializeGrid();

	/** Vẽ khung dây Debug kiểm tra phạm vi ô lưới trong Viewport */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions", meta = (DisplayName = "🔍 Draw Debug Grid Bounds"))
	void DrawDebugGridBounds();

	/** Vẽ trực quan tọa độ (X, Y, Z) của các ô mặt đất trong Viewport để kiểm tra cao độ */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions", meta = (DisplayName = "📍 Draw Debug Cell Coordinates"))
	void DrawDebugCellCoordinates();
};
