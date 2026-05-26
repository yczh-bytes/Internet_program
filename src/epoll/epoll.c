#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>
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

    //创建epoll
    int efd = epoll_create(1);
    if(efd == -1)
    {
        perror("epoll_create");
        exit(1);
    }
    //添加lfd到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;

    ret = epoll_ctl(efd,EPOLL_CTL_ADD,lfd,&ev);
    if(ret == -1)
    {
        perror("epoll_ctl");
        exit(1);
    }

    while(1)
    {
        struct epoll_event evs[1024];

        int nfds = epoll_wait(efd,evs,1024,-1);//一直阻塞等待，返回的是新进来的连接数
        if(nfds == -1)
        {
            perror("epoll_wait");
            exit(1);
        }

        printf("nfds = %d\n",nfds);

        for(int i = 0;i<nfds;i++)
        {
            if(evs[i].data.fd == lfd)//说明有链接进来了
            {
                printf("lfd accept\n");
                int afd = accept(lfd,NULL,NULL);

                //将afd添加的epoll
                ev.events = EPOLLIN;
                ev.data.fd = afd;

                ret = epoll_ctl(efd,EPOLL_CTL_ADD,afd,&ev);
                if(ret == -1)
                {
                    perror("epoll_ctl");
                    exit(1);
                }
            }
            else
            {
                char buf[1024];
                int re = read(evs[i].data.fd,buf,sizeof(buf));

                if(re == -1)
                {
                    close(evs[i].data.fd);
                    ret = epoll_ctl(efd,EPOLL_CTL_DEL,evs[i].data.fd,NULL);
                    if(ret == -1)
                    {
                        perror("epoll_ctl");
                        exit(1);
                    }
                }

                else
                {
                    printf("read %s\n",buf);
                    write(evs[i].data.fd,buf,re);
                }
            }
        }
    }

    return 0;
}
