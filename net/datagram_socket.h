//---------------------------------------------------------------------------
#ifndef BASE_LINUX_NET_DATAGRAM_SOCKET_H_
#define BASE_LINUX_NET_DATAGRAM_SOCKET_H_
//---------------------------------------------------------------------------
#include "../base/share_inc.h"
#include "datagram_packet.h"
//---------------------------------------------------------------------------
namespace net
{

class Socket;

class DatagramSocket
{
public:
    DatagramSocket();
    DatagramSocket(short port);
    DatagramSocket(const InetAddress& inet_addr);
    ~DatagramSocket();

    bool Bind(const InetAddress& inet_addr);
    bool IsBind(){ return is_bind_; }

    //connect 地址(回环地址会失败)
    bool Connect(const InetAddress& server_addr);
    bool Disconnect();
    bool IsConnect() { return connected_; }

    const InetAddress& local_addr() { return local_addr_; }
    const InetAddress& server_addr(){ return server_addr_; }

    void SetRcvBufferSize(int size);
    void SetSndBufferSize(int size);

    int GetRcvBufferSize();
    int GetSndBufferSize();

    void SetBroadcast(bool enable);
    bool IsBroadcast();

    void SetReuseAddress();

    DatagramPacket  Receive(int len);
    void            Receive(DatagramPacket& pkt);

    int Send(const DatagramPacket& pkt);

private:
    int SocketCeate();

private:
    InetAddress local_addr_;
    InetAddress server_addr_;
    bool        connected_;
    bool        is_bind_;

    std::shared_ptr<Socket>  sockfd_;

protected:
    DISALLOW_COPY_AND_ASSIGN(DatagramSocket);
};

}//namespace net
//---------------------------------------------------------------------------
#endif //BASE_LINUX_NET_DATAGRAM_SOCKET_H_