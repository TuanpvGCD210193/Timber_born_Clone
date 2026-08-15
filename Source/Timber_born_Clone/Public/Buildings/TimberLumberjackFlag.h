// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "TimberLumberjackFlag.generated.h"

/**
 * Trại Đốn Gỗ (Lumberjack Flag) - Nơi làm việc của 1 thợ đốn gỗ Hải ly
 * Kích thước: 1x1 ô lưới
 * Chi phí xây dựng: 3 Gỗ
 * Chức năng: Tìm kiếm và chỉ định cây trưởng thành trong bán kính làm việc để đốn hạ
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberLumberjackFlag : public ATimberBuildingBase
{
	GENERATED_BODY()

public:
	ATimberLumberjackFlag();

	// ==========================================
	// LUMBERJACK CONFIGURATION (DATA-DRIVEN)
	// ==========================================

	/** Bán kính quét tìm kiếm cây trưởng thành xung quanh (tính theo số ô lưới) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1"))
	int32 WorkRadius = 10;

	/** Số lượng công nhân Hải ly tối đa tại Flag này (Mặc định: 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1", ClampMax = "1"))
	int32 MaxWorkers = 1;

	/** Số lượng công nhân hiện đang được chỉ định làm việc tại đây */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	int32 CurrentAssignedWorkers = 0;

	/** Sức chứa đệm lưu giữ gỗ tạm thời tại Flag trước khi được vác về kho (Mặc định: 5 gỗ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1"))
	int32 LocalWoodBufferCapacity = 5;

	/** Lượng gỗ hiện đang tồn tại ở điểm đệm Flag */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	int32 CurrentLocalWood = 0;

	// ==========================================
	// WORKER & HARVEST API
	// ==========================================

	/** Chỉ định 1 công nhân Hải ly vào làm việc */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	bool AssignWorker();

	/** Hủy chỉ định công nhân làm việc */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	bool UnassignWorker();

	/** Kiểm tra Flag này đã có thợ đốn gỗ làm việc chưa */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool HasWorker() const;

	/** Thêm gỗ khai thác được vào điểm đệm của Flag */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	int32 AddHarvestedWood(int32 Amount);

	/** Lấy gỗ từ điểm đệm của Flag để vác về Kho */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	int32 TakeWoodFromBuffer(int32 Amount);

	/** Tìm kiếm tọa độ của Cây trưởng thành gần nhất trong bán kính làm việc */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	bool FindNearestHarvestableTree(const ATimberGridManager* GridManager, FIntVector& OutTreeCoord) const;
};
