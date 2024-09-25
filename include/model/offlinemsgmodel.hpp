#ifndef OFFLINEMSGMODEL_H
#define OFFLINEMSGMODEL_H

#include <string>
#include <vector>
using namespace std;

class OfflineMsgModel
{
private:
public:
    // 添加离线消息
    void insert(int useid, string msg);
    // 清空离线消息
    void remove(int userid);
    // 根据用户id检索离线消息
    vector<string> query(int userid);
};

#endif