#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
int main()
{
    //创建socket
    int fd = socket(AF_INET,SOCK_DGRAM,0);

    if(fd == -1)
    {
        perror("socket");
        exit(1);
    }

    //发送信息
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);

    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
    char buf[1024];

    while(1)
    {
        
        fgets(buf,sizeof(buf),stdin);
        sendto(fd,buf,strlen(buf),0,(struct sockaddr*)&addr,sizeof(addr));

        memset(buf,0,sizeof(buf));

        socklen_t len = sizeof(addr);
        int rec = recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr*)&addr,&len);

        printf("server say:%s",buf);
    }

    close(fd);
    return 0;
}