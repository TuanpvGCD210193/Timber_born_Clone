// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BeaverTypes.generated.h"

/**
 * Máy trạng thái hữu hạn (FSM) đại diện cho hành vi sinh hoạt & làm việc của Hải ly
 */
UENUM(BlueprintType)
enum class EBeaverState : uint8
{
	Idle                UMETA(DisplayName = "Đang Rảnh Rỗi (Idle)"),
	MovingToTarget      UMETA(DisplayName = "Đang Di Chuyển (Moving)"),
	Working             UMETA(DisplayName = "Đang Làm Việc / Đốn Cây / Xây Dựng (Working)"),
	CarryingResource    UMETA(DisplayName = "Đang Vác Tài Nguyên Về Kho (Carrying)"),
	Resting             UMETA(DisplayName = "Đang Nghỉ Ngơi / Ngủ (Resting)")
};

/**
 * Nghề nghiệp / Vai trò phân công của Hải ly trong khu định cư
 */
UENUM(BlueprintType)
enum class EBeaverProfession : uint8
{
	Unemployed          UMETA(DisplayName = "Thất Nghiệp / Lao Động Tự Do (Unemployed)"),
	Lumberjack          UMETA(DisplayName = "Tiều Phu Đốn Gỗ (Lumberjack)"),
	Builder             UMETA(DisplayName = "Thợ Xây Móng (Builder)"),
	Hauler              UMETA(DisplayName = "Người Vận Chuyển Kho (Hauler)")
};

/** Giai đoạn hiện tại của một nhiệm vụ xây dựng đã được Agent nhận độc quyền. */
UENUM(BlueprintType)
enum class EConstructionTaskPhase : uint8
{
	None            UMETA(DisplayName = "Không có nhiệm vụ"),
	MovingToSource  UMETA(DisplayName = "Đang tới kho lấy gỗ"),
	MovingToSite    UMETA(DisplayName = "Đang mang gỗ tới móng"),
	MovingToBuild   UMETA(DisplayName = "Đang tới móng xây dựng"),
	Building        UMETA(DisplayName = "Đang thi công")
};

/**
 * Bảng cấu hình chỉ số cơ bản của Hải ly (Data-Driven 100% không hardcode)
 */
USTRUCT(BlueprintType)
struct FBeaverAttributeConfig
{
	GENERATED_BODY()

	/** Tốc độ di chuyển cơ bản trên địa hình tự nhiên (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float BaseMoveSpeed = 300.0f;

	/** Hệ số tăng tốc khi di chuyển trên Đường Đất DirtPath (+50% = 1.5x) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float RoadSpeedMultiplier = 1.5f;

	/** Tốc độ xoay người khi đổi hướng (độ/giây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float RotationSpeed = 720.0f;

	/** Thời gian hoàn thành 1 lượt đốn cây trưởng thành (giây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float TreeChopDuration = 3.0f;

	/** Sức chứa gỗ tối đa trên lưng khi mang vác (Mặc định vác tối đa 2 Gỗ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	int32 MaxWoodCarryCapacity = 2;

	/** Tốc độ gõ búa xây dựng cơ bản của 1 Hải ly (% hoàn thành mỗi giây, vd: 10% = 0.1f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float BuildSpeedPerSecond = 0.10f;

	/** Thể lực tối đa */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float MaxStamina = 100.0f;

	/** Tốc độ tiêu hao thể lực mỗi giây khi làm việc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float StaminaDrainRate = 1.0f;

	/** Tốc độ hồi phục thể lực mỗi giây khi ngủ nghỉ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Attributes")
	float StaminaRecoveryRate = 5.0f;
};
