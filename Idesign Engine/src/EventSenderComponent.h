#pragma once
#ifndef _EVENT_SENDER_COMPONENT_H_
#define _EVENT_SENDER_COMPONENT_H_

#include <string>
#include <vector>

#include "PropertySystem.h"
#include "PropertyMacro.h"

namespace Framework
{
    // ------------------ Single Sender Entry ------------------
    struct EventSenderEntry
    {
        std::string triggerName;
        std::string messageName;
    };

    // Properties for EventSenderEntry
    inline const std::vector g_EventSenderEntryPropsArray =
    {
        PROP_STRING(triggerName, EventSenderEntry),
        PROP_STRING(messageName, EventSenderEntry)
    };

    inline const prop_list g_EventSenderEntryProps{ "EventSenderEntry", g_EventSenderEntryPropsArray };
    inline global_registration g_EventSenderEntryPropsReg(&g_EventSenderEntryProps);


    // ------------------ Component for Entity ------------------
    struct EventSenderComponent
    {
        std::vector<EventSenderEntry> senders;
        // Helper for reflection since we can't reflect vector directly
        int senderCount = 0;
    };

    inline const std::vector g_EventSenderPropsArray =
    {
        // We reflect this hidden int just so the Property System knows the Component exists
        PROP_INT_HIDDEN(senderCount, EventSenderComponent)
    };

    inline const prop_list g_EventSenderProps{ "EventSenderComponent", g_EventSenderPropsArray };
    inline global_registration g_EventSenderPropsReg(&g_EventSenderProps);

} // namespace Framework

#endif // _EVENT_SENDER_COMPONENT_H_