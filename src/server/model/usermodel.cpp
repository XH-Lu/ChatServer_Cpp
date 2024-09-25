#include <iostream>
#include "db.hpp"
#include "usermodel.hpp"
using namespace std;
/***************************
用户相关数据层代码
主要任务：执行与用户相关业务的sql语句
***************************/

bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    // 添加用户数据
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

User UserModel::query(string name)
{
    char sql[1024] = {0};
    // 根据用户名检索用户数据
    sprintf(sql, "select * from user where name = '%s'", name.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    // 根据用户id检索用户数据
    sprintf(sql, "select * from user where id = '%d'", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

bool UserModel::update(User user)
{
    char sql[1024] = {0};
    // 更新用户数据，目前主要为更新用户状态
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }

    return false;
}

void UserModel::resetState()
{
    char sql[1024] = {0};
    // 重置所有用户状态
    sprintf(sql, "update user set state = 'offline' where state = 'online'");

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}