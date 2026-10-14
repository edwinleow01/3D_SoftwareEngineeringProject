///////////////////////////////////////////////////////////////////////////////
///
///	@File  : AssetLoader.h
/// @Brief : 
///
///	@Authors : Edwin
///	@Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _ASSET_LOADER_H_
#define _ASSET_LOADER_H_

#include <AssetCompiler.h>

namespace Framework
{
    // Template-based asset loader for deserializing binary data into objects of type T
    template<typename T>
    struct AssetLoader 
    {
        // Loads a single object of type T from a binary file
        static std::unique_ptr<T> Load(const std::string& binaryPath, const prop_list& props) 
        {
            // Open binary file for reading
            std::ifstream in(binaryPath, std::ios::binary);
            if (!in.is_open()) throw std::runtime_error("Failed to open binary file: " + binaryPath);

            // Allocate and construct object
            auto obj = std::make_unique<T>();

            // Deserialize binary data into object using reflection-based property list
            BinaryToObject(in, *obj, props);

            // Return the loaded object
            return obj;
        }

        // Loads an array of objects of type T from a binary file
        static std::vector<std::unique_ptr<T>> LoadArray(const std::string& binaryPath, const prop_list& props) 
        {
            // Open binary file for reading
            std::ifstream in(binaryPath, std::ios::binary);
            if (!in.is_open()) throw std::runtime_error("Failed to open binary file: " + binaryPath);

            // Read array size
            uint32_t count = 0;
            in.read(reinterpret_cast<char*>(&count), sizeof(count));

            // Valide array size and stream state
            if (!in) throw std::runtime_error("Invalid or empty binary array: " + binaryPath);

            // Prepare result container
            std::vector<std::unique_ptr<T>> result;
            result.reserve(count);

            // Deserialize each object and add to vector
            for (uint32_t i = 0; i < count; ++i) 
            {
                auto obj = std::make_unique<T>();
                BinaryToObject(in, *obj, props);
                result.push_back(std::move(obj));
            }

            // Return the array of loaded objects
            return result;
        }
    };
}
#endif // !_ASSET_LOADER_H_