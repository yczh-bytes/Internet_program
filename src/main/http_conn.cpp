#include "http_conn.h"

int http_conn::m_epollfd = -1;
int http_conn::m_users = 0;

//添加文件描述符
void http_conn::addfd(int sockfd,int epollfd,bool one_shot)
{
    struct epoll_event event;
    event.data.fd = sockfd;
    
    event.events = EPOLLIN|EPOLLRDHUP;
    if(one_shot)
    {
        event.events |= EPOLLONESHOT;//启用ONESHOT模式，防止一个连接被多个线程处理
    }
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
    event.data.fd = sockfd;
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
    init();
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
    if(m_read_index>=read_buffer_size)
    {
        return false;
    }
    while(true)
    {
        ssize_t bytes_read=recv(m_sockfd,m_read_buf+m_read_index,read_buffer_size-m_read_index,0);
        if(bytes_read==-1)
        {
            if(errno==EAGAIN||errno==EWOULDBLOCK)//说明没有数据读了，跳出循环
            {
                break;
            }
            // 真正的读取错误
            return false;
        }
        else if(bytes_read==0)//对方关闭连接
        {
            return false;
        }
        m_read_index += bytes_read;//否则就更新下标位置
    }
    return true;//成功读取
}
  


bool http_conn::write()
{
    if(m_write_index==0)
    {
        return true;
    }
    
    int bytes_sent = send(m_sockfd,m_write_buf,m_write_index,0);
    if(bytes_sent==-1)
    {
        if(errno==EAGAIN||errno==EWOULDBLOCK)
        {
            // 缓冲区满，重新注册写事件等待下次发送
            modfd(m_sockfd,m_epollfd,EPOLLOUT);
            return true;
        }
        return false;
    }
    else if(bytes_sent<m_write_index)
    {
        // 部分发送，移动未发送数据到缓冲区头部
        memmove(m_write_buf,m_write_buf+bytes_sent,m_write_index-bytes_sent);
        m_write_index -= bytes_sent;
        // 重新注册写事件继续发送
        modfd(m_sockfd,m_epollfd,EPOLLOUT);
        return true;
    }
    
    // 全部发送完毕
    m_write_index = 0;
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
    HTTP_CODE ret = process_read();
    if(ret==NO_REQUEST)//请求不完整
    {
        modfd(m_epollfd,m_sockfd,EPOLLIN|EPOLLET);
        return;
    }
}

    
HTPP_CODE http_conn::HTTP_CODE process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;

    while((m_check_state==CHECK_STATE_CONTENT&&line_status==LINE_OK)||(line_status=parse_line())==LINE_OK)
    {
        text = get_line();//获取当前行起始位置
        m_state_line = m_checked_index;//更新当前检查索引

        switch(m_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                parse_request_line(text);
                break;
            }
            case CHECK_STATE_HEADER:
            {
                parse_headers(text);
                break;
            }
            case CHECK_STATE_HEADER:
            {
                parse_content(text);
                break;
            }
            default
            {
                return INTERNAL_ERROR;
            }
        }

    }
}

 HTTP_CODE http_conn::HTTP_CODE parse_request_line(char *text)
 {}

 HTTP_CODE http_conn::HTTP_CODE parse_headers(char *text)
 {}

 HTTP_CODE http_conn::HTTP_CODE parse_content(char *text)
 {}

 HTTP_CODE http_conn::HTTP_CODE parse_line(char *text)
 {}

 void http_conn::init()
 {
m_checked_index=0;
 m_start_line=0;
 m_check_state=CHECK_STATE_REQUESTLINE;

 }


