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

using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp)>;


class ChatService
{
private:
    ChatService();

    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    std::unordered_map<int, const TcpConnectionPtr> _userConnMap;

    UserModel _userModel;

    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    std::mutex _connMutex;

public:
    static ChatService* instance();

    MsgHandler getHandler(int msgid);

    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);

    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);

    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);

    void clientCloseException(const TcpConnectionPtr& conn);

    void reset();

    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);

    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

    void joinGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);


};





#endif