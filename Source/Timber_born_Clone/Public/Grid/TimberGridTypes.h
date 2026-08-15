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
 * Cấu hình sinh địa hình cao nguyên đa tầng & vách đá lồi lõm
 */
USTRUCT(BlueprintType)
struct FTerrainGenConfig
{
	GENERATED_BODY()

	/** Map Seed (0 = Random mỗi lần, >0 = Tái tạo chính xác thế bản đồ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen")
	int32 MapSeed = 0;

	/** Độ cao mặt đất cơ bản (số tầng block, vd: 2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "1", ClampMax = "10"))
	int32 BaseHeight = 2;

	/** Số lượng cao nguyên đồi núi chính */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "0", ClampMax = "10"))
	int32 PlateauCount = 2;

	/** Bán kính vùng cao nguyên */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "3", ClampMax = "30"))
	int32 PlateauRadius = 11;

	/** Số bậc thang cao nguyên xếp chồng (hỗ trợ đồi núi nhiều tầng hùng vĩ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxTiers = 3;

	/** Chiều cao mỗi bậc thang (số block) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "1", ClampMax = "8"))
	int32 TierHeight = 2;

	/** Độ lồi lõm so le của các cột vách đá (0.0 = phẳng, 1.0 = răng cưa phong hóa tự nhiên) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Gen", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CliffJaggedness = 0.45f;

	// --- PHÂN BỔ BỀ MẶT HỮU CƠ (CỎ / ĐẤT HOANG ĐẠI VÙNG - HÌNH 2 & 3) ---

	/** Tần số uốn lượn hữu cơ của ranh giới (0.045 = tạo đại vùng đồng cỏ và đất hoang liền mạch rộng lớn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Biomes", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float NoiseScale = 0.045f;

	/** Tỷ lệ phủ Cỏ tươi tốt trên bề mặt (0.0 đến 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Biomes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GrassRatio = 0.50f;

	/** Độ dày dải Đá đệm tự nhiên uốn lượn giữa Cỏ và Đất (0.0 = không vẽ viền ruy băng) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Biomes", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float RockBandWidth = 0.0f;

	/** Đỉnh cao nhất của cao nguyên luôn là khối Đá (Cliff) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Biomes")
	bool bHillTopAlwaysRock = true;

	// --- QUY TẮC ĐI LẠI (WALKABILITY) ---

	/** Độ chênh lệch chiều cao tối đa cho phép leo trèo (1 block đi được, >=2 block chặn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Walkability", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxClimbableHeight = 1;
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
	int32 ClusterRadius = 5;

	/** Mật độ cây tại tâm cụm rừng (0.0 đến 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CenterDensity = 0.85f;

	/** Mật độ cây tại rìa mép cụm rừng (0.0 đến 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float EdgeDensity = 0.20f;

	// --- PHÂN CẤP MẬT ĐỘ THEO LOẠI KHỐI (STEP 1.3.3) ---

	/** Hệ số mật độ cây mọc trên nền Cỏ xanh (Dày đặc) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen|Biomes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GrassTreeDensity = 0.90f;

	/** Hệ số mật độ cây mọc trên nền Đất khô (Thưa thớt) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forest Gen|Biomes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirtTreeDensity = 0.25f;

	/** Hệ số mật độ cây mọc trên nền Đá (Tuyệt đối 0.0 = không mọc trên đá) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forest Gen|Biomes")
	float RockTreeDensity = 0.0f;
};

/**
 * Cấu hình thời gian sinh trưởng và vòng đời của Cây Rừng (Step 1.4)
 */
USTRUCT(BlueprintType)
struct TIMBER_BORN_CLONE_API FTreeGrowthConfig
{
	GENERATED_BODY()

	/** Bật/tắt cơ chế cây tự động mọc lại sau khi đốn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Lifecycle")
	bool bAutoRegrowth = true;

	/** Thời gian gốc cây (Stump) hồi sinh thành Cây con (Sapling) - tính bằng giây */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Lifecycle", meta = (ClampMin = "1.0"))
	float StumpToSaplingDuration = 10.0f;

	/** Thời gian Cây con (Sapling) lớn thành Cây trưởng thành (Mature) - tính bằng giây */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Lifecycle", meta = (ClampMin = "1.0"))
	float SaplingToMatureDuration = 15.0f;

	/** Lượng Gỗ thu được khi chặt 1 cây trưởng thành */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree Lifecycle", meta = (ClampMin = "1"))
	int32 WoodPerMatureTree = 2;
};
