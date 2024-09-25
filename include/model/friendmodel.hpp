#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "user.hpp"
using namespace std;

class FriendModel
{
private:
public:
    // 添加好友
    void insert(int userid, int friendid);
    // 根据用户id检索好友信息
    vector<User> query(int userid);
};

#endif