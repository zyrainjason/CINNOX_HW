#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define SIZE 1024

#define MULTIPLIER 2
#define BASE_TIME_INTERVAL 500 * 1000     // 500ms
#define MAX_TIME_INTERVAL 8 * 1000 * 1000 // 8s

int gReturnValue = -1;
bool gStop = false;

// config parameter
char gIp[SIZE];
int gPort = 0;
char gMessage[SIZE];
int gMaxRetry = 10;

bool readConf()
{
    bool ret = false;

    FILE *file;
    char line[SIZE];
    char key[SIZE];
    char value[SIZE];

    file = fopen("./conf/conf.txt", "r");
    if (file == NULL)
    {
        printf("fopen() config file failed.\n");
    }
    else
    {
        while (fgets(line, sizeof(line), file) != NULL)
        {
            memset(key, 0, SIZE);
            memset(value, 0, SIZE);
            if (line[0] == '#' || line[0] == '\0' || line[0] == '\n')
            {
                continue;
            }
            // printf("config file line: %s\n", line);
            sscanf(line, "%s = %[^\n]", key, value);
            // printf("config file key: %s, value: %s\n", key, value);

            if (!strcmp(key, "IP"))
            {
                strcpy(gIp, value);
                printf("gIp: %s\n", gIp);
            }
            else if (!strcmp(key, "PORT"))
            {
                gPort = atoi(value);
                printf("gPort: %d\n", gPort);
            }
            else if (!strcmp(key, "MESSAGE"))
            {
                strcpy(gMessage, value);
                printf("gMessage: %s\n", gMessage);
            }
            else if (!strcmp(key, "MAXRETRY"))
            {
                gMaxRetry = atoi(value);
                if (gMaxRetry < 10)
                    gMaxRetry = 10;
                printf("gMaxRetry: %d\n", gMaxRetry);
            }
            else
            {
                printf("Unknown key: %s, value: %s\n", key, value);
            }
        }

        if (gIp != NULL && gPort != 0 && gMessage != NULL)
        {
            ret = true;
        }
    }
    return ret;
}

unsigned int getTimeInterval(unsigned int round)
{
    unsigned int ret;
    // wait_interval = base * multiplier^n

    unsigned int tmpTime = BASE_TIME_INTERVAL;
    for (int i = 0; i < round; ++i)
    {
        tmpTime *= MULTIPLIER;
    }

    if (tmpTime > MAX_TIME_INTERVAL)
        ret = MAX_TIME_INTERVAL;
    else
        ret = tmpTime;
    return ret;
}

void *receiveThread(void *arg)
{
    printf("Enter receiveThread.\n");

    int sockfd = *((int *)arg);

    char responseBuff[SIZE];

    while (!gStop)
    {
        memset(responseBuff, 0, sizeof(responseBuff));
        ssize_t receivedBytes = recv(sockfd, responseBuff, sizeof(responseBuff) - 1, 0);
        if (receivedBytes > 0)
        {
            printf("Get response, echo message: %s\n", responseBuff);
            gReturnValue = 0;
            gStop = true;
            break;
        }
        else if (receivedBytes == 0)
        {
            printf("The peer has performed an orderly shutdown.\n");
        }
        else
        {
            printf("recv() failed, error: %d.\n", errno);
            if (errno == ECONNREFUSED)
                printf("A remote host not running the requested service.\n");
        }
    }

    pthread_exit(NULL);
}

void *sendThread(void *arg)
{
    printf("Enter sendThread.\n");

    int sockfd = *((int *)arg);

    int round = 1;
    while (!gStop)
    {
        printf("\nEnter send message round: %d.\n", round);

        if (send(sockfd, gMessage, sizeof(gMessage), 0) - 1 <= 0)
            printf("send() failed.");

        if (round >= gMaxRetry)
        {
            printf("Reach max retry count: %d.\n", gMaxRetry);
            gReturnValue = 1;
            gStop = true;
            break;
        }

        unsigned int sleepTimeInt = getTimeInterval(round);
        printf("Try to sleep %ds.\n", sleepTimeInt / 1000 / 1000);
        usleep(sleepTimeInt);

        round++;
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    printf("Enter sender.\n");

    bool ret = -1;

    printf("Try to read conf file.\n");
    if (!readConf())
    {
        printf("Read config file error.\n");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        printf("Create socket failed.\n");
        return -1;
    }

    struct sockaddr_in addrServer;
    bzero(&addrServer, sizeof(addrServer));
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(gPort);
    if (inet_aton(gIp, &addrServer.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed.\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0)
    {
        printf("connect() failed.\n");
        return -1;
    }

    pthread_t receiveThreadId, sendThreadId;
    printf("Try to create receive and send thread.\n");
    if (pthread_create(&receiveThreadId, NULL, receiveThread, (void *)&sockfd) != 0)
    {
        printf("pthread_create() receiveThreadId failed.\n");
        return -1;
    }

    if (pthread_create(&sendThreadId, NULL, sendThread, (void *)&sockfd) != 0)
    {
        printf("pthread_create() sendThreadId failed.\n");
        return -1;
    }

    pthread_join(receiveThreadId, NULL);
    pthread_join(sendThreadId, NULL);

    close(sockfd);
    ret = gReturnValue;
    printf("\nTry to close sender, return %d.\n", ret);
    return ret;
}
