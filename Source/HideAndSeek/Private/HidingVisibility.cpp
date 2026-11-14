#include "HidingVisibility.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

bool HideAndSeek::IsCrouchedLocationOccluded(const ACharacter* Character, const FVector& FeetLocation,
                                             const FVector& CameraLocation, float HorizontalMargin, float DebugDuration)
{
	if (!IsValid(Character))
	{
		return false;
	}

	UWorld* World = Character->GetWorld();
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	if (!World || !Capsule || !Movement)
	{
		return false;
	}

	const float Radius = FMath::Abs(Capsule->GetScaledCapsuleRadius()) + HorizontalMargin;

	const float UnscaledHalfHeight = FMath::Max3(
		0.0f,
		Capsule->GetUnscaledCapsuleRadius(),
		Movement->GetCrouchedHalfHeight());

	const float HeightScale = static_cast<float>(FMath::Abs(Capsule->GetComponentTransform().GetScale3D().Z));

	const float HalfHeight = HeightScale * UnscaledHalfHeight;

	const FVector ProbeOffsets[] = {
		FVector(0.0f, 0.0f, HalfHeight),
		FVector(0.0f, 0.0f, HalfHeight * 2.0f),
		FVector(0.0f, 0.0f, HalfHeight * 0.25f),
		FVector(Radius, 0.0f, HalfHeight),
		FVector(-Radius, 0.0f, HalfHeight),
		FVector(0.0f, Radius, HalfHeight),
		FVector(0.0f, -Radius, HalfHeight)
	};

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HiderCoverTrace), true);

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		QueryParams.AddIgnoredActor(*It);
	}

	bool bAllOccluded = true;

	for (const FVector& Offest : ProbeOffsets)
	{
		const FVector ProbeLocation = FeetLocation + Offest;

		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(
			Hit,
			CameraLocation,
			ProbeLocation,
			ECC_Visibility,
			QueryParams);

		const bool bOccluded = bHit && !Hit.bStartPenetrating;
		bAllOccluded &= bOccluded;

		if (DebugDuration > 0.0f)
		{
			const FColor Color = bOccluded ? FColor::Green : FColor::Red;

			DrawDebugLine(World, CameraLocation, ProbeLocation,
			              Color, false, DebugDuration, 0, 1.0f);

			DrawDebugSphere(
				World, ProbeLocation, 4.0f, 8,
				Color, false, DebugDuration);
		}
	}

	return bAllOccluded;
}
