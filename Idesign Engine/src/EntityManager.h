///////////////////////////////////////////////////////////////////////////////
///
///@file EntityManager.h
/// 
/// @brief This file contains the declaration of the Entity Manager. It handles functions
///       such as the the creation of entities and destruction of entities.
///  
///@Authors: Teo Joel
///Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "Types.h"
#include <array>
#include <cassert>
#include <queue>

namespace Framework {

	class EntityManager
	{
	public:
		EntityManager();										// Constructor for our EntityManager class;

		Entity CreateEntity();									// Function for creating an entity

		void DestroyEntity(Entity entity);						// Passes in an entity, and destroys said entity

		void SetSignature(Entity entity, Signature signature);	// Passes in an entity and signature, then sets the signature of the entity to be the signature passed in

		Signature GetSignature(Entity entity);					// Gets the signature of the entity passed in

	
		void Clear(); 	/// Clears all entity data (used at shutdown or scene reset)

	private:
		std::queue<Entity> queueOfAvailableEntities{};			// Using std::queue to store possible entity IDs that are available at one point in time
		std::array<Signature, MAX_ENTITIES> mSignatures{};		// Using std::array to store signatures of entities. The size is just the maximum number of possible entities we can have
		uint32_t mLivingEntityCount{};							// Added a tracker to track how many entities are currently alive
	};

}

#endif