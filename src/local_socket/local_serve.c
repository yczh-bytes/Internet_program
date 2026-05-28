#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    //删除可能存在的socket文件
    unlink("serve.sock");
    
    //创建socket
    int lfd = socket(AF_LOCAL,SOCK_STREAM,0);
    if(lfd == -1)
    {
        perror("socket");
        exit(1);
    }

    //绑定地址
    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    memset(&addr,0,sizeof(addr));
    strcpy(addr.sun_path,"serve.sock");
    int ret = bind(lfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        exit(1);
    }

    //监听
    ret = listen(lfd,10);
    if(ret == -1)
    {
        perror("listen");
        exit(1);
    }

    //等待连接
    int fd = accept(lfd,NULL,NULL);
    if(fd == -1)
    {
        perror("accept");
        exit(1);
    }

    int num = 0;
    char buf[1024];
    while(1)
    {
        memset(buf,0,sizeof(buf));
        //通信
        int rec = recv(fd,buf,sizeof(buf),0);
        if(rec>0)
        {
            printf("client say:%s",buf);
            int send_ret = send(fd,"hello client",14,0);
            if(send_ret == -1)
            {
                perror("send");
                break;
            }
        }
        else if(rec == 0)
        {
            printf("client disconnected\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }
    close(fd);
    close(lfd);
    return 0;
}