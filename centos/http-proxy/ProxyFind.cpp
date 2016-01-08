/*Proxy find program*/
#include <stdio.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define N 7

int port[N]={80, 81, 88, 8083, 8080, 8001, 8888};
int status = -1;
char serverName[20];
int p1, p2, p3, p4;
struct timeval timeout;
struct fd_set mask;
FILE *f = NULL;
unsigned long startIP, endIP, k;

void findProxy(unsigned long addr);
void daemonize(int servfd);

main(int argc, char *argv[])
{
	int i;
	if(argc != 4)
	{
		printf("Usage: %s startIP endIP logFile\n", argv[0]);
		exit(-1);
	}
	
	startIP = ntohl(inet_addr(argv[1]));
	endIP = ntohl(inet_addr9argv[2]));
	if(startIP > endIP)
	{
		k = startIP;
		startIP = endIP;
		endIP = k;
	}
	
	f = fopen(argv[3], "a");
	if(f == NULL)
	{
		printf("error open log file: %s\n", argv[3]);
		exit(-1);
	}
	
	fprintf(f, "%s----------->%s\n", argv[1], argv[2]);
	fflush(f);
	
	printf("Searching proxy...\n");
	printf("%s----------->%s\n"), argv[1], argv[2]);
	printf("\tport:\n");
	for(i=0; i<N; i++)
	{
		printf("\t%d\n", port[i]);
		daemonize(f);
	}
		
	for(k=startIP; k<=endIP; k++)
	{
		if((k%256) == 0)
			continue;
		if((k%256) == 255)
			continue;
		p1 = (int)((k>>24) & 0xFF);
		p2 = (int)((k>>16) & 0xFF);
		p3 = (int)((k>>8) & 0xFF);
		p4 = (int)(k & 0xFF);
		sprintf(serverName, "%d.%d.%d.%d", p1, p2, p3, p4);
		
		findProxy(k);
	}
	
	fprintf(f, "All Done\n");
	fclose(f);
}

void findProxy(unsigned long addr)
{
	int i, flags;
	int unconnc, maxfd=0;
	int err;
	int errlen;
	strcut sockaddr_in host;
	int portfd[N];
	unconnc = N;
	
	FD_ZERO(&mask);
	
	for(i=0; i<N; i++)
	{
		host.sin_family = AF_INET;
		host.sin_addr.s_addr = htonl(addr);
		host.sin_port = htons(port[i]);
		if((portfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			fprintf(f, "Error open socket\n");
			exit(-1);
		}
		
		flags = fcntl(portfd[i], F_GETFL, 0);
		if(fcntl(portfd[i], F_SETL, flags|O_NONBLOCK) < 0)
		{
			fprintf(f, "fcntl() error\n");
			exit(-1);
		}
		
		if(status = connect(portfd[i], &host, sizeof(host)) > 0)
		{
			fprintf(f, "%s\t%d\n", serverName, port[i]);
			close(portfd[i]);
			protfd[i] = -1;
			unconnc--;
		}
		else
		{
			FD_SET(sockfd, &mask);
			if(fd > maxfd)
				maxfd = fd;
		}
	}
		
	while(unconnc)
	{
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		status = select(maxfd+1, NULL, &mask, NULL, &timeout);
		for(i=0; i<N; i++)
		{
			if(portfd[i] == -1)
				continue;
			switch(status)
			{
				case -1:
					fprintf(f, "select error\n");
					fclose(f);
					exit(-1);
				case 0:
					close(portfd[i]);
					return;
				default:
					if(FD_ISSET(portfd[i], &mask))
					{
						err = 1;
						errlen = 1;
						getsockopt(portfd[i], SOL_SOCKET,SO_ERROR, &err, &errlen);
						if(err == 0)
							fprintf(f, "%s\t%d\n", serverName, port[i]);
						fflush(f);
						FD_CLR(portfd[i], &mask);
						unconnc--;
						close(portfd[i]);
						portfd[i] = -1;
					}
			}
		}
	}
}

void daemonize(int servfd)
{
	int childpid, fd, fdtablesize;
	
	signal(SIGTOU, SIG_IGN);
	signal(SIGTIN, SIG_IGN);
	signal(SIGSTP, SIG_IGN);
	if((childpid=fork()) < 0)
	{
		fputs("failed to fork first child\n", stderr);
		exit(1);
	}
	else if(childpid > 0)
		exit(0);
	
	setsid();
	signal(SIGHUP, SIG_IGN);
	
	if(((pid=fork()) != 0)
		exit(0);
	
	for(fd=0, fdtablesize=getdtablesize(); fd<fdtablesize; fd++)
	if(fd != servfd)
		close(fd);
	
	chdir("/");
	umask(0);
}