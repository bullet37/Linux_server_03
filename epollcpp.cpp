/*
File Name: epoll.c
Author: tz116
Date: 2021-11-16
An example for epoll() client
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

#define MAXSIZE 1024
#define EPOLLEVENTS 20

void handle_events(int epollfd, struct epoll_event* events, int num, int sockfd, char* buf) {
	int fd;
	for (int i = 0; i < num; ++i) {
		fd = events[i].data.fd;
		if (events[i].events & EPOLLIN)
			do_read(epollfd, fd, sockfd, buf);
		else if (events[i].events & EPOLLOUT)
			do_write(epollfd, fd, sockfd, buf);
	}
}

void do_read(int epollfd, int fd, int sockfd, char* buf) {
	int nread;
	nread = read(fd, buf, MAXSIZE);
	if (nread == -1) {
		perror("read error:");
		close(fd);
	}
	else if (nread == 0) {
		fprintf(stderr, "server close.\n");
		close(fd);
	}
	else {
		// 若是标准输入读来的数据接下来要发往server
		if (fd == STDIN_FILENO)
			add_event(epollfd, sockfd, EPOLLOUT);
		// 若是server的数据，就输出到stdout,并删除读事件
		else {
			delete_event(epollfd, sockfd, EPOLLIN);
			add_event(epollfd, STDOUT_FILENO, EPOLLOUT);
		}
	}
}
void do_write(int epollfd, int fd, int sockfd, char* buf) {
	int nwrite;
	char temp[100];
	buf[strlen(buf) - 1] = '\0';
	snprintf(temp, sizeof(temp), "%s_%02d\n", buf, count++);
	nwrite = write(fd, temp, strlen(temp));
	if (nwrite == -1) {
		perror("write error:");
		close(fd);
	}
	else {
		if (fd == STDIN_FILENO)
			delete_event(epollfd, sockfd, EPOLLOUT);
		else 
			modify_event(epollfd, sockfd, EPOLLIN);
	}
	memset(buf, 0, MAXSIZE);
}

void add_event(int epollfd, int fd, int state) {
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTR_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state) {
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTR_DEL, fd, &ev);
}

void modify_event(int epollfd, int fd, int state) {
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTR_MOD, fd, &ev);
}