///////////////////////////////////////////////////////////////////////////////
///
///	@File  : AssetManager.cpp
/// @Brief : Manages assets, including textures, audio, fonts, dictionaries, 
///          windows, entities, and shaders, with methods to load and retrieve 
///          these assets in the Framework namespace.
///
///	@Authors : Edwin
///	@Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "AssetManager.h"
#include "AudioSystem.h"

namespace Framework
{
	void AssetManager::SaveAllAssets()
	{
		std::filesystem::path root = GetEngineRootPath();
		std::cout << "[AssetManager] SaveAllAssets using root path: " << root.string() << std::endl;

		// Nvr include windowconfig here
		std::string meshPath = (root / "Assets/JSON/MeshAsset.json").string();
		//std::cout << "  Saving: " << meshPath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<MeshProperties>::value, meshPath, "mesh");

		std::string audioPath = (root / "Assets/JSON/AudioBank.json").string();
		//std::cout << "  Saving: " << audioPath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<AudioBankMetadata>::value, audioPath, "banks");

		std::string texturePath = (root / "Assets/JSON/TextureAsset.json").string();
		//std::cout << "  Saving: " << texturePath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<TextureProperties>::value, texturePath, "textures");

		std::string prefabPath = (root / "Assets/JSON/PrefabAsset.json").string();
		//std::cout << "  Saving: " << prefabPath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<PrefabProperties>::value, prefabPath, "prefabs");

		std::string scenePath = (root / "Assets/JSON/SceneAsset.json").string();
		//std::cout << "  Saving: " << scenePath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<SceneProperties>::value, scenePath, "scenes");

		std::string materialPath = (root / "Assets/JSON/MaterialAsset.json").string();
		//std::cout << "  Saving: " << materialPath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<MaterialMetadata>::value, materialPath, "materials");

		std::string animPath = (root / "Assets/JSON/AnimationAsset.json").string();
		//std::cout << "  Saving: " << animPath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<AnimationProperties>::value, animPath, "animation");

		std::string scriptPath = (root / "Assets/JSON/ScriptAsset.json").string();
		//std::cout << "  Saving: " << scriptPath << std::endl;
		AssetManager::Instance().SaveArrayToJson(TypeName<ScriptEntry>::value, scriptPath, "scripts");

		std::cout << "Files are being saved" << std::endl;
	}

