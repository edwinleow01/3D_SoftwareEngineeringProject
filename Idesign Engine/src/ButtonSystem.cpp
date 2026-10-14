///////////////////////////////////////////////////////////////////////////////
///
/// @file ButtonSystem.cpp
/// 
/// @brief Implementation of the ButtonSystem class, which manages interactive 
///        buttons within the game engine. This class provides functionality for the buttons
///  
/// @authors Joshua Sim Yue Chen
/// @copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <iostream>
#include <sstream>
#include "Coordinator.h"
#include "ButtonSystem.h"
#include "TextureProperties.h"
#include "EngineState.h"
#include "TextProperties.h"
#include "TextureLoader.h"
#include "AssetManager.h"
#include "Graphics.h"

extern Framework::Coordinator ecsInterface;
namespace Framework {

    ButtonSystem::ButtonSystem() : m_inputHandler(nullptr) {}
    ButtonSystem::ButtonSystem(InputHandler* inputHandler)
        : m_inputHandler(inputHandler),
        m_isDraggingSlider(false),
        m_activeKnob(0) {
    }

    void ButtonSystem::SetInputHandler(InputHandler* handler)
    {
        m_inputHandler = handler;
    }

    // ---------------------------------------------------------------------
    // Syncs a slider's knob position, fill bar, and text to a given volume
    // value (0-1). isGamma=true formats text as "X.XX", otherwise "XX%".
    // ---------------------------------------------------------------------
    void ButtonSystem::SyncSliderToValue(const std::string& barTag, const std::string& fillTag,
        const std::string& knobTag, const std::string& textTag,
        float volume, bool isGamma)
    {
        volume = std::clamp(volume, 0.0f, 1.0f);

        // --- Find background bar ---
        auto bars = ecsInterface.GetEntitiesByTag(barTag);
        if (bars.empty()) return;
        Entity bar = bars.front();
        if (!ecsInterface.IsEntityValid(bar) || !ecsInterface.HasComponent<TransformComponent>(bar)) return;
        auto& barTr = ecsInterface.GetComponent<TransformComponent>(bar);
        float barLeft = barTr.position.x - 0.5f * barTr.scale.x;
        float barRight = barTr.position.x + 0.5f * barTr.scale.x;

        // --- Sync knob X ---
        auto knobs = ecsInterface.GetEntitiesByTag(knobTag);
        for (Entity knob : knobs)
        {
            if (!ecsInterface.IsEntityValid(knob) || !ecsInterface.HasComponent<TransformComponent>(knob)) continue;
            auto& knobTr = ecsInterface.GetComponent<TransformComponent>(knob);
            knobTr.position.x = barLeft + volume * (barRight - barLeft);
        }

        // --- Sync fill bar ---
        auto fills = ecsInterface.GetEntitiesByTag(fillTag);
        for (Entity fill : fills)
        {
            if (!ecsInterface.IsEntityValid(fill) || !ecsInterface.HasComponent<TransformComponent>(fill)) continue;
            auto& fillTr = ecsInterface.GetComponent<TransformComponent>(fill);
            fillTr.scale.x = barTr.scale.x * volume;
            fillTr.position.x = barLeft + 0.5f * fillTr.scale.x;
        }

        // --- Sync text ---
        auto texts = ecsInterface.GetEntitiesByTag(textTag);
        for (Entity textEnt : texts)
        {
            if (!ecsInterface.IsEntityValid(textEnt) || !ecsInterface.HasComponent<TextProperties>(textEnt)) continue;
            auto& tp = ecsInterface.GetComponent<TextProperties>(textEnt);
            if (isGamma)
            {
                float gamma = 1.0f + volume * 2.0f; // 1.0=dark, 2.0=neutral, 3.0=bright
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << gamma;
                tp.text = oss.str();
            }
            else
            {
                int percent = static_cast<int>(volume * 100.0f);
                tp.text = std::to_string(percent) + "%";
            }
        }
    }

