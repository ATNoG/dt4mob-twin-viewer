/** @file LoginGameMode.cpp
 *  @brief Implementation of ALoginGameMode. All logic documentation is in the header.
 */
#include "Gameplay/LoginGameMode.h"
#include "Gameplay/LoginPlayerController.h"

ALoginGameMode::ALoginGameMode()
{
	PlayerControllerClass = ALoginPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
