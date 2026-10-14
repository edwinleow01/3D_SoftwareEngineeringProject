
#include "pch.h"
#include "MaterialLoader.h"

namespace Framework
{
    std::unique_ptr<MaterialProperties> LoadMaterialAsset(const std::string& path, const AssetHandle& guid)
    {
        FILE* fp = fopen(path.c_str(), "r");
        if (!fp)
        {
            std::cerr << "[LoadMaterialAsset] Failed to open file: " << path << std::endl;
            return nullptr;
        }

        char buffer[65536];
        rapidjson::FileReadStream is(fp, buffer, sizeof(buffer));
        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);

        if (doc.HasParseError())
        {
            std::cerr << "[LoadMaterialAsset] JSON parse error in " << path << std::endl;
            return nullptr;
        }

        auto mat = std::make_unique<MaterialProperties>();
        mat->guid = guid;

        if (doc.HasMember("ambient") && doc["ambient"].IsArray())
        {
            mat->ambient.x = doc["ambient"][0].GetFloat();
            mat->ambient.y = doc["ambient"][1].GetFloat();
            mat->ambient.z = doc["ambient"][2].GetFloat();
        }

        if (doc.HasMember("albedo") && doc["albedo"].IsArray())
        {
            mat->albedo.x = doc["albedo"][0].GetFloat();
            mat->albedo.y = doc["albedo"][1].GetFloat();
            mat->albedo.z = doc["albedo"][2].GetFloat();
        }

        if (doc.HasMember("specular") && doc["specular"].IsArray())
        {
            mat->specular.x = doc["specular"][0].GetFloat();
            mat->specular.y = doc["specular"][1].GetFloat();
            mat->specular.z = doc["specular"][2].GetFloat();
        }

        if (doc.HasMember("shininess"))
            mat->shininess = doc["shininess"].GetFloat();

        if (doc.HasMember("opacity"))
            mat->opacity = doc["opacity"].GetFloat();

        return mat;
    }

    void SaveMaterialAsset(const MaterialProperties& mat, const std::string& path, const AssetHandle& guid)
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();

        doc.AddMember("guid", rapidjson::Value(guid.id.c_str(), allocator), allocator);

        // Ambient
        rapidjson::Value ambientArr(rapidjson::kArrayType);
        ambientArr.PushBack(mat.ambient.x, allocator);
        ambientArr.PushBack(mat.ambient.y, allocator);
        ambientArr.PushBack(mat.ambient.z, allocator);
        doc.AddMember("ambient", ambientArr, allocator);

        // Albedo
        rapidjson::Value albedoArr(rapidjson::kArrayType);
        albedoArr.PushBack(mat.albedo.x, allocator);
        albedoArr.PushBack(mat.albedo.y, allocator);
        albedoArr.PushBack(mat.albedo.z, allocator);
        doc.AddMember("albedo", albedoArr, allocator);

        // Specular
        rapidjson::Value specularArr(rapidjson::kArrayType);
        specularArr.PushBack(mat.specular.x, allocator);
        specularArr.PushBack(mat.specular.y, allocator);
        specularArr.PushBack(mat.specular.z, allocator);
        doc.AddMember("specular", specularArr, allocator);

        // Scalars
        doc.AddMember("shininess", mat.shininess, allocator);
        doc.AddMember("opacity", mat.opacity, allocator);

        // Write to file
        FILE* fp = fopen(path.c_str(), "w");
        if (!fp)
        {
            std::cerr << "Failed to open material file for writing: " << path << "\n";
            return;
        }

        char buffer[65536];
        rapidjson::FileWriteStream os(fp, buffer, sizeof(buffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        doc.Accept(writer);
        fclose(fp);
    }

    void RegisterMaterialMetadata(const AssetHandle& guid, const std::string& name, const std::string& path)
    {
        auto meta = std::make_unique<MaterialMetadata>();
        meta->guid = guid;
        meta->name = name;
        meta->path = path;

        const prop_list* props = TypeRegistry::Instance().GetPropList(TypeName<MaterialMetadata>::value);
        AssetManager::Instance().RegisterAsset(std::move(meta), props, path);
    }

    AssetHandle GetDefaultMaterial()
    {
        // Search for a material named "WoodPlank"
        for (const auto& [handle, entry] : AssetManager::Instance().GetAssets())
        {
            if (entry.typeName == TypeName<MaterialMetadata>::value)
            {
                auto* meta = static_cast<MaterialMetadata*>(entry.pObject.get());
                if (meta && meta->name == "WoodPlank")
                {
                    return handle.guid; // Found WoodPlank
                }
            }
        }

        // Fallback: if not found, return null handle
        return AssetHandle{};
    }
}