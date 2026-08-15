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

	/** Tọa độ gốc của Nhà Chính (District Center) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|District")
	FIntVector DistrictCenterCoord = FIntVector::ZeroValue;

	/** Tọa độ ô cửa ra vào của Nhà Chính (Nơi bắt đầu tỏa ra mạng lưới đường đi) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|District")
	FIntVector DistrictCenterDoorCoord = FIntVector::ZeroValue;

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

	/** Lưu trữ bền vững dữ liệu ô lưới hiện tại vào Actor / Level Package */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void SaveTerrainData();

	/** Phục hồi và tái tạo lại địa hình từ dữ liệu đã lưu */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void LoadTerrainData();

	// ==========================================
	// SAVED DATA PERSISTENCE
	// ==========================================

	/** Dữ liệu lưu trữ bền vững của ô lưới (được serialize lưu vào file .umap) */
	UPROPERTY(SaveGame)
	TArray<FTimberCell> SavedGridData;

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
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void DrawDebugGridBounds();
};
