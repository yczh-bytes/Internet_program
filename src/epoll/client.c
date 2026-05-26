#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);

    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("connect");
        exit(1);
    }

    printf("客户端已连接到服务器\n");

    int counter = 0;
    char buffer[1024];

    while(1)
    {
        sprintf(buffer, "data %d", counter);
        write(fd, buffer, strlen(buffer));
        printf("发送: %s\n", buffer);
        
        counter++;
        if(counter > 100)
        {
            counter = 0;
        }
        
        sleep(1);
    }

    close(fd);
    return 0;
}