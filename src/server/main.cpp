#include <iostream>
#include <signal.h>

#include "chatserver.hpp"
#include "chatservice.hpp"


void resetHandler(int)
{
    ChatService::instance()->reset();
}

int main(int argc, char **argv)
{
    signal(SIGINT, resetHandler);

    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;

}
