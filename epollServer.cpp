/*
File Name: epollServer.c
Author: tz116
Date: 2021-11-16
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAXEVENTS 100

// ��socket����Ϊ�������ķ�ʽ��
int setnonblocking(int sockfd);
int initServer(int port);
int checkInfds(int infds);
int addClient(int listensock);
int main(int argc, char** argv){
	if (argc != 2){
		printf("Server usage:./tcpepoll port\n"); return -1;
	}
	// ��ʼ����������ڼ�����socket��
	int listensock = initServer(atoi(argv[1]));
	if (listensock < 0){
		printf("Server initserver() failed.\n"); 
		return -1;
	}
	printf("Server listensock=%d\n", listensock);
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	// ����һ��������
	int  epollfd = epoll_create(1);
	// ��Ӽ����������¼�
	struct epoll_event ev;
	ev.data.fd = listensock;
	ev.events = EPOLLIN;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev);
	while (1){
		struct epoll_event events[MAXEVENTS]; // ������¼������Ľṹ���顣
		// �ȴ����ӵ�socket���¼�������
		int infds = epoll_wait(epollfd, events, MAXEVENTS, -1);
		// printf("epoll_wait infds=%d\n",infds);
		// ����ʧ�ܡ�
		if (checkInfds(infds) < 0) break;
		// �������¼������Ľṹ���顣
		for (int ii = 0; ii < infds; ii++){
			if ((events[ii].data.fd == listensock) && (events[ii].events & EPOLLIN)){
				// ��������¼�����listensock����ʾ���µĿͻ�����������
				int clientsock = addClient(listensock);
				if (clientsock < 0){
					printf("Server accept() failed.\n"); 
					continue;
				}
				// ���µĿͻ�����ӵ�epoll��
				memset(&ev, 0, sizeof(struct epoll_event));
				ev.data.fd = clientsock;
				ev.events = EPOLLIN;
				epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);
			}
			else if (events[ii].events & EPOLLIN){
				// �ͻ��������ݹ�����ͻ��˵�socket���ӱ��Ͽ���
				char buffer[1024];
				memset(buffer, 0, sizeof(buffer));
				// ��ȡ�ͻ��˵����ݡ�
				ssize_t isize = read(events[ii].data.fd, buffer, sizeof(buffer));
				// �����˴����socket���Է��رա�
				if (isize <= 0){
					printf("Server client(eventfd=%d) disconnected.\n", events[ii].data.fd);

					// ���ѶϿ��Ŀͻ��˴�epoll��ɾ����
					memset(&ev, 0, sizeof(struct epoll_event));
					ev.events = EPOLLIN;
					ev.data.fd = events[ii].data.fd;
					epoll_ctl(epollfd, EPOLL_CTL_DEL, events[ii].data.fd, &ev);
					close(events[ii].data.fd);
					continue;
				}
				printf("Server recv(eventfd=%d,size=%d):%s\n", events[ii].data.fd, isize, buffer);
				// ���յ��ı��ķ��ظ��ͻ��ˡ�
				write(events[ii].data.fd, buffer, strlen(buffer));
			}
		}
	}
	close(epollfd);
	return 0;
}



// ��ʼ������˵ļ����˿ڡ�
int initServer(int port){
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("Server socket() failed.\n");
		return -1;
	}

	int opt = 1; unsigned int len = sizeof(opt);
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, len);
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("Server bind() failed.\n"); close(sock);
		return -1;
	}

	if (listen(sock, 5) != 0) {
		printf("Server listen() failed.\n"); close(sock);
		return -1;
	}
	return sock;
}

// ��socket����Ϊ�������ķ�ʽ��
int setnonblocking(int sockfd){
	if (fcntl(sockfd,F_SETFL ,fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1)
		return -1;
	return 0;
}

int checkInfds(int infds) {
	// printf("Infds=%d\n",infds)
	// ����ʧ�ܡ�
	if (infds < 0) {
		printf("Server select()/poll() failed.\n"); perror("select()"); return -1;
	}
	// ��ʱ
	else if (infds == 0) {
		printf("Server select()/poll() timeout.\n"); return -2;
	}
	return 0;
}

int addClient(int listensock) {
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	int clientsock = accept(listensock, (struct sockaddr*)&client_addr, &len);
	if (clientsock < 0) {
		printf("Server accept() failed.\n");
		return clientsock;
	}

	printf("Server client(socket=%d),%s:%d connected ok.\n", clientsock, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return clientsock;
}