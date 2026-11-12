// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Implementation of UEntityBehaviorComponent::GetOwnerEntity().
 * @ingroup entitydeps
 */

#include "EntityDependencies/EntityBehaviorComponent.h"
#include "Entities/TempUIActor.h"

ATempUIActor* UEntityBehaviorComponent::GetOwnerEntity() const
{
    return Cast<ATempUIActor>(GetOwner());
}
