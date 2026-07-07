#include <wups.h>
#include <gx2/surface.h>
#include <utils/StringTools.h>
#include <fs/FSUtils.h>
#include "stream_utils.h"
#include "retain_vars.hpp"

uint32_t count = 0;

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, const GX2ColorBuffer *colorBuffer, int32_t scan_target) {
    if(gAppStatus == APP_STATUS_FOREGROUND) {
        int32_t use_scan_target = 4;
        if(gScreen == WUPS_STREAMING_SCREEN_TV){
            use_scan_target = 1;
        }
        // Throttle: only stream every 3rd frame. Streaming every frame floods
        // the Wii U's WiFi/socket buffers -> UDP packet loss -> CRC mismatch on
        // the client -> black screen. (count only advances on matching frames.)
        if(scan_target == use_scan_target  &&  (count++ % 3 == 0)  && colorBuffer != NULL ) {
            streamVideo((GX2ColorBuffer *)colorBuffer);
        }
    }
    real_GX2CopyColorBufferToScanBuffer(colorBuffer,scan_target);
}

WUPS_MUST_REPLACE(GX2CopyColorBufferToScanBuffer,   WUPS_LOADER_LIBRARY_GX2,    GX2CopyColorBufferToScanBuffer);
