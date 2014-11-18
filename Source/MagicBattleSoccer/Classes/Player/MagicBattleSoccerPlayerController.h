

#pragma once

#include "GameFramework/PlayerController.h"
#include "MagicBattleSoccerPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MAGICBATTLESOCCER_API AMagicBattleSoccerPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

private:
	int64 timeServerTimeRequestWasPlaced;
	int64 timeOffsetFromServer;
	bool timeOffsetIsValid;

public:
	/** stores pawn location at last player death, used where player scores a kill after they died **/
	FVector LastDeathLocation;

	/** True if the network time is valid. */
	bool IsNetworkTimeValid();

	/** Gets the current system time in milliseconds */
	static int64 GetLocalTime();

	/** Gets the approximate current network time in milliseconds. */
	int64 GetNetworkTime();

	//Begin AController interface

	/** sets up input */
	virtual void SetupInputComponent() override;

	/** This occurs when play begins */
	virtual void BeginPlay() override;

	/** Sets the pawn */
	virtual void SetPawn(APawn* inPawn) override;

	/** update camera when pawn dies */
	virtual void PawnPendingDestroy(APawn* inPawn) override;

	//End AController interface

	/** try to find spot for death cam */
	bool FindDeathCameraSpot(FVector& CameraLocation, FRotator& CameraRotation);

	//////////////////////////////////////////////////////////////////////////
	// Input handlers

	/** Player move forward event */
	void OnMoveForward(float axisValue);

	/** Player move right event */
	void OnMoveRight(float axisValue);

	/** Player primary action event */
	void OnStartPrimaryAction();

	/** Player primary action event */
	void OnStopPrimaryAction();

	/** Player suicide event */
	void OnSuicide();

protected:
	/** Sent from a client to the server to get the server's system time */
	UFUNCTION(reliable, server, WithValidation)
	void ServerGetServerTime();

	/** Sent from the server to a client to give them the server's system time */
	UFUNCTION(reliable, client)
	void ClientGetServerTime(int64 serverTime);
};