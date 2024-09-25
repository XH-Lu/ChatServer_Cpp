#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

class ChatService
{
private:
    ChatService();
    // 保存业务id+业务函数的映射关系
    std::unordered_map<int, MsgHandler> _msgHandlerMap;
    // 保存用户id+Tcp连接指针的映射关系
    std::unordered_map<int, const TcpConnectionPtr> _userConnMap;
    // 用户相关业务数据库操作
    UserModel _userModel;
    // 离线消息相关业务数据库操作
    OfflineMsgModel _offlineMsgModel;
    // 好友相关业务数据库操作
    FriendModel _friendModel;
    // 群聊相关业务数据库操作
    GroupModel _groupModel;
    // 调用互斥锁，保护_userConnMap同一时刻不被多个线程控制
    std::mutex _connMutex;
    Redis _redis;

public:
    // 单例模式，返回唯一实例
    static ChatService *instance();
    // 返回业务函数
    MsgHandler getHandler(int msgid);
    // 注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 登出业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 客户端意外退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 重置用户状态
    void reset();
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群聊业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群聊业务
    void joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群聊业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 跨服务器聊天业务，同时也是redis的接收消息回调函数，在ChatService构造函数中完成绑定
    void handleRedisSubscribMessage(int userid, string msg);
};

#endif