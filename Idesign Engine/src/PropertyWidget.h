#pragma once
#ifndef _PROPERTY_WIDGET_H_
#define _PROPERTY_WIDGET_H_

// STL Library 
#include <algorithm>  // for std::copy, std::sort, etc.
#include <iterator>   // for std::begin, std::end, etc.
#include <filesystem>
#include <unordered_map>

// ImGui Stuff
#include "imgui.h"
#include <gtc/type_ptr.hpp>

// Files
#include "Coordinator.h"
#include "TypeRegistry.h"
#include "PropertySystem.h"
#include "AssetManager.h"
#include "MeshLoader.h"
#include "TextureLoader.h"
#include "MaterialLoader.h"

// All Components || Properties
#include "TextureProperties.h"
#include "MeshProperties.h"
//#include "AudioProperties.h"
#include "TransformProperties.h"
#include "TextProperties.h"
#include "NavMeshProperties.h"
#include "NavMeshAgentProperties.h"
#include "AnimationProperties.h"
#include "AnimationLoader.h"
#include "LightProperties.h"
#include "PhysicsProperties.h"
#include "ParticleComponent.h"

extern Framework::Coordinator ecsInterface;
namespace Framework
{
    // Scripting Stuff
    namespace fs = std::filesystem;

    // For Entity Inspector
    void DrawInspector(void* pObject, const prop_list& propList);
    void DrawEntityInspector(Entity entity, Coordinator& ecs);

    // For Component Registration with reflection
    void RegisterComponentTypes();
    void AutoRegisterAllReflectedComponents(Coordinator& ecs);

    static const std::unordered_set<std::string> excludedComponents =
    {
        "WindowConfig",
        "PrefabProperties",
        "PrefabComponent",
        "MaterialMetadata",
        "MaterialProperties",
        "SceneProperties",
        "AudioMetadata",
        "ScriptEntry",
        "EventSenderEntry",
        "EventReceiverEntry",
        "BoxColliderEntry"
    };

    //------------------------------------------------------------------------------------------------------------------------------------
    //  TEMPLATE FUNCTIONS FOR WIDGETS
    //------------------------------------------------------------------------------------------------------------------------------------
    template<typename T, typename = void>
    struct has_name_and_path : std::false_type {};

    template<typename T>
    struct has_name_and_path<T, std::void_t<
        decltype(std::declval<T>().name),
        decltype(std::declval<T>().path)
        >> : std::true_type {};

