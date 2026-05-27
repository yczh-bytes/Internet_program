#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    //创建socket
    int fd = socket(AF_INET,SOCK_DGRAM,0);

    if(fd == -1)
    {
        perror("socket");
        exit(1);
    }

    //绑定地址
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9999);
    int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));

    char buf[1024];

    while(1)
    {
        memset(buf,0,sizeof(buf));
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
    
        int rec = recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr*)&clientaddr,&len);

        if(rec>0)
        {
            printf("client say:%s",buf);
        }

        sendto(fd,"hello client",14,0,(struct sockaddr*)&clientaddr,len);
    }
    close(fd);

    return 0;
}