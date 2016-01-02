#include "server/relay_server.h"

#include "common/logger.h"

#include "tcp/tcp_client.h"

#define BUF_SIZE 4096

namespace fasto
{
    namespace siteonyourdevice
    {
        namespace server
        {
            RelayHandler::RelayHandler()
                : client_primary_(NULL), client_secondary_(NULL)
            {

            }

            void RelayHandler::preLooped(tcp::ITcpLoop* server)
            {
            }

            void RelayHandler::accepted(tcp::TcpClient* client)
            {
            }

            void RelayHandler::moved(tcp::TcpClient* client)
            {

            }

            void RelayHandler::closed(tcp::TcpClient* client)
            {
                if(client == client_primary_){
                    client_primary_ = NULL;
                    return;
                }

                if(client == client_secondary_){
                    client_secondary_ = NULL;
                    return;
                }
            }

            void RelayHandler::timerEmited(tcp::ITcpLoop* server, timer_id_type id)
            {

            }

            void RelayHandler::dataReceived(tcp::TcpClient* client)
            {
                char buff[BUF_SIZE] = {0};
                ssize_t nread = 0;
                common::Error err = client->read(buff, BUF_SIZE, nread);
                if((err && err->isError()) || nread == 0){
                    client->close();
                    delete client;
                    return;
                }

                ssize_t nwrite = 0;
                if(client == client_primary_){
                    if(client_secondary_){
                        err = client_secondary_->write(buff, nread, nwrite);
                        if(err){
                            DEBUG_MSG_ERROR(err);
                        }
                    }
                }
                else if(client == client_secondary_){
                    if(client_primary_){
                        err = client_primary_->write(buff, nread, nwrite);
                        if(err){
                            DEBUG_MSG_ERROR(err);
                        }
                    }
                }
                else{
                    NOTREACHED();
                }
            }

            void RelayHandler::dataReadyToWrite(tcp::TcpClient* client)
            {

            }

            void RelayHandler::postLooped(tcp::ITcpLoop* server)
            {

            }
        }
    }
}
