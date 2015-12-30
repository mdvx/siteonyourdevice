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
            else if(event->eventType() == InnerClientAutorizedEvent::EventType){
                //InnerClientAutorizedEvent * ev = static_cast<InnerClientAutorizedEvent*>(event);
            }
            else if(event->eventType() == InnerClientDisconnectedEvent::EventType){
                //InnerClientDisconnectedEvent * ev = static_cast<InnerClientDisconnectedEvent*>(event);
                controller_->disConnect();
            }
            else{

            }
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
                EVENT_BUS()->subscribe<InnerClientAutorizedEvent>(this);
            }

            ~NetworkListener()
            {
                EVENT_BUS()->unsubscribe<InnerClientAutorizedEvent>(this);
                EVENT_BUS()->unsubscribe<InnerClientDisconnectedEvent>(this);
                EVENT_BUS()->unsubscribe<InnerClientConnectedEvent>(this);
            }

            virtual void handleEvent(event_t* event)
            {
                app_->handleEvent(event);
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

        int NetworkEventHandler::start()
        {
            common::Error err = controller_->connect();
            if(err && err->isError()){
                DEBUG_MSG_ERROR(err);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }
    }
}
