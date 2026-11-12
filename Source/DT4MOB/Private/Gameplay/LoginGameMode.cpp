/**
 * @file
 * @brief Implements ALoginGameMode (no pawn, login player controller).
 * @ingroup gameplay
 */
#include "Gameplay/LoginGameMode.h"
#include "Gameplay/LoginPlayerController.h"

ALoginGameMode::ALoginGameMode()
{
	PlayerControllerClass = ALoginPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
