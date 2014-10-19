

#pragma once

#include "GameFramework/GameMode.h"
#include "MagicBattleSoccerGameMode_Menu.generated.h"

/**
 * 
 */
UCLASS()
class MAGICBATTLESOCCER_API AMagicBattleSoccerGameMode_Menu : public AGameMode
{
	GENERATED_UCLASS_BODY()

public:

	// Begin AGameMode interface
	/** skip it, menu doesn't require player start or pawn */
	virtual void RestartPlayer(class AController* NewPlayer) override;

	// End AGameMode interface

};
