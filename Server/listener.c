#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFLEN 1024

char gIp[BUFFLEN];
int gPort = 0;

bool checkArgv(int argc, char *argv[])
{
    bool ret = false;

    // ./listener -ip <IP> -port <PORT>

    // printf("argc=%d\n", argc);
    if (argc != 5)
        return ret;

    for (int i = 1; i < argc; i = i + 2)
    {
        // printf("argv[%d]=%s\n", i, argv[i]);
        if (!strcmp(argv[i], "-ip"))
        {
            strcpy(gIp, argv[i + 1]);
            // printf("gIp=%s\n", gIp);
        }
        else if (!strcmp(argv[i], "-port"))
        {
            gPort = atoi(argv[i + 1]);
            // printf("gPort=%d\n", gPort);
        }
        else
        {
            printf("Unknown cmd:%s\n", argv[i]);
            return ret;
        }
    }

    if (gIp != NULL && gPort != 0)
    {
        printf("Cmd IP=%s, port=%d\n", gIp, gPort);
        ret = true;
    }

    return ret;
}

void showSample()
{
    printf("Sample cmd: ./listener -ip <IP> -port <PORT>\n");
}

int main(int argc, char *argv[])
{
    printf("Enter listener.\n");

    // check argv
    if (!checkArgv(argc, argv))
    {
        printf("Check input arguments failed.\n");
        showSample();
        return -1;
    }

    // UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        printf("Create socket failed.\n");
        return -1;
    }

    // struct server
    struct sockaddr_in addrServer;
    bzero(&addrServer, sizeof(addrServer));
    addrServer.sin_family = PF_INET;
    addrServer.sin_addr.s_addr = inet_addr(gIp);
    addrServer.sin_port = htons(gPort);

    if (bind(sockfd, (struct sockaddr *)&addrServer, sizeof(addrServer)) == -1)
    {
        printf("Bind socket failed.\n");
        return -1;
    }

    struct sockaddr_in addrClient;
    int addrClientLen = sizeof(addrClient);
    char buf[BUFFLEN];

    while (1)
    {
        printf("\nTry to receive a msg.\n");
        ssize_t recvRet = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addrClient, &addrClientLen);
        if (recvRet == -1)
        {
            printf("recvfrom() failed.\n");
            break;
        }
        printf("Received from %s:%d\n", inet_ntoa(addrClient.sin_addr), ntohs(addrClient.sin_port));
        printf("Message: %s\n", buf);

        printf("Try to send back a same msg.\n");
        ssize_t sendtoRet = sendto(sockfd, buf, recvRet, 0, (struct sockaddr *)&addrClient, addrClientLen);
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
