// Fill out your copyright notice in the Description page of Project Settings.


#include "HideAndSeekAIController.h"

#include "Components/StateTreeAIComponent.h"

AHideAndSeekAIController::AHideAndSeekAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	
	BrainComponent = StateTreeComponent;
	
	StateTreeComponent->SetStartLogicAutomatically(false);
	
	bStartAILogicOnPossess = false;
	bStopAILogicOnUnposses = true;
}

void AHideAndSeekAIController::BeginPlay()
{
	Super::BeginPlay();
	
	TryStartStateTree();
}

void AHideAndSeekAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	TryStartStateTree();
}

void AHideAndSeekAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("Unpossessed"));
	
	Super::OnUnPossess();
}

void AHideAndSeekAIController::TryStartStateTree()
{
	if (!HasActorBegunPlay() || GetPawn() == nullptr || StateTreeComponent->IsRunning())
	{
		return;
	}
	
	UE_LOG(LogTemp, Display, TEXT("HNS: Starting StateTree on %s"), *GetName());
	
	StateTreeComponent->StartLogic();
}
