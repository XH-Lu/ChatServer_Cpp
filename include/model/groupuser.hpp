#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群聊中的用户类，继承用户类，多了角色属性
class GroupUser : public User
{
private:
    string role;

public:
    void setRole(string role)
    {
        this->role = role;
    }

    string getRole()
    {
        return role;
    }
};

#endif