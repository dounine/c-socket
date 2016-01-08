/*Proxy auth program*/
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define ZERO (strcut fd_set *)0
#define BUFLEN 256

char target[]="GET http://www.digital.com/index.html\n";
char result[80];
char buf[BUFLEN];
FILE *f, *out;
int port;
int status = -1;
char serverName[30];
int p1, p2, p3, p4;
int sockfd = -1;
struct timeval timeout={2, 0};
struct fd_set rmask, wmask;
struct sockaddr_in host;
unsigned long serverAddr;
int counter;
int i;

void killHandle(void);
void daemonize(int);

main(int argc, char *argv[])
{
	int flags;
	if(argc != 3)
	{
		printf("Usage: %s dataFileName number\n", argv[0]);
		exit(-1);
	}
	
	f = fopen(argv[1], "rb");
	if(!f)
	{
		fprintf(stdout, "open file error\n");
		exit(-1);
	}
	
	strcpy(result, argv[1]);
	strcat(result, ".ok");
	out = fopen(result, "a");
	if(!out)
	{
		fprintf(stdout, "open file error\n");
		exit(-1);
	}
	
	counter = atoi(argv[2]);
	fprintf(stdout, "Free proxy filter...\n");
	fprintf(stdout, "\tInput file:\t%s\n", argv[1]);
	fprintf(stdout, "\tTotal:\t%s\n", argv[2]);
	
	daemonize(f);
	signal(SIGTERM, killHandle);
	signal(SIGPIPE, sigpipeHandle);
	signal(SIGPIPE, SIG_IGN);
	
	for(; counter>0; counter--)
	{
		fscanf(f, "%s%d", serverName, &port);
		bzero((char *)&host, sizeof(host));
		serverAddr = inet_addr(serverName);
		host.sin_family = AF_INET;
		host.sin_addr.s_addr = htonl(serverAddr);
		host.sin_port = htons(port);
		
		if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			fprintf(out, "Error open socket at %s %d\n", serverName, port);
			exit(-1);
		}
		
		flags = fcntl(sockfd, F_GETFL, 0);
		if(fcntl(sockfd, F_SETFL, flags|O_NOBLOCK) < 0)
		{
			fprintf(out, "fcntl() error at %s %d\n", serverName, port);
			exit(-1);
		}
		
		status = connect(sockfd, (struct sockaddr *)&host, sizeof(host));
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		FD_ZERO(&wmask);
		FD_SET(sockfd, &wmask);
		status = select(sockfd+1, ZERO, &wmask, ZERO, &timeout);
		switch(status)
		{
			case -1:
				fprintf(out, "select error\n");
				exit(-1);
			case 0:
				close(sockfd);
				continue;
			default:
				if(FD_ISSET(sockfd, &wmask);
					break;
				else
				{
					close(sockfd);
					continue;
				}
		}
		
		status = write(sockfd, target, sizeof(target));
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		FD_ZERO(&rmask);
		FD_SET(sockfd, &rmask);
		status = select(sockfd+1, &rmask, ZERO, ZERO, &timeout);
		switch(status)
		{
			case -1:
				fprintf(out, "select error\n");
				exit(-1);
			case 0:
				close(sockfd);
				continue;
			default:
				if(FD_ISSET(sockfd, &rmask);
				{
					bzero(buf, BUFLEN);
					status = read(sockfd, buf, BUFLEN);
					close(sockfd);
					if(status <= 0)
						continue;
					if(!strncmp((buf+22), "Digital", 7))
					{
						fprintf(out, "free\t%s\t%d\n", serverName, port);
						fflush(out);
					}
				}
				else
					close(sockfd);
		}
	}
	fclose(f);
	fprintf(out, "Free proxy filter done\rn");
	fclose(out);
}

void killHandle(void)
{
	fprintf(out, "killed at %s\t%d\n", serverName, port);
	exit(0);
}