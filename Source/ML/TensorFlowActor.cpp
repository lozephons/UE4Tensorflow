// Fill out your copyright notice in the Description page of Project Settings.


#include "TensorFlowActor.h"
#include "TensorFlowComponent.h"
#include "UnrealEnginePython.h"

// Sets default values
ATensorFlowActor::ATensorFlowActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TensorFlowComponent = CreateDefaultSubobject<UTensorFlowComponent>("TensorFlowComponent");
	
}

// Called when the game starts or when spawned
void ATensorFlowActor::BeginPlay()
{
	Super::BeginPlay();
	
	//FUnrealEnginePythonModule::AddPythonDependentPlugin(TEXT("tensorflow"));

	//TensorFlowComponent->PythonModule = TEXT("TestModule");
	//TensorFlowComponent->PythonClass = TEXT("TestClass");
	//TensorFlowComponent->InitializePythonComponent();

	//TensorFlowComponent->CallPythonComponentMethodString(TEXT("test"), TEXT(""));
}

// Called every frame
void ATensorFlowActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

