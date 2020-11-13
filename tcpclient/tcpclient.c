#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define TCP_PORT 8088
#define BUFSIZE 1024

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in server_addr;
	char buf[BUFSIZE];
	if (argc < 2) {
		printf("usage: %s IP_ADDRESS\n", argv[0]);
		return -1;
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, argv[1], &(server_addr.sin_addr.s_addr));
	server_addr.sin_port = htons(TCP_PORT);
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		return -1;
	}
	while (1) {
		memset(buf, 0, BUFSIZE);
		printf("서버로 보낼 메시지: ");
		fgets(buf, BUFSIZE, stdin);
		if (send(sockfd, buf, BUFSIZE, MSG_DONTWAIT) <= 0) {
			perror("send()");
			break;
		}

		memset(buf, 0, BUFSIZE);
		if (recv(sockfd, buf, BUFSIZE, 0) <= 0) {
			perror("recv()");
			return -1;
		}
		printf("서버에서 받은 메시지: %s", buf);
		if (strncmp(buf, "q", 1) == 0) {
			break;
		}
	}
	close(sockfd);
	return 0;
}
