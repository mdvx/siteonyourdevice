#include "application/fasto_application.h"

// [-c] config path [-d] run as daemon

int main(int argc, char *argv[])
{
    fasto::FastoApplication application(argc, argv);
    int res = application.exec();
    return res;
}
