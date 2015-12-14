#pragma once

#include "globals.h"

#include "infos.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        typedef common::event_traits<NetworkEventTypes> NetworkEventTraits;
        typedef NetworkEventTraits::event_t NetworkEvent;
        typedef NetworkEventTraits::listener_t NetworkEventListener;

        template<NetworkEventTypes event_t, typename inf_t = void>
        class NetworkEventBaseInfo
                : public common::Event<NetworkEventTypes, event_t>
        {
        public:
            typedef inf_t info_t;
            typedef NetworkEventBaseInfo<event_t, info_t> base_class_t;
            typedef typename common::Event<NetworkEventTypes, event_t>::senders_t senders_t;

            NetworkEventBaseInfo(senders_t* sender, info_t info)
                : common::Event<NetworkEventTypes, event_t>(sender), info_(info)
            {

            }

            info_t info() const
            {
                return info_;
            }

        private:
            const info_t info_;
        };

        template<NetworkEventTypes event_t>
        class NetworkEventBaseInfo<event_t, void>
                : public common::Event<NetworkEventTypes, event_t>
        {
        public:
            typedef void info_t;
            typedef NetworkEventBaseInfo<event_t, void> base_class_t;
            typedef typename common::Event<NetworkEventTypes, event_t>::senders_t senders_t;

            NetworkEventBaseInfo(senders_t* sender)
                : common::Event<NetworkEventTypes, event_t>(sender)
            {

            }
        };

        typedef NetworkEventBaseInfo<InnerClientConnected, UserAuthInfo> InnerClientConnectedEvent;
        typedef NetworkEventBaseInfo<InnerClientAutorized> InnerClientAutorizedEvent;
        typedef NetworkEventBaseInfo<InnerClientDisconnected> InnerClientDisconnectedEvent;
    }
}
