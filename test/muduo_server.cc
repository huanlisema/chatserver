#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
class ChatServer
{
public:
    ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg)
    :_server(loop,listenAddr,nameArg),_loop(loop)
     {
        //注册连接回调
        _server.setConnectionCallback(bind(&ChatServer::onConnection,this,_1));
        // 注册读写回调
        _server.setMessageCallback(bind(&ChatServer::onMessage,this,_1,_2,_3));
        //设置服务端线程数量
        _server.setThreadNum(2);
     };
    void start()
    {
        _server.start();
    }

private:
//连接回调
void onConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        cout << conn->peerAddress().toIpPort() << "->" 
        << conn->localAddress().toIpPort() << "link seccess" << endl;
    }
    else
    {
        cout << conn->peerAddress().toIpPort() << "->" 
        << conn->localAddress().toIpPort() << "link fail" << endl;
            conn->shutdown();//close(fd)
    }
}
//读写回调
void onMessage(const TcpConnectionPtr& conn,Buffer*buffer,Timestamp time)
{
    string buff = buffer->retrieveAllAsString();
    cout << "recv:" << buff << endl;
    conn->send(buff);
}
private:
    TcpServer _server;
    EventLoop *_loop;
};
int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "chatserver");
    server.start();
    loop.loop();
    return 0;
}
