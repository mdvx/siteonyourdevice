#include "application/fasto_application.h"

// [-c] config path [-d] run as daemon

int main(int argc, char *argv[])
{
    using namespace fasto::siteonyourdevice;

    application::FastoApplication application(argc, argv);
    int res = application.exec();
    return res;
}
