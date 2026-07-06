#include <map>
#include <string>
#include <wups.h>
#include <utils/logger.h>
#include "retain_vars.hpp"
#include "EncodingHelper.h"
#include "MJPEGStreamServerUDP.hpp"
#include "HeartBeatServer.hpp"

// Mandatory plugin information.
WUPS_PLUGIN_NAME("Wii U Screen Streaming");
WUPS_PLUGIN_DESCRIPTION("Streams the Wii U screen to a PC on the same network (use StreamingPluginClient).");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("Maschell, ItsOmey");
WUPS_PLUGIN_LICENSE("GPL");

// Something is using "write"...
WUPS_FS_ACCESS()

// Gets called once the loader exists.
INITIALIZE_PLUGIN() {
    socket_lib_init();
    log_init();
}

// Called whenever an application was started.
ON_APPLICATION_START(my_args) {
    socket_lib_init();
    log_init();

    gAppStatus = WUPS_APP_STATUS_FOREGROUND;

    EncodingHelper::destroyInstance();
    EncodingHelper::getInstance()->StartAsyncThread();
    EncodingHelper::getInstance()->setMJPEGStreamServer(HeartBeatServer::getInstance()->getMJPEGServer());
}

ON_APP_STATUS_CHANGED(status) {
    gAppStatus = status;

    if (status == WUPS_APP_STATUS_CLOSED) {
        EncodingHelper::destroyInstance();
        HeartBeatServer::destroyInstance();
    }
}
