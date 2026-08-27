#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LoginGameMode.generated.h"

/**
 * @brief Minimal GameMode for the dedicated login level.
 *
 * No pawn, no world geometry, no Cesium — just the login screen. Deliberately decoupled from
 * ADT4MOBGamemode (which is tile-streaming/Cesium-heavy) so none of the main scene's setup runs
 * before the user is authenticated. Sets PlayerControllerClass to ALoginPlayerController and
 * spawns no pawn.
 */
UCLASS()
class DT4MOB_API ALoginGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALoginGameMode();
};
