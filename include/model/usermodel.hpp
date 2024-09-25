#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

class UserModel
{
private:
public:
    // 添加用户数据
    bool insert(User &user);
    // 根据用户名检索用户数据
    User query(string name);
    // 根据用户id检索用户数据
    User query(int id);
    // 更新用户数据，目前主要为更新用户状态
    bool update(User user);
    // 重置所有用户状态
    void resetState();
};

#endif