#include "friendmodel.hpp"
#include "db.hpp"
/***************************
好友相关数据层代码
主要任务：执行与好友相关业务的sql语句
***************************/

void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    // 添加好友
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    // 根据用户id检索好友信息
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where userid = %d", userid);
    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
