#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <utility>
#include <iostream>
#include <vector>
using namespace std;
using namespace muduo;
ChatService *ChatService::instance()
{
  static ChatService service;
  return &service;
}

ChatService::ChatService()
{
  _msghandlermap.insert(make_pair(NOTHING, bind(&ChatService::donothing, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)));
  // 群组业务管理相关事件处理回调注册
  _msghandlermap.insert(make_pair(CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)));
  _msghandlermap.insert(make_pair(GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)));

  // 连接redis
  if (_redis.connect())
  {
    // 设置上报消息的回调
    _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessagre, this, _1, _2));
  }
}
// 返回业务码对应的处理方法
MsgHandler ChatService::getHandler(int msgid)
{
  auto it = _msghandlermap.find(msgid);
  if (it == _msghandlermap.end())
  {
    LOG_ERROR << "msgid:" << msgid << " can not find handler!";
    return _msghandlermap[NOTHING];
    // 返回一个默认的处理器，空操作
    // return [=](const TcpConnectionPtr &conn, json *js, Timestamp) {
    //     LOG_ERROR << "msgid:" << msgid << " can not find handler!";
    // };
  }
  else
  {
    return _msghandlermap[msgid];
  }
}
// 空操作
void ChatService::donothing(const TcpConnectionPtr &conn, json *js, Timestamp &time)
{
}
// 登入业务
void ChatService::login(const TcpConnectionPtr &conn, json *js, Timestamp &time)
{
  json request = *js;
  int id = request["id"];
  string pwd = request["password"];
  User user = _userModel.query(id);
  if (id == user.getId() && pwd == user.getPwd())
  {
    if (user.getState() == "online")
    {
      json respond;
      respond["msgid"] = LOGIN_MSG_ACK;
      respond["error"] = 2;
      respond["errmsg"] = "用户正在使用，请勿重复登入";
      conn->send(respond.dump());
    }
    else
    {
      // 登入成功
      // 存储用户登入链接
      {
        lock_guard<mutex> lock(_connMutex);
        _userConnMap.insert(make_pair(id, conn));
      }
      // 用户登入成功后,向redis订阅id通道
      _redis.subscribe(id);

      // 更新登入状态
      user.setState("online");
      _userModel.updateState(user);

      json respond;
      respond["msgid"] = LOGIN_MSG_ACK;
      respond["error"] = 0;
      respond["id"] = user.getId();
      respond["name"] = user.getName();
      // 查询离线消息
      vector<string> vec = _offlineMsgModel.query(id);
      if (!vec.empty())
      {
        respond["offlinemsg"] = vec;
        _offlineMsgModel.remove(id);
      }
      // 查询好友信息
      vector<User> friends = _friendModel.query(id);
      if (!friends.empty())
      {
        vector<string> friends2;
        for (auto &onefriend : friends)
        {
          json js;
          js["friendid"] = onefriend.getId();
          js["friendname"] = onefriend.getName();
          js["friendstate"] = onefriend.getState();
          friends2.push_back(js.dump());
        }
        respond["friends"] = friends2;
      }

      // 查询用户的群组信息
      vector<Group> groupuserVec = _groupModel.queryGroups(id);
      if (!groupuserVec.empty())
      {
        // group:[{groupid:[xxx, xxx, xxx, xxx]}]
        vector<string> groupV;
        for (Group &group : groupuserVec)
        {
          json grpjson;
          grpjson["id"] = group.getId();
          grpjson["groupname"] = group.getName();
          grpjson["groupdesc"] = group.getDesc();
          vector<string> userV;
          for (GroupUser &user : group.getUsers())
          {
            json js;
            js["id"] = user.getId();
            js["name"] = user.getName();
            js["state"] = user.getState();
            js["role"] = user.getRole();
            userV.push_back(js.dump());
          }
          grpjson["users"] = userV;
          groupV.push_back(grpjson.dump());
        }

        respond["groups"] = groupV;
      }

      // dump转储成字符串
      conn->send(respond.dump());
    }
  }
  else
  {
    // 用户不存在或密码错误，登入失败
    json respond;
    respond["msgid"] = LOGIN_MSG_ACK;
    respond["error"] = 1;
    respond["errmsg"] = "用户不存在或密码错误";
    conn->send(respond.dump());
  }
}
// 注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json *js, Timestamp &time)
{
  json request = *js;
  string name = request["name"];
  string pwd = request["password"];

  User user;
  user.setName(name);
  user.setPwd(pwd);
  if (_userModel.insert(user))
  {
    // 注册成功
    json respond;
    respond["msgid"] = REG_MSG_ACK;
    respond["error"] = 0;
    respond["id"] = user.getId();
    // dump转储成字符串
    conn->send(respond.dump());
  }
  else
  {
    // 注册失败
    json respond;
    respond["msgid"] = REG_MSG_ACK;
    respond["error"] = 1;
    // dump转储成字符串
    conn->send(respond.dump());
  }
}
// 注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json *js, Timestamp &time)
{
  json request = *js;
  int userid = request["id"].get<int>();
  {
    lock_guard<mutex> lock(_connMutex);
    // 删除链接
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
      _userConnMap.erase(it);
    }
  }
  // 用户注销，取消服务器对应id通道的订阅
  _redis.unsubscribe(userid);

  // 更新用户状态为offline
  User user;
  user.setId(userid);
  user.setState("offline");
  _userModel.updateState(user);
}
// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
  User user;
  {
    lock_guard<mutex> lock(_connMutex);
    // 删除链接
    for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
    {
      if (it->second == conn)
      {
        user.setId(it->first);
        _userConnMap.erase(it);
        break;
      }
    }
  }

  if (user.getId() != -1)
  {
    // 异常退出，取消服务器对应id通道的订阅
    _redis.unsubscribe(user.getId());

    // 更新用户状态为offline
    user.setState("offline");
    _userModel.updateState(user);
  }
}
// 一对一聊天业务{msgid id name to msg}
void ChatService::oneChat(const TcpConnectionPtr &conn, json *js, Timestamp &time)
{
  json request = *js;
  int toid = request["toid"];
  User user;
  user = _userModel.query(toid);
  // to用户存在
  if (user.getId() != -1)
  {
    {
      lock_guard<mutex> lock(_connMutex);
      auto it = _userConnMap.find(toid);
      if (it != _userConnMap.end())
      {
        // 用户在线,并且在同一台服务器上,直接转发
        it->second->send(request.dump());
        return;
      }
    }
    // 查询数据库用户的在线信息
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
      // 在别的服务器登入，需要跨服务器发送-redis发布
      _redis.publish(toid, request.dump());
      return;
    }
    // 用户离线，存储消息
    _offlineMsgModel.insert(toid, request.dump());
  }
  else
  {
    // to用户不存在
    json respond;
    respond["msgid"] = ONE_CHAT_MSG_ACK;
    respond["error"] = 1;
    respond["errmsg"] = "对方用户不存在";
    conn->send(respond.dump());
  }
}
// 处理服务端异常退出
void ChatService::reset()
{
  // 把用户在线状态改为下线
  _userModel.resetState();
}
// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json *js, Timestamp &time)
{
  json request = *js;
  int userid = request["id"];
  int friendid = request["friendid"];
  // 存储好友信息
  _friendModel.insert(userid, friendid);
}
// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json *js, Timestamp time)
{
  json request = *js;
  int userid = request["id"].get<int>();
  string name = request["groupname"];
  string desc = request["groupdesc"];

  // 存储新创建的群组信息
  Group group(-1, name, desc);
  if (_groupModel.createGroup(group))
  {
    // 存储群组创建人信息
    _groupModel.addGroup(userid, group.getId(), "creator");
  }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json *js, Timestamp time)
{
  json request = *js;
  int userid = request["id"].get<int>();
  int groupid = request["groupid"].get<int>();
  _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json *js, Timestamp time)
{
  json request = *js;
  int userid = request["id"].get<int>();
  int groupid = request["groupid"].get<int>();
  vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

  lock_guard<mutex> lock(_connMutex);
  for (int id : useridVec)
  {

    auto it = _userConnMap.find(id);
    if (it != _userConnMap.end())
    {
      // 用户在线，并且在同一台服务器上
      // 转发群消息
      it->second->send(request.dump());
    }
    else
    {
      // 查询数据库用户的在线信息
      User user = _userModel.query(id);
      if (user.getState() == "online")
      {
        // 在别的服务器登入，需要跨服务器发送-redis发布
        _redis.publish(id, request.dump());
      }
      else
      {
        // 存储离线群消息
        _offlineMsgModel.insert(id, request.dump());
      }
    }
  }
}
void ChatService::handleRedisSubscribeMessagre(int userid, string msg)
{
  {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
      it->second->send(msg);
      return;
    }
  }
  //跨服务器传送时,用户离线
  _offlineMsgModel.insert(userid,msg);
}