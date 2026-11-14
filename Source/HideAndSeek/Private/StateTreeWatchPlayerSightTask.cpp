#include "StateTreeWatchPlayerSightTask.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HideAndSeekGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "HideAndSeekGameplayTags.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderDetection, Log, All);

namespace
{
	bool HasClearPlayerView(const FStateTreeWatchPlayerSightInstanceData& Data)
	{
		ACharacter* Character = Cast<ACharacter>(Data.Actor.Get());
		UWorld* World = Data.World.Get();

		if (!IsValid(Character) || !IsValid(World))
		{
			return false;
		}

		const AHideAndSeekGameMode* GameMode = World->GetAuthGameMode<AHideAndSeekGameMode>();

		if (!IsValid(GameMode) || GameMode->GetRoundPhase() != EHideAndSeekRoundPhase::Searching)
		{
			return false;
		}

		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);

		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			return false;
		}

		APawn* PlayerPawn = PlayerController->GetPawn();

		if (!IsValid(PlayerPawn) || PlayerPawn == Character || !IsValid(PlayerController->PlayerCameraManager) ||
			PlayerController->PlayerCameraManager->GetCameraCacheTime() <= 0.0f)
		{
			return false;
		}

		if (FVector::DistSquared(PlayerPawn->GetActorLocation(), Character->GetActorLocation()) > FMath::Square(
			Data.SightRange))
		{
			return false;
		}

		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

		if (!Capsule)
		{
			return false;
		}

		int32 ViewportWidth = 0;
		int32 ViewportHeight = 0;
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

		if (ViewportWidth <= 0 || ViewportHeight <= 0)
		{
			return false;
		}


		FVector CameraLocation;
		FRotator CameraRotation;
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		const FVector Center = Capsule->GetComponentLocation();
		const FVector UpperBody = Center + Capsule->GetUpVector() * Capsule->GetScaledCapsuleHalfHeight() * 0.5f;

		const FVector Points[] = {Center, UpperBody};

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HiderSightTrace), true, Character);

		QueryParams.AddIgnoredActor(PlayerPawn);

		bool bAllVisible = true;

		for (const FVector& Point : Points)
		{
			FVector2D ScreenPosition;

			const bool bOnScreen =
				PlayerController->ProjectWorldLocationToScreen(Point, ScreenPosition, false)
				&& ScreenPosition.X >= 0.0
				&& ScreenPosition.Y >= 0.0
				&& ScreenPosition.X < ViewportWidth
				&& ScreenPosition.Y < ViewportHeight;

			if (!bOnScreen)
			{
				bAllVisible = false;
				continue;
			}

			const bool bBlocked = World->LineTraceTestByChannel(
				CameraLocation,
				Point,
				ECC_Visibility,
				QueryParams);

			bAllVisible &= !bBlocked;

			if (Data.bDrawDebug)
			{
				const FColor Color = bBlocked ? FColor::Red : FColor::Green;

				DrawDebugLine(World, CameraLocation, Point, Color, false,
				              Data.CheckInterval * 1.2f, 0, 1.0f);

				DrawDebugSphere(World, Point, 4.0f, 8, Color, false,
				                Data.CheckInterval * 1.2f);
			}
		}

		return bAllVisible;
	}
}

FStateTreeWatchPlayerSightTask::FStateTreeWatchPlayerSightTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;

	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeWatchPlayerSightTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.World.Reset();
	Data.CheckTimer.Invalidate();
	Data.FirstVisibleTime = -1.0;
	Data.bEventSent = false;

	ACharacter* Character = Cast<ACharacter>(Data.Actor.Get());
	UWorld* World = IsValid(Character) ? Character->GetWorld() : nullptr;

	if (!World
		|| !IsValid(World->GetAuthGameMode<AHideAndSeekGameMode>())
		|| !FMath::IsFinite(Data.SightRange)
		|| Data.SightRange <= 0.0f
		|| !FMath::IsFinite(Data.CheckInterval)
		|| Data.CheckInterval <= 0.0f
		|| !FMath::IsFinite(Data.ConfirmTime)
		|| Data.ConfirmTime < 0.0f)
	{
		UE_LOG(LogHiderDetection, Error, TEXT("Watch Player Sight: invalid Character, GameMode or settings."));

		return EStateTreeRunStatus::Failed;
	}

	Data.World = World;

	const FTimerDelegate Callback = FTimerDelegate::CreateLambda(
		[WeakContext = Context.MakeWeakExecutionContext()]()
		{
			auto StrongContext = WeakContext.MakeStrongExecutionContext();

			FInstanceDataType* Instance = StrongContext.GetInstanceDataPtr<FInstanceDataType>();

			if (!Instance || Instance->bEventSent)
			{
				return;
			}

			UWorld* CurrentWorld = Instance->World.Get();

			if (!CurrentWorld)
			{
				return;
			}

			if (!HasClearPlayerView(*Instance))
			{
				Instance->FirstVisibleTime = -1.0;
				return;
			}

			const double Now = CurrentWorld->GetTimeSeconds();

			if (Instance->FirstVisibleTime < 0.0)
			{
				Instance->FirstVisibleTime = Now;
			}

			if (Now - Instance->FirstVisibleTime < Instance->ConfirmTime)
			{
				return;
			}

			Instance->bEventSent = true;

			if (!StrongContext.SendEvent(TAG_HNS_FoundByPlayer))
			{
				Instance->bEventSent = false;
				return;
			}

			CurrentWorld->GetTimerManager().ClearTimer(Instance->CheckTimer);


			UE_LOG(LogHiderDetection, Log, TEXT("%s: FoundByPlayer event sent. Reason=Sight"),
			       *GetNameSafe(Instance->Actor.Get()));
		});

	FTimerManagerTimerParameters TimerParameters;
	TimerParameters.bLoop = true;
	TimerParameters.bMaxOncePerFrame = true;

	World->GetTimerManager().SetTimer(Data.CheckTimer, Callback, Data.CheckInterval, TimerParameters);

	return EStateTreeRunStatus::Running;
}

void FStateTreeWatchPlayerSightTask::ExitState(FStateTreeExecutionContext& Context,
                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.bEventSent = true;

	if (UWorld* World = Data.World.Get())
	{
		World->GetTimerManager().ClearTimer(Data.CheckTimer);
	}

	Data.CheckTimer.Invalidate();
	Data.World.Reset();
	Data.FirstVisibleTime = -1.0;
}
