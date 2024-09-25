#ifndef GROUP_H
#define GROUP_H
#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;

// 群聊类
class Group
{
private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;

public:
    Group() : id(-1), name(""), desc("") {};

    void setId(int id)
    {
        this->id = id;
    }

    void setName(string name)
    {
        this->name = name;
    }

    void setDesc(string desc)
    {
        this->desc = desc;
    }

    int getId()
    {
        return id;
    }

    string getName()
    {
        return name;
    }

    string getDesc()
    {
        return desc;
    }

    vector<GroupUser> &getUsers()
    {
        return this->users;
    }
};

#endif