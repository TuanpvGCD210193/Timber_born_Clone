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

	/** Static Mesh Mũi tên 3D chỉ hướng Cửa ra vào kết nối với đường đi */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<UStaticMeshComponent> DoorArrowComponent;

	/** Mesh mũi tên của project hướng theo local -Y; +90 độ đưa đầu mũi tên về local +X (ra ngoài cửa). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber", meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float DoorArrowMeshYawOffset = 90.0f;

	/** Widget Component hiển thị Icon Billboard cảnh báo đứt đường trên đầu công trình */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Components")
	TObjectPtr<class UWidgetComponent> UnconnectedIconWidgetComponent;

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

	/** Cho phép người chơi dùng công cụ Demolish để tháo dỡ công trình này không */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	bool bCanBeDemolished = true;

	/** Tọa độ ô lưới gốc (Góc dưới bên trái của công trình trên bản đồ) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FIntVector OriginGridCoord = FIntVector::ZeroValue;

	/** Tọa độ tương đối của ô Cửa ra vào so với OriginGridCoord (vd: (0, 1) hoặc (1, 0)) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FIntVector DoorRelativeCoord = FIntVector(0, 1, 0);

	/** Mô tả công dụng của công trình hiển thị trên bảng UI Inspector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	FString BuildingDescription = TEXT("A standard building structure.");

	// ==========================================
	// UI INSPECTOR VIRTUAL INTERFACE (TÁI SỬ DỤNG CHO MỌI NHÀ)
	// ==========================================

	/** Công trình này có phải là nơi làm việc (Workplace) có thể tuyển thợ không? */
	UFUNCTION(BlueprintPure, Category = "Timber|Inspector")
	virtual bool IsWorkplace() const { return false; }

	/** Số lượng công nhân tối đa có thể làm việc */
	UFUNCTION(BlueprintPure, Category = "Timber|Inspector")
	virtual int32 GetMaxWorkers() const { return 0; }

	/** Số lượng công nhân hiện đang làm việc */
	UFUNCTION(BlueprintPure, Category = "Timber|Inspector")
	virtual int32 GetCurrentWorkers() const { return 0; }

	/** Tuyển thêm 1 công nhân (Hải ly) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Inspector")
	virtual bool AddWorker(class ABeaverAgent* Beaver) { return false; }

	/** Cho thôi việc 1 công nhân */
	UFUNCTION(BlueprintCallable, Category = "Timber|Inspector")
	virtual bool RemoveWorker(class ABeaverAgent* Beaver = nullptr) { return false; }

	/** Công trình này có chức năng chứa kho hàng không? */
	UFUNCTION(BlueprintPure, Category = "Timber|Inspector")
	virtual bool IsStorageFacility() const { return false; }

	/** Sức chứa kho tối đa */
	UFUNCTION(BlueprintPure, Category = "Timber|Inspector")
	virtual int32 GetMaxStorageCapacity() const { return 0; }

	/** Lượng tài nguyên hiện có trong kho */
	UFUNCTION(BlueprintPure, Category = "Timber|Inspector")
	virtual int32 GetCurrentStoredAmount() const { return 0; }

	/** Nhận thêm tài nguyên vào kho (+Gỗ). Trả về số lượng thực tế nhận được. */
	UFUNCTION(BlueprintCallable, Category = "Timber|Inspector")
	virtual int32 StoreResource(int32 Amount) { return 0; }

	/** Rút tài nguyên ra khỏi kho (-Gỗ). Trả về số lượng thực tế rút được. */
	UFUNCTION(BlueprintCallable, Category = "Timber|Inspector")
	virtual int32 WithdrawResource(int32 Amount) { return 0; }

	/** Bật/Tắt hiển thị vùng bán kính làm việc (Work Area Bounds) */
	UFUNCTION(BlueprintCallable, Category = "Timber|Inspector")
	virtual void SetWorkAreaVisible(bool bVisible) {}

	// ==========================================
	// CONSTRUCTION & RESOURCE ECONOMY
	// ==========================================

	// ==========================================
	// CONSTRUCTION & LOGISTICS DATA (STEP 4.3)
	// ==========================================

	/** Lượng gỗ cần thiết để xây dựng hoàn thiện công trình */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Construction", meta = (ClampMin = "0"))
	int32 WoodCost = 10;

	/** Lượng gỗ hải ly đã vận chuyển tới móng hiện tại */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Construction")
	int32 CurrentWoodDelivered = 0;

	/** Lượng gỗ đã được các chú Hải ly nhận nhiệm vụ và đang trên đường mang tới (Tránh cử thừa thợ) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Construction")
	int32 ReservedWoodDelivering = 0;

	/** Mức độ ưu tiên xây dựng (1 = Thấp, 2 = Bình thường/Mặc định, 3 = Cao, 4 = Khẩn cấp) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Construction", meta = (ClampMin = "1", ClampMax = "4"))
	int32 ConstructionPriority = 2;

	/** Thứ tự thời gian đặt móng (FIFO Order ID: Đặt trước thì số nhỏ hơn -> Xây trước) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Construction")
	int64 PlacementOrderIndex = 0;

	/** Số lượng thợ xây tối đa được phép cùng tham gia (Tương ứng với mức Priority: 1-4 thợ) */
	UFUNCTION(BlueprintPure, Category = "Timber|Construction")
	int32 GetMaxAllowedBuilders() const { return FMath::Clamp(ConstructionPriority, 1, 4); }

	/** Số lượng thợ xây hiện đang trực tiếp gõ búa tại móng này */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Construction")
	int32 CurrentActiveBuilders = 0;

	/** Số thợ đã giữ chỗ và đang di chuyển tới móng. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Construction")
	int32 ReservedBuildersEnRoute = 0;

	/** Thời gian cần gõ búa để hoàn thiện (tính bằng giây) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber|Construction", meta = (ClampMin = "1.0"))
	float BuildTimeSeconds = 10.0f;

	/** Tiến độ thi công hiện tại (0.0 = chưa xây, 1.0 = hoàn thành 100%) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber|Construction")
	float CurrentBuildProgress = 0.0f;

	/** Kiểm tra móng đã nhận đủ 100% gỗ cần thiết chưa để cho phép bắt đầu gõ búa */
	UFUNCTION(BlueprintPure, Category = "Timber|Construction")
	bool HasAllRequiredWood() const { return CurrentWoodDelivered >= WoodCost; }

	/** Kiểm tra móng có đang cần thêm gỗ nữa không (tính cả gỗ đang trên đường vận chuyển) */
	UFUNCTION(BlueprintPure, Category = "Timber|Construction")
	bool NeedsMoreWoodDelivery() const { return (CurrentWoodDelivered + ReservedWoodDelivering) < WoodCost; }

	/** Lượng gỗ còn thiếu thực tế cần nạp thêm */
	UFUNCTION(BlueprintPure, Category = "Timber|Construction")
	int32 GetRemainingWoodNeeded() const { return FMath::Max(0, WoodCost - (CurrentWoodDelivered + ReservedWoodDelivering)); }

	/** Cờ báo hiệu công trình đã kết nối đường đi về District Center chưa */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timber")
	bool bIsConnectedToDistrict = false;

	/** Cờ đánh dấu Actor này chỉ là bóng mờ Hologram xem trước (không đăng ký vào GridManager) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber")
	bool bIsHologramPreview = false;

protected:
	/** Con trỏ cache tới GridManager để tránh gọi GetActorOfClass liên tục */
	UPROPERTY()
	mutable TWeakObjectPtr<ATimberGridManager> CachedGridManager = nullptr;

