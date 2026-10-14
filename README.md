# 🎮 3D_SoftwareEngineeringProject
Collaborative 3D Game Engine + Game Project built by Developers and UI/UX Engineers

# 🛠️ IDesign Engine
IDesign Engine is a 3D game engine built by IDesign Studio, designed around a data-driven Entity Component System (ECS) architecture.
It pairs a reflection-driven editor with a Lua scripting layer, so gameplay can be built and iterated on without recompiling the engine.

This project is developed by a team of eleven Year 2 Computer Science and UI/UX students from the Singapore Institute of Technology, in Joint Degree with DigiPen Institute of Technology.

The engine is written in C++, utilizing OpenGL and GLSL for rendering, Jolt for physics, FMOD for audio and Lua for gameplay scripting.
Our primary goal is to build a scalable engine and editor that supports a full game production pipeline — from asset import, to scene and prefab authoring, to scripted gameplay.

## 🧪 Game Showcase — Runes & Remedies
Built on the engine is **Runes & Remedies**, a 3D potion-brewing shop game:
- 🌿 Gather herbs in the Forest and mine ore in the Quarry by day
- ⚗️ Prepare ingredients with the chopping board, mortar, furnace and cauldron, then brew potions
- 🧙 Serve customers their orders by night to earn gold and climb from Novice Alchemist to Legendary Alchemist
- 🎒 Spend gold on upgrades — bigger inventory and backpack, currency multipliers, an auto-cauldron and new areas

## 🚀 Engine Features
- 🧱 Entity-Component System (ECS)
- 🪞 Reflection & Property System (auto-generated serialization and editor inspector widgets)
- 🧩 Entity Properties/Components (Transform, Mesh, Material, Texture, Animation, Physics, Audio, Collider, Timer, Particle etc.)
- 🗂️ Asset Pipeline (GUID-based asset handles, descriptor → binary compilation, JSON serialization, Save/Load)
- 🔄 Asset Hot-Reloading (file watcher on asset folders)
- 🧊 3D Mesh, Material & Texture Loading (Assimp, DDS, async texture uploads)
- 🦴 Skeletal Animation (skinned meshes, bone hierarchy, animation clips)
- 🏗️ Scene & Prefab System (create, instantiate, duplicate, save prefabs and scenes)
- 📜 Lua Scripting (engine, physics, audio, camera, pathfinding and gameplay APIs exposed to scripts)
- 📨 Event System (channel-based publish/subscribe wired to Lua callbacks)
- 🎵 3D Positional Audio (FMOD Studio banks/events, master/BGM/SFX volume)
- 🧭 Navigation Mesh & Pathfinding (Recast/Detour)
- ⚙️ Physics & Collision (Jolt, triggers, raycast interaction)
- 🌳 Instanced Foliage Rendering & Particles
- 🖱️ UI Buttons, HUD, Menus & Cutscenes
- 🎬 Scene Management & Game Phases (Day / Night / Upgrade)

## 🕹️ Controls
- `W` `A` `S` `D` — Move
- `E` — Interact / gather / mine
- `R` — Open Recipe Book
- Mouse — Look around / UI

## ▶️ To Build Engine/Game
Open `Idesign Engine.sln` in Visual Studio 2022 and build the **x64** configuration.

## ⚠️ Important Notes
This repository is a portfolio copy of the project.
The `src` folder only contains the files I developed or contributed to, so the solution will not build as-is without the rest of the team's source files.

## 📚 Libraries Used
- 🎧 FMOD Core
- 🎼 FMOD Studio
- ⚙️ Jolt Physics
- 🧭 Recast & Detour
- 📜 Lua 5.4
- 🧊 Assimp
- 🔤 freetype
- 🌐 glew
- 🖱️ glfw
- 🔁 gleq
- 📐 glm
- 🧰 imgui
- 🪞 RTTR
- 📄 RapidJSON
- 🖼️ lodepng
- 🖼️ stb_image
- 🧱 tinyobjloader

## 📁 Source Code Notes
The src folder contains code that I personally developed or collaborated on with teammates, including:
- 🗂️ Asset Pipeline — AssetManager, AssetCompiler, Mesh/Animation/Material/Texture loaders, FileCheckSystem
- 🪞 Reflection & Editor — PropertyMacro, PropertySystem, TypeRegistry, PropertyWidget, all Properties/Components
- 📜 Scripting & Events — ScriptingSystem (Lua bindings), EventManager, EventSystem
- 🎵 Audio — AudioSystem (FMOD)
- 🎮 Gameplay — GameSystem, PhaseManager, Inventory, Upgrade, SceneInterface
- 🤝 Contributed to — ButtonSystem, EngineState, SceneManager, main loop
