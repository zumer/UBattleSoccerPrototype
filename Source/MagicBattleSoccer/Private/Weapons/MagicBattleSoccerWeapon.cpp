
#include "MagicBattleSoccer.h"
#include "MagicBattleSoccerGoal.h"
#include "MagicBattleSoccerWeapon.h"
#include "MagicBattleSoccerPlayer.h"
#include "MagicBattleSoccerPlayerController.h"

AMagicBattleSoccerWeapon::AMagicBattleSoccerWeapon(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	MyPawn = nullptr;
	bIsEquipped = false;
	bWantsToFire = false;
	bRefiring = false;
	CurrentState = EWeaponState::Idle;
	LastFireTime = 0.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateInstigator = true;
	bNetUseOwnerRelevancy = true;

	// LEGACY
	//IsFiring = false;
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AMagicBattleSoccerWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMagicBattleSoccerWeapon, MyPawn);
}

void AMagicBattleSoccerWeapon::OnRep_MyPawn()
{
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AMagicBattleSoccerWeapon::OnEquip()
{
	AttachMeshToPawn();

	bIsEquipped = true;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState();
}

void AMagicBattleSoccerWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	DetermineWeaponState();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMagicBattleSoccerWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AMagicBattleSoccerWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

//////////////////////////////////////////////////////////////////////////
// Control

bool AMagicBattleSoccerWeapon::CanFire() const
{
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return (bStateOKToFire == true);
}

//////////////////////////////////////////////////////////////////////////
// Reading data

EWeaponState::Type AMagicBattleSoccerWeapon::GetCurrentState() const
{
	return CurrentState;
}

void AMagicBattleSoccerWeapon::SetOwningPawn(AMagicBattleSoccerPlayer* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input - server side

bool AMagicBattleSoccerWeapon::ServerStartFire_Validate()
{
	return true;
}

void AMagicBattleSoccerWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AMagicBattleSoccerWeapon::ServerStopFire_Validate()
{
	return true;
}

void AMagicBattleSoccerWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

bool AMagicBattleSoccerWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AMagicBattleSoccerWeapon::ServerHandleFiring_Implementation()
{
	HandleFiring();
}

void AMagicBattleSoccerWeapon::HandleFiring()
{
	if (CanFire())
	{
		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			FireWeapon();
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(this, &AMagicBattleSoccerWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

void AMagicBattleSoccerWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void AMagicBattleSoccerWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if ((bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}

	SetWeaponState(NewState);
}

void AMagicBattleSoccerWeapon::OnEnterInventory(AMagicBattleSoccerPlayer* NewOwner)
{
	SetOwningPawn(NewOwner);
}

void AMagicBattleSoccerWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetOwningPawn(NULL);
	}

	if (IsAttachedToPawn())
	{
		OnUnEquip();
	}
}

bool AMagicBattleSoccerWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AMagicBattleSoccerWeapon::IsAttachedToPawn() const
{
	return bIsEquipped;
}

void AMagicBattleSoccerWeapon::AttachMeshToPawn()
{
	if (nullptr != MyPawn)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// Attach the weapon to the player's right hand
		USkeletalMeshComponent* PlayerMesh = Cast<USkeletalMeshComponent>(MyPawn->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		AttachRootComponentTo(PlayerMesh, FName(TEXT("RightHand")), EAttachLocation::SnapToTarget);

		// Show our mesh
		SetActorHiddenInGame(false);
	}
}

void AMagicBattleSoccerWeapon::DetachMeshFromPawn()
{
	DetachRootComponentFromParent();
	SetActorHiddenInGame(true);
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

FVector AMagicBattleSoccerWeapon::GetAdjustedAim() const
{
	AMagicBattleSoccerPlayerController* const PlayerController = Instigator ? Cast<AMagicBattleSoccerPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	// If we have a player controller use it for the aim
	if (PlayerController)
	{
		// Aim where the mouse is pointing
		FVector WorldLocation;
		FVector WorldDirection;
		if (!PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		{
			// Failed. Return a zero vector which will result in no fire action taking place.
		}
		else
		{
			// Calculate the point on the plane Z=0 that the mouse is pointing at
			float d = FVector::DotProduct((FVector::ZeroVector - WorldLocation), FVector::UpVector)
				/ FVector::DotProduct(WorldDirection, FVector::UpVector);
			FVector InstigatorPoint = Instigator->GetActorLocation();
			FVector GroundPoint = WorldLocation + WorldDirection * d;
			FinalAim = FVector(GroundPoint.X - InstigatorPoint.X,
				GroundPoint.Y - InstigatorPoint.Y, 0.0f);
			FinalAim.Normalize();
		}
	}
	else if (nullptr != Instigator)
	{
		// Must be an AI player
		return Instigator->GetActorForwardVector();
	}

	return FinalAim;
}

FVector AMagicBattleSoccerWeapon::GetMuzzleLocation() const
{
	return GetActorLocation();
}

void AMagicBattleSoccerWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(this, &AMagicBattleSoccerWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AMagicBattleSoccerWeapon::OnBurstFinished()
{
	GetWorldTimerManager().ClearTimer(this, &AMagicBattleSoccerWeapon::HandleFiring);
	bRefiring = false;
}

//////////////////////////////////////////////////////////////////////////
// LEGACY


/** Activates the weapon's primary function */
/*void AMagicBattleSoccerWeapon::BeginFire_Implementation()
{
	IsFiring = true;
}*/

/** Deactivates the weapon's primary function */
/*void AMagicBattleSoccerWeapon::CeaseFire_Implementation()
{
	IsFiring = false;
}*/
