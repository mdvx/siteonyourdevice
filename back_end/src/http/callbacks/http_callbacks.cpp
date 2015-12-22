#include "http/callbacks/http_callbacks.h"

#include "http/callbacks/file_system_callback.h"
#include "http/callbacks/system_callback.h"

namespace common
{
    std::string convertToString(fasto::siteonyourdevice::HCTypes t)
    {
        return fasto::siteonyourdevice::HCallbackTypes[t];
    }

    template<>
    fasto::siteonyourdevice::HCTypes convertFromString(const std::string& text)
    {
        for (uint32_t i = 0; i < SIZEOFMASS(fasto::siteonyourdevice::HCallbackTypes); ++i){
            if (text == fasto::siteonyourdevice::HCallbackTypes[i]){
                return static_cast<fasto::siteonyourdevice::HCTypes>(i);
            }
        }

        NOTREACHED();
        return fasto::siteonyourdevice::file_system;
    }
}

namespace fasto
{
    namespace siteonyourdevice
    {
        IHttpCallback::IHttpCallback(HCTypes type)
            : type_(type)
        {

        }

        IHttpCallback::~IHttpCallback()
        {

        }

        HCTypes IHttpCallback::type() const
        {
            return type_;
        }

        common::shared_ptr<IHttpCallback> IHttpCallback::createHttpCallback(HCTypes type)
        {
            if(type == file_system){
                return common::shared_ptr<IHttpCallback>(new HttpFileSystemCallback);
            }
            else if(type == system){
                return common::shared_ptr<IHttpCallback>(new HttpSystemCallback);
            }
            else{
                DNOTREACHED();
                return common::shared_ptr<IHttpCallback>();
            }
        }

        common::shared_ptr<IHttpCallback> IHttpCallback::createHttpCallback(const std::string& ns_name, const std::string &name)
        {
            HCTypes htype = common::convertFromString<HCTypes>(ns_name);
            if(htype == file_system){
                return createFileSystemHttpCallback(name);
            }
            else if(htype == system){
                return createSystemHttpCallback(name);
            }
            else{
                DNOTREACHED();
                return common::shared_ptr<IHttpCallback>();
            }
        }
    }
}
