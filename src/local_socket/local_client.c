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
    int lfd = socket(AF_LOCAL,SOCK_STREAM,0);
    if(lfd == -1)
    {
        perror("socket");
        exit(1);
    }   

    
    //连接
    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    memset(&addr,0,sizeof(addr));
    strcpy(addr.sun_path,"serve.sock");
    int ret = connect(lfd,(struct sockaddr*)&addr,sizeof(addr));
    if(ret == -1)
    {
        perror("connect");
        exit(1);
    }

    char buf[1024];
    //发送信息
    while(1)
    {
        fgets(buf,sizeof(buf),stdin);
        send(lfd,buf,strlen(buf),0);
        memset(buf,0,sizeof(buf));
        int rec = recv(lfd,buf,sizeof(buf),0);
        if(rec>0)
        {
            printf("server say:%s",buf);
        }
    }
    close(lfd);
    return 0;
}