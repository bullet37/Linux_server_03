#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv)
{
	if (argc != 3){
		printf("Client usage:./client ip port\n"); return -1;
	}
	int sockfd;
	struct sockaddr_in servaddr;
	char buf[1024];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		printf("Client socket() failed.\n"); 
		return -1; 
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	//if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
	//	printf("Client inet_pton error for %s\n", argv[1]);
	//	exit(0);
	//}

	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("Client connect error: %s(errno: %d)\n", strerror(errno), errno);
		close(sockfd);
		exit(0);
	}

	printf("Client connect ok.\n");

	for (int ii = 0; ii < 10; ii++)
	{
		// 从命令行输入内容。
		memset(buf, 0, sizeof(buf));
		printf("Client please input:"); 
		scanf("%s", buf);
		// sprintf(buf,"1111111111111111111111ii=%08d",ii); //模拟write阻塞
		if (write(sockfd, buf, strlen(buf)) <= 0)
		{
			printf("Client write() failed.\n");
			close(sockfd);
			return -1;
		}
		memset(buf, 0, sizeof(buf));
		if (read(sockfd, buf, sizeof(buf)) <= 0){
			printf("Client read() failed.\n");
			close(sockfd);
			return -1;

		}
		printf("Client recv:%s\n", buf);
		// close(sockfd); break;
	}
	return 0;
}