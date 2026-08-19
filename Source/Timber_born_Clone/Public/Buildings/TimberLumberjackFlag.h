// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "TimberLumberjackFlag.generated.h"

class ABeaverAgent;
class ATimberGridManager;

/**
 * Trại Đốn Gỗ (Lumberjack Flag) - Nơi làm việc của thợ đốn gỗ Hải ly
 * Kích thước: 1x1 ô lưới
 * Chi phí xây dựng: 3 Gỗ
 * Chức năng: Tìm kiếm và chỉ định cây trưởng thành trong bán kính làm việc WorkRadius để đốn hạ
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberLumberjackFlag : public ATimberBuildingBase
{
	GENERATED_BODY()

public:
	ATimberLumberjackFlag();

	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// LUMBERJACK CONFIGURATION (DATA-DRIVEN)
	// ==========================================

	/** Bán kính quét tìm kiếm cây trưởng thành xung quanh (tính theo số ô lưới) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1"))
	int32 WorkRadius = 10;

	/** Số lượng công nhân Hải ly tối đa tại Flag này (Mặc định: 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxWorkers = 1;

	// ==========================================
	// UI INSPECTOR OVERRIDES
	// ==========================================

	virtual bool IsWorkplace() const override { return true; }
	virtual int32 GetMaxWorkers() const override { return MaxWorkers; }
	virtual int32 GetCurrentWorkers() const override { return AssignedWorkerBeavers.Num(); }
	virtual bool AddWorker(ABeaverAgent* Beaver) override;
	virtual bool RemoveWorker(ABeaverAgent* Beaver = nullptr) override;
	virtual void SetWorkAreaVisible(bool bVisible) override;

	// ==========================================
	// WORK AREA & HARVEST API
	// ==========================================

	/** Kiểm tra một tọa độ có nằm bên trong bán kính làm việc WorkRadius không */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool IsCoordInsideWorkRadius(const FIntVector& TargetCoord) const;

	/** Tìm kiếm cây trưởng thành gần nhất bên trong bán kính làm việc */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	bool FindNearestMatureTreeInWorkRadius(const FVector& FromLocation, FIntVector& OutTreeCoord) const;

	/** Vẽ dải đường viền xanh bao quanh toàn bộ khu vực làm việc */
	void DrawWorkAreaBounds();

protected:
	/** Danh sách các chú Hải ly đang được chỉ định làm việc tại Flag này */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Timber")
	TArray<TObjectPtr<ABeaverAgent>> AssignedWorkerBeavers;

	/** Cờ bật/tắt vẽ vùng bán kính làm việc màu xanh */
	UPROPERTY(VisibleInstanceOnly, Category = "Timber")
	bool bIsWorkAreaVisible = false;
};
