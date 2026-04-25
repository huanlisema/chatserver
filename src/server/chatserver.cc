#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
#include <iostream>
#include <string>
#include <muduo/base/Logging.h>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    // 注册读写回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置服务端线程数量
    _server.setThreadNum(2);
}
void ChatServer::start()
{
    _server.start();
}
// 连接回调
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 读写回调
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{

while (buffer->readableBytes() > 0)
    {
        const char* end = (const char*)memchr(buffer->peek(), '\0', buffer->readableBytes());
        if (end == nullptr)
        {
            break;
        }
        
        int msgLen = end - buffer->peek();
        string msg(buffer->peek(), msgLen);
        buffer->retrieve(msgLen + 1);
        
        if (msg.empty())
        {
            continue;
        }
        
        try
        {
            json js = json::parse(msg);
            int msgid = js["msgid"].get<int>();
            auto msgHandler = ChatService::instance()->getHandler(msgid);
            msgHandler(conn, &js, time);
        }
        catch (const std::exception& e)
        {
            cerr << "JSON parse error: " << e.what() << endl;
        }
    }
    // string buff = buffer->retrieveAllAsString();
    // json js = json::parse(buff); // 反序列化解析
    // auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // // 调用业务码回调函数
    // msgHandler(conn, &js, time);
}
