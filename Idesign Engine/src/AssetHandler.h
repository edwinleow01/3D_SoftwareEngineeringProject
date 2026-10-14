///////////////////////////////////////////////////////////////////////////////
///
/// @File  : AssetHandler.h
/// @Brief : Provides GUID generation and asset handle utilities for uniquely
///          identifying and referencing assets within the framework.
///
/// @Authors : Edwin
/// @Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _ASSET_HANDLER_H_
#define _ASSET_HANDLER_H_

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

namespace Framework
{
    // Generates a pseudo-random GUID string in the format XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    // Uses std::random_device and std::mt19937 for randomness
    inline std::string GenerateGUID()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint16_t> dist16(0, 0xFFFF);

        std::stringstream ss;
        ss << std::hex << std::setfill('0')
            << std::setw(4) << dist16(gen)  // First 4 digits
            << std::setw(4) << dist16(gen) << "-"  // Second 4 digits
            << std::setw(4) << dist16(gen) << "-"  // Third group
            << std::setw(4) << ((dist16(gen) & 0x0FFF) | 0x4000) << "-"  // Fourth group (version 4 UUID)
            << std::setw(4) << ((dist16(gen) & 0x3FFF) | 0x8000) << "-"  // Fifth group (variant)
            << std::setw(4) << dist16(gen)
            << std::setw(4) << dist16(gen)
            << std::setw(4) << dist16(gen);  // Final 12 digits
        return ss.str();
    }

    // Represents a unique identifier for an asset
    struct AssetHandle
    {
        std::string id;  // GUID string

        AssetHandle() = default;
        AssetHandle(const std::string& identifier) : id(identifier) {}
        AssetHandle(const char* identifier) : id(identifier) {}

        AssetHandle(AssetHandle&&) noexcept = default;
        AssetHandle& operator=(AssetHandle&&) noexcept = default;

        AssetHandle(const AssetHandle&) = default;
        AssetHandle& operator=(const AssetHandle&) = default;
        
        // Equality comparison based on GUID string
        bool operator==(const AssetHandle& other) const { return id == other.id; }
        bool operator!=(const AssetHandle& other) const { return id != other.id; }

        // Allows AssetHandle to be used in boolean contexts (e.g., if (handle))
        operator bool() const { return !id.empty(); }
        bool empty() const { return id.empty(); }
    };
}

// Specialization of std::hash for AssetHandle
// Enables AssetHandle to be used as a key in unordered_map and other hash-based containers
namespace std
{
    template <>
    struct hash<Framework::AssetHandle>
    {
        std::size_t operator()(const Framework::AssetHandle& handle) const noexcept
        {
            return std::hash<std::string>()(handle.id);
        }
    };
}
#endif // !_ASSET_HANDLER_H_