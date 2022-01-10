/*
File Name: IOserver.c
Author: tz116
Date: 2021-11-16
Test for select() and poll()
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <poll.h>

#define MAXNFDS 1024
int initServer(int port);
int checkInfds(int infds);
ssize_t writeToClient(int eventfd);
int addClient(int listensock);

int main(int argc, char* argv[])
{
	if (argc != 2){
		printf("Server usage: ./server port\n"); return -1;
	}
	// 初始化服务端用于监听的socket。
	int listensock = initServer(atoi(argv[1]));
	printf("Server listensock=%d\n", listensock);
	if (listensock < 0){
		printf("Server initserver() failed.\n"); return -1;
	}
	int maxfd = listensock; // ocket的最大值。

	// test for select()
	// 初始化fd_set，把listensock添加到集合中。
	//fd_set readfdset; // 读事件的集合，包括监听socket和客户端连接上来的socket。
	//FD_ZERO(&readfdset);
	//FD_SET(listensock, &readfdset);
	

	//while (1) {
	//	fd_set tmpfdset = readfdset;
	//	// 调用select函数时，会改变socket集合的内容，所以要把socket集合保存下来，传一个临时的给select。
	//	int infds = select(maxfd + 1, &tmpfdset, NULL, NULL, NULL);
	//			int ret = checkInfds(infds);
	//			if (ret == -1) break;
	//			else if (ret == -2) continue;

	//	// 检查有事情发生的socket，包括监听和客户端连接的socket。
	//	// 这里是客户端的socket事件，每次都要遍历整个集合，因为可能有多个socket有事件。
	//	for (int eventfd = 0; eventfd <= maxfd; eventfd++)
	//	{
	//		if (FD_ISSET(eventfd, &tmpfdset) <= 0) continue;
	//		if (eventfd == listensock){
	//			// 如果发生事件的是listensock，表示有新的客户端连上来。
	//			int clientsock = addClient(listensock);
	//			if(clientsock <0) continue;
	//			// 把新的客户端socket加入集合。
	//			FD_SET(clientsock, &readfdset);
	//			if (maxfd < clientsock) maxfd = clientsock;
	//		}
	//		else {
	//			// 发生了错误或socket被对方关闭。
	//			if (writeToClient(eventfd) <= 0) {
	//				printf("Server client(eventfd=%d) disconnected.\n", eventfd);
	//				close(eventfd);// 关闭客户端的socket。
	//				FD_CLR(eventfd, &readfdset); // 从集合中移去客户端的socket。
	//				// 重新计算maxfd的值，注意，只有当eventfd==maxfd时才需要计算。
	//				if (eventfd == maxfd) {
	//					for (int ii = maxfd; ii > 0; ii--)
	//						if (FD_ISSET(ii, &readfdset)) {
	//							maxfd = ii; 
	//							break;
	//						}
	//					printf("maxfd=%d\n", maxfd);
	//				}
	//			}
	//		}
	//	}
	//}


	//test for poll()
	struct pollfd fds[MAXNFDS]; // fds存放需要监视的socket
	fds[listensock].fd = listensock;
	fds[listensock].events = POLLIN;
	while (1){
		// 超时时间50s
		int infds = poll(fds, maxfd + 1, 50000);
		int ret = checkInfds(infds);
		if (ret == -1) break;
		else if (ret == -2) continue;

		for (int eventfd = 0; eventfd <= maxfd; eventfd++)
		{
			if (fds[eventfd].fd < 0) continue;
			if ((fds[eventfd].revents & POLLIN) == 0) continue;
			fds[eventfd].revents = 0; //先把revents清空。
			if (eventfd == listensock) {
				//如果发生事件的是listensock，表示有新的客户端连上来。
				int clientsock = addClient(listensock);
				if (clientsock < 0) continue;
				if (clientsock > MAXNFDS) {
					printf("clientsock(%d)>MAXNFDS(%d)\n", clientsock, MAXNFDS);
					close(clientsock);
					continue;
				}
				fds[clientsock].fd = clientsock;
				fds[clientsock].events = POLLIN;
				fds[clientsock].revents = 0;
				if (maxfd < clientsock) maxfd = clientsock;
			}
			else{
				if (writeToClient(eventfd) <= 0) {
				  printf("client(eventfd=%d) disconnected.\n",eventfd);
				  close(eventfd);  // 关闭客户端的socket。
				  fds[eventfd].fd = -1;
				  // 重新计算maxfd的值，注意，只有当eventfd==maxfd时才需要计算。
				  if (eventfd == maxfd){
					for (int ii = maxfd; ii > 0; ii--)
					  if (fds[ii].fd != -1){
						maxfd = ii; 
						break;
					  }
					printf("maxfd=%d\n",maxfd);
				  }
				}
			}
		}
	}
	return 0;
}

// 初始化服务端的监听端口。
int initServer(int port)
{
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
		printf("Server bind() failed.\n"); 
		close(sock); 
		return -1;
	}	
	
	if (listen(sock, 5) != 0){
		printf("Server listen() failed.\n"); 
		close(sock); 
		return -1;
	}
	return sock;
}

int checkInfds(int infds) {
	// printf("Infds=%d\n",infds)
	// 返回失败。
	if (infds < 0) {
		printf("Server select()/poll() failed.\n"); perror("select()"); return -1;
	}
	// 超时
	else if (infds == 0) {
		printf("Server select()/poll() timeout.\n"); return -2;
	}
	return 0;
}
// 返回是否要断开链接
ssize_t writeToClient(int eventfd) {
	// 客户端有数据过来或客户端的socket连接被断开。
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	ssize_t ret;
	// 读取客户端的数据。
	if ((ret = read(eventfd, buffer, sizeof(buffer))) > 0) {
		printf("Server recv(eventfd=%d,size=%zu):%s\n", eventfd, ret, buffer);
		// 把收到的报文发回给客户端。
		write(eventfd, buffer, strlen(buffer));
	}
	return ret;
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