#include "Timber_born_Clone/Public/Player/TimberPlayerController.h"
#include "Timber_born_Clone/Public/Player/TimberRTSCamera.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Timber_born_Clone/Public/Buildings/TimberBuildingBase.h"
#include "Timber_born_Clone/Public/Buildings/TimberDistrictCenter.h"
#include "Timber_born_Clone/Public/Buildings/TimberStorage.h"
#include "Timber_born_Clone/Public/Beavers/BeaverAgent.h"
#include "Timber_born_Clone/Public/UI/TimberMasterHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ATimberPlayerController::ATimberPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ATimberPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Cấu hình Input Mode cho phép chuột click cả vào UI Widget và Viewport Game 3D
	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);

	// Tìm và cache con trỏ tới TimberGridManager trong Level
	GetGridManager();

	// 1. Tự động sinh Master HUD Widget (hoặc fallback BuildHUDWidgetClass)
	TSubclassOf<UUserWidget> TargetHUDClass = MasterHUDWidgetClass ? MasterHUDWidgetClass : BuildHUDWidgetClass;
	if (TargetHUDClass)
	{
		UUserWidget* CreatedWidget = CreateWidget<UUserWidget>(this, TargetHUDClass);
		if (CreatedWidget)
		{
			CreatedWidget->AddToViewport(0);
			MasterHUDWidgetInstance = Cast<UTimberMasterHUDWidget>(CreatedWidget);
			UE_LOG(LogTemp, Log, TEXT("ATimberPlayerController: Đã tạo và hiển thị MasterHUD thành công!"));
			
			// Cập nhật số liệu thống kê ban đầu lên HUD
			UpdateMasterHUDStats();
		}
	}
}

void ATimberPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		// Chuột Trái (Left Click & Drag)
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATimberPlayerController::OnLeftMouseDown);
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ATimberPlayerController::OnLeftMouseUp);

		// Chuột Phải (Right Click & Hold Drag Map)
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ATimberPlayerController::OnRightMouseDown);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ATimberPlayerController::OnRightMouseUp);

		// Phím R (Xoay công trình 90 độ)
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ATimberPlayerController::OnRotateKeyPressed);
	}
}

void ATimberPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// ========================================================
	// 1. XỬ LÝ KÉO MAP BẰNG CHUỘT PHẢI (TIMBERBORN RIGHT-CLICK DRAG PAN)
	// ========================================================
	if (bIsRightMousePressed)
	{
		float CurrentMouseX = 0.0f, CurrentMouseY = 0.0f;
		if (GetMousePosition(CurrentMouseX, CurrentMouseY))
		{
			const FVector2D CurrentMousePos(CurrentMouseX, CurrentMouseY);
			const FVector2D TotalDelta = CurrentMousePos - InitialRightClickPos;
			const FVector2D FrameDelta = CurrentMousePos - LastMouseDragPos;
			LastMouseDragPos = CurrentMousePos;

			// Kiểm tra đã vượt ngưỡng để tính là Kéo map chưa
			if (TotalDelta.Size() > RightMouseDragThreshold)
			{
				bHasRightMouseDragged = true;
			}

			if (bHasRightMouseDragged && !FrameDelta.IsNearlyZero())
			{
				APawn* ControlledPawn = GetPawn();
				ATimberRTSCamera* RTSCam = Cast<ATimberRTSCamera>(ControlledPawn);
				if (!RTSCam)
				{
					// Fallback tìm RTS Camera trong Level nếu chưa Possess
					RTSCam = Cast<ATimberRTSCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), ATimberRTSCamera::StaticClass()));
				}

				if (RTSCam)
				{
					// Rê chuột ngang (FrameDelta.X) -> Xoay Yaw Trái/Phải
					// Rê chuột dọc (FrameDelta.Y) -> Xoay Pitch Ngẩng/Cúi
					RTSCam->AddCameraRotationDelta(FrameDelta.X, FrameDelta.Y);
				}
			}
		}
	}

	// Nếu không cầm công cụ nào thì tắt Hologram và không cần quét liên tục
	if (CurrentBrushMode == ETimberBrushMode::None)
	{
		ClearHologramPreview();
		return;
	}

	// Lấy tọa độ ô mặt đất dưới con trỏ chuột
	FVector WorldHitPos;
	const bool bHitGround = GetCursorGridCoord(CurrentHoverGroundCoord, WorldHitPos);

	if (!bHitGround)
	{
		ClearHologramPreview();
		return;
	}

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	// Xử lý hiển thị con trỏ xem trước tương ứng với chế độ
	if (CurrentBrushMode == ETimberBrushMode::PlaceBuilding)
	{
		if (SelectedBuildingClass)
		{
			const ATimberBuildingBase* DefaultBuilding = SelectedBuildingClass->GetDefaultObject<ATimberBuildingBase>();
			const FIntPoint Footprint = DefaultBuilding ? DefaultBuilding->FootprintSize : FIntPoint(1, 1);

			TArray<FIntVector> TempOccupiedCoords;
			bIsCurrentHoverValid = IsAreaValidForBuilding(CurrentHoverGroundCoord, Footprint, BuildingRotationAngle, TempOccupiedCoords);

			UpdateHologramPreview(CurrentHoverGroundCoord, bIsCurrentHoverValid);
		}
	}
	else if (CurrentBrushMode == ETimberBrushMode::PaintPath)
	{
		ClearHologramPreview();

		if (bIsDraggingPath)
		{
			// Đang kéo chuột: Tính toán và vẽ Ghost Preview cho TOÀN BỘ dải đường từ Start -> End
			CalculateDragPathCoords(DragStartGroundCoord, CurrentHoverGroundCoord, CachedDragPathCoords);

			for (const FIntVector& PathCoord : CachedDragPathCoords)
			{
				const bool bCanBuild = !Grid->HasPathAt(PathCoord) && Grid->IsCellEmptyForBuilding(PathCoord);
				const FVector TileCenter = Grid->GridCoordToWorldLocation(PathCoord, true) + FVector(0, 0, -45.0f);
				DrawDebugBox(GetWorld(), TileCenter, FVector(48.0f, 48.0f, 4.0f), bCanBuild ? FColor::Green : FColor::Red, false, 0.05f, 0, 3.0f);
			}
		}
		else
		{
			// Chưa nhấn chuột: Vẽ 1 ô Ghost Preview dưới con trỏ chuột
			const FIntVector PathCoord = FIntVector(CurrentHoverGroundCoord.X, CurrentHoverGroundCoord.Y, CurrentHoverGroundCoord.Z + 1);
			const bool bCanBuild = !Grid->HasPathAt(PathCoord) && Grid->IsCellEmptyForBuilding(PathCoord);
			const FVector TileCenter = Grid->GridCoordToWorldLocation(PathCoord, true) + FVector(0, 0, -45.0f);
			DrawDebugBox(GetWorld(), TileCenter, FVector(48.0f, 48.0f, 4.0f), bCanBuild ? FColor::Green : FColor::Red, false, 0.05f, 0, 2.5f);
		}
	}
	else if (CurrentBrushMode == ETimberBrushMode::Demolish)
	{
		ClearHologramPreview();

		// Kiểm tra xem tại vị trí chuột có Công Trình hay Đường Đi không
		const FIntVector SpaceCoord = FIntVector(CurrentHoverGroundCoord.X, CurrentHoverGroundCoord.Y, CurrentHoverGroundCoord.Z + 1);
		ATimberBuildingBase* HoveredBuilding = Grid->GetBuildingAt(SpaceCoord);
		if (!HoveredBuilding)
		{
			HoveredBuilding = Grid->GetBuildingAt(CurrentHoverGroundCoord);
		}

		if (HoveredBuilding)
		{
			const bool bCanDemolish = HoveredBuilding->bCanBeDemolished;
			const FColor HighlightColor = bCanDemolish ? FColor::Red : FColor(255, 165, 0); // Đỏ nếu xóa được, Cam nếu được bảo vệ

			// 1. Highlight TOÀN BỘ Footprint của Công trình
			const TArray<FIntVector> Occupied = HoveredBuilding->GetOccupiedGridCoords();
			for (const FIntVector& CellCoord : Occupied)
			{
				const FVector TileCenter = Grid->GridCoordToWorldLocation(CellCoord, true) + FVector(0, 0, -40.0f);
				DrawDebugBox(GetWorld(), TileCenter, FVector(48.0f, 48.0f, 8.0f), HighlightColor, false, 0.05f, 0, 3.0f);
			}

			// Vẽ thông tin cảnh báo trên đầu công trình
			const FVector BuildingCenter = HoveredBuilding->GetActorLocation() + FVector(0, 0, 50.0f);
			const FString Msg = bCanDemolish 
				? FString::Printf(TEXT("❌ DEMOLISH: %s (Click để xóa)"), *HoveredBuilding->BuildingName)
				: FString::Printf(TEXT("⛔ PROTECTED: %s (Không thể phá hủy)"), *HoveredBuilding->BuildingName);

			DrawDebugString(GetWorld(), BuildingCenter + FVector(0, 0, 80.0f), Msg, nullptr, HighlightColor, 0.05f, true, 1.2f);
		}
		else if (Grid->HasPathAt(SpaceCoord) || Grid->HasPathAt(CurrentHoverGroundCoord))
		{
			// 2. Highlight ô Đường Đi sắp bị xóa
			const FIntVector PathCoord = Grid->HasPathAt(SpaceCoord) ? SpaceCoord : CurrentHoverGroundCoord;
			const FVector TileCenter = Grid->GridCoordToWorldLocation(PathCoord, true) + FVector(0, 0, 5.0f);
			DrawDebugBox(GetWorld(), TileCenter, FVector(48.0f, 48.0f, 6.0f), FColor::Red, false, 0.05f, 0, 3.0f);
			DrawDebugString(GetWorld(), TileCenter + FVector(0, 0, 30.0f), TEXT("❌ DELETE PATH"), nullptr, FColor::Red, 0.05f, true, 1.0f);

			// Khi kéo chuột: CHỈ CHO PHÉP kéo xóa liên tục các ô ĐƯỜNG ĐI (không xóa bừa bãi công trình)
			if (bIsLeftMouseDown && CurrentHoverGroundCoord != LastProcessedCoord)
			{
				ExecuteDemolish(CurrentHoverGroundCoord);
				LastProcessedCoord = CurrentHoverGroundCoord;
			}
		}
		else
		{
			// 3. Không có gì để xóa: Vẽ ô vuông đỏ nhạt mờ báo hiệu ngay tại bề mặt ô
			const FVector TileCenter = Grid->GridCoordToWorldLocation(SpaceCoord, true) + FVector(0, 0, 5.0f);
			DrawDebugBox(GetWorld(), TileCenter, FVector(48.0f, 48.0f, 2.0f), FColor(150, 50, 50), false, 0.05f, 0, 1.0f);
		}
	}

	// Vẽ khung giàn giáo 3D Wireframe bao quanh móng cho các công trình đang chờ thi công (UnderConstruction)
	for (const TObjectPtr<ATimberBuildingBase>& Building : Grid->RegisteredBuildings)
	{
		if (Building && IsValid(Building) && Building->BuildingState == EBuildingState::UnderConstruction)
		{
			const FVector Center = Building->GetActorLocation() + FVector(0, 0, 50.0f);
			const FVector Extent = FVector(Building->FootprintSize.X * 50.0f - 1.0f, Building->FootprintSize.Y * 50.0f - 1.0f, 48.0f);
			DrawDebugBox(GetWorld(), Center, Extent, Building->GetActorQuat(), FColor(230, 180, 50), false, 0.05f, 0, 2.0f);
		}
	}
}

