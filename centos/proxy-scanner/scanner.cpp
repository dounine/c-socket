#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

using namespace std;    
long long threadCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static bool port_is_open(string ip, int port){

    struct sockaddr_in address;  /* the libc network address data structure */
    short int sock = -1;         /* file descriptor for the network socket */
    fd_set fdset;
    struct timeval tv;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str()); /* assign the address */
    address.sin_port = htons(port);

    /* translate int2port num */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    if(connect(sock, (struct sockaddr *)&address, sizeof(address))<0){
		//cout<<strerror(errno)<<endl;
	}

    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    tv.tv_sec = 1;             /* timeout 秒 */
    tv.tv_usec = 50;		/* 微秒 */

    if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1)
    {
        int so_error;
        socklen_t len = sizeof so_error;

        getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);

        if (so_error == 0){	
	    close(sock);
	    return true;
        }
    }       
    close(sock);
    return false;
}

const int ports[] = {80,3128,8080,8888,9000};
bool task(string ip){
	bool pa = false;
	int po=0;
	int open = false;
	for(int i=0;i<4;i++){
		po = ports[i];
		if (port_is_open(ip, po)){
			open = true;
			break;
		}
	}
	if(open){
		cout << ip << ":" <<po<<" is o"<<endl;	
	}else{
		//cout << ip << ":" <<po<<" is c"<<endl;	
	}
	pthread_mutex_lock(&mutex);
	threadCount--;  
	pthread_mutex_unlock(&mutex);
	return pa;
}
int main(int argc, char **argv){    

	
	struct timeval tv;
	int ip1,ip2,ip3,ip4;
	while(true){
		sleep(0.1); 
		if(threadCount<=1000){
			ostringstream ip;
			gettimeofday(&tv,NULL);
			srand(tv.tv_usec);
			ip1 = 1+(int) (200.0*rand()/(RAND_MAX+1.0));
			ip2 = 1+(int) (255.0*rand()/(RAND_MAX+1.0));
			ip3 = 1+(int) (255.0*rand()/(RAND_MAX+1.0));
			ip4 = 1+(int) (255.0*rand()/(RAND_MAX+1.0));
			ip<<ip1<<"."<<ip2<<"."<<ip3<<"."<<ip4;
			pthread_mutex_lock(&mutex);
			threadCount++;
			pthread_mutex_unlock(&mutex);
			new thread(task, ip.str());		
		}
	}	
	return 0;
}
