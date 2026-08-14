// Copyright Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
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
	// PROCEDURAL TERRAIN & FOREST ACTIONS
	// ==========================================

	/** Sinh bản đồ địa hình phân tầng (Dirt, Grass, Cliff) và các cụm rừng hữu cơ tự nhiên */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Timber|Grid Actions")
	void GenerateNaturalTerrainAndForests();

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