ATimberGridManager* ATimberPlayerController::GetGridManager() const
{
	if (!CachedGridManager.IsValid())
	{
		AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ATimberGridManager::StaticClass());
		CachedGridManager = Cast<ATimberGridManager>(FoundActor);
	}

	return CachedGridManager.Get();
}

bool ATimberPlayerController::GetCursorGridCoord(FIntVector& OutGridCoord, FVector& OutWorldHitPos) const
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	if (HologramPreviewActor)
	{
		QueryParams.AddIgnoredActor(HologramPreviewActor);
	}

	// Bắn tia Raycast từ góc nhìn Camera chuột xuống địa hình
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		OutWorldHitPos = HitResult.Location;

		// Bù nhẹ một khoảng cực nhỏ theo chiều ngược pháp tuyến va chạm để tránh sai số biên
		const FVector AdjustedPos = HitResult.Location - HitResult.ImpactNormal * 1.0f;
		const FIntVector EstimatedCoord = Grid->WorldLocationToGridCoord(AdjustedPos);

		// Tìm ô đất đặc cao nhất tại cột (X, Y) để ghim đúng tầng Z của địa hình
		FIntVector TopGroundCoord;
		if (Grid->GetTopSolidGridCoordAt(EstimatedCoord.X, EstimatedCoord.Y, TopGroundCoord))
		{
			OutGridCoord = TopGroundCoord;
			return true;
		}

		OutGridCoord = EstimatedCoord;
		return true;
	}

	return false;
}

bool ATimberPlayerController::IsAreaValidForBuilding(const FIntVector& GroundCoord, const FIntPoint& Footprint, int32 RotationDeg, TArray<FIntVector>& OutOccupiedCoords) const
{
	OutOccupiedCoords.Empty();

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return false;
	}

	// Xác định chiều dài và chiều rộng sau khi xoay (0, 90, 180, 270 độ)
	int32 SizeX = Footprint.X;
	int32 SizeY = Footprint.Y;
	if (RotationDeg == 90 || RotationDeg == 270)
	{
		SizeX = Footprint.Y;
		SizeY = Footprint.X;
	}

	const int32 BaseHeightZ = GroundCoord.Z;

	// Quét toàn bộ ma trận N x M ô
	for (int32 Dx = 0; Dx < SizeX; ++Dx)
	{
		for (int32 Dy = 0; Dy < SizeY; ++Dy)
		{
			const int32 CheckX = GroundCoord.X + Dx;
			const int32 CheckY = GroundCoord.Y + Dy;

			// 1. Kiểm tra mặt đất bên dưới: Bắt buộc phải có khối đất đặc và cùng cao độ Z
			FIntVector CellGroundCoord;
			if (!Grid->GetTopSolidGridCoordAt(CheckX, CheckY, CellGroundCoord))
			{
				return false;
			}

			if (CellGroundCoord.Z != BaseHeightZ)
			{
				// Không cùng tầng phẳng -> từ chối
				return false;
			}

			// 2. Kiểm tra không gian móng bên trên: Phải là ô trống (không vướng Cây, không vướng Công trình khác)
			const FIntVector BuildingCellCoord = FIntVector(CheckX, CheckY, BaseHeightZ + 1);
			if (!Grid->IsCellEmptyForBuilding(BuildingCellCoord))
			{
				return false;
			}

			OutOccupiedCoords.Add(BuildingCellCoord);
		}
	}

	return true;
}

void ATimberPlayerController::SelectTool_PaintPath()
{
	ClearHologramPreview();
	CurrentBrushMode = ETimberBrushMode::PaintPath;
	SelectedBuildingClass = nullptr;
	SetAllBuildingsDoorArrowVisible(true);
	UE_LOG(LogTemp, Log, TEXT("[HUD] Đã chọn công cụ: Lát Đường (Paint Path)."));
}

void ATimberPlayerController::SelectTool_Demolish()
{
	ClearHologramPreview();
	CurrentBrushMode = ETimberBrushMode::Demolish;
	SelectedBuildingClass = nullptr;
	SetAllBuildingsDoorArrowVisible(false);
	UE_LOG(LogTemp, Log, TEXT("[HUD] Đã chọn công cụ: Phá Hủy Đa Năng (Demolish)."));
}

void ATimberPlayerController::SelectTool_PlaceBuilding(TSubclassOf<ATimberBuildingBase> BuildingClass)
{
	if (!BuildingClass)
	{
		return;
	}

	CurrentBrushMode = ETimberBrushMode::PlaceBuilding;
	SelectedBuildingClass = BuildingClass;
	SetAllBuildingsDoorArrowVisible(false);
	SpawnHologramPreview();

	UE_LOG(LogTemp, Log, TEXT("[HUD] Đã chọn công cụ: Đặt Móng Công Trình [%s]."), *BuildingClass->GetName());
}

void ATimberPlayerController::SelectTool_Deselect()
{
	ClearHologramPreview();
	CurrentBrushMode = ETimberBrushMode::None;
	SelectedBuildingClass = nullptr;
	SetAllBuildingsDoorArrowVisible(false);
	bIsLeftMouseDown = false;
	LastProcessedCoord = FIntVector(-999, -999, -999);
	UE_LOG(LogTemp, Log, TEXT("[HUD] Đã HỦY CHỌN công cụ, trở về con trỏ chuột bình thường."));
}

void ATimberPlayerController::SetAllBuildingsDoorArrowVisible(bool bVisible)
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	for (const TObjectPtr<ATimberBuildingBase>& Building : Grid->RegisteredBuildings)
	{
		if (Building && IsValid(Building))
		{
			Building->SetDoorArrowVisible(bVisible);
		}
	}
}

void ATimberPlayerController::RotateBuildingClockwise()
{
	BuildingRotationAngle = (BuildingRotationAngle + 90) % 360;

	if (HologramPreviewActor)
	{
		HologramPreviewActor->SetActorRotation(FRotator(0.0f, static_cast<float>(BuildingRotationAngle), 0.0f));
	}

	UE_LOG(LogTemp, Log, TEXT("[BRUSH] Đã xoay công trình sang góc: %d độ."), BuildingRotationAngle);
}

