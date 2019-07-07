// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LzPlayer.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UCameraComponent;
class UTensorFlowComponent;

UENUM()
enum class ELzDirection
{
	None,
	Left,
	Right,
	Forward,
	Back,
	ForwardLeft,
	ForwardRight,
	BackLeft,
	BackRight,
	Max
};

UCLASS()
class ML_API ALzPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALzPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void OnInputForward(float Value);
	void OnInputRight(float Value);
	void OnInputOne(float Delta);
	void SaveCapture();
	void OnInputT();

	void OnDoNextGame();
	
	void OnChangeDirection();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USceneCaptureComponent2D*	SceneCapture;

	UTextureRenderTarget2D*		RenderTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCameraComponent*			CameraComponent;

	TArray<FColor> OutBMP;
	TArray<FColor> OutBMP2;

	TArray<uint8> CompressedBitmap;

	UTensorFlowComponent* TensorFlowComponent;

	ELzDirection	Direction;

	FVector			DirVecList[(int)ELzDirection::Max];

	float			StateWidth;
	float			StateHeight;

	bool			bEnd = false;
	bool			bTraining = true;

	FVector			lastLoc;

	FVector			Destination;

	int				nInst = 0;
};
