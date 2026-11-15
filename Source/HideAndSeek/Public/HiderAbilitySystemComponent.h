#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "HiderAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHiderRevealMarkerVisibilityChanged, bool, bRevealed);

DECLARE_MULTICAST_DELEGATE(FOnHiderRevealApplied);

UCLASS(ClassGroup = (HideAndSeek), meta = (BlueprintSpawnableComponent, DisplayName = "Hider Ability System"))
class HIDEANDSEEK_API UHiderAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

	bool IsFound() const;
	bool IsHidden() const;
	bool IsRevealed() const;

	bool SetHidden(bool bHidden);
	bool TryMarkFound();

	bool ApplyRevealMarker();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "HideAndSeek|Debug")
	void DebugApplyRevealMarker();

	UPROPERTY(BlueprintAssignable, Category = "HideAndSeek|Reveal")
	FOnHiderRevealMarkerVisibilityChanged OnRevealMarkerVisibilityChanged;

	FOnHiderRevealApplied OnRevealApplied;

private:
	UPROPERTY(EditDefaultsOnly, Category = "HideAndSeek|Reveal")
	TSubclassOf<UGameplayEffect> RevealMarkerEffect;

	FDelegateHandle RevealMarkerChangedHandle;

	void OnRevealMarkerChanged(const FGameplayTag Tag, int32 NewCount);
	void LogStatus(const TCHAR* Reason) const;
};