void ATimberPlayerController::OnLeftMouseDown()
{
	bIsLeftMouseDown = true;

	// 1. Ở CHẾ ĐỘ CON TRỎ TỰ DO (NONE): ƯU TIÊN RAYCAST TRỰC TIẾP VÀO ACTOR 3D (Độ chính xác 100%)
	if (CurrentBrushMode == ETimberBrushMode::None)
	{
		FHitResult HitResult;
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			if (AActor* HitActor = HitResult.GetActor())
			{
				if (ATimberBuildingBase* Building = Cast<ATimberBuildingBase>(HitActor))
				{
					SelectBuilding(Building);
					return;
				}
			}
		}
	}

	// 2. CÁC CHẾ ĐỘ KHÁC (PaintPath, Demolish, PlaceBuilding) HOẶC FALLBACK: DÙNG GRID MATH
	FVector WorldHitPos;
	if (GetCursorGridCoord(CurrentHoverGroundCoord, WorldHitPos))
	{
		if (CurrentBrushMode == ETimberBrushMode::PaintPath)
		{
			bIsDraggingPath = true;
			DragStartGroundCoord = CurrentHoverGroundCoord;
			CachedDragPathCoords.Empty();
			CachedDragPathCoords.Add(FIntVector(CurrentHoverGroundCoord.X, CurrentHoverGroundCoord.Y, CurrentHoverGroundCoord.Z + 1));
		}
		else
		{
			ExecuteBrushAction(CurrentHoverGroundCoord);
			LastProcessedCoord = CurrentHoverGroundCoord;
		}
	}
	else if (CurrentBrushMode == ETimberBrushMode::None)
	{
		// Click ra ngoài khoảng không vũ trụ -> Hủy chọn công trình
		DeselectBuilding();
	}
}

void ATimberPlayerController::OnLeftMouseUp()
{
	bIsLeftMouseDown = false;
	LastProcessedCoord = FIntVector(-999, -999, -999);

	if (CurrentBrushMode == ETimberBrushMode::PaintPath && bIsDraggingPath)
	{
		ExecuteBatchBuildPath(CachedDragPathCoords);
		bIsDraggingPath = false;
		CachedDragPathCoords.Empty();
	}
}

void ATimberPlayerController::OnRightMouseDown()
{
	bIsRightMousePressed = true;
	bHasRightMouseDragged = false;

	// Lưu tọa độ chuột bắt đầu nhấn
	float MouseX = 0.0f, MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		InitialRightClickPos = FVector2D(MouseX, MouseY);
		LastMouseDragPos = InitialRightClickPos;
	}
}

void ATimberPlayerController::OnRightMouseUp()
{
	bIsRightMousePressed = false;

	// Nếu người chơi KHÔNG kéo chuột (chỉ click nhanh chuột phải) -> Thực hiện Hủy chọn công cụ như cũ!
	if (!bHasRightMouseDragged)
	{
		if (bIsDraggingPath)
		{
			bIsDraggingPath = false;
			CachedDragPathCoords.Empty();
			UE_LOG(LogTemp, Log, TEXT("[BRUSH] Đã hủy thao tác kéo lát đường."));
		}
		else
		{
			SelectTool_Deselect();
		}
	}

	bHasRightMouseDragged = false;
}

void ATimberPlayerController::OnRotateKeyPressed()
{
	if (CurrentBrushMode == ETimberBrushMode::PlaceBuilding)
	{
		RotateBuildingClockwise();
	}
}

void ATimberPlayerController::CalculateDragPathCoords(const FIntVector& StartGroundCoord, const FIntVector& EndGroundCoord, TArray<FIntVector>& OutPathCoords) const
{
	OutPathCoords.Empty();

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	const int32 StartX = StartGroundCoord.X;
	const int32 StartY = StartGroundCoord.Y;
	const int32 EndX = EndGroundCoord.X;
	const int32 EndY = EndGroundCoord.Y;

	const int32 StepX = (EndX >= StartX) ? 1 : -1;
	const int32 StepY = (EndY >= StartY) ? 1 : -1;

	// 1. Chạy dọc trục X từ StartX -> EndX tại StartY
	for (int32 X = StartX; X != EndX + StepX; X += StepX)
	{
		FIntVector GroundCoord;
		if (Grid->GetTopSolidGridCoordAt(X, StartY, GroundCoord))
		{
			const FIntVector PathCoord = FIntVector(X, StartY, GroundCoord.Z + 1);
			OutPathCoords.AddUnique(PathCoord);
		}
	}

	// 2. Chạy dọc trục Y từ StartY -> EndY tại EndX
	for (int32 Y = StartY; Y != EndY + StepY; Y += StepY)
	{
		FIntVector GroundCoord;
		if (Grid->GetTopSolidGridCoordAt(EndX, Y, GroundCoord))
		{
			const FIntVector PathCoord = FIntVector(EndX, Y, GroundCoord.Z + 1);
			OutPathCoords.AddUnique(PathCoord);
		}
	}
}

void ATimberPlayerController::ExecuteBatchBuildPath(const TArray<FIntVector>& PathCoords)
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid || PathCoords.Num() == 0)
	{
		return;
	}

	int32 BuiltCount = 0;
	for (const FIntVector& PathCoord : PathCoords)
	{
		if (!Grid->HasPathAt(PathCoord) && Grid->IsCellEmptyForBuilding(PathCoord))
		{
			if (Grid->BuildPath(PathCoord))
			{
				BuiltCount++;
			}
		}
	}

	if (BuiltCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[BRUSH] Kéo thả thành công! Đã xây dựng đồng loạt %d ô đường đi."), BuiltCount);
	}
}

