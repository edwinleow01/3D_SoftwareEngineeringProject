///////////////////////////////////////////////////////////////////////////////
///
///	@file  : PropertySystem.h
/// @Brief : 
///	
///	@Authors: Edwin Leow
///	@Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _PROPERTY_SYSTEM_H_
#define _PROPERTY_SYSTEM_H_

#include <iostream>
#include <vector>
#include <cassert>
#include <span>
#include <array>
#include <variant>
#include <cstring> // for std::strncpy
#include <functional>

#include "AssetHandler.h"   // For GUIDs

namespace Framework
{
    // A single property descriptor (like "field metadata")
    struct prop
    {
        // The kinds of primitive types we support right now
        enum atomic_type
        {
            INT,
            FLOAT,
            DOUBLE,
            LONG,
            SHORT,
            STRING,
            CHAR,
            BOOL,
            VEC3, VEC2,
            UINT,
            OBJECT,         // a nested struct described by a prop_list
            VECTOR_OBJECT,

            SLIDER_FLOAT,   // Volume, Brightness
            DRAG_FLOAT,     // Scale, Physics, Tuning
            COLOR3          // Color
        };

        // Function pointers to *set* and *get* a value on an object.
        // Lambdas to read/write a field of an object
        using fn_set = void(*)(void* pObject, const void* pData);
        using fn_get = void(*)(const void* pObject, void* pData);

        const char* m_pName = nullptr;   // Property name ("x", "y", etc.)
        atomic_type m_Type;              // What kind of data is this (int, float, …)?

        // To interact with the fields
        fn_set      m_fnSet = nullptr;   // Setter function for this property, to write a new value
        fn_get      m_fnGet = nullptr;   // Getter function for this property, to read the value

        // Type-safe range metadata
        using RangeValue = std::variant<int, float, double, unsigned int>;
        RangeValue m_Min;
        RangeValue m_Max;
        bool m_HasRange = false;
        const char* m_pTooltip = nullptr;

        // Enum support
        struct EnumEntry 
        {
            const char* name;
            int value;
        };
        const EnumEntry* m_pEnumEntries = nullptr;
        size_t m_EnumCount = 0;

        // Property flags
        enum prop_flags 
        {
            NONE = 0,
            READ_ONLY = 1 << 0,
            HIDDEN = 1 << 1,
            ADVANCED = 1 << 2,
        };
        uint32_t m_Flags = prop_flags::NONE;

        // Helper: get min/max as float (for ImGui sliders)
        float GetMinAsFloat() const 
        {
            return std::visit([](auto&& val) -> float { return static_cast<float>(val); }, m_Min);
        }

        float GetMaxAsFloat() const 
        {
            return std::visit([](auto&& val) -> float { return static_cast<float>(val); }, m_Max);
        }

        // For Dropdown 
        std::function<std::vector<std::string>()> GetDropdownOptions;

        prop(const char* name, atomic_type type,
            fn_set setter, fn_get getter,
            RangeValue min = {}, RangeValue max = {},
            bool hasRange = false, const char* tooltip = nullptr,
            const EnumEntry* enumEntries = nullptr, size_t enumCount = 0,
            uint32_t flags = NONE,
            std::function<std::vector<std::string>()> dropdownOptions = {})
            : m_pName(name), m_Type(type),
            m_fnSet(setter), m_fnGet(getter),
            m_Min(min), m_Max(max), m_HasRange(hasRange),
            m_pTooltip(tooltip),
            m_pEnumEntries(enumEntries), m_EnumCount(enumCount),
            m_Flags(flags),
            GetDropdownOptions(dropdownOptions){}
    };

    // Collection of Properties for one type
    struct prop_list
    {
        const char* m_pName;                // Type name
        std::span<const prop> m_Props;      // Span of all its properties
    };

    // Global registry of all reflected types
    // So you can iterate over *all* types at runtime without knowing them at compile time
    struct global_registration
    {
        global_registration* m_pNext;       // Linked-list pointer
        const prop_list* m_pPopList;        // The type's property list

        // Constructor registers this type into the global linked list
        global_registration(const prop_list* pPropList)
        {
            assert(pPropList && "global_registration: prop_list is null");
            assert(pPropList->m_pName && "global_registration: type name is null");
            assert(pPropList->m_Props.size() > 0 && "global_registration: property list is empty");

            m_pPopList = pPropList;
            m_pNext = m_pHead;
            m_pHead = this;
        }
        // Global static head of linked list
        inline static global_registration* m_pHead = nullptr;
    };

    inline const prop_list* FindPropListByName(const std::string& typeName)
    {
        global_registration* current = global_registration::m_pHead;
        while (current)
        {
            if (current->m_pPopList && std::strcmp(current->m_pPopList->m_pName, typeName.c_str()) == 0)
                return current->m_pPopList;

            current = current->m_pNext;
        }
        return nullptr;
    }

    template<typename T>
    const prop_list& GetPropList();
}
#endif // !_PROPERTY_SYSTEM_H_