    void ButtonSystem::Initialize() {
        for (auto entity : mEntities) {
            // Safety check to ensure the entity has the component
            if (!ecsInterface.HasComponent<ButtonComponent>(entity)) continue;

            auto& button = ecsInterface.GetComponent<ButtonComponent>(entity);

            // If we have a string path (GUID) but no GPU ID yet
            if (!button.hoverTextureGuid.empty() && button.hoverTextureGuid != "") {

                // This is likely the call you need. 
                // We want to get the TEXTURE ID from the ASSET PATH.
                auto textureAsset = AssetManager::Instance().Get<TextureProperties>(button.hoverTextureGuid);

                if (textureAsset) {
                    button.hoverTextureID = textureAsset->textureID;
                    std::cout << "[ButtonSystem] Successfully loaded hover texture: " << button.hoverTextureGuid << std::endl;
                }
                else {
                    std::cerr << "[ButtonSystem] Failed to find texture asset for GUID: " << button.hoverTextureGuid << std::endl;
                }
            }
        }

        // --- Sync all sliders to their starting values on startup ---
        // Master, BGM, SFX all start at 100% (volume = 1.0)
        SyncSliderToValue("sliderBar_master", "sliderFill_master", "sliderKnob_master", "volumeText_master", 1.0f, false);
        SyncSliderToValue("sliderBar_bgm", "sliderFill_bgm", "sliderKnob_bgm", "volumeText_bgm", 1.0f, false);
        SyncSliderToValue("sliderBar_sfx", "sliderFill_sfx", "sliderKnob_sfx", "volumeText_sfx", 1.0f, false);
        // Gamma starts at 2.2, which is volume = (2.2 - 1.0) / 3.0 = 0.4
        SyncSliderToValue("sliderBar_gamma", "sliderFill_gamma", "sliderKnob_gamma", "text_gamma", 0.4f, true); // 0.4 == gamma 2.2
    }

    void ButtonSystem::Update(float deltaTime)
    {
        (void)deltaTime;
        for (auto entity : mEntities)
        {
            if (!ecsInterface.HasComponent<ButtonComponent>(entity) ||
                !ecsInterface.HasComponent<TransformComponent>(entity) ||
                !ecsInterface.HasComponent<TextureProperties>(entity))
                continue;

            auto& button = ecsInterface.GetComponent<ButtonComponent>(entity);
            auto& tr = ecsInterface.GetComponent<TransformComponent>(entity);

            if (button.hoverTextureID == 0 && !button.hoverTextureGuid.empty())
            {
                // Assuming AssetManager returns a pointer to a struct containing the textureID
                auto textureAsset = AssetManager::Instance().Get<TextureProperties>(button.hoverTextureGuid);
                if (textureAsset) {
                    button.hoverTextureID = textureAsset->textureID;
                }
            }

            // interpret Transform as �center + full size� in design space
            // button.position = glm::vec2(tr.position.x - 0.5f * tr.scale.x,tr.position.y - 0.5f * tr.scale.y);
            button.position = glm::vec2(tr.position.x, tr.position.y);
            button.size = glm::vec2(tr.scale.x, tr.scale.y);
        }
    }

    // ---------------------------------------------------------------------
    // Click handling in design (1920x1080) coordinates
    // ---------------------------------------------------------------------
    void ButtonSystem::HandleClickUV(float u, float v)
    {
        float designX = u * DESIGN_WIDTH;
        float designY = v * DESIGN_HEIGHT;

        for (auto entity : mEntities)
        {
            if (!ecsInterface.HasComponent<ButtonComponent>(entity) ||
                !ecsInterface.HasComponent<TransformComponent>(entity) ||
                !ecsInterface.HasComponent<TextureProperties>(entity))
                continue;

            auto& button = ecsInterface.GetComponent<ButtonComponent>(entity);
            auto& tr = ecsInterface.GetComponent<TransformComponent>(entity);
            auto& tex = ecsInterface.GetComponent<TextureProperties>(entity);

            // Must be visible UI AND interactable
            if (!tex.isUI) continue;
            if (!tex.uiAlpha > 0.0f) continue;

            float left = tr.position.x - 0.5f * tr.scale.x;
            float right = tr.position.x + 0.5f * tr.scale.x;
            float bottom = tr.position.y - 0.5f * tr.scale.y;
            float top = tr.position.y + 0.5f * tr.scale.y;

            bool inside = (designX > left && designX < right && designY > bottom && designY < top);
            button.isHovered = inside;

            if (inside) button.TriggerClick(entity);
        }
    }

