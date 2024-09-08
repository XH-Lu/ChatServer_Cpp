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

    bool createGroup(Group& group);

    void joinGroup(int userid, int groupid, string role);

    Group queryGroup(int groupid);

    vector<Group> queryGroups(int userid);

    vector<int> queryGroupUsers(int userid, int groupid);

};



#endif