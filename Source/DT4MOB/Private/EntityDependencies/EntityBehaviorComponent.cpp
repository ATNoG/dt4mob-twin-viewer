// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/EntityBehaviorComponent.h"
#include "Entities/TempUIActor.h"

ATempUIActor* UEntityBehaviorComponent::GetOwnerEntity() const
{
    return Cast<ATempUIActor>(GetOwner());
}