    void ButtonSystem::HandleSliderDrag(float u, float v)
    {
        float designX = u * DESIGN_WIDTH;
        float designY = v * DESIGN_HEIGHT;

        auto* input = InputHandler::GetInstance();
        bool isPressed = input->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        bool isHeld = input->IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);
        bool isReleased = input->IsMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT);

        /*--------------------------------------------------
            1) START DRAG � ONLY ON PRESS
        --------------------------------------------------*/
        if (!m_isDraggingSlider && isPressed)
        {
            struct KnobInfo { Entity knob; SliderType type; };
            std::vector<KnobInfo> knobs;

            auto gather = [&](SliderType type, const std::string& tag)
                {
                    auto ents = ecsInterface.GetEntitiesByTag(tag);
                    for (Entity e : ents)
                    {
                        if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<TransformComponent>(e))
                            knobs.push_back({ e, type });
                    }
                };

            gather(SliderType::Master, "sliderKnob_master");
            gather(SliderType::BGM, "sliderKnob_bgm");
            gather(SliderType::SFX, "sliderKnob_sfx");
            gather(SliderType::Gamma, "sliderKnob_gamma");

            for (const auto& info : knobs)
            {
                auto& tr = ecsInterface.GetComponent<TransformComponent>(info.knob);

                float left = tr.position.x - 0.5f * tr.scale.x;
                float right = tr.position.x + 0.5f * tr.scale.x;
                float bottom = tr.position.y - 0.5f * tr.scale.y;
                float top = tr.position.y + 0.5f * tr.scale.y;

                if (designX >= left && designX <= right && designY >= bottom && designY <= top)
                {
                    m_isDraggingSlider = true;
                    m_activeKnob = info.knob;
                    m_activeSliderType = info.type;
                    break;
                }
            }
        }

        /*--------------------------------------------------
            2) DRAG � ONLY WHILE HELD
        --------------------------------------------------*/
        if (m_isDraggingSlider && isHeld)
        {
            std::string barTag;
            std::string textTag;
            float* lastVol = nullptr;

            switch (m_activeSliderType)
            {
            case SliderType::Master:
                barTag = "sliderBar_master";
                textTag = "volumeText_master";
                lastVol = &m_lastMasterVol;
                break;
            case SliderType::BGM:
                barTag = "sliderBar_bgm";
                textTag = "volumeText_bgm";
                lastVol = &m_lastBGMVol;
                break;
            case SliderType::SFX:
                barTag = "sliderBar_sfx";
                textTag = "volumeText_sfx";
                lastVol = &m_lastSFXVol;
                break;
            case SliderType::Gamma:
                barTag = "sliderBar_gamma";
                textTag = "text_gamma";
                lastVol = &m_lastGamma;
                break;
            default: return;
            }

            auto bars = ecsInterface.GetEntitiesByTag(barTag);
            if (bars.empty()) return;

            Entity bar = bars.front();
            if (!ecsInterface.IsEntityValid(bar) || !ecsInterface.HasComponent<TransformComponent>(bar)) return;

            auto& barTr = ecsInterface.GetComponent<TransformComponent>(bar);
            auto& knobTr = ecsInterface.GetComponent<TransformComponent>(m_activeKnob);

            float barLeft = barTr.position.x - 0.5f * barTr.scale.x;
            float barRight = barTr.position.x + 0.5f * barTr.scale.x;

            knobTr.position.x = std::clamp(designX, barLeft, barRight);

            float volume = (knobTr.position.x - barLeft) / (barRight - barLeft);
            volume = std::clamp(volume, 0.0f, 1.0f);

            /* Update the fill bar visual */
            std::string fillBarTag;
            switch (m_activeSliderType)
            {
            case SliderType::Master: fillBarTag = "sliderFill_master"; break;
            case SliderType::BGM:    fillBarTag = "sliderFill_bgm";    break;
            case SliderType::SFX:    fillBarTag = "sliderFill_sfx";    break;
            case SliderType::Gamma:  fillBarTag = "sliderFill_gamma"; break;
            default: break;
            }

            if (!fillBarTag.empty())
            {
                auto fillBars = ecsInterface.GetEntitiesByTag(fillBarTag);
                if (!fillBars.empty())
                {
                    Entity fillBar = fillBars.front();
                    if (ecsInterface.IsEntityValid(fillBar) &&
                        ecsInterface.HasComponent<TransformComponent>(fillBar))
                    {
                        auto& fillTr = ecsInterface.GetComponent<TransformComponent>(fillBar);
                        auto& barTr = ecsInterface.GetComponent<TransformComponent>(bar);

                        // Scale the fill bar width based on volume
                        fillTr.scale.x = barTr.scale.x * volume;

                        // Reposition to align with left edge of background bar
                        // (since scaling happens from center by default)
                        float barLeft = barTr.position.x - 0.5f * barTr.scale.x;
                        fillTr.position.x = barLeft + 0.5f * fillTr.scale.x;
                    }
                }
            }

            /* Apply audio */
            switch (m_activeSliderType)
            {
            case SliderType::Master: g_AudioSystem->SetMasterVolume(volume); break;
            case SliderType::BGM:    g_AudioSystem->SetBGMVolume(volume);    break;
            case SliderType::SFX:    g_AudioSystem->SetSFXVolume(volume);    break;
            case SliderType::Gamma:
            {
                float gamma = 1.0f + volume * 3.0f; // 1.0=dark, 2.0=neutral, 4.0=bright
                auto* gfx = Graphics::GetInstance();
                if (gfx) gfx->SetGamma(gamma);

                // Update text label with actual gamma value
                auto texts = ecsInterface.GetEntitiesByTag(textTag);
                for (Entity textEnt : texts)
                {
                    if (ecsInterface.IsEntityValid(textEnt) && ecsInterface.HasComponent<TextProperties>(textEnt))
                    {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(2) << gamma;
                        ecsInterface.GetComponent<TextProperties>(textEnt).text = oss.str();
                    }
                }
                break;
            }
            default: break;
            }

            /* Update text + click sound only when value changes */
            if (lastVol && fabs(volume - *lastVol) > 0.01f)
            {
                if (m_activeSliderType != SliderType::Gamma)  // ADD THIS GUARD
                {
                    auto texts = ecsInterface.GetEntitiesByTag(textTag);
                    for (Entity textEnt : texts)
                    {
                        if (ecsInterface.IsEntityValid(textEnt) && ecsInterface.HasComponent<TextProperties>(textEnt))
                        {
                            int percent = static_cast<int>(volume * 100.0f);
                            ecsInterface.GetComponent<TextProperties>(textEnt).text = std::to_string(percent) + "%";
                        }
                    }
                }

                if (fabs(volume - *lastVol) > 0.03f)
                {
                    //g_AudioSystem->PlayAudio("event:/ButtonClick");
                }
            }
            // Always update lastVol so text never gets stuck between frames
            if (lastVol) *lastVol = volume;
        }

        /*--------------------------------------------------
            3) RELEASE � ALWAYS RESET
        --------------------------------------------------*/
        if (m_isDraggingSlider && isReleased)
        {
            m_isDraggingSlider = false;
            m_activeKnob = 0;
            m_activeSliderType = SliderType::None;
        }
    }

    void ButtonSystem::UpdateHoverUV(float u, float v) {
        float designX = u * DESIGN_WIDTH;
        float designY = v * DESIGN_HEIGHT;

        for (auto entity : mEntities) {
            if (!ecsInterface.HasComponent<ButtonComponent>(entity) ||
                !ecsInterface.HasComponent<TransformComponent>(entity) ||
                !ecsInterface.HasComponent<TextureProperties>(entity)) {
                continue;
            }


            auto& button = ecsInterface.GetComponent<ButtonComponent>(entity);
            auto& tr = ecsInterface.GetComponent<TransformComponent>(entity);
            auto& tex = ecsInterface.GetComponent<TextureProperties>(entity);

            float left = tr.position.x - 0.5f * tr.scale.x;
            float right = tr.position.x + 0.5f * tr.scale.x;
            float bottom = tr.position.y - 0.5f * tr.scale.y;
            float top = tr.position.y + 0.5f * tr.scale.y;

            bool inside = (designX > left && designX < right && designY > bottom && designY < top);

            // Don't register hover on invisible buttons
            if (!tex.isUI || tex.uiAlpha <= 0.0f) {
                inside = false;
            }

            // TEXTURE SWAP LOGIC
            if (inside && !button.isHovered) {

               
                // --- FIX 2: VALIDATE ORIGINAL ID ---
                // Only store the original if it's actually valid (> 0).
                // If it's 0, it means the base texture hasn't loaded yet.
                if (tex.textureID != 0) {
                    button.originalTextureID = tex.textureID;
                    button.originalTextureGuid = tex.guid;

                    // Only swap if we actually have a hover texture ready
                    if (button.hoverTextureID != 0) {
                        tex.textureID = button.hoverTextureID;
                        tex.guid = button.hoverTextureGuid;

                        // Only play hover sound if the button is actually visible
                        if (g_AudioSystem && tex.isUI && tex.uiAlpha > 0.0f) {
                            g_AudioSystem->PlayAudio("event:/ButtonHover");
                        }

                    }
                }
            }
            else if (!inside && button.isHovered) {
                // Restore only if we have a valid stored original
                if (button.originalTextureID != 0) {
                    tex.textureID = button.originalTextureID;
                    tex.guid = button.originalTextureGuid;
                }
            }

            button.isHovered = inside;
        }
    }
}