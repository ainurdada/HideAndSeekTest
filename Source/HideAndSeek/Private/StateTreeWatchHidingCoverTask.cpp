#include "StateTreeWatchHidingCoverTask.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HideAndSeekGameMode.h"
#include "HideAndSeekGameplayTags.h"
#include "HidingVisibility.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderCover, Log, All);

FStateTreeWatchHidingCoverTask::FStateTreeWatchHidingCoverTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeWatchHidingCoverTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.World.Reset();
	Data.CheckTimer.Invalidate();
	Data.FirstExposedTime = -1.0;
	Data.bCoverLost = false;

	ACharacter* Character = Cast<ACharacter>(Data.Actor.Get());
	UWorld* World = IsValid(Character) ? Character->GetWorld() : nullptr;

	const AHideAndSeekGameMode* GameMode = World ? World->GetAuthGameMode<AHideAndSeekGameMode>() : nullptr;

	if (!IsValid(World)
		|| !IsValid(GameMode)
		|| Data.GoalLocation.ContainsNaN()
		|| !FMath::IsFinite(Data.CoverMargin)
		|| Data.CoverMargin < 0.0f
		|| !FMath::IsFinite(Data.CheckInterval)
		|| Data.CheckInterval <= 0.0f
		|| !FMath::IsFinite(Data.ConfirmTime)
		|| Data.ConfirmTime < 0.0f)
	{
		UE_LOG(LogHiderCover, Error, TEXT("Watch Hiding Cover: invalid Character, GameMode or settings."));

		return EStateTreeRunStatus::Failed;
	}

	Data.World = World;

	if (GameMode->GetRoundPhase() != EHideAndSeekRoundPhase::Hiding)
	{
		return EStateTreeRunStatus::Running;
	}

	const FTimerDelegate Callback = FTimerDelegate::CreateLambda(
		[WeakContext = Context.MakeWeakExecutionContext()]()
		{
			auto StrongContext = WeakContext.MakeStrongExecutionContext();

			FInstanceDataType* Instance = StrongContext.GetInstanceDataPtr<FInstanceDataType>();

			if (!Instance || Instance->bCoverLost)
			{
				return;
			}

			UWorld* CurrentWorld = Instance->World.Get();

			ACharacter* Hider = Cast<ACharacter>(Instance->Actor.Get());

			const AHideAndSeekGameMode* CurrentGameMode = CurrentWorld
				                                              ? CurrentWorld->GetAuthGameMode<AHideAndSeekGameMode>()
				                                              : nullptr;

			if (!IsValid(CurrentWorld))
			{
				return;
			}

			if (!IsValid(Hider) || !IsValid(CurrentGameMode))
			{
				CurrentWorld->GetTimerManager().ClearTimer(Instance->CheckTimer);

				return;
			}

			if (CurrentGameMode->GetRoundPhase() != EHideAndSeekRoundPhase::Hiding)
			{
				Instance->FirstExposedTime = -1.0;

				CurrentWorld->GetTimerManager().ClearTimer(Instance->CheckTimer);

				return;
			}

			APlayerController* PlayerController = UGameplayStatics::GetPlayerController(CurrentWorld, 0);

			if (!IsValid(PlayerController)
				|| !PlayerController->IsLocalController()
				|| !IsValid(PlayerController->PlayerCameraManager)
				|| PlayerController->PlayerCameraManager->GetCameraCacheTime() <= 0.0f)
			{
				Instance->FirstExposedTime = -1.0;
				return;
			}

			FVector CameraLocation;
			FRotator CameraRotation;

			PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

			const bool bCovered = HideAndSeek::IsCrouchedLocationOccluded(
				Hider,
				Instance->GoalLocation,
				CameraLocation,
				Instance->CoverMargin,
				Instance->bDrawDebug
					? Instance->CheckInterval * 1.2f
					: 0.0f);

			if (bCovered)
			{
				Instance->FirstExposedTime = -1.0;
				return;
			}

			const double Now = CurrentWorld->GetTimeSeconds();

			if (Instance->FirstExposedTime < 0.0)
			{
				Instance->FirstExposedTime = Now;
			}

			const double ExposedFor = Now - Instance->FirstExposedTime;

			if (ExposedFor < Instance->ConfirmTime)
			{
				return;
			}

			Instance->bCoverLost = true;

			if (!StrongContext.SendEvent(TAG_HNS_CoverLost))
			{
				Instance->bCoverLost = false;

				UE_LOG(LogHiderCover, Warning, TEXT("%s: Could not send CoverLost event."),
				       *GetNameSafe(Hider));

				return;
			}

			CurrentWorld->GetTimerManager().ClearTimer(Instance->CheckTimer);

			UE_LOG(LogHiderCover, Log,
			       TEXT("%s: CoverLost. Goal=%s, ExposedFor=%.2f s."),
			       *GetNameSafe(Hider),
			       *Instance->GoalLocation.ToCompactString(),
			       ExposedFor);
		});

	FTimerManagerTimerParameters TimerParameters;
	TimerParameters.bLoop = true;
	TimerParameters.bMaxOncePerFrame = true;

	World->GetTimerManager().SetTimer(
		Data.CheckTimer,
		Callback,
		Data.CheckInterval,
		TimerParameters);

	return EStateTreeRunStatus::Running;
}

void FStateTreeWatchHidingCoverTask::ExitState(FStateTreeExecutionContext& Context,
                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (UWorld* World = Data.World.Get())
	{
		World->GetTimerManager().ClearTimer(Data.CheckTimer);
	}

	Data.CheckTimer.Invalidate();
	Data.World.Reset();
	Data.FirstExposedTime = -1.0;
	Data.bCoverLost = false;
}
