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
    // 循环处理 buffer 中的所有完整消息
    while (buffer->readableBytes() > 0)
    {
        // 1. 查找消息结束符 \0
        const char *end = (const char *)memchr(buffer->peek(), '\0', buffer->readableBytes());

        // 2. 没找到 \0，说明还没收到完整消息，等下次
        if (end == nullptr)
        {
            break;
        }

        // 3. 取出完整消息（不包含 \0）
        int msgLen = end - buffer->peek();
        string msg(buffer->peek(), msgLen);

        // 4. 从 buffer 中移除这条消息（包括 \0）
        buffer->retrieve(msgLen + 1);

        // 5. 解析并处理消息
        if (!msg.empty())
        {
            json js = json::parse(msg);
            int msgid = js["msgid"].get<int>();
            auto msgHandler = ChatService::instance()->getHandler(msgid);
            msgHandler(conn, &js, time);
        }
    }
}
