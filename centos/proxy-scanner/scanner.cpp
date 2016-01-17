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
#include <cstdlib>
#include <fstream>
using namespace std;    
long long threadCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define  POINT '.'
void task(const string& ip);
bool port_is_open(const string& ip,const int port);
vector<string> read_file(const string& filename);
vector<string> split(const string& str,const string& sep);
void splIp(const string& ip);
void execIp(const string& ip);

int main(int argc, char **argv){

	string filename = "ranges/China.txt";
        vector<string> ips = read_file(filename);
	for(int i =0,size = ips.size();i<=size;++i){
		splIp(ips[i]);
	}
	return 0;
}

bool port_is_open(const string& ip,const int port){

    struct sockaddr_in address;  /* the libc network address data structure */
    short int sock = -1;         /* file descriptor for the network socket */
    fd_set fdset;
    struct timeval tv;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str()); /* assign the address */
    address.sin_port = htons(port);

    /* translate int2port num */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    //fcntl(sock, F_SETFL, O_NONBLOCK);

    if(connect(sock, (struct sockaddr *)&address, sizeof(address))>0){
	close(sock);
	return true;
    }

    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    tv.tv_sec = 2;             /* timeout 秒 */
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

void task(const string& ip){
	pthread_mutex_lock(&mutex);
       	++threadCount;
       	pthread_mutex_unlock(&mutex);
	if (port_is_open(ip, 8080)){
		cout <<endl<< ip << ":" <<8080<<" is open"<<endl;
	}else{
		//cout << ip << ":" <<8080<<" is close"<<endl;
	}
	pthread_mutex_lock(&mutex);
	--threadCount;  
	pthread_mutex_unlock(&mutex);
}

vector<string> read_file(const string& filename){
	vector<string> vt;
	ifstream file;
	file.open(filename);
	if(!file){
		cerr<<"打开文件错误"<<endl;
	}
	char line[50];
	while(file.getline(line,sizeof(line))){
		string lis(line);
		vt.push_back(lis);
	}
	file.clear();
	file.close();
	return vt;
}
vector<string> split(const string& str, const string& sep){
	
    vector<string> ret_;
    string tmp;
    string::size_type pos_begin = str.find_first_not_of(sep);
    string::size_type comma_pos = 0;

    while (pos_begin != string::npos){
        comma_pos = str.find(sep, pos_begin);
        if (comma_pos != string::npos){
            tmp = str.substr(pos_begin, comma_pos - pos_begin);
            pos_begin = comma_pos + sep.length();
        }else{
            tmp = str.substr(pos_begin);
            pos_begin = comma_pos;
        }

        if (!tmp.empty()){
            ret_.push_back(tmp);
            tmp.clear();
        }
    }
    return ret_;
}

void splIp(const string& ip){
	vector<string> ipLine = split(ip,"\t");
	string start = ipLine[1];
	string end = ipLine[2];
		
	vector<string> startArr = split(start,".");
	vector<string> endArr = split(end,".");
	
	int sta[] = {atoi(startArr[0].c_str()),atoi(startArr[1].c_str()),atoi(startArr[2].c_str()),atoi(startArr[3].c_str())};
	int ena[] = {atoi(endArr[0].c_str()),atoi(endArr[1].c_str()),atoi(endArr[2].c_str()),atoi(endArr[3].c_str())};
	stringstream io;
	for(int i1 = sta[0];i1<=ena[0];++i1){
		for(int i2 = (sta[1]==0?1:sta[1]);i2<=ena[1];++i2){
			for(int i3 = (sta[2]==0?1:sta[2]);i3<=ena[2];++i3){
				for(int i4 = (sta[3]==0?1:sta[3]);i4<=ena[3];++i4){
					io<<i1<<POINT<<i2<<POINT<<i3<<POINT<<i4;
					execIp(io.str());
					io.str("");
				}
			}
		}
	}
}
inline void execIp(const string& ip){
	int br = 1;
	while(br){
		sleep(0.1);
		if(threadCount>=100){
			sleep(1);
		}else{
			br = 0;
		}
        }
      	new thread(task, ip);
}
