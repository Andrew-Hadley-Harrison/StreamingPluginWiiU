#ifndef _HEARTBEAT_SERVER_H_
#define _HEARTBEAT_SERVER_H_
#include <thread>
#include <atomic>
#include <stdint.h>
#include "MJPEGStreamServerUDP.hpp"
#define DEFAULT_TCP_PORT 8092
class HeartBeatServer {
public:
    static HeartBeatServer *getInstance() { if (!instance) instance = new HeartBeatServer(DEFAULT_TCP_PORT); return instance; }
    static void destroyInstance() { if (instance) { delete instance; instance = NULL; } }
    static bool isInstanceConnected() { return instance && instance->connected; }
    MJPEGStreamServer *getMJPEGServer() { return mjpegStreamServer; }
    HeartBeatServer(int32_t port);
    ~HeartBeatServer();
private:
    void run();
    int32_t listenPort;
    std::thread worker;
    std::atomic<bool> shouldExit{false};
    std::atomic<bool> connected{false};
    static HeartBeatServer *instance;
    MJPEGStreamServerUDP *mjpegStreamServer = NULL;
};
#endif
