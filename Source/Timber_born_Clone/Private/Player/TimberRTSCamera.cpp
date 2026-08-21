#include "Timber_born_Clone/Public/Player/TimberRTSCamera.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Timber_born_Clone/Public/Grid/TimberGridManager.h"
#include "Kismet/GameplayStatics.h"

ATimberRTSCamera::ATimberRTSCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Root Scene Component nằm trên mặt đất
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 2. Spring Arm (Cần cẩu camera)
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(SceneRoot);
	SpringArmComp->SetRelativeRotation(FRotator(TargetPitch, TargetYaw, 0.0f));
	SpringArmComp->TargetArmLength = TargetArmLength;
	SpringArmComp->bDoCollisionTest = false; // Tắt va chạm để camera bay xuyên núi mượt mà
	SpringArmComp->bInheritPitch = false;
	SpringArmComp->bInheritRoll = false;
	SpringArmComp->bInheritYaw = false;

	// 3. Camera Component
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// Tự động nhận Player 0 điều khiển
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ATimberRTSCamera::BeginPlay()
{
	Super::BeginPlay();

	TargetLocation = GetActorLocation();
	TargetArmLength = SpringArmComp->TargetArmLength;
	TargetYaw = SpringArmComp->GetRelativeRotation().Yaw;
	TargetPitch = SpringArmComp->GetRelativeRotation().Pitch;

	// Tìm GridManager trong Level
	GridManager = Cast<ATimberGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATimberGridManager::StaticClass()));
}

void ATimberRTSCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (PlayerInputComponent)
	{
		// Di chuyển trục Axis (Nếu người dùng cấu hình Axis Mappings trong Project Settings)
		PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATimberRTSCamera::MoveForwardInput);
		PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATimberRTSCamera::MoveRightInput);

		// Xoay Camera Q / E
		PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ATimberRTSCamera::RotateLeft);
		PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ATimberRTSCamera::RotateRight);

		// Cuộn chuột Thu Phóng (Zoom)
		PlayerInputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ATimberRTSCamera::ZoomIn);
		PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ATimberRTSCamera::ZoomOut);
	}
}

void ATimberRTSCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Xử lý di chuyển liên tục khi đè phím WASD
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		float ForwardVal = 0.0f;
		float RightVal = 0.0f;

		if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up)) ForwardVal += 1.0f;
		if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down)) ForwardVal -= 1.0f;
		if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) RightVal += 1.0f;
		if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left)) RightVal -= 1.0f;

		if (ForwardVal != 0.0f) MoveForwardInput(ForwardVal * DeltaTime);
		if (RightVal != 0.0f) MoveRightInput(RightVal * DeltaTime);

		// Xoay liên tục khi đè Q / E
		if (PC->IsInputKeyDown(EKeys::Q)) RotateYawInput(-1.0f * DeltaTime);
		if (PC->IsInputKeyDown(EKeys::E)) RotateYawInput(1.0f * DeltaTime);
	}

	UpdateCameraMovement(DeltaTime);
}

void ATimberRTSCamera::MoveForwardInput(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	// Di chuyển theo hướng nhìn ngang hiện tại của SpringArm (bỏ qua thành phần Z)
	const FRotator YawRotation(0.0f, TargetYaw, 0.0f);
	const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	TargetLocation += ForwardDir * Value * PanSpeed;
	if (bClampToGridBounds) ClampCameraLocation();
}

void ATimberRTSCamera::MoveRightInput(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	// Di chuyển sang phải theo hướng ngang hiện tại của SpringArm
	const FRotator YawRotation(0.0f, TargetYaw, 0.0f);
	const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	TargetLocation += RightDir * Value * PanSpeed;
	if (bClampToGridBounds) ClampCameraLocation();
}

void ATimberRTSCamera::RotateYawInput(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;
	TargetYaw += Value * OrbitSpeed;
}

void ATimberRTSCamera::RotateLeft()
{
	TargetYaw -= 45.0f; // Xoay 45 độ mỗi lần nhấn phím Q
}

void ATimberRTSCamera::RotateRight()
{
	TargetYaw += 45.0f; // Xoay 45 độ mỗi lần nhấn phím E
}

void ATimberRTSCamera::AddPanDelta(const FVector& WorldDelta)
{
	const float PreservedZ = TargetLocation.Z;
	TargetLocation += FVector(WorldDelta.X, WorldDelta.Y, 0.0f);
	TargetLocation.Z = PreservedZ; // Khóa cứng độ cao Z khi Pan
	if (bClampToGridBounds)
	{
		ClampCameraLocation();
	}
}

void ATimberRTSCamera::AddCameraRotationDelta(float YawDelta, float PitchDelta)
{
	TargetYaw += YawDelta * MouseLookSensitivity;
	TargetPitch = FMath::Clamp(TargetPitch + (PitchDelta * MouseLookSensitivity), MinPitch, MaxPitch);
}

void ATimberRTSCamera::ZoomIn()
{
	TargetArmLength = FMath::Clamp(TargetArmLength - ZoomSpeed, MinZoomDistance, MaxZoomDistance);
}

void ATimberRTSCamera::ZoomOut()
{
	TargetArmLength = FMath::Clamp(TargetArmLength + ZoomSpeed, MinZoomDistance, MaxZoomDistance);
}

void ATimberRTSCamera::UpdateCameraMovement(float DeltaTime)
{
	// 1. Nội suy vị trí di chuyển mượt mà (VInterpTo)
	const FVector CurrentLoc = GetActorLocation();
	const FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, SmoothingFactor);
	SetActorLocation(NewLoc);

	// 2. Nội suy khoảng cách Zoom mượt mà (FInterpTo)
	if (SpringArmComp)
	{
		const float CurrentArmLength = SpringArmComp->TargetArmLength;
		SpringArmComp->TargetArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, SmoothingFactor);

		// 3. Nội suy góc xoay Yaw & Pitch mượt mà (RInterpTo)
		const FRotator CurrentRot = SpringArmComp->GetRelativeRotation();
		const FRotator DesiredRot(TargetPitch, TargetYaw, 0.0f);
		const FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaTime, SmoothingFactor);
		SpringArmComp->SetRelativeRotation(NewRot);
	}
}

void ATimberRTSCamera::ClampCameraLocation()
{
	if (!GridManager)
	{
		GridManager = Cast<ATimberGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATimberGridManager::StaticClass()));
	}

	if (GridManager)
	{
		// Giới hạn camera trong phạm vi bản đồ + mở rộng biên 500cm
		const float MinX = -500.0f;
		const float MaxX = (GridManager->GridSizeX * GridManager->CellSize) + 500.0f;
		const float MinY = -500.0f;
		const float MaxY = (GridManager->GridSizeY * GridManager->CellSize) + 500.0f;

		TargetLocation.X = FMath::Clamp(TargetLocation.X, MinX, MaxX);
		TargetLocation.Y = FMath::Clamp(TargetLocation.Y, MinY, MaxY);
	}
}