void ATimberPlayerController::ExecuteBrushAction(const FIntVector& TargetCoord)
{
	switch (CurrentBrushMode)
	{
	case ETimberBrushMode::None:
		{
			// Ở chế độ con trỏ tự do: Click chuột trái vào công trình để mở bảng Inspector
			if (ATimberGridManager* Grid = GetGridManager())
			{
				const FIntVector SpaceCoord = FIntVector(TargetCoord.X, TargetCoord.Y, TargetCoord.Z + 1);
				ATimberBuildingBase* HitBuilding = Grid->GetBuildingAt(SpaceCoord);
				if (!HitBuilding)
				{
					HitBuilding = Grid->GetBuildingAt(TargetCoord);
				}

				if (HitBuilding)
				{
					SelectBuilding(HitBuilding);
				}
				else
				{
					DeselectBuilding();
				}
			}
		}
		break;
	case ETimberBrushMode::PaintPath:
		ExecutePaintPath(TargetCoord);
		break;
	case ETimberBrushMode::Demolish:
		ExecuteDemolish(TargetCoord);
		break;
	case ETimberBrushMode::PlaceBuilding:
		ExecutePlaceBuilding(TargetCoord);
		break;
	default:
		break;
	}
}

void ATimberPlayerController::ExecutePaintPath(const FIntVector& TargetCoord)
{
	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	// Đường đi DirtPath được xây ngay trên bề mặt của khối đất đặc (Z + 1)
	const FIntVector PathCoord = FIntVector(TargetCoord.X, TargetCoord.Y, TargetCoord.Z + 1);
	if (Grid->HasPathAt(PathCoord))
	{
		// Đã có đường rồi -> Tuyệt đối không xây đè bậc thang lên trời
		return;
	}

	if (Grid->BuildPath(PathCoord))
	{
		UE_LOG(LogTemp, Log, TEXT("[BRUSH] Lát thành công 1 ô đường tại (%d, %d, %d)."), PathCoord.X, PathCoord.Y, PathCoord.Z);
	}
}

