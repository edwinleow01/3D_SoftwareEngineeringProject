///////////////////////////////////////////////////////////////////////////////
///
///@file Types.h
/// 
/// @brief This file contains the definition of certain data types with the using
///       keyword. It is currently being used for the ECS implementation
///  
///@Authors: Teo Joel
///Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <cstdint>
#include <bitset>

namespace Framework {

	/*
		ENTITY RELATED
	*/
	using Entity = std::uint32_t;					// An entity, which is just an ID, is set to be an unsigned integer of 32 bits.
	const Entity MAX_ENTITIES = 9999;				// Maximum number of entities is set to 5000. Can be changed if needed.


	/*
		COMPONENT RELATED
	*/
	using ComponentType = std::uint8_t;				// A component type, which is just the number of components for later use, is set to be unsigned integer of 8 bits.
	const ComponentType MAX_COMPONENTS = 64;		// Maxmimum number of components is set to 32, for our array later.

	/*
		SIGNATURE / ENTITY AND SYSTEM RELATED
	*/
	using Signature = std::bitset<MAX_COMPONENTS>;	// A signature, which tells the program what components an entity has or a system has, is a bitset that has MAX_COMPONENTS number of bits.

}