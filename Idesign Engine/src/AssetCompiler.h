///////////////////////////////////////////////////////////////////////////////
///
///	@file  : AssetCompiler.h
/// @Brief : 
///	
///	@Authors: Edwin Leow
    ///	@Copyright 2025, Digipen Institute of Technology
    ///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef  _ASSET_COMPILER_H_
#define _ASSET_COMPILER_H_

#include <JsonSerialize.h>
#include <PropertySystem.h>
#include "AssetHandler.h"
#include <filesystem>
#include "glm.hpp"
#include "Coordinator.h"
#include "AssetManager.h"

// Properties Files
#include "TextureProperties.h"
#include "MeshProperties.h"
#include "TransformProperties.h"
#include "MaterialProperties.h"
#include "ScriptProperties.h"
#include "AnimationLoader.h"

namespace fs = std::filesystem;
namespace Framework
{
    void loadScene(const std::string& scenePath);
    void saveScene(const std::string& scenePath);

    void saveEntityToPrefab(Entity entity, const std::string& prefabPath);
    void CreatePrefabFromEntity(Entity entity, const std::string& prefabName);
    void DuplicateEntity(Framework::Coordinator& ecs, Entity sourceEntity);

    //Prefab and overloads
    Entity InstantiatePrefab(const std::string& prefabGUID);
    Entity InstantiatePrefab(const std::string& prefabGUID, const glm::vec3& position);
    Entity InstantiatePrefab(const std::string& prefabGUID, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
    
    // Find prefab GUID by name
    std::string FindPrefabGUIDByName(const std::string& prefabName);
    Entity InstantiatePrefabByName(const std::string& prefabName);
    Entity InstantiatePrefabByName(const std::string& prefabName, const glm::vec3& position);
    Entity InstantiatePrefabByName(const std::string& prefabName, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);

    bool MoveFileToAssetFolder(const std::string& sourcePath, const std::string& destinationFolder, std::string& outFinalPath, std::string& outError);

    // Deserialize JSON into a C++ object using reflection-based property list
    void Deserialize(void* pObject, const prop_list& PropList, const rapidjson::Value& obj);

    // Serialize a C++ object into a JSON object using reflection-based property list
    void Serialize(void* pObject, const prop_list& PropList, rapidjson::Value& outObj, rapidjson::Document::AllocatorType& allocator);
    
    // Assign a GUID to the object if it doesn't already have one
    void SetGuid(void* pObject, const prop_list& PropList);

    // Compile a single object from JSON into binary format
    template<typename T>
    bool CompileSingleObject(const std::string& jsonPath, const std::string& outPath, const std::string& descPath, const prop_list& props)
    {
        // Load and Parse JSON file
        FILE* fp = fopen(jsonPath.c_str(), "r");
        char buffer[65536];
        rapidjson::FileReadStream is(fp, buffer, sizeof(buffer));
        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);

        // Check for Parse Errors
        if (doc.HasParseError())
        {
            std::cerr << "JSON parse error\n";
            return false;
        }

        // Create Object and Populate from JSON
        std::unique_ptr<T> obj = std::make_unique<T>();
        Deserialize(obj.get(), props, doc);
        SetGuid(obj.get(), props);

        // Write object to Binary file
        std::ofstream out(outPath, std::ios::binary);
        if (!out.is_open()) return false;

        JsonToBinary(out, *obj, props);
        out.close();

        // Save descriptor file
        rapidjson::Document descDoc;
        descDoc.SetObject();
        auto& allocator = descDoc.GetAllocator();

        Serialize(obj.get(), props, descDoc, allocator);

        FILE* descFp = fopen(descPath.c_str(), "wb");
        if (!descFp)
        {
            std::cerr << "Failed to open descriptor file: " << descPath << "\n";
            return false;
        }

        char writeBuffer[65536];
        rapidjson::FileWriteStream os(descFp, writeBuffer, sizeof(writeBuffer));
        rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
        descDoc.Accept(writer);
        fclose(descFp);

        return true;
    }

    // Compile an array of objects from JSON into Binary Format
    template<typename T>
    bool CompileArray(const std::string& jsonPath, const std::string& outPath, const std::string& descPath, const prop_list& props, const std::string& arrayKey)
    {
        // Load and Parse JSON file
        FILE* fp = fopen(jsonPath.c_str(), "r");
        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(65536);
        rapidjson::FileReadStream is(fp, buffer.get(), 65536);
        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);

        // Validatre the array in JSON
        if (doc.HasParseError() || !doc.HasMember(arrayKey.c_str()) || !doc[arrayKey.c_str()].IsArray())
        {
            std::cerr << "Invalid array format\n";
            return false;
        }

        const auto& arr = doc[arrayKey.c_str()].GetArray();

        // Open Binary Output File
        std::ofstream out(outPath, std::ios::binary);
        if (!out.is_open()) return false;

        // Count the array size
        uint32_t count = static_cast<uint32_t>(arr.Size());
        out.write(reinterpret_cast<char*>(&count), sizeof(count));

