#include "StateTreeWatchPlayerTouchTask.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HideAndSeekAIController.h"
#include "HideAndSeekGameMode.h"
#include "HideAndSeekGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderTouch, Log, All);

namespace
{
	using FTouchData = FStateTreeWatchPlayerTouchInstanceData;

	APawn* GetSearchingPlayerPawn(AActor* Hider)
	{
		UWorld* World = IsValid(Hider) ? Hider->GetWorld() : nullptr;

		const AHideAndSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideAndSeekGameMode>() : nullptr;

		if (!IsValid(GameMode) || GameMode->GetRoundPhase() != EHideAndSeekRoundPhase::Searching)
		{
			return nullptr;
		}

		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);

		return IsValid(PlayerPawn) && PlayerPawn != Hider ? PlayerPawn : nullptr;
	}

	bool HasExistingPlayerContact(const ACharacter& Character, const APawn& PlayerPawn, const float Tolerance)
	{
		const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();

		const UPrimitiveComponent* PlayerRoot = Cast<UPrimitiveComponent>(PlayerPawn.GetRootComponent());

		UWorld* World = Character.GetWorld();

		if (!World || !IsValid(Capsule) || !IsValid(PlayerRoot)
			|| !Capsule->IsQueryCollisionEnabled() || !PlayerRoot->IsQueryCollisionEnabled())
		{
			return false;
		}

		if (Capsule->GetCollisionResponseToChannel(PlayerRoot->GetCollisionObjectType()) != ECR_Block
			|| PlayerRoot->GetCollisionResponseToChannel(Capsule->GetCollisionObjectType()) != ECR_Block)
		{
			return false;
		}

		if (!PlayerRoot->OverlapComponent(
			Capsule->GetComponentLocation(),
			Capsule->GetComponentQuat(),
			Capsule->GetCollisionShape(Tolerance)))
		{
			return false;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(HiderExistingContact), false);

		Params.AddIgnoredActor(&Character);
		Params.AddIgnoredActor(&PlayerPawn);

		return !World->LineTraceTestByChannel(Capsule->GetComponentLocation(), PlayerRoot->GetComponentLocation(),
		                                      ECC_Pawn, Params);
	}

	void TryReportTouch(const FStateTreeWeakExecutionContext& WeakContext, AActor* OtherActor,
	                    const bool bCheckExistingContact)
	{
		auto StrongContext = WeakContext.MakeStrongExecutionContext();

		FTouchData* Data = StrongContext.GetInstanceDataPtr<FTouchData>();

		if (!Data || Data->bEventSent)
		{
			return;
		}

		ACharacter* Character = Cast<ACharacter>(Data->Actor.Get());
		APawn* PlayerPawn = GetSearchingPlayerPawn(Character);

		if (!IsValid(Character) || !PlayerPawn)
		{
			return;
		}

		if (bCheckExistingContact)
		{
			if (!HasExistingPlayerContact(*Character, *PlayerPawn, Data->ContactTolerance))
			{
				return;
			}
		}
		else if (OtherActor != PlayerPawn)
		{
			return;
		}

		Data->bEventSent = true;

		if (!StrongContext.SendEvent(TAG_HNS_FoundByPlayer))
		{
			Data->bEventSent = false;
			return;
		}

		UE_LOG(LogHiderTouch, Log, TEXT("%s: FoundByPlayer event sent. Reason=%s"),
		       *GetNameSafe(Character),
		       bCheckExistingContact ? TEXT("ExistingTouch") : TEXT("Touch"));
	}
}

FStateTreeWatchPlayerTouchTask::FStateTreeWatchPlayerTouchTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeWatchPlayerTouchTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.Controller.Reset();
	Data.GameMode.Reset();
	Data.HitHandle.Reset();
	Data.PhaseHandle.Reset();
	Data.bEventSent = false;
	ACharacter* Character = Cast<ACharacter>(Data.Actor.Get());

	UWorld* World = IsValid(Character) ? Character->GetWorld() : nullptr;

	AHideAndSeekAIController* Controller = IsValid(Character)
		                                       ? Cast<AHideAndSeekAIController>(Character->GetController())
		                                       : nullptr;

	AHideAndSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideAndSeekGameMode>() : nullptr;

	if (!IsValid(Character) || !IsValid(Controller) || !IsValid(GameMode)
		|| !FMath::IsFinite(Data.ContactTolerance)
		|| Data.ContactTolerance < 0.0f
		|| Data.ContactTolerance > 5.0f)
	{
		UE_LOG(LogHiderTouch, Error, TEXT("Watch Player Touch: invalid context or tolerance."));

		return EStateTreeRunStatus::Failed;
	}

	Data.Controller = Controller;
	Data.GameMode = GameMode;

	const FStateTreeWeakExecutionContext WeakContext = Context.MakeWeakExecutionContext();

	Data.HitHandle = Controller->OnControlledPawnHit.AddLambda(
		[WeakContext](AActor* OtherActor)
		{
			TryReportTouch(WeakContext, OtherActor, false);
		});

	Data.PhaseHandle = GameMode->OnRoundPhaseChanged.AddLambda(
		[WeakContext](EHideAndSeekRoundPhase, EHideAndSeekRoundPhase NewPhase)
		{
			if (NewPhase == EHideAndSeekRoundPhase::Searching)
			{
				TryReportTouch(WeakContext, nullptr, true);
			}
		});

	if (APawn* PlayerPawn = GetSearchingPlayerPawn(Character))
	{
		if (HasExistingPlayerContact(*Character, *PlayerPawn, Data.ContactTolerance))
		{
			Data.bEventSent = true;
			Context.SendEvent(TAG_HNS_FoundByPlayer);

			UE_LOG(LogHiderTouch, Log, TEXT("%s: FoundByPlayer event requested. Reason=ExistingTouch"),
			       *GetNameSafe(Character));
		}
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeWatchPlayerTouchTask::ExitState(FStateTreeExecutionContext& Context,
                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.bEventSent = true;

	if (AHideAndSeekAIController* Controller = Data.Controller.Get())
	{
		Controller->OnControlledPawnHit.Remove(Data.HitHandle);
	}

	if (AHideAndSeekGameMode* GameMode = Data.GameMode.Get())
	{
		GameMode->OnRoundPhaseChanged.Remove(Data.PhaseHandle);
	}

	Data.HitHandle.Reset();
	Data.PhaseHandle.Reset();
	Data.Controller.Reset();
	Data.GameMode.Reset();
}
