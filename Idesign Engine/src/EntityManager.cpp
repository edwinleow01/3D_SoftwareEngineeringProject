///////////////////////////////////////////////////////////////////////////////
///
///@file EntityManager.cpp
/// 
/// @brief This file contains the definition of the Entity Manager. It handles functions
///       such as the the creation of entities and destruction of entities.
///  
///@Authors: Teo Joel
///Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "EntityManager.h"

namespace Framework {

	// Constructor for our EntityManager class;
	EntityManager::EntityManager()
	{
		for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
		{
			queueOfAvailableEntities.push(entity);
		}
	}

	// Returns an entity (ID) from the front of the entity queue. Number is taken from queueOfAvailableEntities, which is a queue.
	Entity EntityManager::CreateEntity()
	{
		// if current number of entities are lesser than defined max, carry on with execution. Else error message pops up
		//assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities in existence!");

		Entity id = queueOfAvailableEntities.front();		// Assigns an available ID to an entity. Always gives the front of the queue
		queueOfAvailableEntities.pop();					// Removes the said value from the queue. Because it is given
		++mLivingEntityCount;						// Adds to the number of current entities in existence

		return id;
	}

	// Clear memory on shutdown
	void EntityManager::Clear()
	{
		// Reset the queue of available entities
		std::queue<Entity> emptyQueue;
		std::swap(queueOfAvailableEntities, emptyQueue);

		// Reset all signatures to default
		for (auto& sig : mSignatures)
		{
			sig.reset(); // or sig = Signature(); if Signature is a bitset
		}

		// Reset living entity count
		mLivingEntityCount = 0;
	}	
	// Passes in an entity, and destroys said entity
	void EntityManager::DestroyEntity(Entity entity)
	{
		//assert(entity < MAX_ENTITIES && "Entity out of range!");

		mSignatures[entity].reset();				// Sets the bitset (signature) of the given entity to all 0's
		queueOfAvailableEntities.push(entity);			// Returns the ID that entity was back to the queue of available entities
		--mLivingEntityCount;						// Subtracts to the number of current entities in existence
	}

	// Passes in an entity and signature, then sets the signature of the entity to be the signature passed in
	void EntityManager::SetSignature(Entity entity, Signature signature)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range!");

		mSignatures[entity] = signature;
	}

	// Passes in an entity and returns the signature of the entity
	Signature EntityManager::GetSignature(Entity entity)
	{
		assert(entity < MAX_ENTITIES && "Entity out of range!");

		return mSignatures[entity];
	}

}