        // Serialize each object in array
        for (const auto& item : arr)
        {
            std::unique_ptr<T> obj = std::make_unique<T>();
            Deserialize(obj.get(), props, item);

            // Ensure GUID is set
            SetGuid(obj.get(), props);
            JsonToBinary(out, *obj, props);
        }
        out.close();

        // Write descriptor file
        rapidjson::Document descDoc;
        descDoc.SetObject();
        auto& allocator = descDoc.GetAllocator();

        descDoc.AddMember("count", count, allocator);

        rapidjson::Value entries(rapidjson::kArrayType);
        for (const auto& item : arr) {
            std::unique_ptr<T> obj = std::make_unique<T>();
            Deserialize(obj.get(), props, item);

            // Ensure GUID is set
            SetGuid(obj.get(), props);
            
            rapidjson::Value entry(rapidjson::kObjectType);
            Serialize(obj.get(), props, entry, allocator);
            entries.PushBack(entry, allocator);
        }
        descDoc.AddMember("entries", entries, allocator);

        FILE* descFp = fopen(descPath.c_str(), "wb");
        if (!descFp) {
            std::cerr << "Failed to open descriptor file: " << descPath << "\n";
            return false;
        }

        char writeBuffer[65536];
        rapidjson::FileWriteStream os(descFp, writeBuffer, sizeof(writeBuffer));
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        descDoc.Accept(writer);
        fclose(descFp);

        return true;
    }

    // Serialize object to Binary using reflection-based property list
    template<typename T>
    void JsonToBinary(std::ofstream& out, const T& obj, const prop_list& props)
    {
        for (const auto& p : props.m_Props)
        {
            switch (p.m_Type)
            {
            case prop::INT:    { int          v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::DRAG_FLOAT:
            case prop::SLIDER_FLOAT:
            case prop::FLOAT:  { float        v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::DOUBLE: { double       v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::LONG:   { long         v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::SHORT:  { short        v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::CHAR:   { char         v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::BOOL:   { bool         v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::UINT:   { unsigned int v; p.m_fnGet(&obj, &v); out.write(reinterpret_cast<const char*>(&v), sizeof(v)); break; }
            case prop::STRING:
            {
                std::string value;
                p.m_fnGet(&obj, &value);
                uint32_t length = static_cast<uint32_t>(value.size());
                out.write(reinterpret_cast<const char*>(&length), sizeof(length));
                out.write(value.data(), length);
                break;
            }
            case prop::COLOR3:
            case prop::VEC3:
            {
                glm::vec3 value;
                p.m_fnGet(&obj, &value);
                out.write(reinterpret_cast<char*>(&value.x), sizeof(float));
                out.write(reinterpret_cast<char*>(&value.y), sizeof(float));
                out.write(reinterpret_cast<char*>(&value.z), sizeof(float));
                break;
            }
            default:
                throw std::runtime_error(std::string("Unsupported property type: ") + p.m_pName);
            }
        }
    }

    // Deserialize binary data into object using reflection-based property list
    template<typename T>
    void BinaryToObject(std::istream& in, T& obj, const prop_list& props)
    {
        constexpr uint32_t MAX_STRING_LENGTH = 1024 * 1024;
        for (const auto& p : props.m_Props)
        {
            if (!p.m_fnSet) throw std::runtime_error("Function pointer m_fnSet is null for property: " + std::string(p.m_pName));
            switch (p.m_Type)
            {
            case prop::INT:    { int          v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::DRAG_FLOAT:
            case prop::SLIDER_FLOAT:
            case prop::FLOAT:  { float        v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::DOUBLE: { double       v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::LONG:   { long         v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::SHORT:  { short        v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::CHAR:   { char         v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::BOOL:   { bool         v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::UINT:   { unsigned int v; in.read(reinterpret_cast<char*>(&v), sizeof(v)); if (!in) throw; p.m_fnSet(&obj, &v); break; }
            case prop::STRING:
            {
                uint32_t length;
                in.read(reinterpret_cast<char*>(&length), sizeof(length));

                if (!in || length > MAX_STRING_LENGTH) throw std::runtime_error("Invalid string length for property: " + std::string(p.m_pName));

                std::string value(length, '\0');
                in.read(&value[0], length);

                if (!in) throw std::runtime_error("Failed to read string data for property: " + std::string(p.m_pName));
                p.m_fnSet(&obj, &value);
                break;
            }
            case prop::COLOR3:
            case prop::VEC3:
            {
                glm::vec3 value;
                p.m_fnGet(&obj, &value);
                in.read(reinterpret_cast<char*>(&value.x), sizeof(float));
                in.read(reinterpret_cast<char*>(&value.y), sizeof(float));
                in.read(reinterpret_cast<char*>(&value.z), sizeof(float));
                break;
            }
            default:
                throw std::runtime_error("Unsupported property type: " + std::string(p.m_pName));
            }
        }
    }
}
#endif // ! _ASSET_COMPILER_H_