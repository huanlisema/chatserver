#ifndef CHATSERVICE_H
#define CHATSERVICE_H
// 聊天服务器业务类
#include <functional>
#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace nlohmann;
typedef function<void(const TcpConnectionPtr &conn, json *js, Timestamp &time)> MsgHandler;
class ChatService
{
public:
    // 获取单例
    static ChatService *instance();
    // 登入业务
    void login(const TcpConnectionPtr &conn, json *js, Timestamp &time);
    // 注册业务
    void reg(const TcpConnectionPtr &conn, json *js, Timestamp &time);
    //注销业务
    void loginout(const TcpConnectionPtr &conn, json *js, Timestamp &time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json *js, Timestamp &time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json *js, Timestamp &time);
    // 返回业务码对应的处理方法
    MsgHandler getHandler(int msgid);
    // 空操作
    void donothing(const TcpConnectionPtr &conn, json *js, Timestamp &time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 处理服务端异常退出
    void reset();
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json *js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json *js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json *js, Timestamp time);
    //设置上报消息的回调
    void handleRedisSubscribeMessagre(int userid ,string msg);
private:
    ChatService();
    // 储存业务码和对应的业务处理函数
    unordered_map<int, MsgHandler> _msghandlermap;
    // 存储在线用户的id和通信链接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    // 定义_userConnMap的互斥锁
    mutex _connMutex;
    // 数据操作类对象
    UserModel _userModel;
    offlineMsgModel _offlineMsgModel;
    friendModel _friendModel;
    GroupModel _groupModel;
    //redis操作对象
    Redis _redis;
};
#endif