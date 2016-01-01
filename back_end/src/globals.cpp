#include "globals.h"

#include "common/convert2string.h"

namespace common
{
    std::string convertToString(NetworkEventTypes net)
    {
        return SNetworkEventTypes[net];
    }

    template<>
    NetworkEventTypes convertFromString(const std::string& netStr)
    {
        for (size_t i = 0; i < SIZEOFMASS(SNetworkEventTypes); ++i){
            if (netStr == SNetworkEventTypes[i]){
                return static_cast<NetworkEventTypes>(i);
            }
        }

        NOTREACHED();
        return CountNetworkEvent;
    }
}
