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
    //连接
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
     addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9999);
    int co = connect(fd,(struct sockaddr *)&addr,sizeof(addr));
    if(co==-1)
    {
        perror("connect");
        exit(1);
    }

    //发送信息
    char *msg = "hello server";
    write(fd,msg,strlen(msg));
    //接收信息
    int *buf[1024];
    read(fd,buf,sizeof(buf));
    //输出服务端信息
    printf("服务端信息：%s\n",buf);
    //关闭连接
    close(fd);
   
    return 0;
}