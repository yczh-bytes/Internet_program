
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <errno.h>

#include "http_conn.h"
#include "threadpool.h"

#define MAX_FD 65535//最大并发连接数
#define MAX_EVENT_NUMBER 10000//最大事件数


//信号处理函数
void addsig(int signum,void (*handler)(int))
{
    struct sigaction sa{};
    
    //注册信号捕捉
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    //设置信号捕捉
    if(sigaction(signum,&sa,NULL)!=0)
    {
        throw std::runtime_error("sigaction");
    }
}
int main(int argc,char *argv[])
{
    if(argc<=1)
    {
        printf("Usage:%s port\n",basename(argv[0]));
        return -1;
    }
    //获取端口号
    int port = atoi(argv[1]);

    //注册信号捕捉
    addsig(SIGPIPE,SIG_IGN);

    //创建线程池
    threadpool<http_conn> *pool = NULL;

   try
   {
    pool = new threadpool<http_conn>;
   }
   catch(...)
   {
    exit(-1);
   }

   //创建套接字
   int sockfd = socket(AF_INET,SOCK_STREAM,0);
   if(sockfd==-1)
   {
    throw std::runtime_error("socket");
   }
   //设置端口复用
   int opt = 1;
   if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1)
   {
    throw std::runtime_error("setsockopt");
   }

   //创建连接数组，储存所有的客户端连接信息
   http_conn * users = new http_conn[MAX_FD];
   
   //绑定地址
   struct sockaddr_in addr_;
   addr_.sin_family = AF_INET;
   addr_.sin_addr.s_addr = htonl(INADDR_ANY);
   addr_.sin_port = htons(port);

   int bd = bind(sockfd,(struct sockaddr*)&addr_,sizeof(addr_));
   if(bd==-1)
   {
    throw std::runtime_error("bind");
   }

   //监听
   int ret = listen(sockfd,5);
   if(ret==-1)
   {
    throw std::runtime_error("listen");
   }
   
   //epoll初始化
   int epollfd = epoll_create(1);
   if(epollfd==-1)
   {
    throw std::runtime_error("epoll_create");
   }
   
 //添加监听fd到epoll
    struct epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1)
    {
        throw std::runtime_error("epoll_ctl");
    }
    struct epoll_event m_address[MAX_EVENT_NUMBER];
   http_conn::m_epollfd = epollfd;
    //连接相关成员变量
   //主循环
   while(true)
   {
    int nfds = epoll_wait(epollfd,m_address,MAX_EVENT_NUMBER,-1);//循环等待事件发生
    if(nfds<0&&errno!=EINTR)
    {
     throw std::runtime_error("epoll_wait");
     break;
    }

    for(int i = 0;i<nfds;i++)
    {
        //处理新链接
        if(m_address[i].data.fd==sockfd)
        {
            //接受新连接
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof(client_address);
            int connfd = accept(sockfd,(struct sockaddr*)&client_address,&client_address_len);

            //查看连接数是否达到上限
            if(http_conn::m_users>=MAX_FD-1)
            {
                printf("Error:Too many users\n");
                close(connfd);
                continue;
            }
            //添加新链接到数组
            users[connfd].init(connfd,client_address);
        }
        //处理异常事件
        else if(m_address[i].events&(EPOLLERR|EPOLLRDHUP|EPOLLHUP))
        {
            //关闭连接
            users[m_address[i].data.fd].close_conn();
        }
        //处理读事件
        else if(m_address[i].events&EPOLLIN)
        {
            //读取数据
            users[m_address[i].data.fd].read();
            //添加到线程池
            pool->append(users+m_address[i].data.fd);
        }
        //处理写事件
        else if(m_address[i].events&EPOLLOUT)
        {
            //写入数据
            users[m_address[i].data.fd].write();
        }
    }

   }
    return 0;
}