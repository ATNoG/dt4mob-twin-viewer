// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares RegisterAllEntityTypes(), which fills the entity factory's type table.
 * @ingroup entities
 */

#pragma once

class UDT4MOBEntityFactory;

/**
 * @brief Registers every known Ditto entity type, plus any per-thing mesh overrides, on Factory.
 *
 * Each type is registered with its thingId substring key, the USTRUCT used to parse the
 * thing JSON, a display name, the no-server-handling flag, an optional default mesh and an
 * optional behavior extension class (see UDT4MOBEntityFactory::RegisterType()). This is the
 * one place to edit when adding an entity type. It is kept out of DT4MOBEntityFactory.cpp so
 * that file does not have to include every EntityStructs and EntityDependencies header.
 *
 * @param Factory  Factory to populate. Called once, from the UDT4MOBEntityFactory constructor.
 * @ingroup entities
 */
void RegisterAllEntityTypes(UDT4MOBEntityFactory& Factory);
