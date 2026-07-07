/****************************************************************************
 * Copyright (C) 2016,2017 Maschell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "MJPEGStreamServerUDP.hpp"
#include <stdio.h>
#include <string.h>

#include "retain_vars.hpp"
#include <coreinit/thread.h>
#include <utils/logger.h>
#include <coreinit/cache.h>
#include <nsysnet/socket.h>
#include "crc32.h"

#define MAX_UDP_SIZE 0x578

extern int frame_counter_skipped;

MJPEGStreamServerUDP::MJPEGStreamServerUDP(uint32_t ip, int32_t port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        return;
    }

    // Allow single UDP datagrams up to a full frame in size.
    int sndbuf = 0x40000; // 256 KB
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    struct sockaddr_in connect_addr;
    memset(&connect_addr, 0, sizeof(struct sockaddr_in));
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = port;
    connect_addr.sin_addr.s_addr = ip;

    if(connect(sockfd, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) < 0) {
        socketclose(sockfd);
        sockfd = -1;
    }

    crc32_init(&crc32Buffer);

    OSInitMessageQueue(&dataQueue, dataQueueMessages, DATA_SEND_QUEUE_MESSAGE_COUNT);

    //StartAsyncThread();
}

void MJPEGStreamServerUDP::StartAsyncThread() {
    int32_t priority = 31;
    this->pThread = CThread::create(DoAsyncThread, this, CThread::eAttributeAffCore0 |CThread::eAttributeAffCore2, priority,0x80000);
    this->pThread->resumeThread();
}

void MJPEGStreamServerUDP::DoAsyncThread(CThread *thread, void *arg) {
    MJPEGStreamServerUDP * arg_instance = (MJPEGStreamServerUDP *) arg;
    return arg_instance->DoAsyncThreadInternal(thread);
}

MJPEGStreamServerUDP::~MJPEGStreamServerUDP() {
    StopAsyncThread();

    OSSleepTicks(OSMillisecondsToTicks(100));

    if(pThread != NULL) {
        delete pThread;
        pThread = NULL;
    }

    if (this->sockfd != -1) {
        socketclose(sockfd);
    }
    DEBUG_FUNCTION_LINE("Thread has been closed\n");
}

void MJPEGStreamServerUDP::DoAsyncThreadInternal(CThread *thread) {
    OSMessage message;
    bool breakOut = false;
    while (1) {
        if(breakOut) {
            break;
        }
        while(!OSReceiveMessage(&dataQueue,&message,OS_MESSAGE_FLAGS_NONE)) {
            if(shouldExit) {
                breakOut = true;
                break;
            }
            OSSleepTicks(OSMicrosecondsToTicks(500));
        }

        if(breakOut) {
            break;
        }

        DCFlushRange(&message,sizeof(OSMessage));

        JpegInformation * info = (JpegInformation *) message.args[0];
        if(info != NULL) {
            //DEBUG_FUNCTION_LINE("GOT FRAME INFO! %08X\n",info);
            DCFlushRange(info,sizeof(JpegInformation));
            sendJPEG(info->getBuffer(),info->getSize());
            delete info;
        }
    }
    return;
}

bool MJPEGStreamServerUDP::streamJPEGThreaded(JpegInformation * info) {
    OSMessage message;
    message.message = (void *) 0x11111;
    message.args[0] = (uint32_t) info;
    if(!OSSendMessage(&dataQueue,&message,OS_MESSAGE_FLAGS_NONE)) {
        frame_counter_skipped++;
        //DEBUG_FUNCTION_LINE("Dropping frame\n");
        delete info;
        return false;
    };
    return true;
}

bool MJPEGStreamServerUDP::streamJPEG(JpegInformation * info) {
    if(info != NULL) {
        //return streamJPEGThreaded(info);

            DCFlushRange(info,sizeof(JpegInformation));
            sendJPEG(info->getBuffer(),info->getSize());
            delete info;
            return true;
    }
    return false;
}

// Per-datagram header (16 bytes, big-endian since the Wii U is big-endian):
//   frameId    uint32   increments per frame
//   chunkIndex uint16   0-based index of this chunk within the frame
//   chunkCount uint16   total chunks in this frame
//   crc32      uint32   CRC32 of the whole JPEG (same in every chunk)
//   jpegSize   uint32   total JPEG size (same in every chunk)
// Payload follows (fixed CHUNK_PAYLOAD bytes except the last chunk).
#define CHUNK_HEADER  16
#define CHUNK_PAYLOAD (MAX_UDP_SIZE - CHUNK_HEADER)   // 1400 - 16 = 1384

void MJPEGStreamServerUDP::sendJPEG(uint8_t * buffer, uint64_t size) {
    uint32_t crcValue = crc32_crc(&crc32Buffer, buffer, size);
    uint32_t jpegSize = (uint32_t) size;

    // The Wii U's UDP stack cannot send datagrams larger than the MTU, so a
    // frame is split into <=1400-byte chunks. Each chunk self-describes its
    // frame (id + crc + size + index/count), so the client can reassemble and
    // resync per-frame: one lost chunk drops only that frame, and the next
    // frameId starts fresh (no cascading black screen like the old protocol).
    uint32_t frameId    = ++frameCounter;
    uint32_t chunkCount = (jpegSize + CHUNK_PAYLOAD - 1) / CHUNK_PAYLOAD;
    if (chunkCount == 0) chunkCount = 1;

    uint8_t packet[MAX_UDP_SIZE];
    for (uint32_t idx = 0; idx < chunkCount; idx++) {
        uint32_t offset     = idx * CHUNK_PAYLOAD;
        uint32_t payloadLen = jpegSize - offset;
        if (payloadLen > CHUNK_PAYLOAD) payloadLen = CHUNK_PAYLOAD;

        uint16_t idx16   = (uint16_t) idx;
        uint16_t count16 = (uint16_t) chunkCount;
        memcpy(packet + 0,  &frameId,   4);
        memcpy(packet + 4,  &idx16,     2);
        memcpy(packet + 6,  &count16,   2);
        memcpy(packet + 8,  &crcValue,  4);
        memcpy(packet + 12, &jpegSize,  4);
        memcpy(packet + 16, buffer + offset, payloadLen);

        if (send(sockfd, packet, (int)(CHUNK_HEADER + payloadLen), 0) < 0) {
            // socket error: stop sending this frame
            break;
        }
    }
}

bool MJPEGStreamServerUDP::sendData(uint8_t * data,int32_t length) {
    int len = length;
    int ret = -1;
    while (len > 0) {
        int block = len < MAX_UDP_SIZE ? len : MAX_UDP_SIZE; // take max 508 bytes per UDP packet
        ret = send(sockfd, data, block, 0);

        if(ret < 0) {
            return false;
        }

        len -= ret;
        data += ret;
    }

    return true;
}
