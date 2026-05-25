#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
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

    //创建一个fd_set集合，记录文件描述符
    fd_set reset,tmp;
    FD_ZERO(&reset);
    FD_SET(lfd,&reset);
    FD_ZERO(&tmp);

    int maxfd = lfd;
    while(1)
    {
        tmp = reset;
        //select
        int ret = select(maxfd+1,&tmp,NULL,NULL,NULL);
        if(ret == -1)
        {
            perror("select");
            exit(1);
        }
        else if(ret == 0)
        {
            continue;
        }
        else if(ret>0)
        {
            //说明记录到了文件描述符缓冲期=区的数据发生了变化
            if(FD_ISSET(lfd,&tmp))
            {
                //说明有客户端连接进来了
                //接受连接
                struct sockaddr_in cliaddr;
                socklen_t len = sizeof(cliaddr);
                int cfd = accept(lfd,(struct sockaddr*)&cliaddr,&len);
                if(cfd == -1)
                {
                    perror("accept");
                    continue;
                }
                //将cfd添加到reset集合中
                FD_SET(cfd,&reset);
                //更新maxfd
                maxfd = maxfd>cfd?maxfd:cfd;
                printf("new client connected, fd=%d\n",cfd);
            }
            
            for(int i = lfd+1;i<=maxfd;i++)
            {
                if(FD_ISSET(i,&tmp))
                {
                    //说明这个文件描述符的客户端发来了数据
                    char buf[1024] = {0};
                    int re = read(i,buf,sizeof(buf));
                    if(re == -1)
                    {
                        perror("read");
                        close(i);
                        FD_CLR(i,&reset);
                    }
                    else if(re == 0)
                    {
                        printf("client close, fd=%d\n",i);
                        close(i);
                        FD_CLR(i,&reset);
                    }
                    else
                    {
                        //接收客户端发来的信息
                        printf("client say:%s\n",buf);
                        //回复客户端
                        write(i,buf,re);
                    }
                }
            }
            
        }
    }
    close(lfd);

    return 0;
}