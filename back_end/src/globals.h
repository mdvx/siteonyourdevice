#pragma once

#include "common/event.h"

#define NETWORK_EVENT_LOOP_ID 1

enum NetworkEventTypes
{
    InnerClientConnected = 0,
    InnerClientDisconnected,

    CountNetworkEvent
};

namespace common
{
    template<>
    struct event_traits<NetworkEventTypes>
    {
        typedef IEvent<NetworkEventTypes> event_t;
        typedef IExceptionEvent<NetworkEventTypes> ex_event_t;
        typedef IListener<NetworkEventTypes> listener_t;
        static const unsigned max_count = CountNetworkEvent;
        static const unsigned id = NETWORK_EVENT_LOOP_ID;
    };
}
