// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Timber_born_Clone/Public/Beavers/BeaverTypes.h"
#include "BeaverAgent.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class ATimberGridManager;
class ATimberDistrictCenter;

/**
 * Lớp đại diện cho một chú Hải ly AI di chuyển và làm việc trên lưới Voxel 3D
 */
UCLASS(Blueprintable)
class TIMBER_BORN_CLONE_API ABeaverAgent : public APawn
{
	GENERATED_BODY()

public:
	ABeaverAgent();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// ==========================================
	// MÁY TRẠNG THÁI & THÔNG TIN HẢI LY
	// ==========================================

	/** Tên gọi của chú Hải ly (vd: Beaver #1, Justin Beaver...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FString BeaverName = TEXT("Beaver");

	/** Trạng thái hiện tại của Máy trạng thái hữu hạn (FSM) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	EBeaverState CurrentState = EBeaverState::Idle;

	/** Nghề nghiệp hiện tại của Hải ly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	EBeaverProfession CurrentProfession = EBeaverProfession::Unemployed;

	/** Cấu hình các chỉ số vận tốc, thể lực, thời gian làm việc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FBeaverAttributeConfig AttributeConfig;

	/** Thể lực hiện tại (0 -> MaxStamina) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	float CurrentStamina = 100.0f;

	/** Số lượng gỗ đang mang trên lưng */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	int32 CarriedWoodAmount = 0;

	// ==========================================
	// THIẾT LẬP DEBUG LEVEL
	// ==========================================

	/**
	 * Chế độ Debug hiển thị:
	 * 0: Tắt toàn bộ Debug.
	 * 1: Hiện dòng chữ 3D Trạng thái FSM nổi trên đầu chú Hải ly.
	 * 2: Hiện Trạng thái FSM + Vẽ dải đường line A* mà Hải ly đang đi.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Debug")
	int32 DebugLevel = 1;

	// ==========================================
	// HỆ THỐNG TÌM ĐƯỜNG & DI CHUYỂN A* (WAYPOINTS)
	// ==========================================

	/** Tọa độ ô lưới hiện tại mà Hải ly đang đứng */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Navigation")
	FIntVector CurrentGridCoord = FIntVector::ZeroValue;

	/** Tọa độ ô lưới đích đến */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Navigation")
	FIntVector TargetGridCoord = FIntVector::ZeroValue;

	/** Danh sách các điểm nút 3D (Waypoints) do thuật toán A* cung cấp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Navigation")
	TArray<FVector> CurrentPathWaypoints;

	/** Vị trí chỉ số Waypoint tiếp theo đang cần bước tới */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Navigation")
	int32 CurrentWaypointIndex = 0;

	// ==========================================
	// NƠI LÀM VIỆC & VÒNG LẶP KHAI THÁC GỖ
	// ==========================================

	/** Con trỏ tới Trại Đốn Gỗ mà Hải ly này đang làm việc */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Timber|Work")
	TWeakObjectPtr<class ATimberLumberjackFlag> AssignedWorkplaceFlag;

	/** Tọa độ ô Cây gỗ đang nhắm tới để chặt */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Timber|Work")
	FIntVector TargetTreeCoord = FIntVector::ZeroValue;

	/** Bộ đếm thời gian đốn cây (0.0 -> TreeChopDuration) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Timber|Work")
	float CurrentChopProgressTimer = 0.0f;

	/** Gán nơi làm việc cho Hải ly */
	UFUNCTION(BlueprintCallable, Category = "Timber|Work")
	void AssignWorkplace(class ATimberLumberjackFlag* InFlag);

	/** Cho Hải ly thôi việc, trở về Unemployed */
	UFUNCTION(BlueprintCallable, Category = "Timber|Work")
	void ClearWorkplace();

	/** Bắt đầu chu trình tìm và đốn cây gỗ gần nhất */
	UFUNCTION(BlueprintCallable, Category = "Timber|Work")
	void StartLumberjackWorkLoop();

	/** Tìm kho lưu trữ gần nhất còn chỗ chứa gỗ */
	bool FindNearestAvailableStorage(FIntVector& OutStorageDoorCoord, class ATimberBuildingBase*& OutStorageBuilding) const;

	// ==========================================
	// CÁC HÀM XỬ LÝ GAMEPLAY
	// ==========================================

	/** Ra lệnh cho Hải ly tìm đường A* và di chuyển tới tọa độ ô lưới mục tiêu */
	UFUNCTION(BlueprintCallable, Category = "Timber|Actions")
	bool MoveToGridCoord(const FIntVector& DestinationCoord);

	/** Chuyển đổi trạng thái FSM */
	UFUNCTION(BlueprintCallable, Category = "Timber|Actions")
	void SetBeaverState(EBeaverState NewState);

	/** Thiết lập mức độ Debug (gọi từ Console Command) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Debug")
	void SetDebugLevel(int32 InLevel);

	/** Trả về chuỗi Text đại diện cho State hiện tại kèm màu sắc */
	FString GetStateDebugString(FColor& OutColor) const;

protected:
	// ==========================================
	// VISUAL & COLLISION COMPONENTS
	// ==========================================

	/** Capsule Collision nhẹ nhàng cho phép Overlap xuyên qua nhau */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	/** Mesh 3D thân hình của chú Hải ly */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<UStaticMeshComponent> BodyMeshComponent;

	/** Mesh 3D khúc gỗ hiển thị trên lưng khi đang vác tài nguyên */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<UStaticMeshComponent> CarriedWoodMeshComponent;

	/** Con trỏ cache tới GridManager để tìm đường A* */
	UPROPERTY()
	mutable TWeakObjectPtr<ATimberGridManager> CachedGridManager;

	/** Con trỏ tới Nhà Chính mà Hải ly này trực thuộc */
	UPROPERTY()
	TWeakObjectPtr<ATimberDistrictCenter> AssignedDistrictCenter;

	// ==========================================
	// HÀM NỘI BỘ XỬ LÝ DI CHUYỂN & FSM TICK
	// ==========================================

	/** Cập nhật di chuyển từng frame dọc theo danh sách Waypoints */
	void UpdateMovement(float DeltaTime);

	/** Cập nhật logic của trạng thái FSM hiện tại */
	void UpdateFSM(float DeltaTime);

	/** Vẽ thông tin Debug 3D theo DebugLevel */
	void DrawDebugVisuals();

	/** Lấy con trỏ GridManager trong thế giới */
	ATimberGridManager* GetGridManager() const;
};
