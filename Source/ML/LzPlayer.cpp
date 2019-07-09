// Fill out your copyright notice in the Description page of Project Settings.


#include "LzPlayer.h"
#include "Components/InputComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "FileHelper.h"
#include "TensorFlowComponent.h"
#include "LzUtil.h"

// Sets default values
ALzPlayer::ALzPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera");
	CameraComponent->SetupAttachment(GetRootComponent());
	SceneCapture = CreateDefaultSubobject< USceneCaptureComponent2D>("SceneCapture");
	SceneCapture->SetupAttachment(CameraComponent);

	TensorFlowComponent = CreateDefaultSubobject<UTensorFlowComponent>("TensorFlowComponent");

	int idx = 0;
	DirVecList[idx++] = FVector::ZeroVector;
	DirVecList[idx++] = FVector(1, 0, 0);
	DirVecList[idx++] = FVector(-1, 0, 0);
	DirVecList[idx++] = FVector(0, 1, 0);
	DirVecList[idx++] = FVector(0, -1, 0);
	DirVecList[idx++] = FVector(1, 1, 0);
	DirVecList[idx++] = FVector(-1, 1, 0);
	DirVecList[idx++] = FVector(1, -1, 0);
	DirVecList[idx++] = FVector(-1, -1, 0);

	
}

// Called when the game starts or when spawned
void ALzPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->InitAutoFormat(1280, 720);
	RenderTarget->bGPUSharedFlag = true;

	StateWidth = RenderTarget->GetSurfaceWidth() / 20;
	StateHeight = RenderTarget->GetSurfaceHeight() / 20;

	OutBMP.AddUninitialized(RenderTarget->GetSurfaceWidth() * RenderTarget->GetSurfaceHeight());
	for (FColor& color : OutBMP)
	{
		color.A = 255;
	}

	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->bCaptureEveryFrame = false;

	TensorFlowComponent->PythonModule = TEXT("TestModule");
	TensorFlowComponent->PythonClass = TEXT("Main");
	TensorFlowComponent->InitializePythonComponent();

	Destination = FVector(-10, 48260, lastLoc.Z);

	TensorFlowComponent->CallPythonComponentMethodMLSetup(TEXT("Setup"), StateHeight * StateWidth * 3 + 5, (int)ELzDirection::Max, 100);
	OnInputOne(0.0f);
	TensorFlowComponent->CallPythonComponentMethodMLRunBegin(TEXT("RunBegin"), OutBMP2, 0, GetActorLocation(), (Destination - GetActorLocation()).Size2D());

	lastLoc = GetActorLocation();

	SpawnWalls();
}

void ALzPlayer::SpawnWalls()
{
	for (AActor* CurWall : Walls)
	{
		CurWall->Destroy(true);
	}
	Walls.Empty();

	auto WallClass = LoadObject<UClass>(nullptr, TEXT("Blueprint'/Game/Blueprints/Wall.Wall_C'"));
	for (int i = 0; i < 60; i++)
	{
		int XRand = FMath::Rand() % 4000;
		AActor* Wall = GetWorld()->SpawnActor<AActor>(WallClass, FVector(0, -48320, 112) + FVector(XRand - 2000, (i + 1) * 1500, -100), FRotator::ZeroRotator);
		int ScaleRand = FMath::Rand() % 5;
		Wall->SetActorScale3D(FVector(10 + ScaleRand, 1, 3));
		Walls.Add(Wall);
	}
}

