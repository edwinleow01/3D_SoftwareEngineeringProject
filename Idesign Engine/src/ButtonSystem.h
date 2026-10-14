///////////////////////////////////////////////////////////////////////////////
///
/// @file ButtonSystem.h
/// 
/// @brief Implementation of the ButtonSystem class, which manages interactive 
///        buttons within the game engine. This class provides functionality for the buttons
///  
/// @authors Joshua Sim Yue Chen
/// @copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _BUTTON_SYSTEM_H_
#define _BUTTON_SYSTEM_H_

#include "pch.h"
#include <vector>
#include "Vector2D.h"
#include "System.h"
#include "InputHandler.h"
#include "fstream"
#include "ComponentList.h"
#include "ButtonComponent.h"

namespace Framework
{
    class ButtonSystem : public ISystem
    {
    public:
        enum SliderType { Master, BGM, SFX, Gamma, None };

        ButtonSystem();
        ButtonSystem(InputHandler* inputHandler);

        void Initialize() override;    // Initialize the system
        void Update(float deltaTime) override; // Update the button state (check for clicks and hover)
        std::string GetName() override { return "ButtonSystem"; }
        //void AddButton(ButtonComponent button); // Add a button
        // Hook the global InputHandler singleton into this system
        void SetInputHandler(InputHandler* handler);
        // std::vector<ButtonComponent> GetButtons() const;
        void HandleClickUV(float u, float v);
        void HandleSliderDrag(float u, float v);
        void UpdateHoverUV(float u, float v);
        void SyncSliderToValue(const std::string& barTag, const std::string& fillTag,
            const std::string& knobTag, const std::string& textTag,
            float volume, bool isGamma);

    private:

        //std::vector<ButtonComponent> buttons;             // List of buttons
        InputHandler* m_inputHandler = nullptr;             // Handle input for click events
        static constexpr float DESIGN_WIDTH = 1920.0f;
        static constexpr float DESIGN_HEIGHT = 1080.0f;

        bool m_isDraggingSlider = false;
        Entity m_activeKnob = 0;                // INVALID_ENTITY can just be represented by 0, as it meant NONE

        float m_lastMasterVol = -1.0f;
        float m_lastBGMVol = -1.0f;
        float m_lastSFXVol = -1.0f;
        float m_lastGamma = 0.6f;  // volume=0.6 == gamma 2.2 (neutral default)

        SliderType m_activeSliderType = SliderType::None;     // Track which slider type is currently active
    };
}
#endif // !_BUTTON_SYSTEM_H_