#include "StateTreeWatchRevealedTask.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HiderAbilitySystemComponent.h"
#include "HideAndSeekGameMode.h"
#include "HideAndSeekGameplayTags.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderRehide, Log, All);

FStateTreeWatchRevealedTask::FStateTreeWatchRevealedTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeWatchRevealedTask::EnterState(FStateTreeExecutionContext& Context,
                                                            const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.AbilitySystem.Reset();
	Data.RevealAppliedHandle.Reset();
	Data.bHasExcludedLocation = false;
	Data.ExcludedLocation = FVector::ZeroVector;

	AActor* Actor = Data.Actor.Get();

	UHiderAbilitySystemComponent* AbilitySystem = IsValid(Actor)
		                                              ? Actor->FindComponentByClass<UHiderAbilitySystemComponent>()
		                                              : nullptr;

	if (!IsValid(AbilitySystem))
	{
		UE_LOG(LogHiderRehide, Error, TEXT("%s: Watch Revealed requires HiderAbilitySystemComponent."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Failed;
	}

	Data.AbilitySystem = AbilitySystem;

	Data.RevealAppliedHandle = AbilitySystem->OnRevealApplied.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext()]()
		{
			auto StrongContext = WeakContext.MakeStrongExecutionContext();

			FInstanceDataType* Instance = StrongContext.GetInstanceDataPtr<FInstanceDataType>();

			if (!Instance)
			{
				return;
			}

			AActor* Hider = Instance->Actor.Get();

			UHiderAbilitySystemComponent* HiderASC = Instance->AbilitySystem.Get();

			UWorld* World = IsValid(Hider) ? Hider->GetWorld() : nullptr;

			const AHideAndSeekGameMode* GameMode = World
				                                       ? World->GetAuthGameMode<AHideAndSeekGameMode>()
				                                       : nullptr;

			if (!IsValid(Hider) || !IsValid(HiderASC) || HiderASC->IsFound() || !HiderASC->IsHidden()
				|| !IsValid(GameMode) || GameMode->GetRoundPhase() != EHideAndSeekRoundPhase::Searching)
			{
				return;
			}

			Instance->ExcludedLocation = Hider->GetActorLocation();
			Instance->bHasExcludedLocation = true;

			if (!StrongContext.SendEvent(TAG_HNS_Revealed))
			{
				UE_LOG(LogHiderRehide, Warning, TEXT("%s: Could not send Revealed event."),
				       *GetNameSafe(Hider));

				return;
			}

			UE_LOG(LogHiderRehide, Log, TEXT("%s: Revealed event sent. ExcludedLocation=%s"),
			       *GetNameSafe(Hider),
			       *Instance->ExcludedLocation.ToCompactString());
		});

	return EStateTreeRunStatus::Running;
}

void FStateTreeWatchRevealedTask::ExitState(FStateTreeExecutionContext& Context,
                                            const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (UHiderAbilitySystemComponent* AbilitySystem = Data.AbilitySystem.Get())
	{
		AbilitySystem->OnRevealApplied.Remove(Data.RevealAppliedHandle);
	}

	Data.RevealAppliedHandle.Reset();
	Data.AbilitySystem.Reset();
}
