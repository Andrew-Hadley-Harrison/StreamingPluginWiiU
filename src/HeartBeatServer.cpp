#include "HeartBeatServer.hpp"
#include "EncodingHelper.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

HeartBeatServer *HeartBeatServer::instance = NULL;

HeartBeatServer::HeartBeatServer(int32_t port) : listenPort(port) {
    worker = std::thread(&HeartBeatServer::run, this);
}

HeartBeatServer::~HeartBeatServer() {
    shouldExit = true;
    if (worker.joinable()) worker.join();
    if (mjpegStreamServer) { delete mjpegStreamServer; mjpegStreamServer = NULL; }
}

void HeartBeatServer::run() {
    int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0) return;
    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listenPort);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(listenfd); return; }
    listen(listenfd, 1);
    while (!shouldExit) {
        struct sockaddr_in cli; socklen_t len = sizeof(cli);
        int clientfd = accept(listenfd, (struct sockaddr *)&cli, &len);
        if (clientfd < 0) continue;
        EncodingHelper::getInstance()->setMJPEGStreamServer(NULL);
        if (mjpegStreamServer) { delete mjpegStreamServer; mjpegStreamServer = NULL; }
        mjpegStreamServer = MJPEGStreamServerUDP::createInstance(cli.sin_addr.s_addr, htons(DEFAULT_UDP_CLIENT_PORT));
        EncodingHelper::getInstance()->setMJPEGStreamServer(mjpegStreamServer);
        connected = true;
        while (!shouldExit) {
            unsigned char b;
            int r = recv(clientfd, &b, 1, 0);
            if (r <= 0) break;
            if (b == 0x15) { unsigned char pong = 0x16; send(clientfd, &pong, 1, 0); }
        }
        connected = false;
        EncodingHelper::getInstance()->setMJPEGStreamServer(NULL);
        if (mjpegStreamServer) { delete mjpegStreamServer; mjpegStreamServer = NULL; }
        close(clientfd);
    }
    close(listenfd);
}