void ATimberPlayerController::ExecuteDemolish(const FIntVector& TargetCoord)
{
	ATimberGridManager* Grid = GetGridManager();
	UWorld* World = GetWorld();
	if (!Grid || !World)
	{
		return;
	}

	// Bắn tia Raycast để lấy chính xác vị trí 3D dưới con trỏ chuột
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	if (HologramPreviewActor)
	{
		QueryParams.AddIgnoredActor(HologramPreviewActor);
	}

	FVector HitLocation = FVector::ZeroVector;
	FVector HitNormal = FVector::UpVector;
	FVector RayStart = FVector::ZeroVector;

	if (APlayerCameraManager* CamMgr = PlayerCameraManager)
	{
		RayStart = CamMgr->GetCameraLocation();
	}

	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		HitLocation = HitResult.Location;
		HitNormal = HitResult.ImpactNormal;
	}

	// Tọa độ tính toán trực tiếp từ điểm va chạm 3D (không qua ép tầng Z đất)
	const FVector AdjustedPos = HitLocation - HitNormal * 1.0f;
	const FIntVector ExactHitCoord = Grid->WorldLocationToGridCoord(AdjustedPos);
	const FIntVector SpaceCoord = FIntVector(TargetCoord.X, TargetCoord.Y, TargetCoord.Z + 1);

	// 1. VẼ VISUAL DEBUG TRÊN VIEWPORT (TỒN TẠI TRONG 5.0 GIÂY)
	// - Tia Raycast từ camera xuống mặt đất (Màu Vàng)
	DrawDebugLine(World, RayStart, HitLocation, FColor::Yellow, false, 5.0f, 0, 2.0f);
	// - Điểm chạm 3D của chuột (Cầu màu Đỏ)
	DrawDebugSphere(World, HitLocation, 8.0f, 12, FColor::Red, false, 5.0f);

	// - Khung hộp ô tính toán được từ Raycast (Màu Xanh Dương)
	const FVector CalcBoxCenter = Grid->GridCoordToWorldLocation(ExactHitCoord, true);
	DrawDebugBox(World, CalcBoxCenter, FVector(49.0f, 49.0f, 49.0f), FColor::Cyan, false, 5.0f, 0, 2.5f);
	DrawDebugString(World, CalcBoxCenter + FVector(0, 0, 55.0f), 
		FString::Printf(TEXT("📍 HIT: [%d,%d,%d]"), ExactHitCoord.X, ExactHitCoord.Y, ExactHitCoord.Z), 
		nullptr, FColor::Cyan, 5.0f, true, 1.1f);

	// 2. TÌM KIẾM CÔNG TRÌNH ĐỂ XÓA (Ưu tiên ExactHitCoord -> SpaceCoord -> TargetCoord)
	ATimberBuildingBase* Building = Grid->GetBuildingAt(ExactHitCoord);
	if (!Building)
	{
		Building = Grid->GetBuildingAt(SpaceCoord);
	}
	if (!Building)
	{
		Building = Grid->GetBuildingAt(TargetCoord);
	}

	if (Building)
	{
		// Kiểm tra quyền hạn phá hủy công trình
		if (!Building->bCanBeDemolished)
		{
			const FVector BuildingCenter = Building->GetActorLocation() + FVector(0, 0, 50.0f);
			DrawDebugBox(World, BuildingCenter, FVector(100.0f, 100.0f, 60.0f), FColor(255, 165, 0), false, 3.0f, 0, 4.0f);
			DrawDebugString(World, BuildingCenter + FVector(0, 0, 80.0f), 
				FString::Printf(TEXT("⛔ KHÔNG THỂ PHÁ HỦY: %s"), *Building->BuildingName), 
				nullptr, FColor(255, 165, 0), 3.0f, true, 1.3f);

			UE_LOG(LogTemp, Warning, TEXT("==================== [DEMOLISH TRACE: TỪ CHỐI] ===================="));
			UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click]: HitPos=(%.1f, %.1f, %.1f) | ExactCoord=[%d, %d, %d]"), HitLocation.X, HitLocation.Y, HitLocation.Z, ExactHitCoord.X, ExactHitCoord.Y, ExactHitCoord.Z);
			UE_LOG(LogTemp, Warning, TEXT("⛔ [Công trình]: [%s] là công trình đặc biệt được bảo vệ!"), *Building->BuildingName);
			UE_LOG(LogTemp, Warning, TEXT("==================================================================="));
			return;
		}

		const FString Name = Building->BuildingName;
		const TArray<FIntVector> OccupiedCoords = Building->GetOccupiedGridCoords();
		FString CoordsStr;

		// Vẽ hộp Đỏ Rực tại toàn bộ các ô vừa được giải phóng
		for (const FIntVector& CellCoord : OccupiedCoords)
		{
			const FVector TileCenter = Grid->GridCoordToWorldLocation(CellCoord, true);
			DrawDebugBox(World, TileCenter, FVector(48.0f, 48.0f, 48.0f), FColor::Red, false, 5.0f, 0, 4.0f);
			CoordsStr += FString::Printf(TEXT("[%d,%d,%d] "), CellCoord.X, CellCoord.Y, CellCoord.Z);
		}

		Grid->UnregisterBuilding(Building);
		Building->Destroy();

		UE_LOG(LogTemp, Warning, TEXT("==================== [DEMOLISH TRACE: XÓA CÔNG TRÌNH] ===================="));
		UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click 3D]: HitPos=(X=%.1f, Y=%.1f, Z=%.1f) | Normal=(%.2f, %.2f, %.2f)"), HitLocation.X, HitLocation.Y, HitLocation.Z, HitNormal.X, HitNormal.Y, HitNormal.Z);
		UE_LOG(LogTemp, Warning, TEXT("📍 [Tọa độ Grid Tính Được]: ExactHitCoord=[%d, %d, %d] | GroundCoord=[%d, %d, %d]"), ExactHitCoord.X, ExactHitCoord.Y, ExactHitCoord.Z, TargetCoord.X, TargetCoord.Y, TargetCoord.Z);
		UE_LOG(LogTemp, Warning, TEXT("💥 [Công Trình Bị Xóa]: [%s]"), *Name);
		UE_LOG(LogTemp, Warning, TEXT("📦 [Danh Sách %d Ô Đã Giải Phóng]: %s"), OccupiedCoords.Num(), *CoordsStr);
		UE_LOG(LogTemp, Warning, TEXT("=========================================================================="));
		return;
	}

	// 3. TÌM KIẾM ĐƯỜNG ĐI ĐỂ XÓA (Ưu tiên ExactHitCoord -> SpaceCoord -> TargetCoord)
	FIntVector PathCoordToDel = FIntVector::ZeroValue;
	if (Grid->HasPathAt(ExactHitCoord))
	{
		PathCoordToDel = ExactHitCoord;
	}
	else if (Grid->HasPathAt(SpaceCoord))
	{
		PathCoordToDel = SpaceCoord;
	}
	else if (Grid->HasPathAt(TargetCoord))
	{
		PathCoordToDel = TargetCoord;
	}

	if (PathCoordToDel != FIntVector::ZeroValue)
	{
		Grid->RemovePath(PathCoordToDel);

		// Vẽ hộp Đỏ Rực tại đúng ô đường bị xóa
		const FVector TileCenter = Grid->GridCoordToWorldLocation(PathCoordToDel, true);
		DrawDebugBox(World, TileCenter, FVector(48.0f, 48.0f, 48.0f), FColor::Red, false, 5.0f, 0, 4.0f);
		DrawDebugString(World, TileCenter + FVector(0, 0, 30.0f), 
			FString::Printf(TEXT("💥 DELETED: [%d,%d,%d]"), PathCoordToDel.X, PathCoordToDel.Y, PathCoordToDel.Z), 
			nullptr, FColor::Red, 5.0f, true, 1.2f);

		UE_LOG(LogTemp, Warning, TEXT("==================== [DEMOLISH TRACE: XÓA ĐƯỜNG] ===================="));
		UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click 3D]: HitPos=(X=%.1f, Y=%.1f, Z=%.1f) | Normal=(%.2f, %.2f, %.2f)"), HitLocation.X, HitLocation.Y, HitLocation.Z, HitNormal.X, HitNormal.Y, HitNormal.Z);
		UE_LOG(LogTemp, Warning, TEXT("📍 [Tọa độ Grid Tính Được]: ExactHitCoord=[%d, %d, %d] | GroundCoord=[%d, %d, %d]"), ExactHitCoord.X, ExactHitCoord.Y, ExactHitCoord.Z, TargetCoord.X, TargetCoord.Y, TargetCoord.Z);
		UE_LOG(LogTemp, Warning, TEXT("💥 [Ô Đường Bị Xóa Thực Tế]: [%d, %d, %d]"), PathCoordToDel.X, PathCoordToDel.Y, PathCoordToDel.Z);
		UE_LOG(LogTemp, Warning, TEXT("====================================================================="));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("==================== [DEMOLISH TRACE: KHÔNG CÓ GÌ] ===================="));
	UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click 3D]: HitPos=(X=%.1f, Y=%.1f, Z=%.1f) | ExactHitCoord=[%d, %d, %d]"), HitLocation.X, HitLocation.Y, HitLocation.Z, ExactHitCoord.X, ExactHitCoord.Y, ExactHitCoord.Z);
	UE_LOG(LogTemp, Warning, TEXT("⚠️ Không tìm thấy Công trình hay Đường đi nào tại các ô xung quanh để xóa."));
	UE_LOG(LogTemp, Warning, TEXT("========================================================================"));
}