public:

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
	int32 AddDeliveredWood(int32 Amount);

	/** Tương thích ngược: Giao gỗ tới móng */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	bool DeliverWood(int32 Amount) { return AddDeliveredWood(Amount) > 0; }

	/** Hải ly thợ xây gõ búa tăng tiến độ xây dựng */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	void AdvanceBuildProgress(float WorkDeltaTime);

	/** Lấy tọa độ ô lưới thực tế của ô Cửa ra vào trên bản đồ */
	UFUNCTION(BlueprintPure, Category = "Timber")
	FIntVector GetDoorGridCoord() const;

	/** Lấy vị trí thế giới (World Location) của ô Cửa ra vào */
	UFUNCTION(BlueprintPure, Category = "Timber")
	FVector GetDoorWorldLocation(const ATimberGridManager* GridManager) const;

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

	/** Lấy danh sách các ô đất tiếp giáp bao quanh chu vi ngoài móng công trình (để kết nối đường) */
	UFUNCTION(BlueprintPure, Category = "Timber")
	TArray<FIntVector> GetPerimeterAdjacentCoords() const;

	/** Kiểm tra xem ô cửa của công trình đã có đường lát đè lên chưa */
	UFUNCTION(BlueprintPure, Category = "Timber")
	bool HasPathAtDoor() const;

	/** Ẩn/Hiện Mũi tên 3D chỉ hướng cửa (Bật khi vào chế độ Lát Đường PaintPath) */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	void SetDoorArrowVisible(bool bVisible);

	/** Cập nhật trạng thái kết nối với District Center (Tự động Ẩn/Hiện Icon cảnh báo Unconnected Icon) */
	UFUNCTION(BlueprintCallable, Category = "Timber")
	void UpdateDistrictConnectionVisuals(bool bConnected);

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

	/** Lấy con trỏ GridManager trong thế giới */
	ATimberGridManager* GetGridManager() const;

protected:
	/** Cập nhật Material và trạng thái ẩn/hiện của các Mesh Component tương ứng với BuildingState */
	virtual void UpdateVisuals();

	/** Tính toán vị trí tương đối (Local Offset) của Mũi tên chỉ hướng cửa nằm ngoài mặt tiền móng */
	FVector CalcDoorArrowLocalOffset() const;

	/** Đồng bộ vị trí và hướng local của mũi tên theo mặt tiền công trình. */
	void UpdateDoorArrowTransform();
};
