#include "network/network_event_handler.h"

#include "common/thread/event_bus.h"
#include "common/logger.h"

#include "network/network_controller.h"

namespace fasto
{
    namespace siteonyourdevice
    {
        void NetworkEventHandler::handleEvent(NetworkEvent *event)
        {
            if(event->eventType() == InnerClientConnectedEvent::EventType){
                //InnerClientConnectedEvent * ev = static_cast<InnerClientConnectedEvent*>(event);
            }
            else if(event->eventType() == InnerClientDisconnectedEvent::EventType){
                //InnerClientDisconnectedEvent * ev = static_cast<InnerClientDisconnectedEvent*>(event);
                controller_->disConnect();
            }
            else{

            }
        }

        void NetworkEventHandler::handleExceptionEvent(NetworkEvent* event, common::Error err)
        {
            DEBUG_MSG_FORMAT<512>(common::logging::L_WARNING, "Exception event type: %d, text: %s", (int)(event->eventType()), err->description());
        }

        class NetworkEventHandler::NetworkListener
                : public common::IListener<NetworkEventTypes>
        {
            NetworkEventHandler * const app_;
        public:
            NetworkListener(NetworkEventHandler * app)
                : common::IListener<NetworkEventTypes>(), app_(app)
            {
                EVENT_BUS()->subscribe<InnerClientConnectedEvent>(this);
                EVENT_BUS()->subscribe<InnerClientDisconnectedEvent>(this);
            }

            ~NetworkListener()
            {
                EVENT_BUS()->unsubscribe<InnerClientDisconnectedEvent>(this);
                EVENT_BUS()->unsubscribe<InnerClientConnectedEvent>(this);
            }

            virtual void handleEvent(event_t* event)
            {
                app_->handleEvent(event);
            }

            virtual void handleExceptionEvent(event_t* event, common::Error err)
            {
                app_->handleExceptionEvent(event, err);
            }
        };

        NetworkEventHandler::NetworkEventHandler(NetworkController *controller)
            : controller_(controller)
        {
            networkListener_ = new NetworkListener(this);
        }

        NetworkEventHandler::~NetworkEventHandler()
        {
            delete networkListener_;
        }

        void NetworkEventHandler::start()
        {
            controller_->connect();
        }
    }
}
