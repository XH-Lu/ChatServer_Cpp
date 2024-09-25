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
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 捕捉服务端异常退出信号，设置所有用户状态为"offline"
    signal(SIGINT, resetHandler);

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    // 启动服务器，进入监听状态
    server.start();
    // 阻塞主线程
    loop.loop();

    return 0;
}
