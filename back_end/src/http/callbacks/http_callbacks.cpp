#include "http/callbacks/http_callbacks.h"

#include "http/callbacks/file_system_callback.h"
#include "http/callbacks/system_callback.h"

namespace common
{
    std::string convertToString(fasto::fastoremote::HCTypes t)
    {
        return fasto::fastoremote::HCallbackTypes[t];
    }

    template<>
    fasto::fastoremote::HCTypes convertFromString(const std::string& text)
    {
        for (uint32_t i = 0; i < SIZEOFMASS(fasto::fastoremote::HCallbackTypes); ++i){
            if (text == fasto::fastoremote::HCallbackTypes[i]){
                return static_cast<fasto::fastoremote::HCTypes>(i);
            }
        }

        NOTREACHED();
        return fasto::fastoremote::file_system;
    }
}

namespace fasto
{
    namespace fastoremote
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
            if(type == system){
                return common::shared_ptr<IHttpCallback>(new HttpSystemCallback);
            }
            else if(type == file_system){
                return common::shared_ptr<IHttpCallback>(new HttpFileSystemCallback);
            }
            else{
                return NULL;
            }
        }

        common::shared_ptr<IHttpCallback> IHttpCallback::createHttpCallback(const std::string& name)
        {
            HCTypes t = common::convertFromString<HCTypes>(name);
            return createHttpCallback(t);
        }
    }
}
