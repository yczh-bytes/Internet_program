#include "http_conn.h"

int http_conn::m_epollfd = -1;
int http_conn::m_users = 0;
const char http_conn::doc_root[] = "./www";

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
     init();
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
  

bool http_conn::add_response(const char* fmt,...)
{
    //计算剩余写入空间
    int write_space = write_buffer_size - m_write_index-1;//1给'\0'结尾
    if(write_space<0)
    {
        return false;
    }

    //格式化字符串到缓冲区尾部
    va_list args;
    va_start(args,fmt);

    //实际写入
    int true_write = vsnprintf(m_write_buf+m_write_index,write_space,fmt,args);
    va_end(args);

    //检查是否溢出
    if(true_write < 0 || true_write >= write_space)
    {
        m_write_index = write_buffer_size - 1;
        m_write_buf[m_write_index]='\0';
        return false;
    }

    //成功，更新索引
    m_write_index += true_write;
    return true;
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

    //根据结果构建响应
    switch(ret)
    {
        case NO_REQUEST://请求不完整
    {
        modfd(m_sockfd,m_epollfd,EPOLLIN);
        return;
    }

    case BAD_REQUEST://解析出错
    {
        response_400();
        modfd(m_sockfd,m_epollfd,EPOLLOUT);
        return;
    }

    case NO_RESOURCE://文件不存在
    {
        response_404();
        modfd(m_sockfd,m_epollfd,EPOLLOUT);
        return;
    }

    case FORBIDDEN_REQUEST://无访问权限
    {
        response_403();
        modfd(m_sockfd,m_epollfd,EPOLLOUT);
        return;
    }
    case INTERNAL_ERROR://服务器内部错误
    {
        response_500();
        modfd(m_sockfd,m_epollfd,EPOLLOUT);
        return;
    }

    case FILE_REQUEST://静态文件请求
    {
        response_200();
        modfd(m_sockfd,m_epollfd,EPOLLOUT);
        return;
    }

    default:
    {
        close_conn();
        return ;
    }
}
}

    
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;

    while((m_check_state==CHECK_STATE_CONTENT&&line_status==LINE_OK)||(line_status=parse_line())==LINE_OK)
    {
        text = get_line();//获取当前行起始位置
        m_start_line = m_checked_index;//更新当前检查索引

        switch(m_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parse_request_line(text);
                if(ret==BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = parse_headers(text);
                if(ret==BAD_REQUEST)
                {   
                    return BAD_REQUEST;
                }
                else if(ret == GET_REQUEST)
                {
                   return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = parse_content(text);
                if(ret == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                else if(ret == GET_REQUEST)
                {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }

    }
    return NO_REQUEST;
}

// 从状态机：解析一行（以 \r\n 结尾）
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    // m_checked_index 指向当前正在分析的字节位置
    for(; m_checked_index < m_read_index; ++m_checked_index)
    {
        temp = m_read_buf[m_checked_index];

        if(temp == '\r')
        {
            // 当前字符是'\r'，需要检查下一个字符是否是'\n'
            if(m_checked_index + 1 == m_read_index)
            {
                // '\r'是最后一个已接收的字符，行不完整，需要继续读取
                return LINE_OPEN;
            }
            else if(m_read_buf[m_checked_index + 1] == '\n')
            {
                // 找到完整的 \r\n，将其替换为 \0\0 便于后续处理
                m_read_buf[m_checked_index++] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            // '\r'后面不是'\n'，行语法错误
            return LINE_BAD;
        }
        else if(temp == '\n')
        {
            // 单独出现'\n'（没有前面的'\r'）
            if(m_checked_index > 0 && m_read_buf[m_checked_index - 1] == '\r')
            {
                // 已经在上一个循环处理过 \r\n 的情况，这里应该不会进入
                // 但如果进入说明前一个字符已经检查过是\r，此时替换
                m_read_buf[m_checked_index - 1] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK;
            }
            // 单独的'\n'，考虑到部分客户端可能只发\n，可以宽容处理
            m_read_buf[m_checked_index++] = '\0';
            return LINE_OK;
        }
    }
    // 已检查完所有已读取数据，没有找到行结束符，需要继续读取
    return LINE_OPEN;
}

// 用正则表达式解析请求行: METHOD SP URL SP VERSION
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    // 正则: 大写方法 + 空格 + 非空URL + 空格 + HTTP/x.x
    std::regex re(R"(^([A-Z]+) ([^ ]+) (HTTP/\d+\.\d+)$)");
    std::cmatch m;
    
    if(!std::regex_match(text, m, re))
        return BAD_REQUEST;
    
    std::string method_str = m[1].str();
    std::string url_str    = m[2].str();
    std::string ver_str    = m[3].str();
    
    // METHOD 字符串 → 枚举映射
    if(method_str == "GET")       m_method = GET;
    else if(method_str == "POST")     m_method = POST;
    else if(method_str == "HEAD")     m_method = HEAD;
    else if(method_str == "PUT")      m_method = PUT;
    else if(method_str == "DELETE")   m_method = DELETE;
    else if(method_str == "TRACE")    m_method = TRACE;
    else if(method_str == "OPTIONS")  m_method = OPTIONS;
    else if(method_str == "CONNECT")  m_method = CONNECT;
    else return BAD_REQUEST;
    
    // URL 长度检查
    if(url_str.size() >= sizeof(m_url))
        return BAD_REQUEST;
    strncpy(m_url, url_str.c_str(), sizeof(m_url) - 1);
    
    // 版本字符串存储
    if(ver_str.size() >= sizeof(m_version))
        return BAD_REQUEST;
    strncpy(m_version, ver_str.c_str(), sizeof(m_version) - 1);
    
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// 用正则表达式解析头部字段或空行
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    // 空行 → 头部结束
    if(*text == '\0')
    {
        if(m_content_length > 0)
        {
            // 有请求体, 记录起始位置, 转入 CONTENT 状态
            m_body_start = m_checked_index;
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 无请求体, 请求解析完成
        return GET_REQUEST;
    }
    
    // 匹配: Field-Name: value
    std::regex re(R"(^([[:alnum:]_-]+):\s*(.*?)\s*$)");
    std::cmatch m;
    
    if(!std::regex_match(text, m, re))
        return BAD_REQUEST;
    
    std::string name  = m[1].str();
    std::string value = m[2].str();
    
    // 字段名转小写以便比较
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    if(name == "host")
    {
        if(value.size() >= sizeof(m_host))
            return BAD_REQUEST;
        strncpy(m_host, value.c_str(), sizeof(m_host) - 1);
    }
    else if(name == "content-length")
    {
        try {
            m_content_length = std::stoi(value);
            if(m_content_length < 0) return BAD_REQUEST;
        } catch(...) {
            return BAD_REQUEST;
        }
    }
    else if(name == "connection")
    {
        std::string val_lower = value;
        std::transform(val_lower.begin(), val_lower.end(), val_lower.begin(), ::tolower);
        if(val_lower == "keep-alive")
            m_keepalive = true;
    }
    // 其他头部字段忽略
    
    return NO_REQUEST;
}

// 判断请求体是否接收完整
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    int received = m_read_index - m_body_start;
    if(received >= m_content_length)
        return GET_REQUEST;
    return NO_REQUEST;
}

void http_conn::init()
{
    m_checked_index = 0;
    m_start_line = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_read_index = 0;
    m_write_index = 0;
    // 初始化解析结果成员
    m_method = GET;
    memset(m_url, 0, sizeof(m_url));
    memset(m_version, 0, sizeof(m_version));
    memset(m_host, 0, sizeof(m_host));
    m_content_length = 0;
    m_body_start = 0;
    m_keepalive = false;

    memset(m_real_file, 0, sizeof(m_real_file));
    memset(&m_file_stat, 0, sizeof(m_file_stat));
}

http_conn::HTTP_CODE http_conn::do_request()
{
    // 1. 拼接文件路径: doc_root + url
    strcpy(m_real_file, doc_root);
    strcat(m_real_file, m_url);

    // 2. 如果 URL 以 '/' 结尾，默认补 index.html
    size_t url_len = strlen(m_url);
    if(url_len > 0 && m_url[url_len - 1] == '/')
    {
        strcat(m_real_file, "index.html");
    }
    // 如果 URL 就是 "/"，也指向 index.html
    else if(url_len == 0 || (url_len == 1 && m_url[0] == '/'))
    {
        strcat(m_real_file, "/index.html");
    }

    // 3. stat 检查文件
    int ret = stat(m_real_file, &m_file_stat);
    if(ret == -1)
    {
        if(errno == ENOENT)
            return NO_RESOURCE;
        if(errno == EACCES)
            return FORBIDDEN_REQUEST;
        return INTERNAL_ERROR;
    }

    // 4. 目录检查
    if(S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST;
    }

    // 5. 只支持 GET 和 HEAD
    if(m_method != GET && m_method != HEAD)
        return BAD_REQUEST;

    // 6. 成功
    return FILE_REQUEST;
}

void http_conn::response_200()
{
    FILE* fp = fopen(m_real_file, "rb");
    if(!fp)
    {
        response_500();
        return;
    }
    
    // 构建响应头
    add_response("HTTP/1.1 200 %s\r\n", ok_200_title);
    add_response("Content-Type: text/html\r\n");
    add_response("Connection: close\r\n");
    add_response("\r\n");
    
    // 读取文件内容到缓冲区
    int space = write_buffer_size - m_write_index - 1;
    if(space > 0)
    {
        size_t n = fread(m_write_buf + m_write_index, 1, space, fp);
        m_write_index += n;
    }
    fclose(fp);
}

void http_conn::response_400()
{
    add_response("HTTP/1.1 400 %s\r\n", error_400_title);
    add_response("Content-Type: text/html\r\n");
    add_response("Content-Length: %d\r\n", (int)strlen(error_400_form));
    add_response("Connection: close\r\n");
    add_response("\r\n");
    add_response("%s", error_400_form);
}

void http_conn::response_403()
{
    add_response("HTTP/1.1 403 %s\r\n", error_403_title);
    add_response("Content-Type: text/html\r\n");
    add_response("Content-Length: %d\r\n", (int)strlen(error_403_form));
    add_response("Connection: close\r\n");
    add_response("\r\n");
    add_response("%s", error_403_form);
}

void http_conn::response_404()
{
    add_response("HTTP/1.1 404 %s\r\n", error_404_title);
    add_response("Content-Type: text/html\r\n");
    add_response("Content-Length: %d\r\n", (int)strlen(error_404_form));
    add_response("Connection: close\r\n");
    add_response("\r\n");
    add_response("%s", error_404_form);
}

void http_conn::response_500()
{
    add_response("HTTP/1.1 500 %s\r\n", error_500_title);
    add_response("Content-Type: text/html\r\n");
    add_response("Content-Length: %d\r\n", (int)strlen(error_500_form));
    add_response("Connection: close\r\n");
    add_response("\r\n");
    add_response("%s", error_500_form);
}
