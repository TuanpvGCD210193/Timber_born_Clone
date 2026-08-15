// Copyright (c) 2026 Timberborn Clone Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Timber_born_Clone/Public/Grid/TimberGridTypes.h"
#include "TimberBuildingBase.generated.h"

class ATimberGridManager;

/**
 * Các trạng thái trong vòng đời xây dựng của một Công trình (Timberborn Building Lifecycle)
 */
UENUM(BlueprintType)
enum class EBuildingState : uint8
{
	Ghost_Valid			UMETA(DisplayName = "Hologram Valid (Green)"),
	Ghost_Invalid		UMETA(DisplayName = "Hologram Invalid (Red)"),
	UnderConstruction	UMETA(DisplayName = "Under Construction (Scaffold)"),
	Completed			UMETA(DisplayName = "Completed / Active")
};

/**
 * Base Actor đại diện cho mọi Công trình trong game (Nhà chính, Kho, Trại đốn gỗ, Nhà dân)
 * Quản lý cơ chế Hologram xem trước, Móng giàn giáo, Tiếp nhận gỗ và Tiến độ thi công
 */
UCLASS()
class TIMBER_BORN_CLONE_API ATimberBuildingBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ATimberBuildingBase();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// COMPONENTS
	// ==========================================

	/** Root Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Static Mesh hiển thị công trình khi hoàn thiện */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<UStaticMeshComponent> BuildingMeshComponent;

	/** Static Mesh hiển thị móng / giàn giáo khi đang thi công */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<UStaticMeshComponent> ScaffoldMeshComponent;

	// ==========================================
	// CONFIGURATION & FOOTPRINT (DATA-DRIVEN)
	// ==========================================

	/** Tên hiển thị của công trình */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FString BuildingName = TEXT("Base Building");

	/** Trạng thái hiện tại của công trình */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	EBuildingState BuildingState = EBuildingState::Ghost_Valid;

	/** Kích thước chiếm ô lưới theo chiều ngang (X, Y) (vd: 1x1, 2x2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FIntPoint FootprintSize = FIntPoint(1, 1);

	/** Tọa độ ô lưới gốc (Góc dưới bên trái của công trình trên bản đồ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FIntVector OriginGridCoord = FIntVector::ZeroValue;

	/** Tọa độ tương đối của ô Cửa ra vào so với OriginGridCoord (vd: (0, 1) hoặc (1, 0)) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FIntVector DoorRelativeCoord = FIntVector(0, 1, 0);

	// ==========================================
	// CONSTRUCTION & RESOURCE ECONOMY
	// ==========================================

	/** Lượng gỗ cần thiết để xây dựng hoàn thiện công trình */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "0"))
	int32 WoodCost = 10;

	/** Lượng gỗ hải ly đã vận chuyển tới móng hiện tại */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	int32 CurrentWoodDelivered = 0;

	/** Thời gian cần gõ búa để hoàn thiện (tính bằng giây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "1.0"))
	float BuildTimeSeconds = 10.0f;

	/** Tiến độ thi công hiện tại (0.0 = chưa xây, 1.0 = hoàn thành 100%) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	float CurrentBuildProgress = 0.0f;

	/** Cờ báo hiệu công trình đã kết nối đường đi về District Center chưa */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	bool bIsConnectedToDistrict = false;

	// ==========================================
	// MATERIAL REFERENCES
	// ==========================================

	/** Material Hologram xanh khi vị trí đặt hợp lệ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	TObjectPtr<UMaterialInterface> GhostValidMaterial;

	/** Material Hologram đỏ khi vị trí đặt bị vướng / không hợp lệ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	TObjectPtr<UMaterialInterface> GhostInvalidMaterial;

	/** Material Giàn giáo / Móng xây dựng */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	TObjectPtr<UMaterialInterface> ScaffoldMaterial;

	/** Material gốc khi công trình hoàn thiện */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	TObjectPtr<UMaterialInterface> FinishedMaterial;

	// ==========================================
	// API FUNCTIONS (STEP 3.1.2 IMPLEMENTATION)
	// ==========================================

	/** Chuyển đổi trạng thái công trình và tự động cập nhật hiển thị Mesh/Material */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	virtual void SetBuildingState(EBuildingState NewState);

	/** Hải ly giao gỗ tới móng công trình */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	bool DeliverWood(int32 Amount);

	/** Hải ly thợ xây gõ búa tăng tiến độ xây dựng */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	void AdvanceBuildProgress(float WorkDeltaTime);

	/** Lấy tọa độ ô lưới thực tế của ô Cửa ra vào trên bản đồ */
	UFUNCTION(BlueprintPure, Category = "Timber")
	FIntVector GetDoorGridCoord() const;

	/** Lấy vị trí thế giới (World Location) của ô Cửa ra vào */
	UFUNCTION(BlueprintPure, Category = "Timber")
	FVector GetDoorWorldLocation(const ATimberGridManager* GridManager) const;

	/** Lấy lượng gỗ còn thiếu cần được vận chuyển tới móng */
	UFUNCTION(BlueprintPure, Category = "Timber")
	int32 GetRemainingWoodNeeded() const;

	/** Lấy phần trăm tiến độ thi công (0% -> 100%) */
	UFUNCTION(BlueprintPure, Category = "Timber")
	float GetBuildProgressPercent() const;

	/** Kiểm tra xem móng đã nhận đủ 100% số gỗ yêu cầu chưa */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool HasRequiredWood() const;

	/** Kiểm tra xem công trình đã hoàn thiện 100% và bắt đầu vận hành chưa */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool IsFullyBuilt() const;

	/** Lấy danh sách toàn bộ các tọa độ ô lưới mà công trình này đang chiếm dụng (Footprint) */
	UFUNCTION(BlueprintPure, Category = "Timber")
	TArray<FIntVector> GetOccupiedGridCoords() const;

	// ==========================================
	// CALL-IN-EDITOR DEBUGGER (STEP 3.3.3)
	// ==========================================

	/** Nút bấm Editor: Mô phỏng giao thêm 2 Gỗ tới móng */
	UFUNCTION(CallInEditor, Category = "Timber", meta = (DisplayName = "🪵 Deliver +2 Wood (Debug)"))
	void Editor_DeliverWoodStep();

	/** Nút bấm Editor: Mô phỏng thợ xây gõ búa thi công 2.5 giây */
	UFUNCTION(CallInEditor, Category = "Timber", meta = (DisplayName = "🔨 Hammer Build 2.5s (Debug)"))
	void Editor_AdvanceBuildStep();

	/** Nút bấm Editor: Hoàn thành ngay lập tức công trình (100% Gỗ & Tiến độ) */
	UFUNCTION(CallInEditor, Category = "Timber", meta = (DisplayName = "⚡ Instant Complete 100% (Debug)"))
	void Editor_InstantComplete();

	/** Nút bấm Editor: Đặt lại móng về trạng thái ban đầu để test lại từ đầu */
	UFUNCTION(CallInEditor, Category = "Timber", meta = (DisplayName = "🔄 Reset Móng 0% (Debug)"))
	void Editor_ResetConstruction();

protected:
	/** Cập nhật Material và trạng thái ẩn/hiện của các Mesh Component tương ứng với BuildingState */
	virtual void UpdateVisuals();
};
