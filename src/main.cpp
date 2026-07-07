#include <wups.h>
#include <nsysnet/_socket.h>
#include <coreinit/title.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <thread>
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

// Returns true for the Wii U Menu and other system applications
// (Settings, Mii Maker, etc.) which all live under the 0x00050010 range.
// Running our socket server + capture threads during these hangs the console
// at boot, and they can't be meaningfully streamed anyway.
static bool isSystemMenuTitle() {
    return (uint32_t)(OSGetTitleID() >> 32) == 0x00050010;
}

// Gets called once the loader exists. Keep this minimal - all real setup is
// deferred to ON_APPLICATION_START so nothing heavy runs at plugin load time.
INITIALIZE_PLUGIN() {
    DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN fired\n");
}

// Called whenever an application was started.
ON_APPLICATION_START() {
    DEBUG_FUNCTION_LINE("ON_APPLICATION_START fired\n");

    gAppStatus = APP_STATUS_FOREGROUND;

    if (isSystemMenuTitle()) {
        // Starting sockets/threads synchronously during the Wii U Menu's early
        // boot hangs the console. Defer it to a detached thread so the boot
        // path never blocks: worst case the Menu stream just doesn't start,
        // but the console still boots. Re-check the title after the delay in
        // case the user already launched a game.
        std::thread([]() {
            OSSleepTicks(OSSecondsToTicks(3));
            if (!isSystemMenuTitle()) {
                return; // left the Menu already; the game path handles startup
            }
            socket_lib_init();
            startStreaming();
        }).detach();
        return;
    }

    socket_lib_init();
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
