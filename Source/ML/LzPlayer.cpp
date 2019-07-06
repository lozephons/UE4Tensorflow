// Fill out your copyright notice in the Description page of Project Settings.


#include "LzPlayer.h"
#include "Components/InputComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "FileHelper.h"
#include "TensorFlowComponent.h"

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
}

// Called every frame
void ALzPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	OnInputOne(DeltaTime);

	FRotator ControlRotation = GetControlRotation();
	AddMovementInput(DirVecList[(int)Direction].RotateAngleAxis(ControlRotation.Yaw, FVector(0, 0, 1)), 1);
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

	float NewWidth = RenderTarget->GetSurfaceWidth() / 10;
	float NewHeight = RenderTarget->GetSurfaceHeight() / 10;
	FImageUtils::CropAndScaleImage(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight(), NewWidth, NewHeight, OutBMP, OutBMP2);

	TensorFlowComponent->CallPythonComponentMethodML(TEXT("test"), OutBMP2);
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
	TensorFlowComponent->CallPythonComponentMethodInt(TEXT("test"), TEXT(""));
}