	void AssetManager::CompileAllAssets()
	{
		std::filesystem::path root = GetEngineRootPath();
		std::cout << "[AssetManager] CompileAllAssets using root path: " << root.string() << std::endl;

		//CompileSingleObject<WindowConfig>("src/Data/WindowAsset.json", "src/Data/WindowAsset.cfg", "Descriptor/WindowAsset.desc", g_WindowConfigProps);

		//std::cout << "  Compiling Mesh:\n    JSON: " << (root / "Assets/JSON/MeshAsset.json").string() << "\n    BIN: " << (root / "Resource/MeshAsset.bin").string() << std::endl;
		CompileArray<MeshProperties>((root / "Assets/JSON/MeshAsset.json").string(), (root / "Resource/MeshAsset.bin").string(), (root / "Descriptor/MeshAsset.desc").string(), g_MeshProps, "mesh");

		//std::cout << "  Compiling Audio:\n    JSON: " << (root / "Assets/JSON/AudioAsset.json").string() << "\n    BIN: " << (root / "Resource/AudioAsset.bin").string() << std::endl;
		CompileArray<AudioBankMetadata>((root / "Assets/JSON/AudioBank.json").string(), (root / "Resource/AudioBank.bin").string(), (root / "Descriptor/AudioBank.desc").string(), g_AudioBankMetadataProps, "banks");

		//std::cout << "  Compiling Texture:\n    JSON: " << (root / "Assets/JSON/TextureAsset.json").string() << "\n    BIN: " << (root / "Resource/TextureAsset.bin").string() << std::endl;
		CompileArray<TextureProperties>((root / "Assets/JSON/TextureAsset.json").string(), (root / "Resource/TextureAsset.bin").string(), (root / "Descriptor/TextureAsset.desc").string(), g_TexturePropertiesProps, "textures");

		//std::cout << "  Compiling Prefab:\n    JSON: " << (root / "Assets/JSON/PrefabAsset.json").string() << "\n    BIN: " << (root / "Resource/PrefabAsset.bin").string() << std::endl;
		CompileArray<PrefabProperties>((root / "Assets/JSON/PrefabAsset.json").string(), (root / "Resource/PrefabAsset.bin").string(), (root / "Descriptor/PrefabAsset.desc").string(), g_PrefabProps, "prefabs");

		//std::cout << "  Compiling Scene:\n    JSON: " << (root / "Assets/JSON/SceneAsset.json").string() << "\n    BIN: " << (root / "Resource/SceneAsset.bin").string() << std::endl;
		CompileArray<SceneProperties>((root / "Assets/JSON/SceneAsset.json").string(), (root / "Resource/SceneAsset.bin").string(), (root / "Descriptor/SceneAsset.desc").string(), g_SceneProps, "scenes");

		//std::cout << "  Compiling Material:\n    JSON: " << (root / "Assets/JSON/MaterialAsset.json").string() << "\n    BIN: " << (root / "Resource/MaterialAsset.bin").string() << std::endl;
		CompileArray<MaterialMetadata>((root / "Assets/JSON/MaterialAsset.json").string(), (root / "Resource/MaterialAsset.bin").string(), (root / "Descriptor/MaterialAsset.desc").string(), g_MaterialMetaProps, "materials");

		//std::cout << "  Compiling Animation:\n    JSON: " << (root / "Assets/JSON/AnimationAsset.json").string() << "\n    BIN: " << (root / "Resource/AnimationAsset.bin").string() << std::endl;
		CompileArray<AnimationProperties>((root / "Assets/JSON/AnimationAsset.json").string(), (root / "Resource/AnimationAsset.bin").string(), (root / "Descriptor/AnimationAsset.desc").string(), g_AnimationPropertiesProps, "animation");

		//std::cout << "  Compiling Scripts:\n    JSON: " << (root / "Assets/JSON/ScriptAsset.json").string() << "\n    BIN: " << (root / "Resource/ScriptAsset.bin").string() << std::endl;
		CompileArray<ScriptEntry>((root / "Assets/JSON/ScriptAsset.json").string(), (root / "Resource/ScriptAsset.bin").string(), (root / "Descriptor/ScriptAsset.desc").string(), g_ScriptEntryProps, "scripts");

		std::cout << "Files are being compiled to .binary" << std::endl;
	}

