local M = {}
local isMuted = false

-- Paths to mute and unmute icon
local icon_unmuted = "Assets/Textures/Unmute_Button.png"
local icon_muted   = "Assets/Textures/Mute_Button.png" 

local knobEntity = nil
local barEntity = nil
local fillBarEntity = nil
local textEntity = nil

local savedVolume = 1.0
local savedKnobX = 0.0

function M:on_init()
    SetTexture(self.entity, icon_unmuted)
    
    local knobs = GetEntitiesByTag("sliderKnob_sfx")
    if knobs and #knobs > 0 then
        knobEntity = knobs[1]
    else
        print("[SFX Mute] Warning: Could not find sliderKnob_sfx")
    end
    
    local bars = GetEntitiesByTag("sliderBar_sfx")
    if bars and #bars > 0 then
        barEntity = bars[1]
    else
        print("[SFX Mute] Warning: Could not find sliderBar_sfx")
    end
    
    local fillBars = GetEntitiesByTag("sliderFill_sfx")
    if fillBars and #fillBars > 0 then
        fillBarEntity = fillBars[1]
    else
        print("[SFX Mute] Warning: Could not find sliderFill_sfx")
    end
    
    local texts = GetEntitiesByTag("volumeText_sfx")
    if texts and #texts > 0 then
        textEntity = texts[1]
    else
        print("[SFX Mute] Warning: Could not find volumeText_sfx")
    end

    RegisterButtonCallback(self.entity, function(ent)
        isMuted = not isMuted
        Sound.Mute("sfx_mute", isMuted)

        if isMuted then
            SetTexture(ent, icon_muted)
            
            if knobEntity and barEntity then
                local kx, ky, kz = GetPosition(knobEntity)
                savedKnobX = kx
                
                local bx, by, bz = GetPosition(barEntity)
                local barWidth, barHeight, barDepth = GetScale(barEntity)
                local barLeft = bx - 0.5 * barWidth
                
                SetPosition(knobEntity, barLeft, ky, kz)
            end
            
            if fillBarEntity and barEntity then
                local bx, by, bz = GetPosition(barEntity)
                local barWidth, barHeight, barDepth = GetScale(barEntity)
                local barLeft = bx - 0.5 * barWidth
                
                SetScale(fillBarEntity, 0.01, barHeight, barDepth)
                SetPosition(fillBarEntity, barLeft, by, bz)
            end
            
            if textEntity then
                SetText(textEntity, "0%")
            end
        else
            SetTexture(ent, icon_unmuted)
            
            if knobEntity then
                local kx, ky, kz = GetPosition(knobEntity)
                SetPosition(knobEntity, savedKnobX, ky, kz)
            end
            
            if fillBarEntity and barEntity then
                local bx, by, bz = GetPosition(barEntity)
                local barWidth, barHeight, barDepth = GetScale(barEntity)
                local barLeft = bx - 0.5 * barWidth
                local barRight = bx + 0.5 * barWidth
                
                savedVolume = (savedKnobX - barLeft) / (barRight - barLeft)
                savedVolume = math.max(0.0, math.min(1.0, savedVolume))
                
                local fillWidth = barWidth * savedVolume
                SetScale(fillBarEntity, fillWidth, barHeight, barDepth)
                SetPosition(fillBarEntity, barLeft + 0.5 * fillWidth, by, bz)
            end
            
            if textEntity then
                local percent = math.floor(savedVolume * 100)
                SetText(textEntity, percent .. "%")
            end
        end
    end)
end

function M:on_update(dt)
    local targetIcon = isMuted and icon_muted or icon_unmuted
    SetTexture(self.entity, targetIcon)
end

return M