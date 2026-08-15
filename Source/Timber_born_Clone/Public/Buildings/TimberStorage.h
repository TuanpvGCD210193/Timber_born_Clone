// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "TimberStorage.generated.h"

/**
 * Kho Lưu Trữ (Wood Storage Pile) - Chuyên dùng tích trữ gỗ quy mô lớn
 * Kích thước: 2x2 ô lưới
 * Chi phí xây dựng: 10 Gỗ
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberStorage : public ATimberBuildingBase
{
	GENERATED_BODY()

public:
	ATimberStorage();

	// ==========================================
	// STORAGE CONFIGURATION (DATA-DRIVEN)
	// ==========================================

	/** Sức chứa gỗ tối đa của kho */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "10"))
	int32 MaxCapacity = 100;

	/** Lượng gỗ hiện đang được lưu trữ trong kho */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	int32 StoredWood = 0;

	// ==========================================
	// INVENTORY API
	// ==========================================

	/** Nạp gỗ vào kho. Trả về lượng gỗ thực tế đã nạp được */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	int32 DepositWood(int32 Amount);

	/** Rút gỗ ra khỏi kho. Trả về lượng gỗ thực tế lấy ra được */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	int32 WithdrawWood(int32 Amount);

	/** Kiểm tra kho đã đầy $100\%$ chưa */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool IsFull() const;

	/** Kiểm tra kho có rỗng không */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool IsEmpty() const;

	/** Lấy dung lượng còn trống trong kho */
	UFUNCTION(BlueprintPure, Category = "Timber")
	int32 GetRemainingCapacity() const;

	/** Lấy tỷ lệ lấp đầy kho (0.0 -> 1.0) */
	UFUNCTION(BlueprintPure, Category = "Timber")
	float GetStorageRatio() const;
};
