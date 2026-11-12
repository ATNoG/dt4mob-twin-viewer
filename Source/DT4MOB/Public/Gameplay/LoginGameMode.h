/**
 * @file
 * @brief Declares ALoginGameMode, the game mode used by the dedicated login level.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LoginGameMode.generated.h"

/**
 * @brief Minimal game mode for the dedicated login level.
 *
 * The login level has no pawn, no world geometry and no Cesium; it only shows the login
 * screen. This mode is kept separate from ADT4MOBGamemode (tile streaming and Cesium setup)
 * so none of the main scene's setup runs before the user is authenticated.
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API ALoginGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** @brief Sets PlayerControllerClass to ALoginPlayerController and clears DefaultPawnClass so no pawn spawns. */
	ALoginGameMode();
};
