#include "StateTreeWaitForRoundPhaseTask.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderRoundPhase, Log, All);

namespace
{
	bool HasReachedRoundPhase(const EHideAndSeekRoundPhase CurrentPhase, const EHideAndSeekRoundPhase RequiredPhase)
	{
		switch (RequiredPhase)
		{
		case EHideAndSeekRoundPhase::WaitingToStart:
			return true;

		case EHideAndSeekRoundPhase::Hiding:
			return CurrentPhase == EHideAndSeekRoundPhase::Hiding || CurrentPhase == EHideAndSeekRoundPhase::Searching;

		case EHideAndSeekRoundPhase::Searching:
			return CurrentPhase == EHideAndSeekRoundPhase::Searching;

		default:
			return false;
		}
	}
}

FStateTreeWaitForRoundPhaseTask::FStateTreeWaitForRoundPhaseTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
}

EStateTreeRunStatus FStateTreeWaitForRoundPhaseTask::EnterState(FStateTreeExecutionContext& Context,
                                                                const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.GameMode.Reset();
	Data.PhaseChangedHandle.Reset();
	Data.bWaiting = false;

	AActor* Actor = Data.Actor.Get();
	UWorld* World = IsValid(Actor) ? Actor->GetWorld() : nullptr;

	AHideAndSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideAndSeekGameMode>() : nullptr;

	if (!IsValid(GameMode))
	{
		UE_LOG(LogHiderRoundPhase, Error, TEXT("%s: Wait For Round Phase requires HideAndSeekGameMode."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Failed;
	}

	EHideAndSeekRoundPhase CurrentPhase = GameMode->GetRoundPhase();

	if (HasReachedRoundPhase(CurrentPhase, Data.RequiredPhase))
	{
		UE_LOG(LogHiderRoundPhase, Log, TEXT("%s: phase requirement %s already met; current=%s."),
		       *GetNameSafe(Actor),
		       *UEnum::GetValueAsString(Data.RequiredPhase),
		       *UEnum::GetValueAsString(CurrentPhase));

		return EStateTreeRunStatus::Succeeded;
	}

	Data.GameMode = GameMode;
	Data.bWaiting = true;

	Data.PhaseChangedHandle = GameMode->OnRoundPhaseChanged.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext(),
			RequiredPhase = Data.RequiredPhase](
		EHideAndSeekRoundPhase,
		EHideAndSeekRoundPhase NewPhase)
		{
			if (!HasReachedRoundPhase(NewPhase, RequiredPhase))
			{
				return;
			}

			auto StrongContext = WeakContext.MakeStrongExecutionContext();

			FInstanceDataType* Instance = StrongContext.GetInstanceDataPtr<FInstanceDataType>();

			if (!Instance || !Instance->bWaiting)
			{
				return;
			}

			Instance->bWaiting = false;

			UE_LOG(LogHiderRoundPhase, Log, TEXT("%s: phase requirement %s met; current=%s."),
			       *GetNameSafe(Instance->Actor.Get()),
			       *UEnum::GetValueAsString(RequiredPhase),
			       *UEnum::GetValueAsString(NewPhase));

			StrongContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
		});

	UE_LOG(LogHiderRoundPhase, Log, TEXT("%s: waiting for %s; current=%s."),
	       *GetNameSafe(Actor),
	       *UEnum::GetValueAsString(Data.RequiredPhase),
	       *UEnum::GetValueAsString(CurrentPhase));

	return EStateTreeRunStatus::Running;
}

void FStateTreeWaitForRoundPhaseTask::ExitState(FStateTreeExecutionContext& Context,
                                                const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.bWaiting = false;

	if (AHideAndSeekGameMode* GameMode = Data.GameMode.Get())
	{
		if (Data.PhaseChangedHandle.IsValid())
		{
			GameMode->OnRoundPhaseChanged.Remove(Data.PhaseChangedHandle);
		}
	}

	Data.PhaseChangedHandle.Reset();
	Data.GameMode.Reset();
}
