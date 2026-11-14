// Fill out your copyright notice in the Description page of Project Settings.


#include "HideAndSeekAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "GameFramework/Pawn.h"

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

void AHideAndSeekAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StateTreeComponent->StopLogic(TEXT("Controller EndPlay"));

	UnbindPawnHit();

	Super::EndPlay(EndPlayReason);
}

void AHideAndSeekAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UnbindPawnHit();

	if (APawn* ControlledPawn = GetPawn())
	{
		ObservedPawn = ControlledPawn;

		ControlledPawn->OnActorHit.AddUniqueDynamic(this, &AHideAndSeekAIController::HandleControlledPawnHit);
	}

	TryStartStateTree();
}

void AHideAndSeekAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("Unpossessed"));

	UnbindPawnHit();

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

void AHideAndSeekAIController::HandleControlledPawnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse,
                                                       const FHitResult& Hit)
{
	if (SelfActor == GetPawn() && IsValid(OtherActor))
	{
		OnControlledPawnHit.Broadcast(OtherActor);
	}
}

void AHideAndSeekAIController::UnbindPawnHit()
{
	if (ObservedPawn.IsValid())
	{
		ObservedPawn.Get()->OnActorHit.RemoveDynamic(this, &AHideAndSeekAIController::HandleControlledPawnHit);
	}

	ObservedPawn.Reset();
}
