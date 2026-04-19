#ifndef PUBLIC_H
#define PUBLIC_H
enum EnMsgType
{
     NOTHING = 1,
    LOGIN_MSG ,//登入消息
    LOGIN_MSG_ACK ,//登入响应
    REG_MSG   ,//注册消息
    REG_MSG_ACK   ,//注册响应
    ONE_CHAT_MSG   ,//单对单聊天
    ONE_CHAT_MSG_ACK   ,//单对单聊天响应
    ADD_FRIEND_MSG    ,//添加好友
    CREATE_GROUP_MSG  ,//创建群组
    ADD_GROUP_MSG     ,//添加群组
    GROUP_CHAT_MSG    ,//群组聊天
    LOGINOUT_MSG      ,//注销登入
};
#endif