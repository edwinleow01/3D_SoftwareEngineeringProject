///////////////////////////////////////////////////////////////////////////////
///
/// @file AudioComponent.h
/// @brief Component for attaching 3D audio to entities
/// 
/// @Authors: Belinda Chua Jia Xuan 
/// @Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include <string>
#include <fmod_studio.hpp>
#include <glm.hpp>
#include "AssetHandler.h"
#include "PropertyMacro.h"

namespace Framework
{
    struct AudioBankMetadata
    {
        AssetHandle guid;                   // Unique asset identifer
        std::string name;                   // Bank name (stem of filename)
        std::string path;                   // Relative path to the .bank file
        std::vector<std::string> events;    // List of event paths inside the bank
    };

    // Property registration for AudioBankMetadata
    inline const std::vector g_AudioBankMetadataPropsArray =
    {
        PROP_GUID_HIDDEN(guid,  AudioBankMetadata),
        PROP_STRING(name,       AudioBankMetadata),
        PROP_STRING(path,       AudioBankMetadata),
    };

    inline const prop_list g_AudioBankMetadataProps{ "AudioBankMetadata", g_AudioBankMetadataPropsArray };
    inline global_registration g_AudioBankMetadataReg(&g_AudioBankMetadataProps);

    //------------------------------------------------------------------------------------------------------------------//

    // Player's "Ears"
    struct ListenerComponent
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);

        bool active = true;
        int listenerIndex = 0;
    };

    // Property registration for ListenerComponent
    inline const std::vector g_ListenerComponentPropsArray =
    {
        PROP_VEC3(position,     ListenerComponent),
        PROP_VEC3(forward,      ListenerComponent),
        PROP_VEC3(up,           ListenerComponent),
        PROP_VEC3(velocity,     ListenerComponent),
        PROP_BOOL(active,       ListenerComponent),
        PROP_INT(listenerIndex, ListenerComponent)
    };

    inline const prop_list g_ListenerComponentProps{ "ListenerComponent", g_ListenerComponentPropsArray };
    inline global_registration g_ListenerComponentReg(&g_ListenerComponentProps);

    //------------------------------------------------------------------------------------------------------------------//

    // Audio Source || Sound Emitter
    struct AudioComponent
    {
        AssetHandle guid;
        std::string eventName;
        FMOD::Studio::EventInstance* eventInstance = nullptr;

        // 3D audio properties
        bool is3D = true;
        float minDistance = 1.0f;
        float maxDistance = 100.0f;

        // Playback properties
        float volume = 1.0f;
        bool isPlaying = false;
        bool loop = false;
        bool playOnStart = true;

        // Runtime data
        glm::vec3 lastPosition = glm::vec3(0.0f);

        // Optional: internal ID or handle for audio system
        int channelID = -1;
    };

    // Property registration for AudioComponent
    inline const std::vector g_AudioComponentPropsArray =
    {
        PROP_GUID_HIDDEN(guid,          AudioComponent),
        PROP_STRING_HIDDEN(eventName,   AudioComponent),
        PROP_BOOL(is3D,                 AudioComponent),
        PROP_FLOAT(minDistance,         AudioComponent),
        PROP_FLOAT(maxDistance,         AudioComponent),
        PROP_SLIDER_FLOAT(volume,       AudioComponent, 0.0f, 1.0f),
        PROP_BOOL(loop,                 AudioComponent),
        PROP_BOOL(playOnStart,          AudioComponent),
        PROP_BOOL(isPlaying,            AudioComponent),
        PROP_VEC3(lastPosition,         AudioComponent)
    };

    inline const prop_list g_AudioComponentProps{ "AudioComponent", g_AudioComponentPropsArray };
    inline global_registration g_AudioComponentReg(&g_AudioComponentProps);
}
#endif // AUDIO_COMPONENT_H