#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
#include "TimberMapPreset.generated.h"

/**
 * Data Asset lưu trữ vĩnh viễn toàn bộ cấu trúc và dữ liệu ô lưới của một Bản Đồ (Feat 1.7)
 * Áp dụng nguyên tắc SOLID: Phân tách trách nhiệm lưu trữ độc lập khỏi TimberGridManager
 */
UCLASS(BlueprintType)
class TIMBER_BORN_CLONE_API UTimberMapPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UTimberMapPreset();

	/** Tên định danh hiển thị của bản đồ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Info")
	FString MapName = TEXT("Custom Hand-Crafted Map");

	/** Mô tả ngắn về địa hình hoặc tác giả */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Info")
	FString Description = TEXT("Bản đồ được thiết kế và nướng bằng Timber Level Design Tool.");

	// ==========================================
	// KÍCH THƯỚC Ô LƯỚI (GRID DIMENSIONS)
	// ==========================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Dimensions")
	int32 GridSizeX = 48;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Dimensions")
	int32 GridSizeY = 24;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Dimensions")
	int32 GridSizeZ = 16;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Dimensions")
	float CellSize = 100.0f;

	// ==========================================
	// DỮ LIỆU ĐỊA HÌNH ĐÓNG GÓI VĨNH VIỄN
	// ==========================================

	/** Mảng toàn bộ các ô Voxel của bản đồ đã được đóng gói */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Data")
	TArray<FTimberCell> PresetCells;
};
