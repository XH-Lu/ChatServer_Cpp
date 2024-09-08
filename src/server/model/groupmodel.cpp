#include "groupmodel.hpp"
#include "db.hpp"

bool GroupModel::createGroup(Group& group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup (groupname, groupdesc) values ('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;

}

void GroupModel::joinGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values (%d, %d, '%s')",
            groupid, userid, role.c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }

}

Group GroupModel::queryGroup(int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from allgroup where id = %d", groupid);
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                mysql_free_result(res);
                return group;
            }
        }
        
    }
    return Group();
}

vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d", userid);
    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
        for(Group& group : groupVec)
        {
            sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join groupuser b on b.userid = a.id where b.groupid = %d", group.getId());
            MYSQL_RES* res = mysql.query(sql);
            if(res != nullptr)
            {
                MYSQL_ROW row;
                while((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser guser;
                    guser.setId(atoi(row[0]));
                    guser.setName(row[1]);
                    guser.setState(row[2]);
                    guser.setRole(row[3]);
                    group.getUsers().push_back(guser);
                }
                mysql_free_result(res);
            }
        }
    }

    return groupVec;

}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);
    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;


}