// Called every frame
void ALzPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	OnInputOne(DeltaTime);

	float TimeElapsedThreshold = 60 * 5;
	if (nInst < 100)
		TimeElapsedThreshold = 60 * 2;
	else if(nInst < 200)
		TimeElapsedThreshold = 60 * 5;
	else if(nInst < 300)
		TimeElapsedThreshold = 60 * 10;
	else if(nInst < 500)
		TimeElapsedThreshold = 60 * 15;

	if (bEnd || TimeElapsed > TimeElapsedThreshold || SamePositionTime > 10.f)
	{
		nInst++;
		SamePositionTime = 0;
		TimeElapsed = 0.0f;
		//if (nInst < 100)
		{
			OnDoNextGame();
			return;
		}
	}

	if (bEnd == false)
	{
		TimeElapsed += DeltaTime;
		FVector CurLoc = GetActorLocation();

		int ActionIdx = TensorFlowComponent->CallPythonComponentMethodMLGetNextAction(TEXT("GetNextAction"), bTraining);
		Direction = static_cast<ELzDirection>(ActionIdx);

		float LocDiff = (CurLoc - lastLoc).Size2D();
		if (LocDiff < KINDA_SMALL_NUMBER)
			SamePositionTime += DeltaTime;
		else
			SamePositionTime = 0;

		float lastDistToDest = (Destination - lastLoc).Size2D();
		float distToDestination = (Destination - CurLoc).Size2D();
		float reward = (lastDistToDest - distToDestination) * 100 - distToDestination;

		int done = distToDestination < 10.f ? 1 : 0;

		LzUtil::LOG(0.3f, this, TEXT("Action : %d, Reward : %f, Dist : %f"), ActionIdx, reward, distToDestination);

		if (TensorFlowComponent->CallPythonComponentMethodMLRun(TEXT("Run"), distToDestination, OutBMP2, (int)Direction, CurLoc, reward, done))
		{
			bEnd = true;
			Direction = ELzDirection::None;
			return;
		}

		FRotator ControlRotation = GetControlRotation();
		AddMovementInput(DirVecList[(int)Direction].RotateAngleAxis(ControlRotation.Yaw, FVector(0, 0, 1)), 1);

		lastLoc = CurLoc;
	}

	LzUtil::LOG(0.0f, this, TEXT("CurTrainingNum : %d, Location : %s, TimeElapsed : %f, SamePosTime : %f"), nInst + 1, *GetActorLocation().ToString(), TimeElapsed, SamePositionTime);
}

// Called to bind functionality to input
void ALzPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Forward", this, &ALzPlayer::OnInputForward);
	PlayerInputComponent->BindAxis("Backward", this, &ALzPlayer::OnInputForward);
	PlayerInputComponent->BindAxis("Left", this, &ALzPlayer::OnInputRight);
	PlayerInputComponent->BindAxis("Right", this, &ALzPlayer::OnInputRight);

	PlayerInputComponent->BindAction("Test", IE_Pressed, this, &ALzPlayer::SaveCapture);
	PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ALzPlayer::OnInputT);

	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ALzPlayer::OnDoNextGame);

	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &ALzPlayer::OnChangeDirection);
}

void ALzPlayer::OnChangeDirection()
{
	Direction = static_cast<ELzDirection>(((int)Direction + 1) % (int)ELzDirection::Max);
}

void ALzPlayer::OnInputForward(float Value)
{
	if (Value == 0.0f)
		return;

	FRotator ControlRotation = GetControlRotation();
	AddMovementInput(FVector(0, 1, 0).RotateAngleAxis(ControlRotation.Yaw, FVector(0, 0, 1)), Value);
}

void ALzPlayer::OnInputRight(float Value)
{
	if (Value == 0.0f)
		return;

	FRotator ControlRotation = GetControlRotation();
	AddMovementInput(FVector(1, 0, 0).RotateAngleAxis(ControlRotation.Yaw, FVector(0, 0, 1)), Value);
}

void ALzPlayer::OnInputOne(float Delta)
{
	SceneCapture->CaptureScene();

	FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);

	Resource->ReadPixels(OutBMP, ReadPixelFlags);

	for (FColor& color : OutBMP)
	{
		color.A = 255;
	}

	FImageUtils::CropAndScaleImage(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight(), StateWidth, StateHeight, OutBMP, OutBMP2);
}

void ALzPlayer::SaveCapture()
{
	SceneCapture->CaptureScene();

	FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);

	Resource->ReadPixels(OutBMP, ReadPixelFlags);

	FIntPoint DestSize(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight());
	FImageUtils::CompressImageArray(DestSize.X, DestSize.Y, OutBMP, CompressedBitmap);

	FString fullPath = FPaths::Combine(TEXT(""), TEXT("lzCapture.bmp"));

	bool imageSavedOK = FFileHelper::SaveArrayToFile(CompressedBitmap, *fullPath);
}

void ALzPlayer::OnInputT()
{
	SetActorLocation(FVector(0, -48320, GetActorLocation().Z));
	//SpawnWalls();

	bTraining = false;
	bEnd = false;
}

void ALzPlayer::OnDoNextGame()
{
	SetActorLocation(StartPos);
	SpawnWalls();

	OnInputOne(0.0f);
	TensorFlowComponent->CallPythonComponentMethodMLRunBegin(TEXT("RunBegin"), OutBMP2, 0, GetActorLocation(), (Destination - GetActorLocation()).Size2D());

	bTraining = true;
	bEnd = false;
}