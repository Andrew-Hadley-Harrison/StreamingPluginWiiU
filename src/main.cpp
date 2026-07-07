#include <wups.h>
#include <nsysnet/_socket.h>
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

WUPS_USE_WUT_DEVOPTAB();

static void startStreaming() {
    DEBUG_FUNCTION_LINE("startStreaming: begin\n");
    EncodingHelper::destroyInstance();
    EncodingHelper::getInstance()->StartAsyncThread();
    EncodingHelper::getInstance()->setMJPEGStreamServer(HeartBeatServer::getInstance()->getMJPEGServer());
    DEBUG_FUNCTION_LINE("startStreaming: done, heartbeat server should be listening\n");
}

// Gets called once the loader exists.
INITIALIZE_PLUGIN() {
    socket_lib_init();
    log_init();
    DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN fired\n");
}

// Called whenever an application was started.
ON_APPLICATION_START() {
    socket_lib_init();
    log_init();
    DEBUG_FUNCTION_LINE("ON_APPLICATION_START fired\n");

    gAppStatus = APP_STATUS_FOREGROUND;
    startStreaming();
}

ON_ACQUIRED_FOREGROUND() {
    gAppStatus = APP_STATUS_FOREGROUND;
}

ON_RELEASE_FOREGROUND() {
    gAppStatus = APP_STATUS_BACKGROUND;
}

ON_APPLICATION_ENDS() {
    gAppStatus = APP_STATUS_BACKGROUND;
    EncodingHelper::destroyInstance();
    HeartBeatServer::destroyInstance();
}
