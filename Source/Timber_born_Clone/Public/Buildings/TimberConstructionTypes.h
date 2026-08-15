// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimberConstructionTypes.generated.h"

class ATimberBuildingBase;

/**
 * Phân loại công việc vận chuyển & thi công (Hauling & Construction Job Types)
 */
UENUM(BlueprintType)
enum class EHaulJobType : uint8
{
	Wood_HarvestToStorage		UMETA(DisplayName = "Chuyển Gỗ Từ Flag Về Kho"),
	Wood_StorageToConstruction	UMETA(DisplayName = "Cung Ứng Gỗ Từ Kho Tới Móng"),
	Construct_Hammer			UMETA(DisplayName = "Thợ Xây Thi Công Gõ Búa")
};

/**
 * Độ ưu tiên của công việc (Priority Level)
 */
UENUM(BlueprintType)
enum class EHaulJobPriority : uint8
{
	Low			UMETA(DisplayName = "Ưu Tiên Thấp (1)"),
	Normal		UMETA(DisplayName = "Ưu Tiên Bình Thường (2)"),
	High		UMETA(DisplayName = "Ưu Tiên Cao (3)"),
	Urgent		UMETA(DisplayName = "Khẩn Cấp (4)")
};

/**
 * Cấu trúc đại diện cho 1 nhiệm vụ vận chuyển tài nguyên hoặc thi công xây dựng
 */
USTRUCT(BlueprintType)
struct TIMBER_BORN_CLONE_API FHaulJob
{
	GENERATED_BODY()

	/** ID duy nhất của Job */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	int32 JobID = 0;

	/** Loại công việc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	EHaulJobType JobType = EHaulJobType::Wood_StorageToConstruction;

	/** Mức độ ưu tiên */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	EHaulJobPriority Priority = EHaulJobPriority::Normal;

	/** Tọa độ ô lưới điểm lấy hàng (Kho nguồn / Cây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	FIntVector SourceCoord = FIntVector::ZeroValue;

	/** Công trình đích cần giao gỗ hoặc cần gõ búa */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	TWeakObjectPtr<ATimberBuildingBase> TargetBuilding = nullptr;

	/** Số lượng gỗ cần vận chuyển trong chuyến này */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	int32 WoodAmount = 1;

	/** Cờ báo hiệu Job này đã có Hải ly nhận làm chưa */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	bool bIsClaimed = false;

	/** ID của Hải ly đang thực hiện Job này */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Job")
	int32 ClaimedBeaverID = -1;
};
