#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;

// 全局变量
User _clientUser;
vector<User> _clientFriends;
vector<Group> _clientGroups;
bool isLogin = false;

// 函数声明
void showCurrentUserInfo();

void readTaskHandler(int clientfd);

string getCurrentTime();

void mainMenu(int clientfd);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        exit(-1);
    }

    for(;;)
    {
        cout << "=============================" << endl;
        cout << "1.登录" << endl;
        cout << "2.注册" << endl;
        cout << "3.退出" << endl;
        cout << "=============================" << endl;
        cout << "请输入操作id：" << endl;
        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "请输入用户名：";
            cin.getline(name, 50);
            cout << "请输入密码：";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        // 登录成功
                        _clientUser.setId(responsejs["id"].get<int>());
                        _clientUser.setName(responsejs["name"]);
                        isLogin = true;

                        if (responsejs.contains("friends"))
                        {
                            _clientFriends.clear();
                            vector<string> friendVec = responsejs["friends"];
                            for (string &str : friendVec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                _clientFriends.push_back(user);
                            }
                        }

                        if (responsejs.contains("groups"))
                        {
                            _clientGroups.clear();
                            vector<string> groupVec = responsejs["groups"];
                            for (string &str : groupVec)
                            {
                                json js = json::parse(str);
                                Group group;
                                group.setId(js["id"].get<int>());
                                group.setName(js["groupname"]);
                                group.setDesc(js["groupdesc"]);
                                vector<string> guserVec = js["users"];
                                for (string &s : guserVec)
                                {
                                    GroupUser guser;
                                    json j = json::parse(s);
                                    guser.setId(j["id"].get<int>());
                                    guser.setName(j["name"]);
                                    guser.setState(j["state"]);
                                    guser.setRole(j["role"]);
                                    group.getUsers().push_back(guser);
                                }
                                _clientGroups.push_back(group);
                            }
                        }
                        showCurrentUserInfo();
                        if (responsejs.contains("offlinemsg"))
                        {
                            cout << "--------------------离线消息--------------------" << endl;
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                                else
                                {
                                    cout << js["time"].get<string>() << " [" << js["groupid"] << "]" << js["groupname"].get<string>() <<"-->"<< " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }
                        static int readThreadNum = 0;
                        if(readThreadNum == 0)
                        {
                            std::thread readTask(readTaskHandler, clientfd);
                            readTask.detach();
                            readThreadNum++;
                        }


                        mainMenu(clientfd);
                    }
                }
            }
            break;
        }
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "请输入用户名：" << endl;
            cin.getline(name, 50);
            cout << "请输入密码：" << endl;
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << name << "用户名已存在，注册失败" << endl;
                    }
                    else
                    {
                        cerr << name << "注册成功，用户id为 " << responsejs["id"] << "，请记好您的id号" << endl;
                    }
                }
            }
            break;
        }
        case 3:
        {
            close(clientfd);
            exit(0);
        }
        default:
            cerr << "invalid input" << endl;
            break;
        }
    }
    return 0;
}

void showCurrentUserInfo()
{
    cout << "====================用户信息====================" << endl;
    cout << "登录用户id：" << _clientUser.getId() << "\n用户名：" << _clientUser.getName() << endl;
    cout << "--------------------好友列表--------------------" << endl;
    if (!_clientFriends.empty())
    {
        for (User &u : _clientFriends)
        {
            cout << u.getId() << " " << u.getName() << " " << u.getState() << endl;
        }
    }
    cout << "-------------------- 群列表 --------------------" << endl;
    if (!_clientGroups.empty())
    {
        for (Group &g : _clientGroups)
        {
            cout << g.getId() << " " << g.getName() << " " << g.getDesc() << " " << endl;
            for (GroupUser &gu : g.getUsers())
            {
                cout << "    " << gu.getId() << " " << gu.getName() << " " << gu.getState() << " " << gu.getRole() << endl;
            }
        }
    }
    cout << "===================== 结束 =====================" << endl;
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        if (ONE_CHAT_MSG == js["msgid"].get<int>())
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (GROUP_CHAT_MSG == js["msgid"].get<int>())
        {
            cout << js["time"].get<string>() << " [" << js["groupid"] << "]" << js["groupname"].get<string>() << "-->" << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday, (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

void help(int, string);
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void joingroup(int, string);
void groupchat(int, string);
void logout(int, string);

unordered_map<string, string> commandMap = {
    {"help", "显示所有命令，格式：help"},
    {"chat", "一对一聊天，格式：chat:friendid:message"},
    {"addfriend", "添加好友，格式：addfriend:friendid"},
    {"creategroup", "创建群组，格式：creategroup:groupname:groupdesc"},
    {"joingroup", "加入群组，格式：joingroup:groupid"},
    {"groupchat", "群组聊天，格式：groupchat:groupid:message"},
    {"logout", "退出登录，格式：logout"}};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"joingroup", joingroup},
    {"groupchat", groupchat},
    {"logout", logout}};

void mainMenu(int clientfd)
{
    help(0, "");

    char buffer[1024] = {0};
    while(isLogin)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        else
        {
            it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
        }
    }
}

void help(int, string)
{
    cout << "--------------------所有指令--------------------" << endl;
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = _clientUser.getId();
    js["name"] = _clientUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = _clientUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = _clientUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

void joingroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = JOIN_GROUP_MSG;
    js["id"] = _clientUser.getId();
    js["groupid"] = groupid;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send joingroup msg error ->" << buffer << endl;
    }
}

void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = _clientUser.getId();
    js["name"] = _clientUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error ->" << buffer << endl;
    }
}

void logout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = _clientUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        cerr << "send logout msg error ->" << buffer <<endl;
    }
    else
    {
        isLogin = false;
    }


}
