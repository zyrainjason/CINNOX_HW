#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define SIZE 1024

#define MULTIPLIER 2
#define BASE_TIME_INTERVAL 500*1000 // 500ms
#define MAX_TIME_INTERVAL 8*1000*1000 // 8s

// config parameter
char ip[SIZE];
int port = 0;
char message[SIZE];

bool readConf()
{
	bool ret = false;

	FILE *file;
	char line[SIZE];
	char key[SIZE];
	char value[SIZE];

    file = fopen("./conf/conf.txt", "r");
    if (file == NULL) {
        printf("fopen() config file failed.\n");
    }
	else {
		while (fgets(line, sizeof(line), file) != NULL) {
			memset(key,0,SIZE); memset(value,0,SIZE); 
			if (line[0] == '#' || line[0] == '\0' || line[0] == '\n') {
				continue;
			}
			//printf("config file line: %s\n", line);
			sscanf(line, "%s = %[^\n]", key, value);
			//printf("config file key: %s, value: %s\n", key, value);

			if (!strcmp(key, "IP")){
				strcpy(ip, value);
				printf("ip: %s\n", ip);
			}else if (!strcmp(key, "PORT")){
				port = atoi(value);
				printf("port: %d\n", port);
			}else if (!strcmp(key, "MESSAGE")){
				strcpy(message, value);
				printf("message: %s\n", message);
			}
		}

		if (ip != NULL && port != 0 && message != NULL) {
			ret = true;
		}
	}
	fclose(file);
	return ret;
}

unsigned int getTimeInterval(unsigned int round) {
	unsigned int ret;
	// wait_interval = base * multiplier^n

	unsigned int tmpTime = BASE_TIME_INTERVAL;
	for (int i = 0; i < round; ++i){
		tmpTime *= MULTIPLIER;
	}

	if (tmpTime > MAX_TIME_INTERVAL)
		ret = MAX_TIME_INTERVAL;
	else
		ret = tmpTime;
	return ret;
}

int main(int argc , char *argv[])
{
	printf("Enter sender.\n");

	bool ret = -1;

	printf("Try to read conf file.\n");
	if (!readConf()){
		printf("Read config file error.\n");
		return -1;
	}

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd == -1)
	{
		printf("Create socket failed.\n");
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		printf("setsockopt() failed.\n");
		return -1;
	}

	struct sockaddr_in addrServer;
	bzero(&addrServer,sizeof(addrServer));
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);
	if (inet_aton(ip , &addrServer.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		return -1;
	}
	int addrServerLen = sizeof(addrServer);

	int maxRetryCount = 10;
	int round = 1;
	while (round <= maxRetryCount){
		printf("\nEnter round %d.\n", round);

		// send to server
		if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&addrServer, addrServerLen) < 0) {
			printf("sendto() failed.");
		}

		// Receive response
		char response[SIZE];
		memset(response, 0, sizeof(response));
		if (recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr*)&addrServer, &addrServerLen) < 0) {
			printf("recvfrom() failed.\n");
		}else {
			printf("Get response: %s\n", response);
			if (strcmp(response, message) == 0) {
				printf("Response msg is equel to what we send to.", response);
				ret = 0;
				break;
			}
		}
		
		if (round >= maxRetryCount)
			break;

		// cal sleep time
		unsigned int sleepTimeInt = getTimeInterval(round);
		printf("Try to sleep %ds, round = %d\n", sleepTimeInt/1000/1000, round);
		usleep(sleepTimeInt); // us

		round++;
	}

	if (ret != 0 && round >= 10) {
		printf("After reach max retry count, still not get equel response msg.\n");
		ret = 1;
	}

	close(sockfd);

	printf("Try to close sender, ret = %d.\n", ret);
	return ret;
}
