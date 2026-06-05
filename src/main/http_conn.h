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



// HTTP响应状态行定义
const char ok_200_title[] = "OK";
const char error_400_title[] = "Bad Request";
const char error_400_form[] = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char error_403_title[] = "Forbidden";
const char error_403_form[] = "You do not have permission to get file from this server.\n";
const char error_404_title[] = "Not Found";
const char error_404_form[] = "The requested file was not found on this server.\n";
const char error_500_title[] = "Internal Error";
const char error_500_form[] = "There was an unusual problem serving the requested file.\n";

class http_conn
{
public:
static int m_epollfd;//共享epollfd实例
static int m_users;//当前用户连接数
static const int write_buffer_size = 2408;//写入缓冲区大小
static const int read_buffer_size = 1024;// 读取缓冲区大小
    http_conn();
    ~http_conn();
    //实现业务逻辑
    void process();
    void addfd(int sockfd,int epollfd,bool one_shot);
    void removefd(int sockfd,int epollfd);
    void modfd(int sockfd,int epollfd,int ev);
    void init(int sockfd,struct sockaddr_in client_address);
    void close_conn();
    bool read();//非阻塞读取
    bool write();//非阻塞写入
    
//HTTP方法枚举 
//定义支持的HTTP方法
    enum METHOD{GET = 0,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,CONNECT};
/*•主状态机状态 
解析客户端请求时的主状态：
◦`CHECK_STATE_REQUESTLINE`：正在分析请求行。
◦`CHECK_STATE_HEADER`：正在分析头部字段。
◦`c`：正在解析请求体。*/
    enum CHECK_STATE{CHECK_STATE_REQUESTLINE=0,CHECK_STATE_HEADER,CHECK_STATE_HEADER};

  
/*•HTTP处理结果码 
可能的解析结果：
◦`NO_REQUEST`：请求不完整，需继续读取。
◦`GET_REQUEST`：获得完整正确请求。
◦`BAD_REQUEST`：请求语法错误。
◦`NO_RESOURCE`：服务器无此资源。
◦`FORBIDDEN_REQUEST`：无权限访问。
◦`FILE_REQUEST`：请求的是文件。
◦`INTERNAL_ERROR`：服务器内部错误。
`CLOSED_CONNECTION`：客户端已关闭连接*/
  enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};
  /*•从状态机状态（解析行）
每行解析可能的状态：
◦`LINE_OK`：完整读取一行。
◦`LINE_BAD`：行语法错误。
◦`LINE_OPEN`：行数据不完整。*/
    enum LINE_STATUS{LINE_OK=0,LINE_BAD,LINE_OPEN};

    HTTP_CODE process_read();//读取报文，返回HTTP_CODE
    HTTP_CODE parse_request_line(char *text);//解析请求首行
    HTTP_CODE parse_headers(char *text);//解析请求头
    HTTP_CODE parse_content(char *text);//解析请求体
    HTTP_CODE parse_line(char *text);//解析请求行


private:
   

int m_sockfd;//当前连接的fd值
struct sockaddr_in m_addr;//客户端地址信息
char m_read_buf[read_buffer_size];//读取缓冲区
int m_read_index = 0;//标识已读取客户端数据的最后一个字节的下一个位置

char m_write_buf[write_buffer_size];//写入缓冲区
int m_write_index = 0;//标识已写入数据的最后一个字节的下一个位置

int m_checked_index;//当前分析字母在缓冲区的位置
int m_start_line;//当前解析行的起始位置

CHECK_STATE m_check_state;//主状态机当前所处的位置

//内联函数返回当前起始行位置
char* get_line(){return m_read_buf+m_start_line;}
void init();//初始化

};
