#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include  <arpa/inet.h>
#include <stdio.h>
#include <string.h>

int main()
{
    //创建socket关键字
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1)
    {
        perror("socket");
        exit(1);
    }
    //绑定地址
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9999);
    int bd = bind(fd,(struct sockaddr *)&addr,sizeof(addr));

    if(bd==-1)
    {
        perror("bind");
        exit(1);
    }
    //监听
    int ls = listen(fd,10);
    if(ls==-1)
    {
        perror("listen");
        exit(1);
    }

    //接收客户端连接
    int fd2 = accept(fd,NULL,NULL);
    if(fd2==-1)
    {
        perror("accept");
        exit(1);
    }

    //输出服务端信息
    printf("服务端地址：%d %d\n",addr.sin_addr.s_addr,addr.sin_port);

    //读取客户端信息
    int *buf[1024];
    read(fd2,buf,sizeof(buf));

    //输出客户端信息
    printf("客户端信息：%s\n",buf);
    //向客户端发送信息
    char *msg = "hello client";
    write(fd2,msg,strlen(msg));

    //关闭连接
    close(fd);
    close(fd2);
    return 0;
}
