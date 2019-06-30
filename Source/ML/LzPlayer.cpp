// Fill out your copyright notice in the Description page of Project Settings.


#include "LzPlayer.h"
#include "Components/InputComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "FileHelper.h"

// Sets default values
ALzPlayer::ALzPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera");
	CameraComponent->SetupAttachment(GetRootComponent());
	SceneCapture = CreateDefaultSubobject< USceneCaptureComponent2D>("SceneCapture");
	SceneCapture->SetupAttachment(CameraComponent);
}

// Called when the game starts or when spawned
void ALzPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->InitAutoFormat(1280, 720);

	OutBMP.AddUninitialized(RenderTarget->GetSurfaceWidth() * RenderTarget->GetSurfaceHeight());
	for (FColor& color : OutBMP)
	{
		color.A = 255;
	}

	SceneCapture->TextureTarget = RenderTarget;
}

// Called every frame
void ALzPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ALzPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Forward", this, &ALzPlayer::OnInputForward);
	PlayerInputComponent->BindAxis("Backward", this, &ALzPlayer::OnInputForward);
	PlayerInputComponent->BindAxis("Left", this, &ALzPlayer::OnInputRight);
	PlayerInputComponent->BindAxis("Right", this, &ALzPlayer::OnInputRight);

	PlayerInputComponent->BindAction("Test", IE_Pressed, this, &ALzPlayer::OnInputOne);
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

void ALzPlayer::OnInputOne()
{
	SceneCapture->CaptureScene();

	FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);

	Resource->ReadPixels(OutBMP, ReadPixelFlags);

	FIntPoint DestSize(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight());
	TArray<uint8> CompressedBitmap;
	FImageUtils::CompressImageArray(DestSize.X, DestSize.Y, OutBMP, CompressedBitmap);

	FString fullPath = FPaths::Combine(TEXT(""), TEXT("lzCapture.bmp"));

	bool imageSavedOK = FFileHelper::SaveArrayToFile(CompressedBitmap, *fullPath);
}