    template<typename AssetType>
    void DrawAssetDropdown(const std::string& label, AssetType& component)
    {
        std::vector<std::string> assetNames;
        std::vector<AssetHandle> assetGuids;

        // 1. Lock the Asset Manager (If your engine supports it).
        // If you don't have a mutex getter, just ensure this doesn't run while loading scenes.
        // std::lock_guard<std::mutex> lock(AssetManager::Instance().GetMutex());

        const auto& assets = AssetManager::Instance().GetAssets();

        // Iterate over AssetKey + AssetEntry
        for (const auto& [key, entry] : assets)
        {
            // SAFETY CHECK 1: Ensure the shared pointer is valid before doing ANYTHING
            if (!entry.pObject)
                continue;

            // SAFETY CHECK 2: Compare types. 
            // We assume TypeName<T>::value is a const char*. 
            // If entry.typeName is corrupted, this is where it usually crashes.
            if (entry.typeName != TypeName<AssetType>::value)
                continue;

            auto* asset = static_cast<AssetType*>(entry.pObject.get());

            // SAFETY CHECK 3: Ensure the cast didn't result in a nullptr
            if (!asset)
                continue;

            if constexpr (has_name_and_path<AssetType>::value)
            {
                // SAFETY CHECK 4: Ensure strings are valid
                bool hasName = !asset->name.empty();
                bool hasPath = !asset->path.empty();

                if (!hasName && !hasPath)
                    continue;

                std::string displayName = hasName ? asset->name : asset->path;
                assetNames.push_back(displayName);
            }
            else
            {
                continue;
            }

            assetGuids.push_back(key.guid);
        }

        // Prepare ImGui pointers (Must be done AFTER assetNames is finished filling)
        std::vector<const char*> labelPtrs;
        labelPtrs.reserve(assetNames.size());
        for (const auto& name : assetNames)
            labelPtrs.push_back(name.c_str());

        // Find current selection
        int currentIndex = -1;
        for (size_t i = 0; i < assetGuids.size(); ++i)
        {
            // This comparison is safe if both are std::string or valid IDs
            if (assetGuids[i] == component.guid)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        ImGui::Separator();
        ImGui::Text("%s", label.c_str());

        // --- Searchable Combo ---
        // We store a per-label search buffer using a static map keyed by label string.
        static std::unordered_map<std::string, std::string> s_SearchBuffers;
        std::string& searchStr = s_SearchBuffers[label];

        // Build filtered list
        std::vector<int> filteredIndices;
        filteredIndices.reserve(assetNames.size());
        for (int i = 0; i < static_cast<int>(assetNames.size()); ++i)
        {
            if (searchStr.empty())
            {
                filteredIndices.push_back(i);
            }
            else
            {
                // Case-insensitive substring match
                std::string haystack = assetNames[i];
                std::string needle = searchStr;
                std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
                std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
                if (haystack.find(needle) != std::string::npos)
                    filteredIndices.push_back(i);
            }
        }

        const char* previewLabel = (currentIndex >= 0 && currentIndex < static_cast<int>(assetNames.size()))
            ? assetNames[currentIndex].c_str()
            : "Select...";

        if (ImGui::BeginCombo(label.c_str(), previewLabel))
        {
            // Search input – autofocus when the popup first opens
            ImGui::SetNextItemWidth(-1.f);
            char searchBuf[128] = {};
            std::strncpy(searchBuf, searchStr.c_str(), sizeof(searchBuf) - 1);
            if (ImGui::InputTextWithHint("##search", "Search...", searchBuf, sizeof(searchBuf)))
                searchStr = searchBuf;
            ImGui::SetItemDefaultFocus();

            ImGui::Separator();

            for (int fi : filteredIndices)
            {
                bool isSelected = (fi == currentIndex);
                if (ImGui::Selectable(assetNames[fi].c_str(), isSelected))
                {
                    currentIndex = fi;
                    searchStr.clear(); // clear search after selection
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            if (filteredIndices.empty())
                ImGui::TextDisabled("No results for \"%s\"", searchStr.c_str());

            ImGui::EndCombo();
        }

        // Apply selection whenever a valid index is set
        if (currentIndex >= 0 && currentIndex < static_cast<int>(assetGuids.size()))
        {
            component.guid = assetGuids[currentIndex];

            // Re-fetch the asset to update component data
            if (auto* asset = AssetManager::Instance().Get<AssetType>(component.guid))
            {
                if constexpr (has_name_and_path<AssetType>::value)
                {
                    // Update name/path only if valid
                    if (!asset->name.empty() || !asset->path.empty()) {
                        component.name = asset->name.empty() ? asset->path : asset->name;
                        component.path = asset->path;
                    }
                }

                // Handle Mesh Specifics (Auto-load Animation vs Static)
                if constexpr (std::is_same_v<AssetType, MeshProperties>)
                {
                    if (asset->vao == 0 && !asset->path.empty())
                    {
                        // Check for Animation
                        auto animGUID = AssetManager::Instance().FindGuid<AnimationProperties>(asset->name);
                        Framework::AnimationProperties* animAsset = nullptr;

                        if (!animGUID.empty()) {
                            animAsset = AssetManager::Instance().Get<AnimationProperties>(animGUID);
                        }

                        if (animAsset)
                        {
                            if (animAsset->skeleton.bones.empty()) {
                                AnimationLoader::loadAnimation(animAsset->path, *animAsset);
                            }
                            MeshLoader::loadAnimatedMesh(asset->path, *animAsset, *asset);
                            std::cout << "[Editor] Auto-loaded ANIMATED mesh: " << asset->name << std::endl;
                        }
                        else
                        {
                            MeshLoader::loadMesh(asset->path, *asset);
                            std::cout << "[Editor] Loaded STATIC mesh: " << asset->name << std::endl;
                        }
                    }
                    component.vao = asset->vao;
                    component.vbo = asset->vbo;
                    component.ebo = asset->ebo;
                    component.indexCount = asset->indexCount;
                }

                // Handle Texture Specifics
                if constexpr (std::is_same_v<AssetType, TextureProperties>)
                {
                    if (asset->textureID == 0)
                    {
                        TextureLoader::loadTexture(*asset, true);
                    }
                    component.textureID = asset->textureID;
                }

                // Handle NavMesh
                if constexpr (std::is_same_v<AssetType, NavMeshComponent>)
                {
                    component.guid = assetGuids[currentIndex];
                }

                // Handle Animation Specifics
                else if constexpr (std::is_same_v<AssetType, AnimationProperties>)
                {
                    if (!asset->path.empty())
                    {
                        if (asset->skeleton.bones.empty()) {
                            AnimationLoader::loadAnimation(asset->path, *asset);
                        }

                        component.skeleton = asset->skeleton;
                        component.availableAnimations = asset->availableAnimations;
                        component.globalInverseTransform = asset->globalInverseTransform;
                        component.currentTime = 0.0f;
                        component.currentAnimation = nullptr;
                        component.StartDefaultAnimation();

                        // Check size before resize to avoid weird vector errors
                        if (component.skeleton.bones.size() > 0) {
                            component.finalBoneMatrices.resize(component.skeleton.bones.size(), glm::mat4(1.0f));
                        }
                    }
                }

                // Handle Scripts
                if constexpr (std::is_same_v<AssetType, ScriptProperties>)
                {
                    component.script = asset->script;
                }
            }
        }
    }

    template <size_t N>
    bool DrawEnumDropdown(const char* label, int* value, const char* const (&names)[N]) {
        int idx = std::max(0, std::min<int>(*value, (int)N - 1));
        bool changed = false;
        if (ImGui::BeginCombo(label, names[idx])) {
            for (int i = 0; i < (int)N; ++i) {
                bool selected = (i == idx);
                if (ImGui::Selectable(names[i], selected)) {
                    *value = i;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    // Specialization for AudioComponent
    template<>
    inline void DrawAssetDropdown<AudioComponent>(const std::string& label, AudioComponent& component)
    {
        ImGui::Separator();
        ImGui::Text("%s", label.c_str());

        // First dropdown: select bank
        std::string bankName = "<None>";
        if (auto* bank = AssetManager::Instance().Get<AudioBankMetadata>(component.guid))
            bankName = bank->name;

        // Build bank list for searching
        static std::unordered_map<std::string, std::string> s_AudioSearchBuffers;
        std::string& bankSearch = s_AudioSearchBuffers["##bank"];

        std::vector<std::string> bankNames;
        std::vector<AssetHandle> bankGuids;
        for (auto& [key, entry] : AssetManager::Instance().GetAssets())
        {
            if (entry.typeName != TypeName<AudioBankMetadata>::value) continue;
            auto* bank = static_cast<AudioBankMetadata*>(entry.pObject.get());
            if (!bank || bank->name.empty()) continue;
            bankNames.push_back(bank->name);
            bankGuids.push_back(key.guid);
        }

        // Filter banks by search
        std::vector<int> filteredBankIndices;
        for (int i = 0; i < static_cast<int>(bankNames.size()); ++i)
        {
            if (bankSearch.empty()) { filteredBankIndices.push_back(i); continue; }
            std::string h = bankNames[i], n = bankSearch;
            std::transform(h.begin(), h.end(), h.begin(), ::tolower);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (h.find(n) != std::string::npos) filteredBankIndices.push_back(i);
        }

        if (ImGui::BeginCombo("Select Bank", bankName.c_str()))
        {
            ImGui::SetNextItemWidth(-1.f);
            char bankBuf[128] = {};
            std::strncpy(bankBuf, bankSearch.c_str(), sizeof(bankBuf) - 1);
            if (ImGui::InputTextWithHint("##bankSearch", "Search...", bankBuf, sizeof(bankBuf)))
                bankSearch = bankBuf;
            ImGui::Separator();

            for (int fi : filteredBankIndices)
            {
                bool isSelected = (component.guid == bankGuids[fi]);
                if (ImGui::Selectable(bankNames[fi].c_str(), isSelected))
                {
                    component.guid = bankGuids[fi];
                    component.eventName.clear();
                    bankSearch.clear();
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            if (filteredBankIndices.empty())
                ImGui::TextDisabled("No results for \"%s\"", bankSearch.c_str());

            ImGui::EndCombo();
        }

        // Second dropdown: select event from chosen bank
        if (auto* bank = AssetManager::Instance().Get<AudioBankMetadata>(component.guid))
        {
            int currentEventIndex = -1;
            for (int i = 0; i < bank->events.size(); ++i)
            {
                if (bank->events[i] == component.eventName)
                {
                    currentEventIndex = i;
                    break;
                }
            }

            std::string& eventSearch = s_AudioSearchBuffers["##event"];

            // Filter events by search
            std::vector<int> filteredEventIndices;
            for (int i = 0; i < static_cast<int>(bank->events.size()); ++i)
            {
                if (eventSearch.empty()) { filteredEventIndices.push_back(i); continue; }
                std::string h = bank->events[i], n = eventSearch;
                std::transform(h.begin(), h.end(), h.begin(), ::tolower);
                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                if (h.find(n) != std::string::npos) filteredEventIndices.push_back(i);
            }

            const char* preview = (currentEventIndex >= 0) ? bank->events[currentEventIndex].c_str() : "<None>";
            if (ImGui::BeginCombo("Select Event", preview))
            {
                ImGui::SetNextItemWidth(-1.f);
                char eventBuf[128] = {};
                std::strncpy(eventBuf, eventSearch.c_str(), sizeof(eventBuf) - 1);
                if (ImGui::InputTextWithHint("##eventSearch", "Search...", eventBuf, sizeof(eventBuf)))
                    eventSearch = eventBuf;
                ImGui::Separator();

                for (int fi : filteredEventIndices)
                {
                    bool isSelected = (currentEventIndex == fi);
                    if (ImGui::Selectable(bank->events[fi].c_str(), isSelected))
                    {
                        component.eventName = bank->events[fi];
                        eventSearch.clear();
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                if (filteredEventIndices.empty()) ImGui::TextDisabled("No results for \"%s\"", eventSearch.c_str());
                ImGui::EndCombo();
            }
        }
    }

    // Overload for NavMeshComponent using MeshProperties as asset type
    inline void DrawAssetDropdown(const std::string& label, NavMeshComponent& navComp)
    {
        std::vector<std::string> meshNames;
        std::vector<AssetHandle> meshGuids;

        // Iterate over AssetKey + AssetEntry
        for (const auto& [key, entry] : AssetManager::Instance().GetAssets())
        {
            if (entry.typeName != TypeName<MeshProperties>::value)
                continue;

            auto* mesh = static_cast<MeshProperties*>(entry.pObject.get());
            if (!mesh || (mesh->name.empty() && mesh->path.empty()))
                continue;

            std::string name = mesh->name.empty() ? mesh->path : mesh->name;
            meshNames.push_back(name);
            meshGuids.push_back(key.guid); // <-- use key.guid
        }

        std::vector<const char*> labelPtrs;
        for (const auto& name : meshNames)
            labelPtrs.push_back(name.c_str());

        int currentIndex = -1;
        for (size_t i = 0; i < meshGuids.size(); ++i)
        {
            if (meshGuids[i] == navComp.guid)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        ImGui::Separator();
        ImGui::Text("%s", label.c_str());

        // Searchable combo for NavMesh
        static std::unordered_map<std::string, std::string> s_NavSearchBuffers;
        std::string& navSearch = s_NavSearchBuffers[label];

        std::vector<int> filteredNavIndices;
        for (int i = 0; i < static_cast<int>(meshNames.size()); ++i)
        {
            if (navSearch.empty()) { filteredNavIndices.push_back(i); continue; }
            std::string h = meshNames[i], n = navSearch;
            std::transform(h.begin(), h.end(), h.begin(), ::tolower);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (h.find(n) != std::string::npos) filteredNavIndices.push_back(i);
        }

        const char* navPreview = (currentIndex >= 0 && currentIndex < static_cast<int>(meshNames.size()))
            ? meshNames[currentIndex].c_str() : "Select...";

        if (ImGui::BeginCombo(label.c_str(), navPreview))
        {
            ImGui::SetNextItemWidth(-1.f);
            char navBuf[128] = {};
            std::strncpy(navBuf, navSearch.c_str(), sizeof(navBuf) - 1);
            if (ImGui::InputTextWithHint("##navSearch", "Search...", navBuf, sizeof(navBuf)))
                navSearch = navBuf;
            ImGui::Separator();

            for (int fi : filteredNavIndices)
            {
                bool isSelected = (fi == currentIndex);
                if (ImGui::Selectable(meshNames[fi].c_str(), isSelected))
                {
                    navComp.guid = meshGuids[fi];
                    navSearch.clear();
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            if (filteredNavIndices.empty())
                ImGui::TextDisabled("No results for \"%s\"", navSearch.c_str());

            ImGui::EndCombo();
        }
    }

    inline void DrawTexturePreview(const TextureProperties& tex)
    {
        if (tex.textureID != 0)
        {
            ImGui::Text("Preview:");
            ImGui::Image((ImTextureID)(uintptr_t)tex.textureID, ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0)); // Flip both horizontally and vertically
            ImGui::Text("Size: %d x %d", tex.width, tex.height);
        }
        else ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No texture loaded.");
    }

    inline void DrawMaterialSelector(AssetHandle& materialGuid)
    {
        static bool isCreatingNewMaterial = false;
        static bool wasCreatingNewMaterial = false;
        static MaterialProperties newMaterial;
        static char newName[64] = "NewMaterial";
        static int lastSelectedIndex = -1;

        std::vector<std::string> assetNames;
        std::vector<AssetHandle> assetGuids;

        // Add placeholder for "No material assigned"
        assetNames.push_back("No material assigned");
        assetGuids.push_back(AssetHandle{}); // Null handle

        // Populate existing materials from MaterialMetadata (for names/paths)
        for (const auto& [key, entry] : AssetManager::Instance().GetAssets())
        {
            if (entry.typeName != TypeName<MaterialMetadata>::value)
                continue;

            auto* meta = static_cast<MaterialMetadata*>(entry.pObject.get());
            if (!meta || (meta->name.empty() && meta->path.empty()))
                continue;

            assetNames.push_back(meta->name.empty() ? meta->path : meta->name);
            assetGuids.push_back(key.guid); // use key.guid instead of guid
        }

        // Add "Create New..." option
        assetNames.push_back("Create New...");
        assetGuids.push_back(AssetHandle{}); // Placeholder

        std::vector<const char*> labelPtrs;
        for (const auto& name : assetNames)
            labelPtrs.push_back(name.c_str());

        // Find current selection
        int currentIndex = 0;
        for (size_t i = 0; i < assetGuids.size(); ++i)
        {
            if (assetGuids[i] == materialGuid)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        ImGui::Separator();
        ImGui::Text("Select Material");

        // Searchable combo for materials
        static std::string s_MatSearch;

        // Filter — always show "No material assigned" (index 0) and "Create New..." (last index),
        // but only filter the real material entries in between.
        int lastIdx = static_cast<int>(assetGuids.size()) - 1; // "Create New..."
        std::vector<int> filteredMatIndices;
        filteredMatIndices.push_back(0); // always show "No material assigned"
        for (int i = 1; i < lastIdx; ++i)
        {
            if (s_MatSearch.empty()) { filteredMatIndices.push_back(i); continue; }
            std::string h = assetNames[i], n = s_MatSearch;
            std::transform(h.begin(), h.end(), h.begin(), ::tolower);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (h.find(n) != std::string::npos) filteredMatIndices.push_back(i);
        }
        filteredMatIndices.push_back(lastIdx); // always show "Create New..."

        const char* matPreview = (currentIndex >= 0 && currentIndex < static_cast<int>(assetNames.size())) ? assetNames[currentIndex].c_str() : "Select...";

        if (ImGui::BeginCombo("Select Material", matPreview))
        {
            ImGui::SetNextItemWidth(-1.f);
            char matBuf[128] = {};
            std::strncpy(matBuf, s_MatSearch.c_str(), sizeof(matBuf) - 1);
            if (ImGui::InputTextWithHint("##matSearch", "Search...", matBuf, sizeof(matBuf)))
                s_MatSearch = matBuf;
            ImGui::Separator();

            for (int fi : filteredMatIndices)
            {
                bool isSelected = (fi == currentIndex);
                if (ImGui::Selectable(assetNames[fi].c_str(), isSelected))
                {
                    currentIndex = fi;
                    s_MatSearch.clear();

                    isCreatingNewMaterial = (currentIndex == lastIdx);

                    if (isCreatingNewMaterial && !wasCreatingNewMaterial)
                    {
                        newMaterial = MaterialProperties{};
                        strcpy(newName, "NewMaterial");
                    }

                    if (!isCreatingNewMaterial && currentIndex != 0)
                    {
                        materialGuid = assetGuids[currentIndex];
                        std::cout << "[DrawMaterialSelector] Assigned GUID=" << materialGuid.id << std::endl;
                    }
                    else if (currentIndex == 0)
                    {
                        materialGuid = AssetHandle{}; // Clear
                    }

                    lastSelectedIndex = currentIndex;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            if (filteredMatIndices.size() <= 2) // only "No material" + "Create New..." visible
                ImGui::TextDisabled("No results for \"%s\"", s_MatSearch.c_str());

            ImGui::EndCombo();
        }

        // Draw material property editors outside the combo (same as before)
        if (!isCreatingNewMaterial && currentIndex != 0 && currentIndex != lastIdx)
        {
            if (auto* material = AssetManager::Instance().Get<MaterialProperties>(materialGuid))
            {
                ImGui::ColorEdit3("Albedo", glm::value_ptr(material->albedo));
                ImGui::ColorEdit3("Ambient", glm::value_ptr(material->ambient));
                ImGui::ColorEdit3("Specular", glm::value_ptr(material->specular));
                ImGui::DragFloat("Shininess", &material->shininess, 0.1f, 0.0f, 128.0f);
                ImGui::DragFloat("Opacity", &material->opacity, 0.01f, 0.0f, 1.0f);
            }
        }

        wasCreatingNewMaterial = isCreatingNewMaterial;

        // Show creation UI
        if (isCreatingNewMaterial)
        {
            ImGui::Separator();
            ImGui::Text("Create New Material");

            ImGui::InputText("New Material Name", newName, IM_ARRAYSIZE(newName));
            ImGui::ColorEdit3("Ambient", glm::value_ptr(newMaterial.ambient));
            ImGui::ColorEdit3("Albedo", glm::value_ptr(newMaterial.albedo));
            ImGui::ColorEdit3("Specular", glm::value_ptr(newMaterial.specular));
            ImGui::DragFloat("Shininess", &newMaterial.shininess, 0.1f, 0.0f, 128.0f);
            ImGui::DragFloat("Opacity", &newMaterial.opacity, 0.01f, 0.0f, 1.0f);

            if (ImGui::Button("Save Material"))
            {
                AssetHandle newGuid = GenerateGUID();
                std::string path = "Assets/Materials/" + std::string(newName) + ".material";

                // Save to disk
                SaveMaterialAsset(newMaterial, path, newGuid);

                // Register metadata
                RegisterMaterialMetadata(newGuid, newName, path);

                // Register actual MaterialProperties asset
                auto material = std::make_unique<MaterialProperties>(newMaterial);
                material->guid = newGuid;
                AssetManager::Instance().RegisterAsset(std::move(material), &g_MaterialProps, path);

                // Assign new material guid to mesh
                materialGuid = newGuid;

                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Material saved and applied!");
                isCreatingNewMaterial = false;
            }
        }
    }

    inline void DrawScriptPropertiesWidget(ScriptProperties& comp, Entity entity)
    {
        ImGui::Separator();
        ImGui::Text("Attached Scripts");

        // Make a visible drop zone
        ImGui::Button("Drop Script Here", ImVec2(200, 30));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT_GUID"))
            {
                std::string guidStr((const char*)payload->Data, payload->DataSize - 1);
                AssetHandle guid{ guidStr };

                if (ecsInterface.HasComponent<ScriptProperties>(entity))
                {
                    auto& sp = ecsInterface.GetComponent<ScriptProperties>(entity);
                    auto* asset = AssetManager::Instance().Get<ScriptEntry>(guid);
                    if (asset)
                    {
                        ScriptEntry newEntry;
                        newEntry.guid = guid;
                        newEntry.scriptPath = asset->scriptPath;
                        newEntry.hotReload = true;
                        sp.scripts.push_back(newEntry);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Build dropdown list from AssetManager (all ScriptAssets)
        std::vector<std::string> assetNames;
        std::vector<AssetHandle> assetGuids;
        for (const auto& [key, entry] : AssetManager::Instance().GetAssets())
        {
            if (entry.typeName != TypeName<ScriptEntry>::value) // <-- use your ScriptAsset type
                continue;

            auto* asset = static_cast<ScriptEntry*>(entry.pObject.get());
            if (!asset) continue;

            std::string name = asset->scriptPath;
            assetNames.push_back(name);
            assetGuids.push_back(key.guid);
        }

        std::vector<const char*> labelPtrs;
        for (auto& s : assetNames)
            labelPtrs.push_back(s.c_str());

        // Now draw one dropdown per ScriptEntry
        for (size_t i = 0; i < comp.scripts.size(); ++i)
        {
            auto& entry = comp.scripts[i];
            ImGui::PushID((int)i);

            ImGui::Text("Script %d", (int)i);

            int currentIndex = -1;
            for (size_t j = 0; j < assetGuids.size(); ++j)
            {
                if (assetGuids[j] == entry.guid)
                {
                    currentIndex = static_cast<int>(j);
                    break;
                }
            }

            if (!labelPtrs.empty())
            {
                // Searchable combo — one search buffer per script slot index
                static std::unordered_map<int, std::string> s_ScriptSearchBuffers;
                std::string& scriptSearch = s_ScriptSearchBuffers[(int)i];

                std::vector<int> filteredScriptIndices;
                for (int si = 0; si < static_cast<int>(assetNames.size()); ++si)
                {
                    if (scriptSearch.empty()) { filteredScriptIndices.push_back(si); continue; }
                    std::string h = assetNames[si], n = scriptSearch;
                    std::transform(h.begin(), h.end(), h.begin(), ::tolower);
                    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                    if (h.find(n) != std::string::npos) filteredScriptIndices.push_back(si);
                }

                const char* scriptPreview = (currentIndex >= 0 && currentIndex < static_cast<int>(assetNames.size())) ? assetNames[currentIndex].c_str() : "Select...";

                if (ImGui::BeginCombo("##ScriptCombo", scriptPreview))
                {
                    ImGui::SetNextItemWidth(-1.f);
                    char scriptBuf[128] = {};
                    std::strncpy(scriptBuf, scriptSearch.c_str(), sizeof(scriptBuf) - 1);
                    if (ImGui::InputTextWithHint("##scriptSearch", "Search...", scriptBuf, sizeof(scriptBuf)))
                        scriptSearch = scriptBuf;
                    ImGui::Separator();

                    for (int fi : filteredScriptIndices)
                    {
                        bool isSelected = (fi == currentIndex);
                        if (ImGui::Selectable(assetNames[fi].c_str(), isSelected))
                        {
                            if (entry.guid != assetGuids[fi])
                            {
                                entry.guid = assetGuids[fi];
                                auto* asset = AssetManager::Instance().Get<ScriptEntry>(entry.guid);
                                if (asset)
                                    entry.scriptPath = asset->scriptPath;
                                entry.initialized = false;
                                entry.lastWriteTicks = 0;
                            }
                            scriptSearch.clear();
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    if (filteredScriptIndices.empty())
                        ImGui::TextDisabled("No results for \"%s\"", scriptSearch.c_str());

                    ImGui::EndCombo();
                }
            }
            else ImGui::TextDisabled("No ScriptAssets loaded");

            ImGui::Checkbox("Hot Reload", &entry.hotReload);

            if (ImGui::Button("Remove Script"))
            {
                comp.scripts.erase(comp.scripts.begin() + i);
                ImGui::PopID();
                break;
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (ImGui::Button("Add Script Slot"))
        {
            ScriptEntry newEntry;
            newEntry.guid = AssetHandle{};   // empty until user picks
            newEntry.scriptPath.clear();
            comp.scripts.push_back(newEntry);
        }
    }
}
#endif // !_PROPERTY_WIDGET_H_