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

        template<NetworkEventTypes event_t, typename inf_t>
        class NetworkEventBaseInfo
                : public common::Event<NetworkEventTypes, event_t>
        {
        public:
            typedef inf_t info_t;
            typedef common::Event<NetworkEventTypes, event_t> base_class_t;
            typedef typename base_class_t::senders_t senders_t;

            NetworkEventBaseInfo(senders_t* sender, info_t info)
                : base_class_t(sender), info_(info)
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
            typedef common::Event<NetworkEventTypes, event_t> base_class_t;
            typedef typename base_class_t::senders_t senders_t;

            NetworkEventBaseInfo(senders_t* sender)
                : base_class_t(sender)
            {
            }
        };

        typedef NetworkEventBaseInfo<InnerClientConnected, void> InnerClientConnectedEvent;
        typedef NetworkEventBaseInfo<InnerClientAutorized, UserAuthInfo> InnerClientAutorizedEvent;
        typedef NetworkEventBaseInfo<InnerClientDisconnected, void> InnerClientDisconnectedEvent;
        typedef common::IExceptionEvent<NetworkEventTypes> NetworkExceptionEvent;
    }
}
