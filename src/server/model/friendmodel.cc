#include "friendmodel.hpp"
#include "db.h"
// 添加好友
void friendModel::insert(int userid, int friendid)
{
     // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 获取好友信息
vector<User> friendModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d", userid);
    vector<User> friends;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row; 
            while ((row = mysql_fetch_row(res))!= nullptr)
            {
                User onefriend;
                onefriend.setId(atoi(row[0]));
                onefriend.setName(row[1]);
                onefriend.setState(row[2]);
                friends.push_back(onefriend);

            }
            mysql_free_result(res);
            return friends;
        }
    }
    return friends;
}