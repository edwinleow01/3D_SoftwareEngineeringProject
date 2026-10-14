#pragma once
#ifndef _EVENT_RECEIVER_COMPONENT_H_
#define _EVENT_RECEIVER_COMPONENT_H_

#include <string>
#include <vector>

#include "PropertySystem.h"
#include "PropertyMacro.h"

namespace Framework
{
    // ------------------ Single Receiver Entry ------------------
    struct EventReceiverEntry
    {
        std::string messageToListenFor;
        std::string luaFunctionToCall;

        // Runtime state (hidden)
        bool isSubscribed = false;
    };

    inline const std::vector g_EventReceiverEntryPropsArray =
    {
        PROP_STRING(messageToListenFor, EventReceiverEntry),
        PROP_STRING(luaFunctionToCall,  EventReceiverEntry),

        // We hide the runtime state
        PROP_BOOL_HIDDEN(isSubscribed,  EventReceiverEntry)
    };

    inline const prop_list g_EventReceiverEntryProps{ "EventReceiverEntry", g_EventReceiverEntryPropsArray };
    inline global_registration g_EventReceiverEntryPropsReg(&g_EventReceiverEntryProps);


    // ------------------ Component for Entity ------------------
    struct EventReceiverComponent
    {
        std::vector<EventReceiverEntry> receivers;

        // Helper for reflection since we can't reflect vector directly
        int receiverCount = 0;
    };

    inline const std::vector g_EventReceiverPropsArray =
    {
        // We reflect this hidden int just so the Property System knows the Component exists
        PROP_INT_HIDDEN(receiverCount, EventReceiverComponent)
    };

    inline const prop_list g_EventReceiverProps{ "EventReceiverComponent", g_EventReceiverPropsArray };
    inline global_registration g_EventReceiverPropsReg(&g_EventReceiverProps);

} // namespace Framework

#endif // _EVENT_RECEIVER_COMPONENT_H_