#include <functional>
#include "chatserver.hpp"
#include "chatservice.hpp"
/***************************
网络层代码
主要任务：初始化TcpServer类对象，绑定监听端口，设计连接回调函数和消息回调函数
***************************/

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    // 连接消息回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 业务消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置线程池数量
    _server.setThreadNum(4);
}

ChatServer::~ChatServer()
{
}

void ChatServer::start()
{
    // 进入监听状态
    _server.start();
}

// 根据原型编写连接回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 根据原型编写消息回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 读取缓存，转换为字符串
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);

    // 根据buffer中的业务id调用相应的业务函数
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);
}
