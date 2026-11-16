#include "StateTreeHiderStatusTask.h"

#include "GameFramework/Actor.h"
#include "HiderAbilitySystemComponent.h"
#include "HideAndSeekGameplayTags.h"
#include "StateTreeExecutionContext.h"
#include "Engine/World.h"
#include "HideAndSeekGameMode.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderStatusTask, Log, All);

FStateTreeHiderStatusTask::FStateTreeHiderStatusTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeHiderStatusTask::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.AbilitySystem.Reset();
	Data.bStatusChanged = false;

	AActor* Actor = Data.Actor.Get();

	UHiderAbilitySystemComponent* AbilitySystem = IsValid(Actor)
		                                              ? Actor->FindComponentByClass<UHiderAbilitySystemComponent>()
		                                              : nullptr;

	if (!IsValid(AbilitySystem))
	{
		UE_LOG(LogHiderStatusTask, Error, TEXT("%s: Hider Status requires HiderAbilitySystemComponent."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Failed;
	}

	if (Data.StatusTag == TAG_HNS_Status_Hidden)
	{
		const bool bWasHidden = AbilitySystem->IsHidden();

		if (!AbilitySystem->SetHidden(true))
		{
			return EStateTreeRunStatus::Failed;
		}

		Data.bStatusChanged = !bWasHidden;
	}
	else if (Data.StatusTag == TAG_HNS_Status_Found)
	{
		UWorld* World = Actor->GetWorld();

		AHideAndSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideAndSeekGameMode>() : nullptr;

		if (!IsValid(GameMode))
		{
			UE_LOG(LogHiderStatusTask, Error, TEXT("%s: Found status requires HideAndSeekGameMode."),
			       *GetNameSafe(Actor));

			return EStateTreeRunStatus::Failed;
		}

		Data.bStatusChanged = AbilitySystem->TryMarkFound();

		if (Data.bStatusChanged && !GameMode->NotifyHiderFound(Actor))
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	else
	{
		UE_LOG(LogHiderStatusTask, Error, TEXT("%s: Hider Status supports only Hidden or Found. Received: %s"),
		       *GetNameSafe(Actor),
		       *Data.StatusTag.ToString());

		return EStateTreeRunStatus::Failed;
	}

	Data.AbilitySystem = AbilitySystem;

	return EStateTreeRunStatus::Running;
}

void FStateTreeHiderStatusTask::ExitState(FStateTreeExecutionContext& Context,
                                          const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.StatusTag == TAG_HNS_Status_Hidden)
	{
		if (UHiderAbilitySystemComponent* AbilitySystem = Data.AbilitySystem.Get())
		{
			AbilitySystem->SetHidden(false);
		}
	}

	Data.AbilitySystem.Reset();
}
