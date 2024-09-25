#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

class GroupModel
{
private:
public:
    // 创建群聊
    bool createGroup(Group &group);
    // 加入群聊
    void joinGroup(int userid, int groupid, string role);
    // 根据群聊id检索群聊
    Group queryGroup(int groupid);
    // 根据用户id检索群聊信息
    vector<Group> queryGroups(int userid);
    // 根据用户id和群聊id检索群聊中所有用户信息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif