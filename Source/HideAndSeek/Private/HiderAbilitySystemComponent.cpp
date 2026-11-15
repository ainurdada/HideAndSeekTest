#include "HiderAbilitySystemComponent.h"

#include "GameFramework/Actor.h"
#include "HideAndSeekGameplayTags.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogHiderASC, Log, All);

void UHiderAbilitySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	const bool bWasFound = IsFound();
	const bool bWasHidden = IsHidden() && !bWasFound;

	SetLooseGameplayTagCount(TAG_HNS_Status_Unfound, bWasFound ? 0 : 1);
	SetLooseGameplayTagCount(TAG_HNS_Status_Found, bWasFound ? 1 : 0);
	SetLooseGameplayTagCount(TAG_HNS_Status_Hidden, bWasHidden ? 1 : 0);

	LogStatus(TEXT("Initialized"));

	if (!RevealMarkerChangedHandle.IsValid())
	{
		RevealMarkerChangedHandle = RegisterGameplayTagEvent(
				TAG_HNS_Marker_Revealed,
				EGameplayTagEventType::NewOrRemoved)
			.AddUObject(
				this,
				&UHiderAbilitySystemComponent::OnRevealMarkerChanged);
	}
}

void UHiderAbilitySystemComponent::UninitializeComponent()
{
	if (RevealMarkerChangedHandle.IsValid())
	{
		UnregisterGameplayTagEvent(
			RevealMarkerChangedHandle,
			TAG_HNS_Marker_Revealed,
			EGameplayTagEventType::NewOrRemoved);

		RevealMarkerChangedHandle.Reset();
	}

	Super::UninitializeComponent();
}

bool UHiderAbilitySystemComponent::IsFound() const
{
	return GetOwnedGameplayTags().HasTagExact(TAG_HNS_Status_Found);
}

bool UHiderAbilitySystemComponent::IsHidden() const
{
	return GetOwnedGameplayTags().HasTagExact(TAG_HNS_Status_Hidden);
}

bool UHiderAbilitySystemComponent::IsRevealed() const
{
	return GetOwnedGameplayTags().HasTagExact(TAG_HNS_Marker_Revealed);
}

bool UHiderAbilitySystemComponent::SetHidden(bool bHidden)
{
	if (bHidden)
	{
		const bool bIsUnfound = GetOwnedGameplayTags().HasTagExact(TAG_HNS_Status_Unfound);

		if (IsFound() || !bIsUnfound)
		{
			UE_LOG(LogHiderASC, Warning, TEXT("%s: SetHidden(true) requires Unfound and no Found."),
			       *GetNameSafe(GetOwner()));

			return false;
		}
	}

	const bool bWasHidden = IsHidden();

	SetLooseGameplayTagCount(TAG_HNS_Status_Hidden, bHidden ? 1 : 0);

	if (bWasHidden != bHidden)
	{
		LogStatus(bHidden ? TEXT("Hidden enabled") : TEXT("Hidden cleared"));
	}

	return true;
}

bool UHiderAbilitySystemComponent::TryMarkFound()
{
	if (IsFound())
	{
		return false;
	}

	SetLooseGameplayTagCount(TAG_HNS_Status_Found, 1);
	SetLooseGameplayTagCount(TAG_HNS_Status_Unfound, 0);
	SetLooseGameplayTagCount(TAG_HNS_Status_Hidden, 0);
	
	FGameplayTagContainer MarkerTags;
	MarkerTags.AddTag(TAG_HNS_Marker_Revealed);
	RemoveActiveEffectsWithGrantedTags(MarkerTags);
	
	LogStatus(TEXT("Marked found"));

	return true;
}

bool UHiderAbilitySystemComponent::ApplyRevealMarker()
{
	if (IsFound())
	{
		UE_LOG(LogHiderASC, Log, TEXT("%s: Reveal marker ignored: already Found."),
		       *GetNameSafe(GetOwner()));

		return false;
	}

	const UGameplayEffect* EffectDefinition = RevealMarkerEffect.GetDefaultObject();

	if (!EffectDefinition || !EffectDefinition->GetGrantedTags().HasTagExact(TAG_HNS_Marker_Revealed))
	{
		UE_LOG(LogHiderASC, Error, TEXT("%s: RevealMarkerEffect must grant HideAndSeek.Marker.Revealed."),
		       *GetNameSafe(GetOwner()));

		return false;
	}

	const FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(
		RevealMarkerEffect,
		1.0f,
		MakeEffectContext());

	if (!Spec.IsValid())
	{
		UE_LOG(LogHiderASC, Error, TEXT("%s: Reveal marker requires a valid spec"),
		       *GetNameSafe(GetOwner()));

		return false;
	}

	const bool bWasRevealed = IsRevealed();

	const FActiveGameplayEffectHandle Handle = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	if (!Handle.IsValid())
	{
		UE_LOG(LogHiderASC, Warning, TEXT("%s: Reveal marker application failed."),
		       *GetNameSafe(GetOwner()));

		return false;
	}

	UE_LOG(LogHiderASC, Log, TEXT("%s: Reveal marker %s, Duration=%.2f"),
	       *GetNameSafe(GetOwner()),
	       bWasRevealed ? TEXT("refreshed") : TEXT("applied"),
	       Spec.Data->GetDuration());
	
	OnRevealApplied.Broadcast();

	return true;
}

void UHiderAbilitySystemComponent::DebugApplyRevealMarker()
{
	const UWorld* World = GetWorld();

	if (!World || !World->IsGameWorld())
	{
		UE_LOG(LogHiderASC, Warning, TEXT("DebugApplyRevealMarker requires a running game instance."));

		return;
	}

	ApplyRevealMarker();
}

void UHiderAbilitySystemComponent::OnRevealMarkerChanged(const FGameplayTag Tag, int32 NewCount)
{
	UE_LOG(LogHiderASC, Log, TEXT("%s: %s Count=%d"),
	       *GetNameSafe(GetOwner()),
	       *Tag.ToString(),
	       NewCount);
	
	OnRevealMarkerVisibilityChanged.Broadcast(NewCount > 0);
}

void UHiderAbilitySystemComponent::LogStatus(const TCHAR* Reason) const
{
	UE_LOG(LogHiderASC, Log, TEXT("%s: %s | Unfound=%d Hidden=%d Found=%d"),
	       *GetNameSafe(GetOwner()),
	       Reason,
	       GetGameplayTagCount(TAG_HNS_Status_Unfound),
	       GetGameplayTagCount(TAG_HNS_Status_Hidden),
	       GetGameplayTagCount(TAG_HNS_Status_Found));
}
