#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    //创建socket
    int lfd = socket(AF_INET,SOCK_STREAM,0);

    if(lfd == -1)
    {
        perror("socket");
        exit(1);
    }
    //绑定
    struct sockaddr_in addr;
    addr.sin_port = htons(9999);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        exit(1);
    }

    //监听
    ret = listen(lfd, 10);
    if(ret == -1)
    {
        perror("listen");
        exit(1);
    }

    //创建poll结构体
    struct pollfd pfds[1024];
    for(int i = 0;i<1024;i++)
    {
        pfds[i].fd = -1;
        pfds[i].events = 0;
    }
    pfds[0].fd = lfd;
    pfds[0].events = POLLIN;

    int nfds = 0;
    while(1)
    {
       //poll
       int ret = poll(pfds,nfds+1,-1);
       if(ret == -1)
       {
           perror("poll");
           exit(1);
       }
        else if(ret>0)
        {
            for(int i=0;i<=nfds;i++)
            {
                if(pfds[i].fd == -1)
                {
                    continue;
                }
                
                if(pfds[i].fd == lfd && (pfds[i].revents & POLLIN))
                {
                    struct sockaddr_in cliaddr;
                    socklen_t len = sizeof(cliaddr);
                    int cfd = accept(lfd,(struct sockaddr*)&cliaddr,&len);
                    if(cfd == -1)
                    {
                        perror("accept");
                        continue;
                    }
                    
                    int j;
                    for(j=1;j<1024;j++)
                    {
                        if(pfds[j].fd == -1)
                        {
                            pfds[j].fd = cfd;//将客户端加入到poll数组当中
                            pfds[j].events = POLLIN;
                            break;
                        }
                    }
                    
                    if(j > nfds)
                    {
                        nfds = j;
                    }
                    
                    printf("new client connected, fd=%d\n",cfd);
                }
                else if(pfds[i].fd != lfd && (pfds[i].revents & POLLIN))
                {
                    char buf[1024] = {0};
                    int re = read(pfds[i].fd,buf,sizeof(buf));
                    if(re == -1)
                    {
                        perror("read");
                        close(pfds[i].fd);
                        pfds[i].fd = -1;
                        continue;
                    }
                    else if(re == 0)
                    {
                        printf("client close, fd=%d\n",pfds[i].fd);
                        close(pfds[i].fd);
                        pfds[i].fd = -1;
                    }
                    else
                    {
                        printf("client say:%s\n",buf);
                        write(pfds[i].fd,buf,re);
                    }
                }
            }
        }
    }
    close(lfd);

    return 0;
}