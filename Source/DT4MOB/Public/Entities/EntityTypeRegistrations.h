// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class UDT4MOBEntityFactory;

/**
 * @brief Registers every known Ditto entity type (struct, display name, default mesh, and
 *        optional behavior extension) on Factory, plus any per-instance mesh overrides.
 *
 * This is the one place to touch when adding a new entity type — see
 * UDT4MOBEntityFactory::RegisterType() for what each call does. Kept out of
 * DT4MOBEntityFactory.cpp so that file doesn't need to include every EntityStruct/
 * EntityDependencies header just to add a type.
 *
 * @param Factory  The factory instance to populate. Called once from its constructor.
 */
void RegisterAllEntityTypes(UDT4MOBEntityFactory& Factory);
