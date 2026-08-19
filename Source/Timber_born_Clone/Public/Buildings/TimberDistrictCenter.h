// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "TimberDistrictCenter.generated.h"

/**
 * Nhà Chính (District Center) - Trái tim của toàn bộ khu định cư Hải ly
 * Kích thước: 2x2 ô lưới
 * Chức năng: Lưu trữ gỗ khởi đầu, giới hạn dân số và là gốc phát tỏa mạng lưới Đường Đi
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberDistrictCenter : public ATimberBuildingBase
{
	GENERATED_BODY()

public:
	ATimberDistrictCenter();

	// ==========================================
	// DISTRICT CONFIGURATION (DATA-DRIVEN)
	// ==========================================

	/** Sức chứa gỗ tối đa của kho Nhà Chính */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "0"))
	int32 MaxWoodStorage = 50;

	/** Lượng gỗ hiện có trong kho Nhà Chính (Bắt đầu với 20 gỗ khởi nghiệp) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "0"))
	int32 CurrentWoodStock = 20;

	/** Số lượng Hải ly tối đa mà Nhà Chính này có thể quản lý */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1"))
	int32 MaxBeaverCapacity = 10;

	/** Số lượng Hải ly sinh ra ban đầu khi bắt đầu game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1"))
	int32 StartingBeaverCount = 3;

	/** Bán kính phục vụ tối đa của District (tính theo số bước đường đi) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "10"))
	int32 MaxDistrictRangeSteps = 70;

	// ==========================================
	// UI INSPECTOR OVERRIDES
	// ==========================================

	virtual bool IsStorageFacility() const override { return true; }
	virtual int32 GetMaxStorageCapacity() const override { return MaxWoodStorage; }
	virtual int32 GetCurrentStoredAmount() const override { return CurrentWoodStock; }
	virtual int32 StoreResource(int32 Amount) override { return AddWood(Amount); }

	// ==========================================
	// STORAGE API
	// ==========================================

	/** Nhập thêm gỗ vào kho Nhà Chính. Trả về lượng gỗ thực tế đã nhập thành công */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	int32 AddWood(int32 Amount);

	/** Xuất gỗ ra khỏi kho Nhà Chính để dùng cho việc xây dựng. Trả về lượng gỗ lấy ra được */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	int32 RemoveWood(int32 Amount);

	/** Kiểm tra kho Nhà Chính đã đầy gỗ chưa */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool IsStorageFull() const;

	/** Lấy tỷ lệ lấp đầy kho gỗ (0.0 -> 1.0) */
	UFUNCTION(BlueprintPure, Category = "Timber")
	float GetStorageFillRatio() const;
};
