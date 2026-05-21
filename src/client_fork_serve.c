#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

int main()
{
    // 回收子进程
    signal(SIGCHLD, SIG_IGN);

    // 创建socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd == -1)
    {
        perror("socket");
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

    while(1)
    {
        struct sockaddr_in client_addr;

        socklen_t len = sizeof(client_addr);

        int client_fd = accept(fd,
                              (struct sockaddr*)&client_addr,
                              &len);

        if(client_fd == -1)
        {
            perror("accept");
            continue;
        }

        // 创建子进程
        pid_t pid = fork();

        if(pid == -1)
        {
            perror("fork");
            close(client_fd);
            continue;
        }

        // 子进程
        if(pid == 0)
        {
            close(fd);

            char ip[32];

            inet_ntop(AF_INET,
                      &client_addr.sin_addr,
                      ip,
                      sizeof(ip));

            printf("client ip:%s port:%d\n",
                   ip,
                   ntohs(client_addr.sin_port));

            char buf[1024];

            while(1)
            {
                int re = read(client_fd,
                             buf,
                             sizeof(buf)-1);

                if(re == -1)
                {
                    perror("read");
                    break;
                }
                else if(re == 0)
                {
                    printf("client close\n");
                    break;
                }

                buf[re] = '\0';

                printf("client msg:%s\n", buf);

                // 回显
                write(client_fd, buf, strlen(buf));
            }

            close(client_fd);

            exit(0);
        }

        // 父进程
        else
        {
            close(client_fd);
        }
    }

    close(fd);

    return 0;
}