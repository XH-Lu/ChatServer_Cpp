#ifndef OFFLINEMSGMODEL_H
#define OFFLINEMSGMODEL_H

#include<string>
#include<vector>
using namespace std;

class OfflineMsgModel
{
private:

public:
    void insert(int useid, string msg);

    void remove(int userid);

    vector<string> query(int userid);

};


#endif