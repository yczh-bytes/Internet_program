#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include <stdexcept>
#include <thread>



class http_conn
{
public:
static int m_epollfd;//共享epollfd实例
static int m_users;//当前用户连接数
    http_conn();
    ~http_conn();
    void process();
    void addfd(int sockfd,int epollfd,bool one_shot);
    void removefd(int sockfd,int epollfd);
    void modfd(int sockfd,int epollfd,int ev);
    void init(int sockfd,struct sockaddr_in client_address);
    void close_conn();

private:

int m_sockfd;//当前连接的fd值
struct sockaddr_in m_addr;//客户端地址信息

};