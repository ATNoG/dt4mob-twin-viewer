// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file
 * @brief Registers DT4MOB as the primary game module, using the default module implementation
 *        (no custom startup or shutdown logic).
 * @ingroup core
 */
#include "DT4MOB.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, DT4MOB, "DT4MOB");
