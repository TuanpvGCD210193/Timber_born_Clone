// Copyright Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimberGridTypes.generated.h"

/**
 * Các loại khối (Block Types) trên hệ thống ô lưới 3D
 */
UENUM(BlueprintType)
enum class ETimberBlockType : uint8
{
	None			UMETA(DisplayName = "None / Air"),
	Dirt			UMETA(DisplayName = "Dirt"),
	Grass			UMETA(DisplayName = "Grass"),
	Cliff			UMETA(DisplayName = "Cliff / Rock"),
	Water			UMETA(DisplayName = "Water"),
	TreeStump		UMETA(DisplayName = "Tree Stump"),
	TreeSapling		UMETA(DisplayName = "Tree Sapling"),
	TreeMature		UMETA(DisplayName = "Mature Tree"),
	DirtPath		UMETA(DisplayName = "Dirt Path")
};

/**
 * Giai đoạn sinh trưởng của Cây
 */
UENUM(BlueprintType)
enum class ETreeGrowthStage : uint8
{
	None			UMETA(DisplayName = "None"),
	Stump			UMETA(DisplayName = "Stump / Regrowing"),
	Sapling			UMETA(DisplayName = "Sapling"),
	Growing			UMETA(DisplayName = "Growing"),
	Mature			UMETA(DisplayName = "Mature Tree")
};

/**
 * Cấu trúc dữ liệu đại diện cho 1 ô lưới 3D
 */
USTRUCT(BlueprintType)
struct FTimberCell
{
	GENERATED_BODY()

	/** Tọa độ ô lưới (X, Y, Z) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimberCell")
	FIntVector GridCoord = FIntVector::ZeroValue;

	/** Loại khối hiện tại của ô */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimberCell")
	ETimberBlockType BlockType = ETimberBlockType::None;

	/** Ô này có thể đi bộ lên được không (dành cho A* Pathfinding) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimberCell")
	bool bIsWalkable = false;

	/** Chỉ số Instance trong Instanced Static Mesh Component tương ứng (-1 nếu không có mesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimberCell")
	int32 InstanceIndex = INDEX_NONE;

	/** Giai đoạn sinh trưởng của cây (nếu ô này chứa cây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimberCell")
	ETreeGrowthStage TreeStage = ETreeGrowthStage::None;

	/** Bộ đếm thời gian sinh trưởng của cây */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimberCell")
	float TreeGrowthTimer = 0.0f;

	FTimberCell()
		: GridCoord(FIntVector::ZeroValue)
		, BlockType(ETimberBlockType::None)
		, bIsWalkable(false)
		, InstanceIndex(INDEX_NONE)
		, TreeStage(ETreeGrowthStage::None)
		, TreeGrowthTimer(0.0f)
	{
	}

	FTimberCell(const FIntVector& InCoord, ETimberBlockType InType = ETimberBlockType::None, bool bInWalkable = false)
		: GridCoord(InCoord)
		, BlockType(InType)
		, bIsWalkable(bInWalkable)
		, InstanceIndex(INDEX_NONE)
		, TreeStage(ETreeGrowthStage::None)
		, TreeGrowthTimer(0.0f)
	{
	}
};
USTRUCT(BlueprintType)
struct FTimberBlockMeshConfig
{
	GENERATED_BODY()

	/** Static Mesh hiển thị cho khối */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Config")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	/** Material ghi đè (nếu muốn đổi màu / texture, để trống sẽ dùng material mặc định của mesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Config")
	TObjectPtr<UMaterialInterface> OverrideMaterial = nullptr;

	/** Tỷ lệ kích thước của Mesh (Mặc định 1.0 = 100x100x100 cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Config")
	FVector MeshScale = FVector(1.0f);

	/** Độ lệch vị trí Transform (nếu Pivot của Mesh không nằm ở đáy hoặc tâm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Config")
	FVector MeshOffset = FVector::ZeroVector;
};

/**
 * Cấu hình sinh địa hình đồi núi phân tầng tự nhiên
 */
USTRUCT(BlueprintType)
struct FTerrainGenConfig
{
	GENERATED_BODY()

	/** Độ cao mặt đất cơ bản (số tầng block, vd: 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "1", ClampMax = "10"))
	int32 BaseHeight = 1;

	/** Số lượng ngọn đồi / cao nguyên đá nhô lên */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "0", ClampMax = "10"))
	int32 HillCount = 2;

	/** Bán kính mỗi ngọn đồi (số ô lưới) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "2", ClampMax = "20"))
	int32 HillRadius = 5;

	/** Chiều cao thêm của đồi núi (số tầng block) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "1", ClampMax = "8"))
	int32 HillHeight = 2;
};

/**
 * Cấu hình sinh cụm rừng cây hữu cơ tự nhiên
 */
USTRUCT(BlueprintType)
struct FForestClusterConfig
{
	GENERATED_BODY()

	/** Số lượng cụm rừng trên bản đồ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen", meta = (ClampMin = "0", ClampMax = "20"))
	int32 ClusterCount = 3;

	/** Bán kính mỗi cụm rừng (số ô lưới) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen", meta = (ClampMin = "2", ClampMax = "20"))
	int32 ClusterRadius = 4;

	/** Mật độ cây tại tâm cụm rừng (0.0 đến 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CenterDensity = 0.85f;

	/** Mật độ cây tại rìa mép cụm rừng (0.0 đến 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float EdgeDensity = 0.20f;
};
