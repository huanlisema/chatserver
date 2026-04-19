#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include "user.hpp"
#include <vector>
using namespace std;
class friendModel
{
public:
    // 添加好友
    void insert(int userid, int friendid);
    // 获取好友信息
    vector<User> query(int userid);
};

#endif