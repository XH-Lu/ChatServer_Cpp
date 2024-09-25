#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include "chatservice.hpp"
#include "public.hpp"
using namespace std;
/***************************
业务层代码
主要任务：创建ChatService实例化对象，设计登录、一对一聊天、群聊等业务函数
***************************/

ChatService::ChatService()
{
    // 绑定业务id与对应方法
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({JOIN_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    if (_redis.connect())
    {
        // redis服务器连接成功，设置redis的接收消息回调为跨服务器聊天业务
        _redis.initNotifyHandler(bind(&ChatService::handleRedisSubscribMessage, this, _1, _2));
    }
}

ChatService *ChatService::instance()
{
    static ChatService singleService;
    return &singleService;
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 业务id不正确
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!\n";
        };
    }
    else
    {
        // 返回对应业务方法
        return _msgHandlerMap[msgid];
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User usr;
    usr.setName(name);
    usr.setPwd(pwd);
    // 向数据库注册新用户
    bool state = _userModel.insert(usr);
    if (state)
    {
        // 注册成功，返回用户id
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = usr.getId();
        conn->send(response.dump());
        conn->send("\n");
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errMsg"] = "注册失败!";
        conn->send(response.dump());
        conn->send("\n");
    }
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    // 通过用户名登录，在数据库检索对应密码
    User user = _userModel.query(name);
    if ((user.getName() == name) && (user.getPwd() == pwd))
    {
        // 验证用户名和密码通过
        if (user.getState() == "online")
        {
            // 重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录，不允许重复登陆";
            conn->send(response.dump());
            conn->send("\n");
        }
        else
        {
            // 登陆成功
            int id = user.getId();
            string name = user.getName();
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 登录成功后，服务器向redis订阅id通道的消息
            _redis.subscribe(id);

            // 返回用户信息
            user.setState("online");
            _userModel.update(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = id;
            response["name"] = name;

            // 返回离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
            // 返回好友列表
            vector<User> friendVec = _friendModel.query(id);
            if (!friendVec.empty())
            {
                vector<string> _vec;
                for (User &_f : friendVec)
                {
                    json js;
                    js["id"] = _f.getId();
                    js["name"] = _f.getName();
                    js["state"] = _f.getState();
                    _vec.push_back(js.dump());
                }
                response["friends"] = _vec;
            }
            // 返回群列表
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> _vec;
                for (Group &_g : groupVec)
                {
                    json js;
                    js["id"] = _g.getId();
                    js["groupname"] = _g.getName();
                    js["groupdesc"] = _g.getDesc();
                    vector<string> guserVec;
                    for (GroupUser &_gu : _g.getUsers())
                    {
                        json j;
                        j["id"] = _gu.getId();
                        j["name"] = _gu.getName();
                        j["state"] = _gu.getState();
                        j["role"] = _gu.getRole();
                        guserVec.push_back(j.dump());
                    }
                    js["users"] = guserVec;
                    _vec.push_back(js.dump());
                }
                response["groups"] = _vec;
            }

            conn->send(response.dump());
            conn->send("\n");
        }
    }
    else
    {
        // 用户名或密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
        conn->send("\n");
    }
}

void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            // 用户退出登录，_userConnMap删除连接信息
            _userConnMap.erase(it);
        }
    }
    // 用户退出登录，取消userid通道的订阅
    _redis.unsubscribe(userid);

    User user(userid, "", "", "offline");
    // 向数据库更新用户状态为"offline"
    _userModel.update(user);
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    // 用户异常退出，未提供用户id，采用遍历方式查找意外退出的用户
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 用户退出登录，取消userid通道的订阅
    _redis.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.update(user);
    }
}

void ChatService::reset()
{
    // 重置用户状态为"offline"
    _userModel.resetState();
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        // 如果用户在本服务器的_userConnMap上说明两个人属于同一台服务器
        if (it != _userConnMap.end())
        {
            // 如果聊天对象在线，发送消息
            it->second->send(js.dump());
            return;
        }
    }

    // 如果用户不在当前服务器，通过表查询用户是否在线，如果在线说明他在其他服务器，通过redis推送消息实现聊天
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        // 用户在其他服务器登录，向redis发布用户id+消息，订阅该用户的服务器会收到聊天消息
        _redis.publish(toid, js.dump());
        return;
    }

    // 如果聊天对象离线，将信息保存至离线消息表
    _offlineMsgModel.insert(toid, js.dump());
    return;
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 向好友表添加好友信息
    _friendModel.insert(userid, friendid);
    _friendModel.insert(friendid, userid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group;
    group.setName(name);
    group.setDesc(desc);
    if (_groupModel.createGroup(group))
    {
        // 群聊创建成功，将创建人加入群聊，并设置creator身份
        _groupModel.joinGroup(userid, group.getId(), "creator");
    }
}

void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 加入群聊
    _groupModel.joinGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    js["groupname"] = _groupModel.queryGroup(groupid).getName();
    // 检索群聊里所有的用户id
    vector<int> userVec = _groupModel.queryGroupUsers(userid, groupid);
    {
        lock_guard<mutex> lock(_connMutex);
        for (int id : userVec)
        {
            // 向每个用户发送群聊消息
            auto it = _userConnMap.find(id);
            if (it != _userConnMap.end())
            {
                // 如果用户在本服务器上登录，在_userConnMap中能够检索到，直接发送消息
                it->second->send(js.dump());
            }
            else
            {
                // 如果不在本服务器，可能在其他服务器或者离线
                User user = _userModel.query(id);
                if (user.getState() == "online")
                {
                    // 用户在其他服务器登录，向redis发布用户id+消息
                    _redis.publish(id, js.dump());
                }
                else
                {
                    // 用户离线，在离线消息表加入数据
                    _offlineMsgModel.insert(id, js.dump());
                }
            }
        }
    }
}

void ChatService::handleRedisSubscribMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        // 如果聊天对象在线，发送消息
        it->second->send(msg);
        return;
    }
    // 聊天对象离线，将信息保存至离线消息表
    _offlineMsgModel.insert(userid, msg);
}
