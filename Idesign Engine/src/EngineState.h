///////////////////////////////////////////////////////////////////////////////
///
///@file Enginestate.h
/// 
/// @brief This file contains the definition of the Entity State. It handles state flags
///       and is utilized to manage engine state between editor and simulation / debug.
///  
/// @Authors: Joshua Sim
/// Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pch.h"

namespace Framework
{
    // Current enum represents each logical mode of the engine/UI
    enum EngineMode
    {
        Editor,
        Play,
        Paused,
        Debug,
        Menu,
        HowToPlay,
        Upgrade,
        Settings_Audio,
        Settings_Gamma,
        Dialogue,
        MapSelection,
        Cutscene,
        Intro,
        Day2,
        Day3,
        Day4,
        Day5,
        Win,
        Lose,
        Credit,
        DayStats,   // Per-day results screen       (shown after night phase ends)
        EndStats    // End-of-game journey screen   (shown after win/lose cutscene)
    };

    class EngineState
    {
    public:

        // Adds a new active state on top of the stack
        void PushState(EngineMode state)
        {
            if (!stateStack.empty() && stateStack.back() == state) return;
            stateStack.push_back(state);
        }

        // Remove the current state, returning to the prev one
        void PopState()
        {
            if (stateStack.size() > 1) stateStack.pop_back();
        }

        // Hard Reset, remove all UI/nav states and return to gameplay/editor
        void ClearToBase()
        {
            while (stateStack.size() > 1) stateStack.pop_back();
        }

        // Returns the current active state aka top of stack
        EngineMode Current() const
        {
            return stateStack.back();
        }

        // Check for current state at the top
        bool Is(EngineMode state) const
        {
            return Current() == state;
        }

        // Check if state is within stack
        bool HasState(EngineMode state) const
        {
            for (EngineMode s : stateStack)
                if (s == state) return true;
            return false;
        }

        // Check if any MENU/UI overlay is open
        bool IsAnyUIOpen() const
        {
            return Is(EngineMode::Menu) ||
                   Is(EngineMode::Paused) ||
                   Is(EngineMode::HowToPlay) ||
                   Is(EngineMode::Settings_Audio) ||
                   Is(EngineMode::Settings_Gamma) ||
                   Is(EngineMode::Dialogue) ||
                   Is(EngineMode::Upgrade) ||
                   Is(EngineMode::MapSelection) ||
                   Is(EngineMode::Cutscene) ||
                   Is(EngineMode::Win) ||
                   Is(EngineMode::Lose) ||
                   Is(EngineMode::Day2) ||
                   Is(EngineMode::Day3) ||
                   Is(EngineMode::Day4) ||
                   Is(EngineMode::Day5) ||
                   Is(EngineMode::Credit) ||
                   Is(EngineMode::DayStats) ||
                   Is(EngineMode::EndStats) ||
                   Is(EngineMode::Intro);
        }

        // Check if current scene is actively in gameplay (Play Mode, with no overlays)
        bool IsInGameplay() const
        {
            return Is(EngineMode::Play) && !IsAnyUIOpen();
        }

        void PrintStack() const
        {
            std::cout << "[EngineState] Current Stack (Bottom -> Top): ";
            for (size_t i = 0; i < stateStack.size(); ++i)
            {
                switch (stateStack[i])
                {
                    case Editor:         std::cout << "Editor";          break;
                    case Play:           std::cout << "Play";            break;
                    case Paused:         std::cout << "Paused";          break;
                    case Debug:          std::cout << "Debug";           break;
                    case Menu:           std::cout << "Menu";            break;
                    case HowToPlay:      std::cout << "HowToPlay";       break;
                    case Upgrade:        std::cout << "Upgrade";         break;
                    case Settings_Audio: std::cout << "Settings_Audio";  break;
                    case Settings_Gamma: std::cout << "Settings_Gamma";  break;
                    case Dialogue:       std::cout << "Dialogue";        break;
                    case DayStats:       std::cout << "DayStats";        break;
                    case EndStats:       std::cout << "EndStats";        break;
                    default:             std::cout << "Unknown";         break;
                }
                if (i < stateStack.size() - 1) std::cout << " | ";
            }
            std::cout << std::endl;
        }

    private:
        std::vector<EngineMode> stateStack{ };
    };
    extern EngineState engineState;
}