	void AssetManager::LoadAllAssets()
	{
		std::filesystem::path root = GetEngineRootPath();
		std::cout << "[AssetManager] LoadAllAssets using root path: " << root.string() << std::endl;

		// Mesh & Texture is loaded under Graphics.cpp due to some initialization required under graphics.cpp
		std::string windowCfgPath = (root / "Resource/WindowAsset.cfg").string();
		//std::cout << "  Loading: " << windowCfgPath << std::endl;
		AssetManager::Instance().LoadBinary<WindowConfig>(windowCfgPath, g_WindowConfigProps);

		std::string prefabBinPath = (root / "Resource/PrefabAsset.bin").string();
		//std::cout << "  Loading: " << prefabBinPath << std::endl;
		AssetManager::Instance().LoadBinaryArray<PrefabProperties>(prefabBinPath, g_PrefabProps);

		std::string sceneBinPath = (root / "Resource/SceneAsset.bin").string();
		//std::cout << "  Loading: " << sceneBinPath << std::endl;
		AssetManager::Instance().LoadBinaryArray<SceneProperties>(sceneBinPath, g_SceneProps);

		std::string materialBinPath = (root / "Resource/MaterialAsset.bin").string();
		//std::cout << "  Loading: " << materialBinPath << std::endl;
		AssetManager::Instance().LoadBinaryArray<MaterialMetadata>(materialBinPath, g_MaterialMetaProps, [=](MaterialMetadata& meta) // Capture root by copy
			{
				// Register metadata
				AssetManager::Instance().RegisterAsset(std::make_unique<MaterialMetadata>(meta), &g_MaterialMetaProps, meta.path);

				// Load and register properties
				// IMPORTANT: Assumes meta.path is relative to the root
				std::string fullMaterialPath = (root / meta.path).string();

				// Added debug line here
				//std::cout << "    Loading individual material: " << fullMaterialPath << std::endl;
				
				auto material = LoadMaterialAsset(fullMaterialPath, meta.guid);
				if (material)
				{
					material->guid = meta.guid;
					//std::cout << "[AssetManager] Registering MaterialProperties for GUID=" << meta.guid.id << " at path=" << meta.path << std::endl; // Changed meta.path to meta.key, assuming key is the intended identifier
					AssetManager::Instance().RegisterAsset(std::move(material), &g_MaterialProps, meta.path); // Changed meta.path to meta.key
				}
				else std::cerr << "[AssetManager] Failed to load material file: " << fullMaterialPath << " for GUID=" << meta.guid << std::endl;
			}
		);

		std::string scriptBinPath = (root / "Resource/ScriptAsset.bin").string();
		//std::cout << "  Loading: " << scriptBinPath << std::endl;
		AssetManager::Instance().LoadBinaryArray<ScriptEntry>(scriptBinPath, g_ScriptEntryProps);
	}

