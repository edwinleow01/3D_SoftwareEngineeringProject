///////////////////////////////////////////////////////////////////////////////
///
///	@file  : PropertyMacro.h
/// @Brief : 
///	
///	@Authors: Edwin Leow
///	@Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _PROPERTY_MACRO_H_
#define _PROPERTY_MACRO_H_

#include "PropertySystem.h"

//------------------------------------------------------------------------------------------------------------------------------------
//  Property Starting MACRO
//------------------------------------------------------------------------------------------------------------------------------------
// HOW TO USE (dont know why if i use this, intellisense give me thousand errors)
/*
PROPERTY_BEGIN(WindowConfig, WindowConfigProps)
    PROP_STRING(guid, WindowConfig),
    PROP_INT(x, WindowConfig),
    PROP_INT(y, WindowConfig),
    PROP_STRING(programName, WindowConfig)
PROPERTY_END(WindowConfig, WindowConfigProps)
*/
#define PROPERTY_BEGIN(TypeName, PropArrayName) \
    namespace { \
    static const Framework::prop PropArrayName[] = {

#define PROPERTY_END(TypeName, PropArrayName) \
    }; \
    static const Framework::prop_list TypeName##_PropList = { \
        #TypeName, std::span<const Framework::prop>(PropArrayName, sizeof(PropArrayName) / sizeof(Framework::prop)) \
    }; \
    static Framework::global_registration TypeName##_Registration(&TypeName##_PropList); \
    namespace Framework { \
        template<> inline const prop_list& GetPropList<TypeName>() { return TypeName##_PropList; } \
    }

//------------------------------------------------------------------------------------------------------------------------------------
//  INT MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_INT(field, type) \
    PROP_INT_FLAGS(field, type, Framework::prop::NONE)

#define PROP_INT_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::INT, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const int*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<int*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_INT_READONLY(field, type) \
    PROP_INT_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_INT_HIDDEN(field, type) \
    PROP_INT_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  FLOAT MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_FLOAT(field, type) \
    PROP_FLOAT_FLAGS(field, type, Framework::prop::NONE)

#define PROP_FLOAT_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::FLOAT, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const float*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<float*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_FLOAT_READONLY(field, type) \
    PROP_FLOAT_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_FLOAT_HIDDEN(field, type) \
    PROP_FLOAT_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  DOUBLE MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_DOUBLE(field, type) \
    PROP_DOUBLE_FLAGS(field, type, Framework::prop::NONE)

#define PROP_DOUBLE_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::DOUBLE, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const double*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<double*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_DOUBLE_READONLY(field, type) \
    PROP_DOUBLE_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_DOUBLE_HIDDEN(field, type) \
    PROP_DOUBLE_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  LONG MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_LONG(field, type) \
    Framework::prop{ #field, Framework::prop::LONG, \
        [](void* pObj, const void* pData) { assert(pObj && pData); static_cast<type*>(pObj)->field = *static_cast<const long*>(pData); }, \
        [](const void* pObj, void* pData) { assert(pObj && pData); *static_cast<long*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, Framework::prop::NONE, {} }

//------------------------------------------------------------------------------------------------------------------------------------
//  SHORT MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_SHORT(field, type) \
    Framework::prop{ #field, Framework::prop::SHORT, \
        [](void* pObj, const void* pData) { assert(pObj && pData); static_cast<type*>(pObj)->field = *static_cast<const short*>(pData); }, \
        [](const void* pObj, void* pData) { assert(pObj && pData); *static_cast<short*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, Framework::prop::NONE, {} }

//------------------------------------------------------------------------------------------------------------------------------------
//  STRING MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_STRING(field, type) \
    PROP_STRING_FLAGS(field, type, Framework::prop::NONE)

#define PROP_STRING_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::STRING, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const std::string*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<std::string*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_STRING_READONLY(field, type) \
    PROP_STRING_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_STRING_HIDDEN(field, type) \
    PROP_STRING_FLAGS(field, type, Framework::prop::HIDDEN)

#define PROP_STRING_DROPDOWN(field, type, optionsFn) \
    Framework::prop{ #field, Framework::prop::STRING_DROPDOWN, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const std::string*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<std::string*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, Framework::prop::NONE, optionsFn }

#define PROP_STRING_CHAR(field, type) \
    Framework::prop{ #field, Framework::prop::STRING, \
        [](void* pObj, const void* pData) { \
            std::strncpy(static_cast<type*>(pObj)->field, static_cast<const char*>(pData), sizeof(static_cast<type*>(pObj)->field)); \
            static_cast<type*>(pObj)->field[sizeof(static_cast<type*>(pObj)->field) - 1] = '\0'; \
        }, \
        [](const void* pObj, void* pData) { \
            std::strncpy(static_cast<char*>(pData), static_cast<const type*>(pObj)->field, sizeof(static_cast<const type*>(pObj)->field)); \
            static_cast<char*>(pData)[sizeof(static_cast<const type*>(pObj)->field) - 1] = '\0'; \
        }, \
        {}, {}, false, nullptr, nullptr, 0, Framework::prop::NONE, {} }

//------------------------------------------------------------------------------------------------------------------------------------
//  CHAR MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_CHAR(field, type) \
    PROP_CHAR_FLAGS(field, type, Framework::prop::NONE)

#define PROP_CHAR_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::CHAR, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const char*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<char*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_CHAR_READONLY(field, type) \
    PROP_CHAR_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_CHAR_HIDDEN(field, type) \
    PROP_CHAR_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  BOOL MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_BOOL(field, type) \
    PROP_BOOL_FLAGS(field, type, Framework::prop::NONE)

#define PROP_BOOL_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::BOOL, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const bool*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<bool*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_BOOL_READONLY(field, type) \
    PROP_BOOL_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_BOOL_HIDDEN(field, type) \
    PROP_BOOL_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  UNSIGNED INT MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_UINT(field, type) \
    PROP_UINT_FLAGS(field, type, Framework::prop::NONE)

#define PROP_UINT_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::UINT, \
        [](void* pObj, const void* pData) { static_cast<type*>(pObj)->field = *static_cast<const unsigned int*>(pData); }, \
        [](const void* pObj, void* pData) { *static_cast<unsigned int*>(pData) = static_cast<const type*>(pObj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_UINT_READONLY(field, type) \
    PROP_UINT_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_UINT_HIDDEN(field, type) \
    PROP_UINT_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  VEC3 MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_VEC3(field, type) \
    Framework::prop{ #field, Framework::prop::VEC3, \
        [](void* pObj, const void* pData) { \
            assert(pObj && pData); \
            static_cast<type*>(pObj)->field = *static_cast<const glm::vec3*>(pData); \
        }, \
        [](const void* pObj, void* pData) { \
            assert(pObj && pData); \
            *static_cast<glm::vec3*>(pData) = static_cast<const type*>(pObj)->field; \
        } \
    }

//------------------------------------------------------------------------------------------------------------------------------------
//  VEC2 MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_VEC2(field, type) \
    Framework::prop{ #field, Framework::prop::VEC2, \
        [](void* pObj, const void* pData) { \
            assert(pObj && pData); \
            static_cast<type*>(pObj)->field = *static_cast<const glm::vec2*>(pData); \
        }, \
        [](const void* pObj, void* pData) { \
            assert(pObj && pData); \
            *static_cast<glm::vec2*>(pData) = static_cast<const type*>(pObj)->field; \
        } \
    }

//------------------------------------------------------------------------------------------------------------------------------------
//  GUID MACRO
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_GUID_FLAGS(field, type, flags) \
    Framework::prop{ #field, Framework::prop::STRING, \
        [](void* pObj, const void* pData) { \
            static_cast<type*>(pObj)->field = Framework::AssetHandle(*static_cast<const std::string*>(pData)); \
        }, \
        [](const void* pObj, void* pData) { \
            *static_cast<std::string*>(pData) = static_cast<const type*>(pObj)->field.id; \
        }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {} }

#define PROP_GUID_READONLY(field, type) \
    PROP_GUID_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_GUID_HIDDEN(field, type) \
    PROP_GUID_FLAGS(field, type, Framework::prop::HIDDEN)

//------------------------------------------------------------------------------------------------------------------------------------
//  TYPENAME MACRO
//------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
struct TypeName;

#define REGISTER_TYPE_NAME(Type) \
    template<> struct TypeName<Type> { static constexpr const char* value = #Type; }

//------------------------------------------------------------------------------------------------------------------------------------
//  WIDGET || OTHER MACROS
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_DRAG_FLOAT_FLAGS(field, type, flags) \
    Framework::prop(#field, Framework::prop::DRAG_FLOAT, \
        [](void* obj, const void* in) { static_cast<type*>(obj)->field = *static_cast<const float*>(in); }, \
        [](const void* obj, void* out) { *static_cast<float*>(out) = static_cast<const type*>(obj)->field; }, \
        {}, {}, false, nullptr, nullptr, 0, flags, {})

#define PROP_DRAG_FLOAT(field, type) \
    PROP_DRAG_FLOAT_FLAGS(field, type, Framework::prop::NONE)

#define PROP_DRAG_FLOAT_HIDDEN(field, type) \
    PROP_DRAG_FLOAT_FLAGS(field, type, Framework::prop::HIDDEN)

#define PROP_DRAG_FLOAT_READONLY(field, type) \
    PROP_DRAG_FLOAT_FLAGS(field, type, Framework::prop::READ_ONLY)

#define PROP_COLOR3(field, type) \
    Framework::prop(#field, Framework::prop::COLOR3, \
        [](void* obj, const void* in) { static_cast<type*>(obj)->field = *static_cast<const glm::vec3*>(in); }, \
        [](const void* obj, void* out) { *static_cast<glm::vec3*>(out) = static_cast<const type*>(obj)->field; })

#define PROP_SLIDER_FLOAT(field, type, min, max) \
    Framework::prop(#field, Framework::prop::SLIDER_FLOAT, \
        [](void* obj, const void* in) { static_cast<type*>(obj)->field = *static_cast<const float*>(in); }, \
        [](const void* obj, void* out) { *static_cast<float*>(out) = static_cast<const type*>(obj)->field; }, \
        min, max, true)

//------------------------------------------------------------------------------------------------------------------------------------
//  SCRIPT MACROS
//------------------------------------------------------------------------------------------------------------------------------------
#define PROP_VECTOR_OBJECT(field, ElementType, ElementProps)           \
    Framework::prop{                                                         \
        #field,                                                        \
        Framework::prop::VECTOR_OBJECT,                                        \
        offsetof(Framework::ScriptProperties, field),                  \
        0, ElementProps, sizeof(ElementType)                           \
    }


#endif // !_PROPERTY_MACRO_H_