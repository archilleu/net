#!/usr/bin/python3
# test target: test_tcp_connection

import socket
import threading
import time
import random
import struct

HOST = '127.0.0.1'
PORT = 9999

REPLY = 1
NOTIFY = 2

client_nums = 50
#client_nums = 1
client_list = []
cond = threading.Condition()

def fetch_local_ipv6_address(port=PORT):
    if not socket.has_ipv6:
        raise Exception("the local machine has no IPv6 support enabled")
    #addrs = socket.getaddrinfo("localhost", port, socket.AF_INET6, 0, socket.SOL_TCP)
    addrs = ('::1', port, 0, 0)
    return addrs
    # example output: [(23, 0, 6, '', ('::1', 10008, 0, 0))]
    if len(addrs) == 0:
        raise Exception("there is no IPv6 address configured for localhost")
    entry0 = addrs[0]
    sockaddr = entry0[-1]
    return sockaddr


def ClientConnect():
    try:
        addr = fetch_local_ipv6_address(PORT)
        print(addr)
        client = socket.socket(socket.AF_INET6, socket.SOCK_STREAM);
        client.connect(addr)
        print("connect nums:peer:", client.getpeername(), "local:", client.getsockname())

        return client

    except OSError as e:
        print(e)

def ClientDisconnect():
        print("disconnect num peer:", client.getpeername(), "local:", client.getsockname())
        client.close()

        return

def SendEncode(dat, conn_ptr):
        codec = struct.Struct(">ii")
        header = codec.pack(len(dat), REPLY)
        conn_ptr.send(header)
        conn_ptr.send(dat)

        return;

def RecvDecode(conn_ptr):
        rcv = conn_ptr.recv(8, socket.MSG_WAITALL)
        if 8 > len(rcv):
            return None

        codec   = struct.Struct(">ii")
        header  = codec.unpack(rcv)
        dat = conn_ptr.recv(header[0], socket.MSG_WAITALL)
        return (dat, header[1])

def RecvNotify(dat, conn_ptr):
    print("recv notify")

    addr = conn_ptr.getsockname()
    str_addr = addr[0] + ":" + str(addr[1])

    if str_addr != dat.decode("utf-8"):
        assert False,"notify error"

def Reconnect(tname, conn_ptr):
    conn_ptr.close()
    conn_ptr = ClientConnect()
    client_list[tname] = conn_ptr
    return conn_ptr

client_list = []
def ClientSendData(tname, conn_ptr):
    for i in range(10000):
        print("thread name:", tname, "times:", i)
        #byte_data   = bytearray(random.randint(1, 1024*64))
        byte_data   = bytearray(1024*64)
        byte_len    = len(byte_data)
        for i in range(byte_len):
            v = random.randint(0, 255)
            byte_data[i] = v;

        try:
            SendEncode(byte_data, conn_ptr)
        except BrokenPipeError as e:
            print(e)
            conn_ptr = Reconnect(tname, conn_ptr)
            continue

        pair = RecvDecode(conn_ptr)

        if None == pair:
            conn_ptr = Reconnect(tname, conn_ptr)
            continue

        if NOTIFY == pair[1]:
            RecvNotify(pair[0], conn_ptr)
            pair = RecvDecode(conn_ptr)
            if None == pair:
                conn_ptr = Reconnect(tname, conn_ptr)
                continue

        if REPLY == pair[1]:
            tmp = byte_data[0:len(pair[0])]
            print("tmp", len(tmp), "pair", len(pair[0]));
            if byte_len > len(pair[0]):
                conn_ptr = Reconnect(tname, conn_ptr)
                continue
            elif byte_len == len(pair[0]):
                print("tmp", len(tmp), "pair", len(pair[0]));
                assert tmp==pair[0], "rcv error"
                print('recv dat')
            else:
                #print("tmp len", len(tmp), "pair len:", len(pair[0]))
                assert False


        #随机断线~
        if 1 == random.randint(1, 50):
            conn_ptr = Reconnect(tname, conn_ptr)

    return

if "__main__" == __name__:
    #测试单线程
    import pdb; pdb.set_trace()
    client = ClientConnect()
    #ClientSendData("s", client)
    ClientDisconnect()

    #测试多线程
    for i in range(client_nums):
        client_list.append(ClientConnect())

    t_connect = []
    for i in range(client_nums):
        t_connect.append(threading.Thread(target=ClientSendData,
                                          args=(i, client_list[i],)))

    for i in range(client_nums):
        t_connect[i].start()

    for i in range(client_nums):
        t_connect[i].join()

    print("finished")


