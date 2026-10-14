#pragma once
#ifndef _SCRIPT_PROPERTIES_H_
#define _SCRIPT_PROPERTIES_H_

#include <string>
#include <vector>

#include "PropertySystem.h"
#include "PropertyMacro.h"

namespace Framework
{
    // ------------------ Per-script entry ------------------
    struct ScriptEntry
    {
        AssetHandle guid;
        std::string scriptPath;   // which .lua file
        bool        hotReload = true;

        // runtime fields (not serialized, internal use)
        bool        initialized = false;
        uint64_t    lastWriteTicks = 0;   // we will NOT reflect this
        int         tableRef = -1;
    };

    // Properties for ScriptEntry (only what we want reflected/serialized)
    inline const std::array g_ScriptEntryPropsArray =
    {
        PROP_GUID_HIDDEN(guid, ScriptEntry),
        PROP_STRING(scriptPath, ScriptEntry),
        PROP_BOOL(hotReload,    ScriptEntry),

        // Hidden runtime fields (editor & JSON ignore them)
        PROP_BOOL_HIDDEN(initialized, ScriptEntry),
        PROP_INT_HIDDEN(tableRef,    ScriptEntry),
        // lastWriteTicks is completely unreflected on purpose
    };

    inline const prop_list g_ScriptEntryProps{ "ScriptEntry", g_ScriptEntryPropsArray };
    inline global_registration g_ScriptEntryPropsReg(&g_ScriptEntryProps);


    // ------------------ Component for an entity ------------------
    struct ScriptProperties
    {
        std::vector<ScriptEntry> scripts;

        int scriptCount = 0;
    };

    // Properties for ScriptProperties itself
    // Note: we CANNOT directly expose the vector<ScriptEntry> with the current
    // property system, so we just add a hidden helper int.
    inline const std::array g_ScriptPropertiesPropsArray =
    {
        PROP_INT_HIDDEN(scriptCount, ScriptProperties),
    };

    inline const prop_list g_ScriptPropertiesProps{ "ScriptProperties", g_ScriptPropertiesPropsArray };
    inline global_registration g_ScriptPropertiesPropsReg(&g_ScriptPropertiesProps);

} // namespace Framework

#endif // _SCRIPT_PROPERTIES_H_
