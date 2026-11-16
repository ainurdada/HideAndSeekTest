// Fill out your copyright notice in the Description page of Project Settings.


#include "HideAndSeekGameMode.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/Class.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HiderAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogHideAndSeekRound, Log, All);

AHideAndSeekGameMode::AHideAndSeekGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHideAndSeekGameMode::StartPlay()
{
	Super::StartPlay();

	if (RoundPhase != EHideAndSeekRoundPhase::WaitingToStart)
	{
		return;
	}

	if (!FMath::IsFinite(HidingDuration) || HidingDuration <= 0.0f ||
		(bShowRoundDebug && (!FMath::IsFinite(DebugRefreshInterval) || DebugRefreshInterval <= 0.0f)))
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("Round settings contain an invalid duration."));
		return;
	}

	if (!CollectRoundHiders())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		HidingTimer,
		this,
		&AHideAndSeekGameMode::StartSearching,
		HidingDuration,
		false);

	SetRoundPhase(EHideAndSeekRoundPhase::Hiding);

	if (bShowRoundDebug)
	{
		GetWorldTimerManager().SetTimer(
			DebugTimer,
			this,
			&AHideAndSeekGameMode::RefreshRoundDebug,
			DebugRefreshInterval,
			true);
	}
}

float AHideAndSeekGameMode::GetHidingTimeRemaining() const
{
	if (RoundPhase != EHideAndSeekRoundPhase::Hiding)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, GetWorldTimerManager().GetTimerRemaining(HidingTimer));
}

bool AHideAndSeekGameMode::NotifyHiderFound(AActor* Hider)
{
	if (!IsValid(Hider))
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("Found notification requires a valid hider."));

		return false;
	}

	const TWeakObjectPtr<AActor> HiderKey(Hider);

	if (!RoundHiders.Contains(HiderKey))
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("%s: Found notification rejected: not in the round roster."),
		       *GetNameSafe(Hider));

		return false;
	}

	const UHiderAbilitySystemComponent* AbilitySystem = Hider->FindComponentByClass<UHiderAbilitySystemComponent>();

	if (!IsValid(AbilitySystem) || !AbilitySystem->IsFound())
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("%s: Found notification requires the Found status."),
		       *GetNameSafe(Hider));

		return false;
	}

	if (FoundHiders.Contains(HiderKey))
	{
		UE_LOG(LogHideAndSeekRound, Log, TEXT("%s: Duplicate Found notification ignored."),
		       *GetNameSafe(Hider));

		return true;
	}

	if (RoundPhase != EHideAndSeekRoundPhase::Searching)
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("%s: New Found notification requires Searching."),
		       *GetNameSafe(Hider));

		return false;
	}

	FoundHiders.Add(HiderKey);

	UE_LOG(LogHideAndSeekRound, Log, TEXT("Found %d/%d: %s."),
	       FoundHiders.Num(),
	       RoundHiders.Num(),
	       *GetNameSafe(Hider));

	if (FoundHiders.Num() == RoundHiders.Num())
	{
		SetRoundPhase(EHideAndSeekRoundPhase::Finished);
	}
	else
	{
		OnRoundPresentationChanged.Broadcast();
		RefreshRoundDebug();
	}

	return true;
}

void AHideAndSeekGameMode::RequestRestartRound()
{
	if (bRestartRequested)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World) || !World->IsGameWorld())
	{
		UE_LOG(LogHideAndSeekRound, Warning, TEXT("Restart requires a running game world."));

		return;
	}

	const FString MapPackageName = UWorld::RemovePIEPrefix(World->GetPackage()->GetName());

	if (MapPackageName.IsEmpty() || !FPackageName::DoesPackageExist(MapPackageName))
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("Cannot restart: map package does not exist: %s."),
		       *MapPackageName);

		return;
	}

	bRestartRequested = true;

	UE_LOG(LogHideAndSeekRound, Log, TEXT("Restart requested: %s."),
	       *MapPackageName);

	UGameplayStatics::OpenLevel(this, FName(*MapPackageName));
}

void AHideAndSeekGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(HidingTimer);
	GetWorldTimerManager().ClearTimer(DebugTimer);

	if (GEngine)
	{
		GEngine->RemoveOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()));
	}

	Super::EndPlay(EndPlayReason);
}

void AHideAndSeekGameMode::StartSearching()
{
	if (RoundPhase == EHideAndSeekRoundPhase::Hiding)
	{
		SetRoundPhase(EHideAndSeekRoundPhase::Searching);
	}
}

void AHideAndSeekGameMode::SetRoundPhase(EHideAndSeekRoundPhase NewPhase)
{
	if (RoundPhase == NewPhase)
	{
		return;
	}

	const EHideAndSeekRoundPhase PreviousPhase = RoundPhase;
	RoundPhase = NewPhase;

	UE_LOG(LogHideAndSeekRound, Log, TEXT("%s -> %s"), *UEnum::GetValueAsString(PreviousPhase),
	       *UEnum::GetValueAsString(RoundPhase));

	OnRoundPhaseChanged.Broadcast(PreviousPhase, RoundPhase);
	OnRoundPresentationChanged.Broadcast();

	RefreshRoundDebug();
}

void AHideAndSeekGameMode::RefreshRoundDebug()
{
	if (!bShowRoundDebug || !GEngine)
	{
		return;
	}

	FString Message;
	FColor Color = FColor::White;

	switch (RoundPhase)
	{
	case EHideAndSeekRoundPhase::WaitingToStart:
		Message = TEXT("HNS | WaitingToStart");
		break;

	case EHideAndSeekRoundPhase::Hiding:
		Message = FString::Printf(TEXT("HNS | Hiding: %.1f s"), GetHidingTimeRemaining());
		Color = FColor::Yellow;
		break;

	case EHideAndSeekRoundPhase::Searching:
		Message = FString::Printf(
			TEXT("HNS | Searching | Found: %d/%d"),
			GetFoundHiderCount(),
			GetTotalHiderCount());

		Color = FColor::Green;
		break;

	case EHideAndSeekRoundPhase::Finished:
		Message = FString::Printf(
			TEXT("HNS | Finished | All hiders found! (%d/%d)"),
			GetFoundHiderCount(),
			GetTotalHiderCount());

		Color = FColor::Cyan;
		break;
	}

	Message += TEXT(" | R: Restart round");

	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), DebugRefreshInterval * 2.0f,
	                                 Color, Message);
}

bool AHideAndSeekGameMode::CollectRoundHiders()
{
	RoundHiders.Reset();
	FoundHiders.Reset();

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Pawn = *It;

		if (!IsValid(Pawn) || Pawn->IsPlayerControlled())
		{
			continue;
		}

		const UHiderAbilitySystemComponent* AbilitySystem = Pawn->FindComponentByClass<UHiderAbilitySystemComponent>();

		if (!IsValid(AbilitySystem))
		{
			continue;
		}

		if (AbilitySystem->IsFound())
		{
			UE_LOG(LogHideAndSeekRound, Error, TEXT("%s: Cannot start a round with an already Found hider."),
			       *GetNameSafe(Pawn));

			RoundHiders.Reset();
			return false;
		}

		RoundHiders.Add(TWeakObjectPtr<AActor>(Pawn));

		UE_LOG(LogHideAndSeekRound, Log, TEXT("Registered hider: %s."),
		       *GetNameSafe(Pawn));
	}

	if (RoundHiders.Num() == 0)
	{
		UE_LOG(LogHideAndSeekRound, Error, TEXT("Cannot start round: no hiders registered."));

		return false;
	}

	UE_LOG(LogHideAndSeekRound, Log, TEXT("Round roster ready: %d hiders."),
	       RoundHiders.Num());

	return true;
}
