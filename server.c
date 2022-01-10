/* 
File Name: server.c 
Author: tz116
Date: 2021-11-16
*/

#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<unistd.h>
#include<sys/types.h>  
#include<sys/socket.h>  // bind()等函数与socket宏
#include<netinet/in.h>  // socketaddr_in 需要

#define DEFAULT_PORT 8000  
#define MAXLINE 4096  

int MyRecv(int socketfd, char* buf, size_t len) {
    unsigned int recved = 0;
    int ret;
    while (recved < len)
    {
        do {
            ret = read(socketfd, buf, len - recved);
        } while (ret < 0 && errno == EINTR);
        if (ret < 0) return ret;
        else if (ret == 0) return recved;
        recved += ret;
        buf += ret;
    }
}
int main(int argc, char** argv)
{
    int    socket_fd, connect_fd;
    struct sockaddr_in     servaddr;
    char    buff[4096];
    int     n;
    //初始化Socket  
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Server create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    //初始化  
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址,适用于多个网卡  
    servaddr.sin_port = htons(DEFAULT_PORT);//设置的端口为DEFAULT_PORT  

    //将本地地址绑定到所创建的套接字上  
    if (bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        printf("Server bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    //开始监听是否有客户端连接,排队数最多为10 
    if (listen(socket_fd, 10) == -1) {
        printf("Server listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    printf("======Server waiting for client's request======\n");
    while (1) {
        //阻塞直到有客户端连接，不然多浪费CPU资源。  
        // accept默认会阻塞进程
        if ((connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1) {
            printf("Server accept socket error: %s(errno: %d)", strerror(errno), errno);
            continue;
        }
        //先接受一个整数确定包长度  
        if ((n = MyRecv(connect_fd, buff, sizeof(int))) < 0) {
            printf("Server Read error\n");
            exit(-1);
        }
        int len = (int)ntohl(*(int*)buff);
        printf("Server recived len:%d\n", len);
        if ((n = MyRecv(connect_fd, buff, len)) < 0) {
            exit(-1);
        }

        //向客户端发送回应数据  
        // 创建子进程，向链接套接字发送信息
  /*      if (!fork()) { 
            if (send(connect_fd, "Hello,you are connected!\n", 26, 0) == -1)
                perror("send error");
            close(connect_fd);
            exit(0);
        }*/
        buff[len] = '\0';
        printf("Server recieve msg from client: %s\n", buff);
        close(connect_fd);
    }
    close(socket_fd);
    return 0;
}
//all:server client
//server: server.o
//        g++ -g -o server server.o
//client: client.o
//        g++ -g -o client client.o
//server.o: server.cpp
//        g++ -g -c server.cpp
//client.o: client.cpp
//        g++ -g -c client.cpp
//clean: all
//        rm all