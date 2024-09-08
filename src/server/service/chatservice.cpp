#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include "chatservice.hpp"
#include "public.hpp"
using namespace std;


ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({JOIN_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});


}

ChatService* ChatService::instance()
{
    static ChatService singleService;
    return &singleService;
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
       
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp){
             LOG_ERROR << "msgid:" << msgid << "can not find handler!\n";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }

}


void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User usr;
    usr.setName(name);
    usr.setPwd(pwd);
    bool state = _userModel.insert(usr);
    if(state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = usr.getId();
        conn->send(response.dump());
        conn->send("\n");
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errMsg"] = "注册失败!";
        conn->send(response.dump());
        conn->send("\n");
    }
}

void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user = _userModel.query(name);
    if((user.getName() == name) && (user.getPwd()==pwd))
    {
        if(user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录，不允许重复登陆";
            conn->send(response.dump());
            conn->send("\n");
        }
        else
        {
            //登陆成功
            int id = user.getId();
            string name = user.getName();
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            //返回用户信息
            user.setState("online");
            _userModel.update(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = id;
            response["name"] = name;

            //返回离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
            //返回好友列表
            vector<User> friendVec = _friendModel.query(id);
            if(!friendVec.empty())
            {
                vector<string> _vec;
                for(User& _f : friendVec)
                {
                    json js;
                    js["id"] = _f.getId();
                    js["name"] = _f.getName();
                    js["state"] = _f.getState();
                    _vec.push_back(js.dump());
                }
                response["friends"] = _vec;
            }
            //返回群列表
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if(!groupVec.empty())
            {
                vector<string> _vec;
                for(Group& _g : groupVec)
                {
                    json js;
                    js["id"] = _g.getId();
                    js["groupname"] = _g.getName();
                    js["groupdesc"] = _g.getDesc();
                    vector<string> guserVec;
                    for(GroupUser& _gu : _g.getUsers())
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
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
        conn->send("\n");
    }
}

void ChatService::logout(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    User user(userid, "", "", "offline");
    _userModel.update(user);
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin(); it!=_userConnMap.end(); ++it)
        {
            if(it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.update(user);
    }
}

void ChatService::reset()
{
    _userModel.resetState();
}

void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }

    _offlineMsgModel.insert(toid, js.dump());
    return;

}

void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);


}


void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group;
    group.setName(name);
    group.setDesc(desc);
    if(_groupModel.createGroup(group))
    {
        _groupModel.joinGroup(userid, group.getId(), "creator");
    }

}

void ChatService::joinGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{  
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.joinGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{  
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    js["groupname"] = _groupModel.queryGroup(groupid).getName();
    vector<int> userVec = _groupModel.queryGroupUsers(userid, groupid);
    {
        lock_guard<mutex> lock(_connMutex);
        for(int id : userVec)
        {
            auto it = _userConnMap.find(id);
            if(it != _userConnMap.end())
            {
                it->second->send(js.dump());
            }
            else
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}


 


