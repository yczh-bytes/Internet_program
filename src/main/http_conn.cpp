#include "http_conn.h"

int http_conn::m_epollfd = -1;
int http_conn::m_users = 0;

//添加文件描述符
void http_conn::addfd(int sockfd,int epollfd,bool one_shot)
{
    struct epoll_event event;
    event.data.fd = sockfd;
    
    event.events = EPOLLIN|EPOLLRDHUP|EPOLLONESHOT;//LT模式
    int ret = epoll_ctl(epollfd,EPOLL_CTL_ADD,sockfd,&event);
    if(ret==-1)
    {
     throw std::runtime_error("epoll_ctl");
    }

}
//删除文件描述符
void http_conn::removefd(int sockfd,int epollfd)
{
    int ret = epoll_ctl(epollfd,EPOLL_CTL_DEL,sockfd,NULL);
    if(ret==-1)
    {
     throw std::runtime_error("epoll_ctl");
    }

    close(sockfd);
}
//修改文件描述符
void http_conn::modfd(int sockfd,int epollfd,int ev)
{
    struct epoll_event event;
event.events = ev | EPOLLRDHUP|EPOLLONESHOT;//LT模式
    int ret = epoll_ctl(epollfd,EPOLL_CTL_MOD,sockfd,&event);
    if(ret==-1)
    {
     throw std::runtime_error("epoll_ctl");
    }
}
//初始化连接
void http_conn::init(int sockfd,struct sockaddr_in client_address)
{
    m_sockfd = sockfd;
    m_addr = client_address;
    //设置端口复用
    int opt = 1;
    int ret = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(ret==-1)
    {
     throw std::runtime_error("setsockopt");
    }
    //添加至epoll
    addfd(sockfd,m_epollfd,true);//启用ONESHOT模式

    //用户基数增加
    m_users++;
}
//关闭连接
void http_conn::close_conn()
{
    if(m_sockfd!=-1)
    {
        removefd(m_sockfd,m_epollfd);
        // removefd已经调用close，这里不需要再重复close
        m_sockfd = -1;
        //用户基数减少
        m_users--;
    }
}

bool http_conn::read()
{
    printf("read\n");
    return true;
}
bool http_conn::write()
{
    printf("write\n");
    return true;
}
http_conn::http_conn()
{
}
http_conn::~http_conn()
{
}
void http_conn::process()
{
    printf("process\n");
}


