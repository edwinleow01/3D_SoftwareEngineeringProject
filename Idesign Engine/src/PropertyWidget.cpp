
#include "pch.h"
#include "imgui.h"
#include "PropertySystem.h"
#include "PropertyWidget.h"
#include <ItemComponent.h>

namespace Framework
{

    
    inline void DrawAnimationWidget(AnimationProperties& anim)
    {
        ImGui::Separator();
        ImGui::Text("Animation Controls");

        // --- Current animation name display ---
        std::string currentName = anim.currentAnimation
            ? anim.currentAnimation->name
            : "None";
        ImGui::LabelText("Playing", "%s", currentName.c_str());
        ImGui::DragFloat("Current Time", &anim.currentTime, 0.1f, 0.0f,
            anim.currentAnimation ? anim.currentAnimation->duration : 0.0f);

        // --- Dropdown of all available animations ---
        if (anim.availableAnimations.empty())
        {
            ImGui::TextDisabled("No animations loaded");
            return;
        }

        // Build list once per frame from the map keys
        std::vector<const char*> animNames;
        std::vector<std::string> animKeys;
        int currentIndex = -1;
        int i = 0;
        for (auto& [key, ptr] : anim.availableAnimations)
        {
            animKeys.push_back(key);
            animNames.push_back(animKeys.back().c_str());
            if (anim.currentAnimation == ptr)
                currentIndex = i;
            ++i;
        }

        if (ImGui::BeginCombo("Play Animation", currentIndex >= 0
            ? animNames[currentIndex] : "Select..."))
        {
            for (int j = 0; j < (int)animNames.size(); ++j)
            {
                bool selected = (j == currentIndex);
                if (ImGui::Selectable(animNames[j], selected))
                {
                    // Force play even if same animation (reset time)
                    anim.currentAnimation = anim.availableAnimations[animKeys[j]];
                    anim.currentTime = 0.0f;
                    // Also update defaultAnimationName so it serialises correctly
                    anim.defaultAnimationName = animKeys[j];
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // --- Loop / Speed controls inline ---
        ImGui::Checkbox("Loop", &anim.bLoops);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.f);
        ImGui::DragFloat("Speed", &anim.animationSpeed, 0.01f, 0.0f, 5.0f);

        // --- Manual stop/play buttons ---
        if (ImGui::Button("Restart"))
            anim.currentTime = 0.0f;
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
            anim.currentAnimation = nullptr;
    }

    void DrawInspector(void* pObject, const prop_list& propList)
    {
        int propIndex = 0;

        for (const auto& prop : propList.m_Props)
        {
            // Push a unique ID for each property instance
            ImGui::PushID(reinterpret_cast<uintptr_t>(pObject));
            ImGui::PushID(prop.m_pName);
            //printf("Inspecting property [%d]: %s\n", propIndex, prop.m_pName);

            if (prop.m_Flags & prop::HIDDEN)
            {
                //printf("  Skipped (HIDDEN)\n");
                ++propIndex;
                ImGui::PopID(); // pop prop.m_pName
                ImGui::PopID(); // pop pObject
                continue;
            }

            ImGui::Text("%s", prop.m_pName);
            if (prop.m_pTooltip)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", prop.m_pTooltip);
            }

            bool readOnly = (prop.m_Flags & prop::READ_ONLY);
            //printf("  Calling getter for type %d...\n", prop.m_Type);

            switch (prop.m_Type)
            {
            case prop::INT:
            {
                int value = 0;
                prop.m_fnGet(pObject, &value);
                if (prop.m_HasRange)
                    ImGui::SliderInt(prop.m_pName, &value, std::get<int>(prop.m_Min), std::get<int>(prop.m_Max));
                else
                    ImGui::InputInt(prop.m_pName, &value);
                if (!readOnly && ImGui::IsItemEdited())
                    prop.m_fnSet(pObject, &value);
                break;
            }
            case prop::FLOAT:
            {
                float value = 0.0f;
                prop.m_fnGet(pObject, &value);
                if (prop.m_HasRange)
                    ImGui::SliderFloat(prop.m_pName, &value, prop.GetMinAsFloat(), prop.GetMaxAsFloat());
                else
                    ImGui::InputFloat(prop.m_pName, &value);
                if (!readOnly && ImGui::IsItemEdited())
                    prop.m_fnSet(pObject, &value);
                break;
            }
            case prop::DOUBLE:
            {
                double value = 0.0;
                prop.m_fnGet(pObject, &value);
                ImGui::InputDouble(prop.m_pName, &value);
                if (!readOnly && ImGui::IsItemEdited())
                    prop.m_fnSet(pObject, &value);
                break;
            }
            case prop::BOOL:
            {
                bool value = false;
                prop.m_fnGet(pObject, &value);
                ImGui::Checkbox(prop.m_pName, &value);
                if (!readOnly && ImGui::IsItemEdited())
                    prop.m_fnSet(pObject, &value);
                break;
            }
            case prop::CHAR:
            {
                char buffer[2] = {};
                prop.m_fnGet(pObject, buffer);
                buffer[1] = '\0';
                ImGui::InputText(prop.m_pName, buffer, sizeof(buffer));
                // This is to avoid any label collision if multiple CHAR fields exist in the future 
                // ImGui:InputTextWithHint("##char", buffer, sizeof(buffer)); 
                if (!readOnly && ImGui::IsItemEdited())
                {
                    char value = buffer[0];
                    prop.m_fnSet(pObject, &value);
                }
                break;
            }
            case prop::STRING:
            {
                std::string value;
                prop.m_fnGet(pObject, &value);

                char buffer[256] = {};
                std::strncpy(buffer, value.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                if (readOnly) ImGui::BeginDisabled();
                if (ImGui::InputText(prop.m_pName, buffer, sizeof(buffer)))
                {
                    if (!readOnly)
                    {
                        std::string newValue(buffer);
                        prop.m_fnSet(pObject, &newValue);
                    }
                }
                if (readOnly) ImGui::EndDisabled();
                break;
            }
            case prop::UINT:
            {
                unsigned int value = 0;
                prop.m_fnGet(pObject, &value);

                if (readOnly) ImGui::BeginDisabled();

                if (prop.m_HasRange)
                {
                    ImGui::SliderScalar(prop.m_pName, ImGuiDataType_U32, &value,
                        &std::get<unsigned int>(prop.m_Min),
                        &std::get<unsigned int>(prop.m_Max));
                }
                else
                {
                    ImGui::InputScalar(prop.m_pName, ImGuiDataType_U32, &value);
                }

                if (readOnly) ImGui::EndDisabled();

                if (!readOnly && ImGui::IsItemEdited())
                    prop.m_fnSet(pObject, &value);

                break;
            }
            case prop::VEC3:
            {
                glm::vec3 vec = {};
                prop.m_fnGet(pObject, &vec);
                if (ImGui::InputFloat3(prop.m_pName, glm::value_ptr(vec)))
                {
                    if (!readOnly)
                        prop.m_fnSet(pObject, &vec);
                }
                break;
            }
            case prop::VEC2:
            {
                glm::vec2 value{};
                prop.m_fnGet(pObject, &value);

                if (prop.m_HasRange)
                {
                    // If you want to enforce ranges, use SliderFloat2
                    ImGui::SliderFloat2(prop.m_pName, &value.x, prop.GetMinAsFloat(), prop.GetMaxAsFloat());
                }
                else
                {
                    ImGui::InputFloat2(prop.m_pName, &value.x);
                }

                if (!readOnly && ImGui::IsItemEdited())
                {
                    prop.m_fnSet(pObject, &value);
                }
                break;
            }
            case prop::DRAG_FLOAT:
            {
                float value = 0.0f;
                prop.m_fnGet(pObject, &value);

                float speed = 0.01f;
                float min = prop.m_HasRange ? prop.GetMinAsFloat() : 0.0f;
                float max = prop.m_HasRange ? prop.GetMaxAsFloat() : 100.0f;

                if (readOnly) ImGui::BeginDisabled();
                if (ImGui::DragFloat(prop.m_pName, &value, speed, min, max))
                {
                    if (!readOnly)
                        prop.m_fnSet(pObject, &value);
                }
                if (readOnly) ImGui::EndDisabled();
                break;
            }
            case prop::COLOR3:
            {
                glm::vec3 color = {};
                prop.m_fnGet(pObject, &color);

                if (readOnly) ImGui::BeginDisabled();
                if (ImGui::ColorEdit3(prop.m_pName, glm::value_ptr(color)))
                {
                    if (!readOnly)
                        prop.m_fnSet(pObject, &color);
                }
                if (readOnly) ImGui::EndDisabled();
                break;
            }
            case prop::SLIDER_FLOAT:
            {
                float value = 0.0f;
                prop.m_fnGet(pObject, &value);

                float min = prop.m_HasRange ? prop.GetMinAsFloat() : 0.0f;
                float max = prop.m_HasRange ? prop.GetMaxAsFloat() : 1.0f;

                if (readOnly) ImGui::BeginDisabled();
                if (ImGui::SliderFloat(prop.m_pName, &value, min, max))
                {
                    if (!readOnly)
                        prop.m_fnSet(pObject, &value);
                }
                if (readOnly) ImGui::EndDisabled();
                break;
            }
            default:
                ImGui::Text("Unsupported type: %d", prop.m_Type);
                break;
            }
            ImGui::PopID(); // pop prop.m_pName
            ImGui::PopID(); // pop pObject
            //printf("  Finished rendering property [%d]\n", propIndex);
            ImGui::Separator();
            ++propIndex;
        }
        //printf("DrawInspector completed.\n");
    }

    void DrawEntityInspector(Entity entity, Coordinator& ecs)
    {
        // Get all currently attached components
        auto attached = ecs.GetAllComponents(entity);

        for (const auto& [typeName, componentPtr] : attached)
        {
            const prop_list* propList = TypeRegistry::Instance().GetPropList(typeName);
            if (!propList || !propList->m_pName || propList->m_Props.empty())
                continue;

            if (ImGui::CollapsingHeader(propList->m_pName, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (typeName == "TransformComponent")
                {
                    auto& tr = *static_cast<TransformComponent*>(componentPtr);

                    // We use a unique ID scope for the transform to prevent widget conflicts
                    ImGui::PushID("TransformProps");

                    // 1. Position
                    if (ImGui::DragFloat3("Position", &tr.position.x, 0.1f))
                    {
                        // Optional: Add logic here if you need to wake up physics bodies 
                        // when the position is manually changed in the editor.
                    }

                    // 2. Rotation
                    // Note: Most engines wrap these to keep the inspector clean
                    if (ImGui::DragFloat3("Rotation", &tr.rotation.x, 0.5f))
                    {
                        auto wrap = [](float a) {
                            return fmodf(a + 180.0f, 360.0f) - 180.0f;
                            };
                        tr.rotation.x = wrap(tr.rotation.x);
                        tr.rotation.y = wrap(tr.rotation.y);
                        tr.rotation.z = wrap(tr.rotation.z);
                    }

                    // 3. Scale
                    if (ImGui::DragFloat3("Scale", &tr.scale.x, 0.01f))
                    {
                        // Prevent negative scale or zero scale which can break matrix math
                        if (tr.scale.x < 0.001f) tr.scale.x = 0.001f;
                        if (tr.scale.y < 0.001f) tr.scale.y = 0.001f;
                        if (tr.scale.z < 0.001f) tr.scale.z = 0.001f;
                    }

                    ImGui::PopID();

                    // IMPORTANT: Skip the default DrawInspector for this component 
                    // because we just manually drew the fields.
                    continue;
                }

                else if (typeName == "TextureProperties")
                {
                    auto& tex = *static_cast<TextureProperties*>(componentPtr);
                    DrawAssetDropdown<TextureProperties>("Select Texture", tex);
                    DrawTexturePreview(tex);

                }

                else if (typeName == "MeshProperties")
                {
                    auto& mesh = *static_cast<MeshProperties*>(componentPtr);
                    DrawAssetDropdown<MeshProperties>("Select Mesh", mesh);

                    mesh.materialGuid = GetDefaultMaterial();

                    if (ImGui::Button("Reload Mesh"))
                    {
                         MeshLoader::loadMesh(mesh.path, mesh);
                    }
                    DrawMaterialSelector(mesh.materialGuid);
                }

                else if (typeName == "AudioComponent")
                {
                    auto& audio = *static_cast<AudioComponent*>(componentPtr);
                    DrawAssetDropdown<AudioComponent>("Select Audio", audio);
                }

                else if (typeName == "NavMeshComponent")
                {
                    auto& navMesh = *static_cast<NavMeshComponent*>(componentPtr);

                    // Unique ID suffix using entity or typeName
                    std::string idSuffix = "##NavMesh_" + std::to_string(static_cast<uint32_t>(entity));

                    DrawAssetDropdown("NavMesh Source" + idSuffix, navMesh);

                    if (ImGui::Button(("Clear Runtime Buffers" + idSuffix).c_str()))
                    {
                        navMesh.positions.clear();
                        navMesh.indices.clear();
                    }

                    if (ImGui::Button(("Reset NavMesh Component" + idSuffix).c_str()))
                    {
                        navMesh = NavMeshComponent{};
                    }
                }

                else if (typeName == "AnimationProperties")
                {
                    auto& anim = *static_cast<AnimationProperties*>(componentPtr);
                    DrawAssetDropdown<AnimationProperties>("Select Animation", anim);
                    DrawAnimationWidget(anim);

                    if (ImGui::Button("Remove AnimationProperties"))
                    {
                        TypeRegistry::Instance().RemoveComponent(typeName, ecs, entity);
                    }
                    continue;  // skip DrawInspector — all fields are handled above
                }
                else if (typeName == "LightProperties")
                {
                    auto& light = *static_cast<LightProperties*>(componentPtr);
                    const char* lightTypeNames[] = { "Directional", "Point", "Spot" };
                    int currentTypeIndex = static_cast<int>(light.type);

                    // Draw the Dropdown
                    if (ImGui::Combo("Light Type", &currentTypeIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
                    {
                        // Cast the selected int back to the Enum Class
                        light.type = static_cast<LightType>(currentTypeIndex);
                    }
                }
                else if (typeName == "NavMeshAgentComponent")
                {
                    auto& agent = *static_cast<NavMeshAgentComponent*>(componentPtr);
                    std::string idSuffix = "##Agent_" + std::to_string(static_cast<uint32_t>(entity));

                    if (ImGui::Button(("Reset Agent Data" + idSuffix).c_str()))
                    {
                        agent.currentPath.clear();
                        agent.currentWaypointIndex = 0;
                        agent.hasPath = false;
                        agent.isMoving = false;
                        agent.currentSpeed = 0.0f;
                    }

                    if (agent.hasPath)
                    {
                        ImGui::Text("Current Waypoint: %d / %d", agent.currentWaypointIndex + 1, static_cast<int>(agent.currentPath.size()));
                        if (ImGui::TreeNode("Path Preview"))
                        {
                            for (size_t i = 0; i < agent.currentPath.size(); ++i)
                            {
                                ImGui::BulletText("Waypoint %zu: (%.2f, %.2f, %.2f)", i,
                                    agent.currentPath[i].x, agent.currentPath[i].y, agent.currentPath[i].z);
                            }
                            ImGui::TreePop();
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("No active path.");
                    }
                }
                else if (typeName == "PhysicsProperties")
                {
                    auto& phys = *static_cast<PhysicsProperties*>(componentPtr);

                    // Your existing controls
                    DrawEnumDropdown("Motion Type", &phys.motionType, Framework::kMotionTypeNames);
                    DrawEnumDropdown("Category", &phys.category, Framework::kPhysicsCategoryNames);
                }

                else if (typeName == "ScriptProperties")
                {
                    auto& scripts = *static_cast<ScriptProperties*>(componentPtr);
                    DrawScriptPropertiesWidget(scripts, entity);

                    // We fully handle this component ourselves, so we DON'T call DrawInspector
                    if (ImGui::Button("Remove ScriptProperties"))
                    {
                        TypeRegistry::Instance().RemoveComponent(typeName, ecs, entity);
                    }
                    continue;
                }
                // 1. Handle EVENT RECEIVER (List Version)
                if (typeName == "EventReceiverComponent")
                {
                    auto& receiverComp = *static_cast<Framework::EventReceiverComponent*>(componentPtr);

                    // Track which index to remove (if any)
                    int indexToRemove = -1;

                    // Loop through existing receivers
                    for (int i = 0; i < receiverComp.receivers.size(); ++i)
                    {
                        auto& entry = receiverComp.receivers[i];

                        // IMPORTANT: Push ID to ensure unique ID for every button/input in this loop
                        ImGui::PushID(i);

                        ImGui::Text("Receiver #%d", i + 1);

                        // --- Input 1: Message to Listen For ---
                        char msgListenBuffer[256];
                        strncpy_s(msgListenBuffer, sizeof(msgListenBuffer), entry.messageToListenFor.c_str(), _TRUNCATE);

                        if (ImGui::InputText("Listen For", msgListenBuffer, sizeof(msgListenBuffer)))
                        {
                            entry.messageToListenFor = msgListenBuffer;
                            // Force re-subscription logic in System
                            entry.isSubscribed = false;
                        }

                        // --- Input 2: Lua Function ---
                        char luaFuncBuffer[256];
                        strncpy_s(luaFuncBuffer, sizeof(luaFuncBuffer), entry.luaFunctionToCall.c_str(), _TRUNCATE);

                        if (ImGui::InputText("Lua Function", luaFuncBuffer, sizeof(luaFuncBuffer)))
                        {
                            entry.luaFunctionToCall = luaFuncBuffer;
                            // Safer to reset subscription if function changes too
                            entry.isSubscribed = false;
                        }

                        // --- Remove Button ---
                        // Make the button red to indicate destruction
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("Remove Entry"))
                        {
                            indexToRemove = i;
                        }
                        ImGui::PopStyleColor();

                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    // Handle Removal
                    if (indexToRemove != -1)
                    {
                        receiverComp.receivers.erase(receiverComp.receivers.begin() + indexToRemove);
                    }

                    // Handle Addition
                    if (ImGui::Button("+ Add New Receiver"))
                    {
                        receiverComp.receivers.push_back({}); // Add empty/default entry
                    }
                }

                // 2. Handle EVENT SENDER (List Version)
                else if (typeName == "EventSenderComponent")
                {
                    auto& senderComp = *static_cast<Framework::EventSenderComponent*>(componentPtr);

                    int indexToRemove = -1;

                    for (int i = 0; i < senderComp.senders.size(); ++i)
                    {
                        auto& entry = senderComp.senders[i];

                        ImGui::PushID(i);
                        ImGui::Text("Sender #%d", i + 1);

                        // --- Input 1: Trigger Name ---
                        char triggerBuffer[256];
                        strncpy_s(triggerBuffer, sizeof(triggerBuffer), entry.triggerName.c_str(), _TRUNCATE);

                        if (ImGui::InputText("Trigger On", triggerBuffer, sizeof(triggerBuffer)))
                        {
                            entry.triggerName = triggerBuffer;
                        }

                        // --- Input 2: Message Name ---
                        char msgSendBuffer[256];
                        strncpy_s(msgSendBuffer, sizeof(msgSendBuffer), entry.messageName.c_str(), _TRUNCATE);

                        if (ImGui::InputText("Send Msg", msgSendBuffer, sizeof(msgSendBuffer)))
                        {
                            entry.messageName = msgSendBuffer;
                        }

                        // --- Remove Button ---
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("Remove Entry"))
                        {
                            indexToRemove = i;
                        }
                        ImGui::PopStyleColor();

                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    // Handle Removal
                    if (indexToRemove != -1)
                    {
                        senderComp.senders.erase(senderComp.senders.begin() + indexToRemove);
                    }

                    // Handle Addition
                    if (ImGui::Button("+ Add New Sender"))
                    {
                        senderComp.senders.push_back({});
                    }
                }
                // 3. Handle SCRIPTS (Existing logic)
                else if (typeName == "ScriptProperties")
                {
                    auto& scripts = *static_cast<ScriptProperties*>(componentPtr);
                    DrawScriptPropertiesWidget(scripts, entity);

                    if (ImGui::Button("Remove ScriptProperties"))
                    {
                        TypeRegistry::Instance().RemoveComponent(typeName, ecs, entity);
                    }
                }
                else if (typeName == "TextProperties")
                {
                    // Cast to your specific component type
                    auto& textComp = *static_cast<TextProperties*>(componentPtr);

                    // Draw the standard stuff using your helper (Text string, Color, etc.)
                    // Note: If you don't have a helper for TextComponent, you can draw inputs manually here.
                    // Assuming DrawInspector handles the string/color automatically, we just handle alignment manually.

                    // --- Manual String Input (If needed) ---
                    char textBuffer[256];
                    strncpy_s(textBuffer, sizeof(textBuffer), textComp.text.c_str(), _TRUNCATE);
                    if (ImGui::InputText("Text Content", textBuffer, sizeof(textBuffer))) {
                        textComp.text = textBuffer;
                    }

                    ImGui::DragFloat("Scale", &textComp.scale, 0.01f, 0.0f, 100.0f);

                    // --- Manual Color Input (If needed) ---
                    ImGui::ColorEdit3("Color", &textComp.color.x);

                    // --- ALIGNMENT DROPDOWN ---
                    const char* alignOptions[] = { "Left", "Center", "Right" };

                    // STEP 1: Create a temporary 'int' copy of the value
                    // We use static_cast to force the Enum to become an Int for ImGui
                    int tempAlign = static_cast<int>(textComp.alignment);

                    // Safety clamp to prevent crashes if data is corrupt
                    if (tempAlign < 0) tempAlign = 0;
                    if (tempAlign > 2) tempAlign = 0;

                    // STEP 2: Pass the address of the 'int' to ImGui
                    if (ImGui::Combo("Alignment", &tempAlign, alignOptions, IM_ARRAYSIZE(alignOptions)))
                    {
                        // STEP 3: Convert the 'int' back to Enum
                        // We use static_cast to force the Int back into the Enum storage
                        textComp.alignment = tempAlign;
                    }
                    ImGui::Checkbox("Billboard Mode", &textComp.isBillboard);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("If checked, text always faces the camera.");
                    }
                    ImGui::Checkbox("Screen-Space (UI)", &textComp.screenSpace);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(
                            "If checked, this text is drawn as HUD/UI.\n"
                            "Transform.position.x/y are treated as normalized screen coords in [0,1]."
                        );
                    }
                }

                if (typeName == "BoxColliderComponent")
                {
                    auto& colliderComp = *static_cast<Framework::BoxColliderComponent*>(componentPtr);
                    int indexToRemove = -1;

                    for (int i = 0; i < colliderComp.colliders.size(); ++i)
                    {
                        auto& entry = colliderComp.colliders[i];
                        ImGui::PushID(i);
                        ImGui::Text("Collider #%d", i + 1);

                        ImGui::DragFloat3("Size", &entry.size.x, 0.1f);
                        ImGui::DragFloat3("Center", &entry.center.x, 0.1f);

                        // --- 1. TRIGGER LOGIC (Walk In) ---
                        ImGui::Separator();
                        ImGui::Checkbox("Is Trigger (Walk-in)", &entry.canPassThrough);

                        if (entry.canPassThrough)
                        {
                            ImGui::Indent();
                            char trigBuff[256];
                            strncpy_s(trigBuff, sizeof(trigBuff), entry.triggerMessage.c_str(), _TRUNCATE);
                            if (ImGui::InputText("Trigger Event", trigBuff, sizeof(trigBuff))) {
                                entry.triggerMessage = trigBuff;
                            }
                            ImGui::TextDisabled("(Fires on Enter/Exit collision)");
                            ImGui::Unindent();
                        }

                        // --- 2. INTERACT LOGIC (Press E) ---
                        ImGui::Separator();
                        ImGui::Checkbox("Is Interactable (Press E)", &entry.isInteractable);

                        if (entry.isInteractable)
                        {
                            char promptBuff[256];
                            strncpy_s(promptBuff, sizeof(promptBuff), entry.promptText.c_str(), _TRUNCATE);
                            if (ImGui::InputText("Prompt Text", promptBuff, sizeof(promptBuff))) {
                                entry.promptText = promptBuff;
                            }
                            ImGui::TextDisabled("(Displayed in UI: 'E to Pickup Rock')");

                            ImGui::Indent();
                            char intBuff[256];
                            strncpy_s(intBuff, sizeof(intBuff), entry.interactMessage.c_str(), _TRUNCATE);
                            if (ImGui::InputText("Interact Event", intBuff, sizeof(intBuff))) {
                                entry.interactMessage = intBuff;
                            }
                            ImGui::TextDisabled("(Fires via Raycast + E)");
                            ImGui::Unindent();
                        }

                        // --- Remove ---
                        ImGui::Separator();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("Remove Collider")) indexToRemove = i;
                        ImGui::PopStyleColor();

                        ImGui::PopID();
                        ImGui::Separator();
                    }


                    if (indexToRemove != -1)
                    {
                        colliderComp.colliders.erase(colliderComp.colliders.begin() + indexToRemove);
                    }

                    if (ImGui::Button("+ Add Collider"))
                    {
                        colliderComp.colliders.push_back({}); // Add default
                    }
                }
                else if (typeName == "ButtonComponent")
                {
                    auto& button = *static_cast<ButtonComponent*>(componentPtr);

                    ImGui::Checkbox("Is Hovered (Debug)", &button.isHovered);
                    ImGui::Checkbox("Is Front Button", &button.isFrontButton);
                    ImGui::Checkbox("Interactable", &button.interactable);

                    ImGui::Separator();
                    ImGui::Text("Hover Texture Override");

                    ImGui::PushID("HoverTexDrop");

                    // 1. Setup the wrapper with the saved GUID
                    TextureProperties tempWrapper;
                    tempWrapper.guid = button.hoverTextureGuid;

                    // 2. Fetch the real asset to populate the missing data (like size and ID)
                    if (auto* realTex = AssetManager::Instance().Get<TextureProperties>(button.hoverTextureGuid))
                    {
                        tempWrapper.textureID = realTex->textureID;
                        tempWrapper.width = realTex->width;     // Populates the X size
                        tempWrapper.height = realTex->height;   // Populates the Y size
                    }

                    // 3. Draw the dropdown AND your preview function!
                    DrawAssetDropdown<TextureProperties>("Select Hover Asset", tempWrapper);
                    DrawTexturePreview(tempWrapper);

                    // 4. Save the modified values back
                    button.hoverTextureGuid = tempWrapper.guid;
                    button.hoverTextureID = tempWrapper.textureID;

                    ImGui::PopID();
                }

                else if (typeName == "ItemComponent")
                {
                    auto& item = *static_cast<Framework::ItemComponent*>(componentPtr);

                    int currentItemIndex = Framework::ItemNameToId(item.name);

                    if (ImGui::Combo("Item Select", &currentItemIndex, &Framework::ItemGetter, nullptr, Framework::kItemCount))
                    {
                        // If selection changed, update the string immediately
                        if (currentItemIndex >= 0 && currentItemIndex < Framework::kItemCount)
                        {
                            // CHANGED: Access the database directly to get the ID string
                            item.name = Framework::GetItemDatabase()[currentItemIndex].id;
                        }
                    }

                    if (currentItemIndex == -1)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown Item: %s", item.name.c_str());

                        char nameBuffer[256];
                        strncpy_s(nameBuffer, sizeof(nameBuffer), item.name.c_str(), _TRUNCATE);
                        if (ImGui::InputText("Manual Name override", nameBuffer, sizeof(nameBuffer)))
                        {
                            item.name = nameBuffer;
                        }
                    }
                    else
                    {
                        // Just show the filename hint if it is valid
                        ImGui::TextDisabled("Texture: Assets/Textures/%s.png", item.name.c_str());
                    }

                    ImGui::Separator();

                    // --- Booleans ---
                    ImGui::Checkbox("Is Ingredient", &item.isIngredient);
                    ImGui::Checkbox("Is Choppable", &item.isChoppable);
                    ImGui::Checkbox("Destroy When Pick?", &item.toDestroyWhenPicked);
                }



                // 4. Handle EVERYTHING ELSE (Default Inspector)
                else
                {
                    DrawInspector(componentPtr, *propList);
                }

                if (typeName != "CollectibleProperties")
                {
                    if (ImGui::Button(std::string("Remove ").append(typeName).c_str()))
                    {
                        TypeRegistry::Instance().RemoveComponent(typeName, ecs, entity);
                        break;
                    }
                }

                if (typeName == "ParticleEmitterComponent")
                {
                    auto& em = *static_cast<ParticleEmitterComponent*>(componentPtr);

                    std::string idSuffix = "##Particle_" + std::to_string((uint32_t)entity);

                    ImGui::Checkbox(("Active" + idSuffix).c_str(), &em.active);

                    ImGui::Separator();

                    // Texture dropdown (billboard PNG)
                    TextureProperties tmp{};
                    tmp.guid = em.textureGuid;

                    DrawAssetDropdown<TextureProperties>("Particle Texture" + idSuffix, tmp);
                    em.textureGuid = tmp.guid;

                    if (tmp.textureID != 0)
                        ImGui::Image((ImTextureID)(uintptr_t)tmp.textureID, ImVec2(96, 96), ImVec2(0, 1), ImVec2(1, 0));

                    ImGui::Checkbox(("Pixel Filter (Nearest)" + idSuffix).c_str(), &em.pixelFilter);

                    ImGui::Separator();

                    // Loop vs duration
                    ImGui::Checkbox(("Loop" + idSuffix).c_str(), &em.loop);
                    if (!em.loop)
                        ImGui::DragFloat(("Duration (s)" + idSuffix).c_str(), &em.duration, 0.05f, 0.0f, 30.0f);

                    if (ImGui::Button(("Restart Emission" + idSuffix).c_str()))
                    {
                        em.timeSinceStart = 0.0f;
                        em.emitAccumulator = 0.0f;
                        em.particles.clear();
                    }

                    ImGui::Separator();

                    // Spray controls
                    ImGui::DragFloat3(("Direction" + idSuffix).c_str(), &em.direction.x, 0.05f);
                    ImGui::DragFloat(("Spread Angle (deg)" + idSuffix).c_str(), &em.spreadAngleDeg, 0.1f, 0.0f, 180.0f);

                    ImGui::DragFloat(("Range" + idSuffix).c_str(), &em.range, 0.05f, 0.0f, 100.0f);
                    ImGui::DragFloat(("Particles/sec" + idSuffix).c_str(), &em.particlesPerSecond, 1.0f, 0.0f, 5000.0f);

                    ImGui::DragFloat(("Particle Speed" + idSuffix).c_str(), &em.particleSpeed, 0.05f, 0.0f, 200.0f);
                    ImGui::DragFloat(("Particle Size" + idSuffix).c_str(), &em.particleSize, 0.005f, 0.0f, 10.0f);

                    ImGui::Checkbox(("Local Space" + idSuffix).c_str(), &em.localSpace);
                    ImGui::DragFloat3(("Emitter Offset" + idSuffix).c_str(), &em.emitterOffset.x, 0.05f);

                    const char* emissionShapeNames[] = { "Point", "Box", "Sphere" };
                    int emissionShape = em.emissionShape;
                    if (emissionShape < 0 || emissionShape > 2)
                        emissionShape = 0;

                    if (ImGui::Combo(("Emission Shape" + idSuffix).c_str(), &emissionShape, emissionShapeNames, IM_ARRAYSIZE(emissionShapeNames)))
                    {
                        em.emissionShape = emissionShape;
                    }

                    if (em.emissionShape == 1)
                    {
                        ImGui::DragFloat3(("Box Half Extents" + idSuffix).c_str(), &em.emissionBoxHalfExtents.x, 0.05f, 0.0f, 100.0f);
                    }
                    else if (em.emissionShape == 2)
                    {
                        ImGui::DragFloat(("Emission Radius" + idSuffix).c_str(), &em.emissionRadius, 0.05f, 0.0f, 100.0f);
                    }

                    ImGui::DragInt(("Max Particles" + idSuffix).c_str(), &em.maxParticles, 10, 0, 200000);
                }
            }
        }

        // Show "Add [Component]" buttons for missing ones
        for (global_registration* reg = global_registration::m_pHead; reg; reg = reg->m_pNext)
        {
            const prop_list* propList = reg->m_pPopList;
            if (!propList || !propList->m_pName || propList->m_Props.empty()) continue;
            if (attached.contains(propList->m_pName)) continue;
            if (excludedComponents.count(propList->m_pName)) continue;

            if (ImGui::Button(std::string("Add ").append(propList->m_pName).c_str()))
            {
                TypeRegistry::Instance().AddComponent(propList->m_pName, ecs, entity);

                if (std::string(propList->m_pName) == "BoxColliderComponent")
                {
                    if (!ecs.HasComponent<MeshProperties>(entity))
                    {
                        MeshProperties meshComponent{};
                        meshComponent.debugShape = DebugShapeType::Box;
                        meshComponent.debugcolor = glm::vec3(0.0f, 1.0f, 0.0f); // Green
                        MeshLoader::loadMesh(meshComponent.path, meshComponent);

                        ecs.AddComponent<MeshProperties>(entity, meshComponent);

                        std::cout << "[Editor] Auto-added Debug Mesh for BoxCollider on Entity " << entity << std::endl;
                    }
                    else
                    {
                        MeshProperties& meshComponent = ecs.GetComponent<MeshProperties>(entity);
                        meshComponent.debugShape = DebugShapeType::Box;
                        meshComponent.debugcolor = glm::vec3(0.0f, 1.0f, 0.0f); // Green
                        MeshLoader::loadMesh(meshComponent.path, meshComponent);
                    }
                }
            }
        }
    }

    void RegisterComponentTypes()
    {
        // Only if the component nids to be under the entity inspector
        TypeRegistry::Instance().Register<TextureProperties>("TextureProperties", &g_TexturePropertiesProps);
        TypeRegistry::Instance().Register<MeshProperties>("MeshProperties", &g_MeshProps);
        TypeRegistry::Instance().Register<AudioComponent>("AudioComponent", &g_AudioComponentProps);
        TypeRegistry::Instance().Register<ListenerComponent>("ListenerComponent", &g_ListenerComponentProps);
        TypeRegistry::Instance().Register<TextProperties>("TextProperties", &g_TextComponentProps);
        TypeRegistry::Instance().Register<TransformComponent>("TransformComponent", &g_TransformProps);
        TypeRegistry::Instance().Register<NavMeshComponent>("NavMeshComponent", &g_NavMeshProps);
        TypeRegistry::Instance().Register<NavMeshAgentComponent>("NavMeshAgentComponent", &g_NavMeshAgentProps);
        TypeRegistry::Instance().Register<AnimationProperties>("AnimationProperties", &g_AnimationPropertiesProps);
        TypeRegistry::Instance().Register<LightProperties>("LightProperties", &g_LightProps);
        TypeRegistry::Instance().Register<PhysicsProperties>("PhysicsProperties", &g_PhysicsProps);
        TypeRegistry::Instance().Register<EventReceiverComponent>("EventReceiverComponent", &g_EventReceiverProps);
        TypeRegistry::Instance().Register<EventSenderComponent>("EventSenderComponent", &g_EventSenderProps);
        TypeRegistry::Instance().Register<ScriptProperties>("ScriptProperties", &g_ScriptPropertiesProps);
        TypeRegistry::Instance().Register<ScriptEntry>("ScriptEntry", &g_ScriptEntryProps);
        TypeRegistry::Instance().Register<BoxColliderComponent>("BoxColliderComponent", &g_BoxColliderProps);
        TypeRegistry::Instance().Register<ItemComponent>("ItemComponent", &g_ItemProps);
        TypeRegistry::Instance().Register<ButtonComponent>("ButtonComponent", &g_ButtonComponentProps);
        TypeRegistry::Instance().Register<ParticleEmitterComponent>("ParticleEmitterComponent", &g_ParticleEmitterProps);
        TypeRegistry::Instance().Register<TimerComponent>("TimerComponent", &g_TimerComponentProps);
    }

    void AutoRegisterAllReflectedComponents(Coordinator& ecs)
    {
        for (global_registration* reg = global_registration::m_pHead; reg; reg = reg->m_pNext)
        {
            const prop_list* propList = reg->m_pPopList;
            if (!propList || !propList->m_pName) continue;

            const std::string typeName = propList->m_pName;

            // 1. Get the pointer to the function in the registry
            if (auto* getterPtr = TypeRegistry::Instance().Get(typeName))
            {
                // 2. [FIX] Create a local COPY of the std::function.
                // This decouples it from the TypeRegistry map's memory address.
                TypeGetter getterCopy = *getterPtr;

                // 3. [FIX] Capture the COPY by value 'getterCopy'
                // We capture &ecs by reference because ecs outlives this lambda.
                ecs.RegisterRawGetter(typeName, [getterCopy, &ecs](Entity e) -> void*
                    {
                        // 4. Call the copied function directly. 
                        // This is safe because the function lives inside this lambda now.
                        return getterCopy(ecs, e);
                    });
            }
        }
    }

}