void ATimberPlayerController::ExecutePlaceBuilding(const FIntVector& GroundCoord)
{
	if (!SelectedBuildingClass || !bIsCurrentHoverValid)
	{
		return;
	}

	ATimberGridManager* Grid = GetGridManager();
	UWorld* World = GetWorld();
	if (!Grid || !World)
	{
		return;
	}

	const ATimberBuildingBase* DefaultBuilding = SelectedBuildingClass->GetDefaultObject<ATimberBuildingBase>();
	const FIntPoint Footprint = DefaultBuilding ? DefaultBuilding->FootprintSize : FIntPoint(1, 1);
	int32 SizeX = Footprint.X;
	int32 SizeY = Footprint.Y;
	if (BuildingRotationAngle == 90 || BuildingRotationAngle == 270)
	{
		SizeX = Footprint.Y;
		SizeY = Footprint.X;
	}

	const FIntVector BuildingOrigin = FIntVector(GroundCoord.X, GroundCoord.Y, GroundCoord.Z + 1);
	const FVector CornerLocation = Grid->GridCoordToWorldLocation(BuildingOrigin, false);
	// Căn giữa móng công trình theo kích thước NxM ngay tại bề mặt sàn
	const FVector SpawnWorldLocation = CornerLocation + FVector(SizeX * 50.0f, SizeY * 50.0f, 0.0f);
	const FRotator SpawnRotation = FRotator(0.0f, static_cast<float>(BuildingRotationAngle), 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATimberBuildingBase* NewBuilding = World->SpawnActor<ATimberBuildingBase>(SelectedBuildingClass, SpawnWorldLocation, SpawnRotation, SpawnParams);
	if (NewBuilding)
	{
		NewBuilding->OriginGridCoord = BuildingOrigin;
		
		// Đặt công trình về trạng thái Móng Giàn Giáo (UnderConstruction) sẵn sàng nhận gỗ thi công
		NewBuilding->SetBuildingState(EBuildingState::UnderConstruction);
		NewBuilding->CurrentWoodDelivered = 0;
		NewBuilding->CurrentBuildProgress = 0.0f;

		// Đăng ký công trình vào GridManager
		Grid->RegisterBuilding(NewBuilding);

		UE_LOG(LogTemp, Warning, TEXT("[BRUSH] Đã đặt móng công trình [%s] tại (%d, %d, %d)!"),
			*NewBuilding->BuildingName, BuildingOrigin.X, BuildingOrigin.Y, BuildingOrigin.Z);
	}
}

void ATimberPlayerController::SpawnHologramPreview()
{
	ClearHologramPreview();

	if (!SelectedBuildingClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	HologramPreviewActor = World->SpawnActor<ATimberBuildingBase>(SelectedBuildingClass, FVector(0, 0, -10000), FRotator::ZeroRotator, SpawnParams);
	if (HologramPreviewActor)
	{
		HologramPreviewActor->bIsHologramPreview = true;
		HologramPreviewActor->SetBuildingState(EBuildingState::Ghost_Valid);
		HologramPreviewActor->SetActorEnableCollision(false);
		HologramPreviewActor->SetActorHiddenInGame(false);
	}
}

void ATimberPlayerController::UpdateHologramPreview(const FIntVector& GroundCoord, bool bIsValid)
{
	if (!HologramPreviewActor)
	{
		SpawnHologramPreview();
	}

	ATimberGridManager* Grid = GetGridManager();
	if (!Grid)
	{
		return;
	}

	const ATimberBuildingBase* DefaultBuilding = SelectedBuildingClass ? SelectedBuildingClass->GetDefaultObject<ATimberBuildingBase>() : nullptr;
	const FIntPoint Footprint = DefaultBuilding ? DefaultBuilding->FootprintSize : FIntPoint(1, 1);
	int32 SizeX = Footprint.X;
	int32 SizeY = Footprint.Y;
	if (BuildingRotationAngle == 90 || BuildingRotationAngle == 270)
	{
		SizeX = Footprint.Y;
		SizeY = Footprint.X;
	}

	const FIntVector BuildingOrigin = FIntVector(GroundCoord.X, GroundCoord.Y, GroundCoord.Z + 1);
	const FVector CornerLocation = Grid->GridCoordToWorldLocation(BuildingOrigin, false);
	const FVector TargetLocation = CornerLocation + FVector(SizeX * 50.0f, SizeY * 50.0f, 0.0f);
	const FRotator TargetRotation = FRotator(0.0f, static_cast<float>(BuildingRotationAngle), 0.0f);

	if (HologramPreviewActor)
	{
		HologramPreviewActor->SetActorLocation(TargetLocation);
		HologramPreviewActor->SetActorRotation(TargetRotation);
		HologramPreviewActor->SetActorHiddenInGame(false);

		// Đổi màu Hologram: Xanh nếu hợp lệ, Đỏ nếu không hợp lệ
		const EBuildingState DesiredState = bIsValid ? EBuildingState::Ghost_Valid : EBuildingState::Ghost_Invalid;
		if (HologramPreviewActor->BuildingState != DesiredState)
		{
			HologramPreviewActor->SetBuildingState(DesiredState);
		}
	}

	// Vẽ thêm khung Footprint 3D phát sáng bao quanh chân móng để nhìn rõ 100% trong mọi góc camera
	if (SelectedBuildingClass)
	{
		const FVector BoxCenter = TargetLocation + FVector(0.0f, 0.0f, -40.0f);
		const FVector BoxExtent = FVector(SizeX * 50.0f, SizeY * 50.0f, 6.0f);
		
			DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, bIsValid ? FColor::Green : FColor::Red, false, 0.05f, 0, 3.0f);
	}
}

void ATimberPlayerController::ClearHologramPreview()
{
	if (HologramPreviewActor)
	{
		HologramPreviewActor->Destroy();
		HologramPreviewActor = nullptr;
	}
}

void ATimberPlayerController::DebugBeavers(int32 Level)
{
	TArray<AActor*> FoundBeavers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABeaverAgent::StaticClass(), FoundBeavers);

	for (AActor* Actor : FoundBeavers)
	{
		if (ABeaverAgent* Beaver = Cast<ABeaverAgent>(Actor))
		{
			Beaver->SetDebugLevel(Level);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[BEAVER DEBUG] Đã chuyển DebugLevel = %d cho toàn bộ %d chú Hải ly trong Level!"), Level, FoundBeavers.Num());
}

void ATimberPlayerController::SelectBuilding(ATimberBuildingBase* Building)
{
	if (SelectedBuildingActor.IsValid())
	{
		SelectedBuildingActor->SetWorkAreaVisible(false);
	}

	SelectedBuildingActor = Building;

	if (SelectedBuildingActor.IsValid())
	{
		// Cập nhật lại toàn bộ trạng thái kết nối đường đi tức thời để UI hiển thị chính xác 100%
		if (ATimberGridManager* Grid = GetGridManager())
		{
			Grid->UpdateAllBuildingsConnectionStatus();
		}

		SelectedBuildingActor->SetWorkAreaVisible(true);

		// Tạo hoặc hiển thị Widget Building Inspector nếu có cấu hình Class
		if (BuildingInspectorWidgetClass && !BuildingInspectorWidgetInstance)
		{
			BuildingInspectorWidgetInstance = CreateWidget<UUserWidget>(this, BuildingInspectorWidgetClass);
			if (BuildingInspectorWidgetInstance)
			{
				BuildingInspectorWidgetInstance->AddToViewport(15);
			}
		}

		if (BuildingInspectorWidgetInstance)
		{
			BuildingInspectorWidgetInstance->SetVisibility(ESlateVisibility::Visible);
		}

		UE_LOG(LogTemp, Log, TEXT("🔍 [INSPECTOR] Đã chọn công trình: '%s' (Đã kết nối đường: %s)"),
			*SelectedBuildingActor->BuildingName,
			SelectedBuildingActor->bIsConnectedToDistrict ? TEXT("CÓ (True)") : TEXT("KHÔNG (False)"));
	}
	else
	{
		DeselectBuilding();
	}
}

void ATimberPlayerController::DeselectBuilding()
{
	if (SelectedBuildingActor.IsValid())
	{
		SelectedBuildingActor->SetWorkAreaVisible(false);
		SelectedBuildingActor = nullptr;
	}

	if (BuildingInspectorWidgetInstance)
	{
		BuildingInspectorWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool ATimberPlayerController::AddWorkerToSelectedBuilding()
{
	if (!SelectedBuildingActor.IsValid() || !SelectedBuildingActor->IsWorkplace())
	{
		return false;
	}

	// Tìm 1 chú Hải ly đang Unemployed gần nhất trong Level
	TArray<AActor*> FoundBeavers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABeaverAgent::StaticClass(), FoundBeavers);

	for (AActor* Actor : FoundBeavers)
	{
		if (ABeaverAgent* Beaver = Cast<ABeaverAgent>(Actor))
		{
			if (Beaver->CurrentProfession == EBeaverProfession::Unemployed)
			{
				return SelectedBuildingActor->AddWorker(Beaver);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("⚠️ [NO UNEMPLOYED] Không còn Hải ly nào đang rảnh rỗi (Unemployed) để tuyển dụng!"));
	return false;
}

bool ATimberPlayerController::RemoveWorkerFromSelectedBuilding()
{
	if (!SelectedBuildingActor.IsValid() || !SelectedBuildingActor->IsWorkplace())
	{
		return false;
	}

	return SelectedBuildingActor->RemoveWorker();
}

void ATimberPlayerController::UpdateMasterHUDStats()
{
	if (!MasterHUDWidgetInstance || !GetWorld())
	{
		return;
	}

	// 1. TÍNH TỔNG SỐ GỖ VÀ SỨC CHỨA KHO TOÀN ĐỊNH CƯ
	int32 TotalWoodStock = 0;
	int32 TotalWoodCapacity = 0;

	// Quét Nhà Chính
	TArray<AActor*> DistrictCenters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATimberDistrictCenter::StaticClass(), DistrictCenters);
	for (AActor* Actor : DistrictCenters)
	{
		if (ATimberDistrictCenter* DC = Cast<ATimberDistrictCenter>(Actor))
		{
			TotalWoodStock += DC->CurrentWoodStock;
			TotalWoodCapacity += DC->MaxWoodStorage;
		}
	}

	// Quét các Kho Lưu Trữ
	TArray<AActor*> StorageBuildings;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATimberStorage::StaticClass(), StorageBuildings);
	for (AActor* Actor : StorageBuildings)
	{
		if (ATimberStorage* Store = Cast<ATimberStorage>(Actor))
		{
			TotalWoodStock += Store->StoredWood;
			TotalWoodCapacity += Store->MaxCapacity;
		}
	}

	// Đẩy thống kê gỗ xuống Master HUD
	MasterHUDWidgetInstance->UpdateResourceDisplay(TotalWoodStock, TotalWoodCapacity);

	// 2. TÍNH THỐNG KÊ DÂN SỐ & VIỆC LÀM HẢI LY
	TArray<AActor*> AllBeavers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABeaverAgent::StaticClass(), AllBeavers);

	const int32 TotalBeaverCount = AllBeavers.Num();
	int32 EmployedCount = 0;

	for (AActor* Actor : AllBeavers)
	{
		if (ABeaverAgent* Beaver = Cast<ABeaverAgent>(Actor))
		{
			if (Beaver->CurrentProfession != EBeaverProfession::Unemployed)
			{
				EmployedCount++;
			}
		}
	}

	// Giới hạn dân số mặc định theo Nhà chính (10 Hải ly)
	const int32 MaxPopCap = (DistrictCenters.Num() > 0) ? 10 : 0;

	// Đẩy thống kê dân số xuống Master HUD
	MasterHUDWidgetInstance->UpdatePopulationDisplay(TotalBeaverCount, EmployedCount, MaxPopCap);

	// Đẩy trạng thái công cụ cọ vẽ hiện tại (để sáng viền nút)
	MasterHUDWidgetInstance->UpdateToolDisplay(static_cast<uint8>(CurrentBrushMode));
}
