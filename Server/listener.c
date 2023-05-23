#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFLEN 512
#define PORT 8887

int main(int argc , char *argv[])
{
	printf("Enter listener.\n");

	// UDP socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1)
	{
		printf("Create socket failed.\n");
		return -1;
	}

	// struct server
	struct sockaddr_in addrServer;
    bzero(&addrServer,sizeof(addrServer));
    addrServer.sin_family = PF_INET;
    addrServer.sin_addr.s_addr = inet_addr("127.0.0.1");
    addrServer.sin_port = htons(PORT);

	if( bind(sockfd, (struct sockaddr*)&addrServer, sizeof(addrServer) ) == -1)
	{
		printf("Bind socket failed.\n");
		return -1;
	}

	struct sockaddr_in addrClient;
	int addrClientLen = sizeof(addrClient);
	char buf[512];

	while(1)
	{
		printf("\nReady to receive msg.\n");

		ssize_t recvRet = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addrClient, &addrClientLen);
		if (recvRet == -1)
		{
			printf("recvfrom() failed.\n");
			break;
		}
		printf("Received from %s:%d\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port));
		printf("Message: %s\n" , buf);

		ssize_t sendtoRet = sendto(sockfd, buf, recvRet, 0, (struct sockaddr*)&addrClient, addrClientLen);
		if (sendtoRet == -1)
		{
			printf("sendto() failed.\n");
			break;
		}
	}

	printf("Ready to close socket.\n");
	close(sockfd);

	return 0;
}