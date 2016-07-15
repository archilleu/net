//---------------------------------------------------------------------------
#include "test_tcp_client.h"
#include "../tcp_client.h"
#include "../event_loop.h"
#include "../tcp_connection.h"
//---------------------------------------------------------------------------
using namespace net;
using namespace net::test;
//---------------------------------------------------------------------------
bool TestTCPClient::DoTest()
{
    if(false == Test_Normal())  return false;

    return true;
}
namespace
{
    const char* SVR_IP  = "127.0.0.1";
    const int   SVR_PORT= 9981;
    EventLoop*  g_loop = 0;
    TCPClient* g_client = 0;
};
//---------------------------------------------------------------------------
void TestTCPClient::OnConnection(const TCPConnPtr& conn_ptr)
{
    std::cout << "OnConnection:";
    std::cout << conn_ptr->name() << std::endl;

    std::unique_lock<std::mutex> lock(mutex_);
    flag_ = true;
    cond_.notify_one();
}
//---------------------------------------------------------------------------
void TestTCPClient::OnRead(const TCPConnPtr& , Buffer& buffer, base::Timestamp rcv_time)
{
    std::cout << "time:" << rcv_time.Datetime(true) << "OnRead" << "size:" << buffer.ReadableBytes() << std::endl;
    
    const std::string& msg = msg_queue_.front();
    if(msg.size() > buffer.ReadableBytes())
        return;

    std::string msg_rcv(buffer.Peek(), msg.size());
    buffer.Retrieve(msg.size());

    if(msg != msg_rcv)
        assert(0);
    msg_queue_.pop();

    std::unique_lock<std::mutex> lock(mutex_);
    flag_ = true;
    cond_.notify_one();

    return;
}
//---------------------------------------------------------------------------
void TestTCPClient::OnWriteComplete(const TCPConnPtr& )
{
    std::cout << "OnWriteComplete" << std::endl;
}
//---------------------------------------------------------------------------
void TestTCPClient::OnThreadSend()
{
    size_t size = msg_queue_.size();
    size = 0;
    for(size_t i=0; i<size; i++)
    {
        {
        std::unique_lock<std::mutex> lock(mutex_);
        while(!flag_)
            cond_.wait(lock);
        flag_ = false;
        }
        
        TCPConnPtr conn_ptr = g_client->connection();
        if(conn_ptr)
        {
            const std::string& msg =  msg_queue_.front();
            conn_ptr->Send(msg.data(), msg.length());
        }
    }

    g_loop->Quit();
    return;
}
//---------------------------------------------------------------------------
bool TestTCPClient::Test_Normal()
{
    for(size_t i=0; i<base::UNIT_KB*64; i++)
    {
        std::string msg(base::UNIT_KB, static_cast<char>(rand()%256));
        msg_queue_.push(std::move(msg));
    }


    EventLoop loop;
    g_loop = &loop;
    InetAddress svr(SVR_IP, SVR_PORT);
    TCPClient client(&loop, svr, "my test");
    g_client = &client;
    client.set_callback_connection(std::bind(&TestTCPClient::OnConnection, this, std::placeholders::_1));
    client.set_callback_read(std::bind(&TestTCPClient::OnRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    client.set_callback_write_complete(std::bind(&TestTCPClient::OnWriteComplete, this, std::placeholders::_1));
    client.EnableRetry();

    client.Connect();
    thread_.Start();
    loop.Loop();
    thread_.Join();
    client.Disconnect();
    std::cout << "count:" << g_client->connection().use_count()<< std::endl;
    return true;
}
//---------------------------------------------------------------------------
