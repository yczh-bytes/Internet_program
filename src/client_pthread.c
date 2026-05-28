#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>


//封装一个结构体，存储服务端信息，用以传入线程函数
struct clientInfo
{
    int fd;
    pthread_t pid;
    struct sockaddr_in addr;
};

//线程函数
void * working(void *arg)
{
    struct clientInfo *info = (struct clientInfo*)arg;
    //输出打印相关信息
    char ip[32];
    char *port = inet_ntop(AF_INET,&info->addr.sin_addr,ip,sizeof(ip));
    printf("client ip:%s port:%d\n",ip,ntohs(info->addr.sin_port));
    char buf[1024];

    while(1)
    {
        memset(buf,0,sizeof(buf));
        int re = read(info->fd,buf,sizeof(buf)-1);
        if(re==-1)
        {
            perror("read");
            break;
        }
        else if(re==0)
        {
            printf("client close\n");
            break;
        }
        buf[re]='\0';
        printf("client msg:%s\n",buf);
        // 回显
        write(info->fd,buf,strlen(buf));
        sleep(1);
        
    }

    close(info->fd);
    return NULL;
}
int main()
{
    //创建端口复用
    int opt = 1;//赋值为1传入参数代表开启端口复用
   
    // 创建socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd == -1)
    {
        perror("socket");
        exit(1);
    }

     //设置端口复用
    if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1)
    {
        perror("setsockopt");
        exit(1);
    }

    // 地址结构体
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9999);

    // 绑定
    if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // 监听
    if(listen(fd, 10) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("server start...\n");

    //接收连接实现线程连接
    while(1)
    {
        struct clientInfo info;
        socklen_t len = sizeof(info.addr);
        info.fd = accept(fd,(struct sockaddr*)&info.addr,&len);
        if(info.fd==-1)
        {
            perror("accept");
            continue;
        }
        // 创建线程
        if(pthread_create(&info.pid,NULL,working,&info)==-1)
        {
            perror("pthread_create");
            continue;
        }
        //分离线程
        pthread_detach(info.pid);
    }
    //关闭连接
    close(fd);
    return 0;
}