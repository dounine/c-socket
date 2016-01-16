#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <thread>
using namespace std;

#define BUFLEN 600
char buf[BUFLEN];

void checkProxy();
int main(void){
	for(int i =0;i<10;i++){
		new thread(checkProxy);
		//t->join();
		//checkProxy();
	}
	sleep(10);
	return 0;
}
const char target[]="GET http://www.haosou.com/\n";
const char *title = "<title>好搜 — 用好搜，特顺手</title>";
void checkProxy(){
	int sockfd = -1;
	struct timeval timeout;
	
	int status = -1;
	fd_set rmask, wmask;	
	struct sockaddr_in address;
	const char *serverName = "121.69.30.2";//proxy host 121.69.30.2
	int port = 8118;//8118
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(serverName);
	address.sin_port = htons(port);
	
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	
	connect(sockfd, (struct sockaddr *)&address, sizeof(address));

	timeout.tv_sec = 1;//timeout times 3s
	timeout.tv_usec = 0;
	FD_ZERO(&wmask);
	FD_SET(sockfd, &wmask);
	status = select(sockfd+1, NULL, &wmask, NULL, &timeout);
	if(status==0){
		close(sockfd);
	}else{
		if(!FD_ISSET(sockfd, &wmask)){
			close(sockfd);
		}
	}
	cout<<strerror(errno)<<endl;
	status = write(sockfd, target, sizeof(target));
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	FD_ZERO(&rmask);
	FD_SET(sockfd, &rmask);
	status = select(sockfd+1, &rmask, NULL, NULL, &timeout);
	if(status==1){
		if(FD_ISSET(sockfd, &rmask)){
			bzero(buf, BUFLEN);
			status = read(sockfd, buf, BUFLEN);
			close(sockfd);
			if(strstr(buf,title)){
				cout<<"proxy ip exist"<<endl;
			}
		}
	}
	cout<<"proxy ip valid done"<<endl;
}
