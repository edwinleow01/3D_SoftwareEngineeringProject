///////////////////////////////////////////////////////////////////////////////
///
///	@file  : WindowConfigProperties.h
/// @Brief : 
///	
///	@Authors: Edwin Leow
///	@Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _WINDOW_CONFIG_PROPERTIES_H_
#define _WINDOW_CONFIG_PROPERTIES_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"

namespace Framework
{
    struct WindowConfig
    {
        AssetHandle guid;
        int x = 0;
        int y = 0;
        std::string programName;
    };

    inline const std::vector g_WindowConfigPropsArray =
    {
        PROP_GUID_READONLY(guid, WindowConfig),
        PROP_INT(x, WindowConfig),
        PROP_INT(y, WindowConfig),
        PROP_STRING(programName, WindowConfig),
    };

    inline const prop_list g_WindowConfigProps{ "WindowConfig", g_WindowConfigPropsArray };
    inline global_registration g_WindowConfigReg(&g_WindowConfigProps);
}
#endif // !_WINDOW_CONFIG_PROPERTIES_H_