	bool AssetManager::DeleteAssetFile(const std::string& filePath)
	{
		// Assumes filePath is relative to the engine root
		std::filesystem::path fullPath = std::filesystem::path(GetEngineRootPath()) / filePath;

		try
		{
			if (std::filesystem::exists(fullPath))
			{
				std::filesystem::remove(fullPath);
				std::cout << "[AssetManager] Deleted file: " << fullPath.string() << std::endl;
				return true;
			}
			else
			{
				std::cerr << "[AssetManager] File not found: " << fullPath.string() << std::endl;
				return false;
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			std::cerr << "[AssetManager] File deletion error: " << e.what() << std::endl;
			return false;
		}
	}

	void AssetManager::Unload(const std::string& guid)
	{
		AssetHandle handle(guid);
		std::filesystem::path root = GetEngineRootPath();

		// Collect all keys with this GUID
		std::vector<AssetKey> keysToErase;
		for (auto& [key, entry] : m_Assets) // Note: removed const to allow prop.m_fnGet
		{
			if (key.guid == handle)
			{
				// Delete file from disk if path property exists
				std::string filePath;
				if (entry.pProps)
				{
					for (const auto& prop : entry.pProps->m_Props)
					{
						if (std::string(prop.m_pName) == "path" && prop.m_Type == prop::STRING && prop.m_fnGet)
						{
							prop.m_fnGet(entry.pObject.get(), &filePath);
							break;
						}
					}
				}

				if (!filePath.empty())
				{
					// Assumes filePath is relative to the engine root
					std::filesystem::path fullPath = root / filePath;
					if (std::filesystem::exists(fullPath))
					{
						std::error_code ec;
						std::filesystem::remove(fullPath, ec);
						if (ec)
							std::cerr << "Failed to delete file: " << fullPath.string() << "\nReason: " << ec.message() << std::endl;
						else
							std::cout << "Deleted file from disk: " << fullPath.string() << std::endl;
					}
				}

				keysToErase.push_back(key);
			}
		}

		// Erase all entries for this GUID
		for (const auto& key : keysToErase) m_Assets.erase(key);
	}

	bool AssetManager::SaveToJson(void* pObject, const std::string& filePath, const prop_list& props)
	{
		assert(pObject != nullptr);
		assert(!props.m_Props.empty());

		rapidjson::Document doc;
		doc.SetObject();
		auto& allocator = doc.GetAllocator();

		Serialize(pObject, props, doc, allocator);

		// Assumes filePath is relative to the engine root
		std::filesystem::path fullPath = std::filesystem::path(GetEngineRootPath()) / filePath;
		std::ofstream ofs(fullPath);
		if (!ofs.is_open()) return false;

		rapidjson::OStreamWrapper osw(ofs);
		rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
		doc.Accept(writer);
		return true;
	}

	void AssetManager::SaveArrayToJson(const std::string& typeName, const std::string& filePath, const std::string& arrayKey)
	{
		assert(!typeName.empty());
		assert(!arrayKey.empty());

		std::filesystem::path fullPath = std::filesystem::path(GetEngineRootPath()) / filePath;

		rapidjson::Document doc;
		doc.SetObject();
		auto& allocator = doc.GetAllocator();

		// Track which GUIDs already exist (to avoid duplicates)
		std::set<std::string> existingGuids;

		// Store the array in original order
		rapidjson::Value array(rapidjson::kArrayType);

		// ========================================================================
		// STEP 1: Copy existing JSON entries AS-IS (preserve order)
		// ========================================================================
		if (std::filesystem::exists(fullPath))
		{
			std::ifstream ifs(fullPath);
			if (ifs.is_open())
			{
				rapidjson::IStreamWrapper isw(ifs);
				rapidjson::Document existingDoc;
				existingDoc.ParseStream(isw);

				if (!existingDoc.HasParseError() &&
					existingDoc.HasMember(arrayKey.c_str()) &&
					existingDoc[arrayKey.c_str()].IsArray())
				{
					const rapidjson::Value& existingArray = existingDoc[arrayKey.c_str()];

					std::cout << "[SaveArrayToJson] Preserving " << existingArray.Size()
						<< " existing " << typeName << " entries\n";

					// Copy each entry AS-IS
					for (rapidjson::SizeType i = 0; i < existingArray.Size(); ++i)
					{
						const rapidjson::Value& item = existingArray[i];

						if (item.HasMember("guid") && item["guid"].IsString())
						{
							std::string guid = item["guid"].GetString();
							existingGuids.insert(guid);

							// Deep copy and append in original order
							rapidjson::Value copy(item, allocator);
							array.PushBack(copy, allocator);
						}
						else
						{
							std::cerr << "[SaveArrayToJson] Warning: Skipping entry without GUID\n";
						}
					}
				}
			}
		}

		// ========================================================================
		// STEP 2: Append ONLY new assets (not already in JSON)
		// ========================================================================
		const auto& assets = GetAssets();
		int addedCount = 0;

		for (const auto& [key, entryObject] : assets)
		{
			if (entryObject.typeName != typeName) continue;

			// Skip if already in JSON
			if (existingGuids.count(key.guid.id) > 0) continue;

			// This is a NEW asset - append it
			rapidjson::Value obj(rapidjson::kObjectType);
			Serialize(entryObject.pObject.get(), *entryObject.pProps, obj, allocator);
			array.PushBack(obj, allocator);

			addedCount++;
			std::cout << "[SaveArrayToJson] Appending new " << typeName
				<< " with GUID: " << key.guid.id << "\n";
		}

		std::cout << "[SaveArrayToJson] Added " << addedCount << " new entries, "
			<< "total now: " << array.Size() << "\n";

		doc.AddMember(rapidjson::Value(arrayKey.c_str(), allocator), array, allocator);

		// ========================================================================
		// STEP 3: Write to file
		// ========================================================================
		std::ofstream ofs(fullPath);
		if (ofs.is_open())
		{
			rapidjson::OStreamWrapper osw(ofs);
			rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
			writer.SetIndent(' ', 4);
			doc.Accept(writer);

			std::cout << "[SaveArrayToJson] Successfully saved to " << filePath << "\n";
		}
		else
		{
			std::cerr << "[SaveArrayToJson] ERROR: Failed to open file for writing\n";
		}
	}

	bool AssetManager::HasTextureByGUID(const std::string& guid) const
	{
		AssetHandle handle(guid);
		AssetKey key{ handle, TypeName<TextureProperties>::value };

		auto it = m_Assets.find(key);
		return it != m_Assets.end();
	}
}