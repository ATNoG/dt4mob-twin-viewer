// Fill out your copyright notice in the Description page of Project Settings.

/** @file MapPinEntity.cpp
 *  @brief Implementation of AMapPinEntity. All logic documentation is in the header.
 */
#include "Entities/MapPinEntity.h"

// Sets default values
AMapPinEntity::AMapPinEntity()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMapPinEntity::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMapPinEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
