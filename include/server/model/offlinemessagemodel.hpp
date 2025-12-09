#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <string>
#include <vector>
using namespace std;
class offlineMsgModel
{
    public:
        //储存离线消息
        void insert(int userid,string msg);
        //删除离线消息
        void remove(int userid);
        //查询并推送离线消息
        vector<string> query(int userid);
    private: 
};

#endif