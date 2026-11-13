#pragma once

#include "CoreMinimal.h"

class ACharacter;

namespace HideAndSeek
{
	bool IsCrouchedLocationOccluded(
		const ACharacter* Character,
		const FVector& FeetLocation,
		const FVector& CameraLocation,
		float HorizontalMargin,
		float DebugDuration);
}
