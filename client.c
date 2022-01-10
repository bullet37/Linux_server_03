/* 
File Name: client.c 
Author: tz116
Date: 2021-11-16
*/

#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<unistd.h>
#include<arpa/inet.h>
#include<iostream>

#pragma part(1)//使得struct结构不再填充
#define MAXLINE 4096  
#define DEFAULT_PORT 8000  


int MySend(int socketfd, char* buf, size_t len) {
    if (len == 0) return 0;
    unsigned int sended = 0;
    int ret;
    while (sended<len)
    {
        do {
            ret = send(socketfd, buf, len - sended, 0);
        } while (ret < 0 && errno == EINTR);
        if (ret < 0) return sended;
        sended += ret;
        buf += ret;
    }
    return(len);
}

int main(int argc, char** argv)
{
    int    sockfd, n, rec_len;
    char    recvline[4096], sendline[4096];
  
    struct sockaddr_in    servaddr;


    if (argc != 2) {
        printf("Client usage: ./client <ipaddress>\n");
        exit(0);
    }


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Client create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(DEFAULT_PORT);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("Client inet_pton error for %s\n", argv[1]);
        exit(0);
    }


    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Client connect error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }


    printf("Client send msg to server: \n");
    fgets(sendline, 4096, stdin);
    // 在发送信息前加上发送信息的长度防止粘包
    int len = strlen(sendline);
    
    printf("Client send length:%d\n", len);

    char* pBuff = new char[100];
    *(int*)(pBuff) = htonl(len);
    memcpy(pBuff + sizeof(int), sendline, len);
    len += sizeof(int);
    int writelen = 0;
    if ((writelen = MySend(sockfd, pBuff, len)) < 0)
    {
        printf("Client send msg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    else printf("Client Write sucess, writelen: %d\n", writelen);

    //char    buf[MAXLINE];
    //if ((rec_len = recv(sockfd, buf, MAXLINE, 0)) == -1) {
    //    perror("recv error");
    //    exit(1);
    //}
    //buf[rec_len] = '\0';
    //printf("Received : %s ", buf);

    close(sockfd);
    return 0;
}