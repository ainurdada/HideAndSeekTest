// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "HideAndSeekGameMode.generated.h"

UENUM(BlueprintType)
enum class EHideAndSeekRoundPhase : uint8
{
	WaitingToStart,
	Hiding,
	Searching
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHideAndSeekRoundPhaseChanged, EHideAndSeekRoundPhase, EHideAndSeekRoundPhase);

UCLASS()
class HIDEANDSEEK_API AHideAndSeekGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHideAndSeekGameMode();

	virtual void StartPlay() override;

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	EHideAndSeekRoundPhase GetRoundPhase() const
	{
		return RoundPhase;
	}

	UFUNCTION(BlueprintPure, Category = "HideAndSeek|Round")
	float GetHidingTimeRemaining() const;

	FOnHideAndSeekRoundPhaseChanged OnRoundPhaseChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "HideAndSeek|Round", meta=(ClampMin = 0.1f, Units = "s"))
	float HidingDuration = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideAndSeek|Debug")
	bool bShowRoundDebug = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HideAndSeek|Debug",
		meta = (ClampMin = "0.05", Units = "s"))
	float DebugRefreshInterval = 0.2f;

private:
	void StartSearching();
	void SetRoundPhase(EHideAndSeekRoundPhase NewPhase);
	void RefreshRoundDebug();
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "HideAndSeek|Round", meta=(AllowPrivateAccess="true"))
	EHideAndSeekRoundPhase RoundPhase = EHideAndSeekRoundPhase::WaitingToStart;
	
	FTimerHandle HidingTimer;
	FTimerHandle DebugTimer;
};
