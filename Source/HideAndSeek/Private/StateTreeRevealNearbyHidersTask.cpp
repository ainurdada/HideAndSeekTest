#include "StateTreeRevealNearbyHidersTask.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HiderAbilitySystemComponent.h"
#include "HideAndSeekGameMode.h"
#include "HideAndSeekGameplayTags.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderReveal, Log, All);

FStateTreeRevealNearbyHidersTask::FStateTreeRevealNearbyHidersTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeRevealNearbyHidersTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.RevealedCount = 0;
	Data.FailedCount = 0;

	AActor* Source = Data.Actor.Get();

	UWorld* World = IsValid(Source) ? Source->GetWorld() : nullptr;

	UHiderAbilitySystemComponent* SourceASC = IsValid(Source)
		                                          ? Source->FindComponentByClass<UHiderAbilitySystemComponent>()
		                                          : nullptr;

	if (!IsValid(World) || !IsValid(SourceASC) || !SourceASC->IsFound())
	{
		UE_LOG(LogHiderReveal, Error, TEXT("%s: Reveal Nearby Hiders requires a valid Found hider."),
		       *GetNameSafe(Source));

		return EStateTreeRunStatus::Failed;
	}

	if (!Data.bFirstDiscovery)
	{
		UE_LOG(LogHiderReveal, Log, TEXT("%s: Reveal skipped: FirstDiscovery=false."),
		       *GetNameSafe(Source));

		return EStateTreeRunStatus::Succeeded;
	}

	const AHideAndSeekGameMode* GameMode = World->GetAuthGameMode<AHideAndSeekGameMode>();

	if (!IsValid(GameMode))
	{
		UE_LOG(LogHiderReveal, Error, TEXT("%s: Reveal requires HideAndSeekGameMode."),
		       *GetNameSafe(Source));

		return EStateTreeRunStatus::Failed;
	}

	if (GameMode->GetRoundPhase() == EHideAndSeekRoundPhase::Finished)
	{
		UE_LOG(LogHiderReveal, Log, TEXT("%s: Reveal skipped: round already finished."),
		       *GetNameSafe(Source));

		return EStateTreeRunStatus::Succeeded;
	}

	if (GameMode->GetRoundPhase() != EHideAndSeekRoundPhase::Searching)
	{
		UE_LOG(LogHiderReveal, Error, TEXT("%s: Reveal requires the Searching phase."),
		       *GetNameSafe(Source));

		return EStateTreeRunStatus::Failed;
	}

	if (!FMath::IsFinite(Data.RevealRadius) || Data.RevealRadius <= 0.0f
		|| (Data.bDrawDebug && (!FMath::IsFinite(Data.DebugDuration) || Data.DebugDuration <= 0.0f)))
	{
		UE_LOG(LogHiderReveal, Error, TEXT("%s: Invalid reveal radius or debug duration."),
		       *GetNameSafe(Source));

		return EStateTreeRunStatus::Failed;
	}

	const FVector Origin = Source->GetActorLocation();

	const double RadiusSquared = FMath::Square(static_cast<double>(Data.RevealRadius));

	if (Data.bDrawDebug)
	{
		DrawDebugSphere(
			World,
			Origin,
			Data.RevealRadius,
			32,
			FColor::Cyan,
			false,
			Data.DebugDuration,
			0,
			1.0f);
	}

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Target = *It;

		if (!IsValid(Target) || Target == Source || Target->IsPlayerControlled())
		{
			continue;
		}

		UHiderAbilitySystemComponent* TargetASC = Target->FindComponentByClass<UHiderAbilitySystemComponent>();

		if (!IsValid(TargetASC) || TargetASC->IsFound() || !TargetASC->IsHidden()
			|| !TargetASC->GetOwnedGameplayTags().HasTagExact(TAG_HNS_Status_Unfound))
		{
			continue;
		}

		const FVector TargetLocation = Target->GetActorLocation();

		const double DistanceSquared = FVector::DistSquared(Origin, TargetLocation);

		if (DistanceSquared > RadiusSquared)
		{
			continue;
		}

		if (!TargetASC->ApplyRevealMarker())
		{
			++Data.FailedCount;

			UE_LOG(LogHiderReveal, Error, TEXT("%s -> %s: Failed to apply reveal marker."),
			       *GetNameSafe(Source),
			       *GetNameSafe(Target));

			continue;
		}

		++Data.RevealedCount;

		UE_LOG(LogHiderReveal, Log, TEXT("%s -> %s: Revealed, Distance=%.0f cm."),
		       *GetNameSafe(Source),
		       *GetNameSafe(Target),
		       FMath::Sqrt(DistanceSquared));

		if (Data.bDrawDebug)
		{
			DrawDebugLine(
				World,
				Origin,
				TargetLocation,
				FColor::Green,
				false,
				Data.DebugDuration,
				0,
				2.0f);
		}
	}

	UE_LOG(LogHiderReveal, Log, TEXT("%s: Reveal finished. Radius=%.0f, Revealed=%d, Failed=%d."),
	       *GetNameSafe(Source),
	       Data.RevealRadius,
	       Data.RevealedCount,
	       Data.FailedCount);

	return Data.FailedCount == 0 ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}
