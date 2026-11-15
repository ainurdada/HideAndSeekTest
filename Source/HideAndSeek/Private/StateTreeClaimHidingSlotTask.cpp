#include "StateTreeClaimHidingSlotTask.h"

#include "AIController.h"
#include "AITypes.h"
#include "GameFramework/Pawn.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HidingSpotBehaviorDefinition.h"
#include "SmartObjectRequestTypes.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "StructUtils/StructView.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HidingVisibility.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderSmartObject, Log, All)

FStateTreeClaimHidingSlotTask::FStateTreeClaimHidingSlotTask()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
	bShouldCopyBoundPropertiesOnExitState = false;

	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FStateTreeClaimHidingSlotTask::EnterState(FStateTreeExecutionContext& Context,
                                                              const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AActor* Actor = Data.Actor.Get();

	Data.ClaimHandle.Invalidate();
	Data.GoalLocation = FVector::ZeroVector;
	Data.Subsystem.Reset();

	if (!IsValid(Actor) || !Actor->GetWorld())
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("Claim failed: Actor or World is invalid."));
		return EStateTreeRunStatus::Failed;
	}

	ACharacter* Character = Cast<ACharacter>(Actor);
	AAIController* AIController = Character ? Cast<AAIController>(Character->GetController()) : nullptr;

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Actor->GetWorld());

	if (!AIController || !NavSystem)
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("%s: selection failed: AIController or NavigationSystem is missing."),
		       *GetNameSafe(Actor));
		return EStateTreeRunStatus::Failed;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(Actor, 0);

	if (!IsValid(PlayerController) || !PlayerController->IsLocalController() ||
		!IsValid(PlayerController->PlayerCameraManager) ||
		PlayerController->PlayerCameraManager->GetCameraCacheTime() <= 0.0f)
	{
		UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: waiting for the player camera."),
		       *GetNameSafe(Actor));
		return EStateTreeRunStatus::Failed;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	if (!FMath::IsFinite(Data.MinDistanceFromExcludedLocation) || Data.MinDistanceFromExcludedLocation <= 0.0f
		|| (Data.bHasExcludedLocation && Data.ExcludedLocation.ContainsNaN()))
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("%s: Invalid excluded location settings."),
		       *GetNameSafe(Actor));

		return EStateTreeRunStatus::Failed;
	}

	if (!FMath::IsFinite(Data.SearchRadius) || Data.SearchRadius <= 0.0f)
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("Claim failed: SearchRadius must be positive."));
		return EStateTreeRunStatus::Failed;
	}

	if (!FMath::IsFinite(Data.MaxGoalOffsetXY) || !FMath::IsFinite(Data.MaxGoalOffsetZ) || Data.MaxGoalOffsetXY < 0.0f
		|| Data.MaxGoalOffsetZ < 0.0f)
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("Selection failed: goal offsets must be finite and non-negative."));
		return EStateTreeRunStatus::Failed;
	}

	if (!FMath::IsFinite(Data.CoverMargin) || !FMath::IsFinite(Data.VisibilityDebugDuration) ||
		Data.CoverMargin < 0.0f || Data.VisibilityDebugDuration < 0.0f)
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("Selection failed: cover parameters are invalid."));
		return EStateTreeRunStatus::Failed;
	}

	USmartObjectSubsystem* Subsystem = Actor->GetWorld()->GetSubsystem<USmartObjectSubsystem>();

	if (!Subsystem)
	{
		UE_LOG(LogHiderSmartObject, Warning, TEXT("Claim failed: SmartObjectSubsystem is unavailable."));
		return EStateTreeRunStatus::Failed;
	}

	Data.Subsystem = Subsystem;

	const FVector Origin = Actor->GetActorLocation();
	const double RadiusSquared = FMath::Square(static_cast<double>(Data.SearchRadius));

	FSmartObjectRequest Request;
	Request.QueryBox = FBox::BuildAABB(Origin, FVector(Data.SearchRadius));
	Request.Filter.BehaviorDefinitionClasses.Add(UHidingSpotBehaviorDefinition::StaticClass());
	Request.Filter.ClaimPriority = ESmartObjectClaimPriority::Normal;
	Request.Filter.bShouldIncludeClaimedSlots = false;
	Request.Filter.bShouldIncludeDisabledSlots = false;

	const FSmartObjectActorUserData UserData(Actor);
	const FConstStructView UserDataView = FConstStructView::Make(UserData);

	TArray<FSmartObjectRequestResult> Results;
	Subsystem->FindSmartObjects(Request, Results, UserDataView);

	struct FCandidate
	{
		FSmartObjectSlotHandle SlotHandle;
		FVector Location;
		double DistanceSquared;
	};

	TArray<FCandidate> Candidates;

	for (const FSmartObjectRequestResult& Result : Results)
	{
		const TOptional<FVector> Location = Subsystem->GetSlotLocation(Result.SlotHandle);

		if (!Location.IsSet())
		{
			continue;
		}

		if (Data.bHasExcludedLocation && FVector::DistSquared2D(Location.GetValue(), Data.ExcludedLocation)
			< FMath::Square(static_cast<double>(Data.MinDistanceFromExcludedLocation)))
		{
			UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: rejected %s: too close to revealed location."),
			       *GetNameSafe(Actor),
			       *LexToString(Result.SlotHandle));

			continue;
		}

		const double DistanceSquared = FVector::DistSquared(Origin, Location.GetValue());

		if (DistanceSquared <= RadiusSquared)
		{
			Candidates.Add(FCandidate{
				Result.SlotHandle,
				Location.GetValue(),
				DistanceSquared
			});
		}
	}

	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		return A.DistanceSquared < B.DistanceSquared;
	});

	for (const FCandidate& Candidate : Candidates)
	{
		// Check if we can get to candidate location
		FAIMoveRequest MoveRequest(Candidate.Location);
		MoveRequest
			.SetUsePathfinding(true)
			.SetAllowPartialPath(false)
			.SetRequireNavigableEndLocation(true)
			.SetProjectGoalLocation(false)
			.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());

		FPathFindingQuery Query;

		if (!AIController->BuildPathfindingQuery(MoveRequest, Query))
		{
			UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: rejected %s: could not build navigation query."),
			       *GetNameSafe(Actor),
			       *LexToString(Candidate.SlotHandle));
			continue;
		}

		const FPathFindingResult PathResult = NavSystem->FindPathSync(Query);

		if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid() || !PathResult.Path->IsValid() ||
			PathResult.Path->IsPartial())
		{
			UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: rejected %s: no complete path."),
			       *GetNameSafe(Actor),
			       *LexToString(Candidate.SlotHandle));
			continue;
		}

		const FVector NavigableGoal = PathResult.Path->GetEndLocation();

		const double OffsetXYSquared = FVector::DistSquared2D(NavigableGoal, Candidate.Location);

		const double OffsetZ = FMath::Abs(NavigableGoal.Z - Candidate.Location.Z);

		if (OffsetXYSquared > FMath::Square(static_cast<double>(Data.MaxGoalOffsetXY)) || OffsetZ > Data.MaxGoalOffsetZ)
		{
			UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: rejected %s: path endpoint offset XY=%.1f Z=%.1f."),
			       *GetNameSafe(Actor),
			       *LexToString(Candidate.SlotHandle),
			       FMath::Sqrt(OffsetXYSquared),
			       OffsetZ);
			continue;
		}

		if (!HideAndSeek::IsCrouchedLocationOccluded(
			Character,
			NavigableGoal,
			CameraLocation,
			Data.CoverMargin,
			Data.VisibilityDebugDuration))
		{
			UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: rejected %s: crouched cover test failed."),
			       *GetNameSafe(Actor),
			       *LexToString(Candidate.SlotHandle));
			continue;
		}

		// Claim candidate
		const FSmartObjectClaimHandle Claim = Subsystem->MarkSlotAsClaimed(
			Candidate.SlotHandle,
			ESmartObjectClaimPriority::Normal,
			UserDataView
		);

		if (!Claim.IsValid())
		{
			continue;
		}

		Data.ClaimHandle = Claim;
		Data.GoalLocation = NavigableGoal;

		Subsystem->RegisterSlotInvalidationCallback(
			Claim,
			FOnSlotInvalidated::CreateLambda([WeakContext = Context.MakeWeakExecutionContext()](
				const FSmartObjectClaimHandle& InvalidatedHandle,
				ESmartObjectSlotState)
				{
					auto StrongContext = WeakContext.MakeStrongExecutionContext();

					FInstanceDataType* Instance = StrongContext.GetInstanceDataPtr<FInstanceDataType>();

					if (Instance && Instance->ClaimHandle == InvalidatedHandle)
					{
						UE_LOG(LogHiderSmartObject, Warning, TEXT("%s: hiding slot invalidated."),
						       *GetNameSafe(Instance->Actor.Get()));

						Instance->ClaimHandle.Invalidate();

						StrongContext.FinishTask(EStateTreeFinishTaskType::Failed);
					}
				}));


		UE_LOG(LogHiderSmartObject, Log, TEXT("%s: Claimed %s, Goal=%s"),
		       *GetNameSafe(Actor),
		       *LexToString(Claim),
		       *Data.GoalLocation.ToCompactString());

		return EStateTreeRunStatus::Running;
	}


	UE_LOG(LogHiderSmartObject, Verbose, TEXT("%s: no free, reachable and claimable hiding slot within %.0f cm."),
	       *GetNameSafe(Actor), Data.SearchRadius);

	return EStateTreeRunStatus::Failed;
}

void FStateTreeClaimHidingSlotTask::ExitState(FStateTreeExecutionContext& Context,
                                              const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	USmartObjectSubsystem* Subsystem = Data.Subsystem.Get();

	if (Subsystem && Data.ClaimHandle.IsValid() && Subsystem->IsSmartObjectSlotValid(Data.ClaimHandle.SlotHandle))
	{
		Subsystem->UnregisterSlotInvalidationCallback(Data.ClaimHandle);

		const bool bReleased = Subsystem->MarkSlotAsFree(Data.ClaimHandle);

		UE_LOG(LogHiderSmartObject, Log, TEXT("%s: Release=%s, %s"),
		       *GetNameSafe(Data.Actor.Get()),
		       bReleased ? TEXT("true") : TEXT("false"),
		       *LexToString(Data.ClaimHandle));
	}

	Data.ClaimHandle.Invalidate();
	Data.Subsystem.Reset();
}
