///////////////////////////////////////////////////////////////////////////////
///
///@file Enginestate.cpp
/// 
/// @brief This file contains the definition of the Entity State. It handles state flags
///       and is utilized to manage engine state between editor and simulation / debug.
///  
///@Authors: Joshua Sim
///Copyright 2024, Digipen Institute of Technology
///                engineState.ClearToBase();
///////////////////////////////////////////////////////////////////////////////mm

#include "pch.h"
#include "EngineState.h"

namespace Framework {
    // Define the global instance
    EngineState engineState;
}