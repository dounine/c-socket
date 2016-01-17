#include <sys/socket.h>
#include <sys/time.h>
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
#include <sstream>
#include <stdio.h>
#include <cstdlib>
#include <netinet/tcp.h>	//Provides declarations for tcp header
#include <netinet/ip.h>	//Provides declarations for ip header
using namespace std;
long long threadCount = 0;//限制开启线程数
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define SYN_TIMEOUT 0 //syn发送时间
#define SYN_PORT 80   //扫描端口
void task();
bool process_packet(unsigned char* , int);
unsigned short csum(unsigned short * , int );
char * hostname_to_ip(char * );
void get_local_ip (char *);
void montage(const string&);

struct pseudo_header    //needed for checksum calculation
{
	unsigned int source_address;
	unsigned int dest_address;
	unsigned char placeholder;
	unsigned char protocol;
	unsigned short tcp_length;
	
	struct tcphdr tcp;
};

struct in_addr dest_ip;

int main(int argc, char *argv[]){

	string host = "101.200.198.";
	
	thread *tt = new thread(task);
	for(int i =0;i<255;i++){
		//sleep(1);
		ostringstream ip;
		ip<<host<<i;
		//pthread_mutex_lock(&mutex);
	       	//++threadCount;
	       	//pthread_mutex_unlock(&mutex);
	   	new thread(montage,ip.str());
		//montage(ip.str());
	}
	
	tt->join();
	return 0;
}

void montage(const string& h){
	//创建socket套接字
	
	int sock = socket (AF_INET, SOCK_RAW , IPPROTO_TCP);	
	fcntl(sock, F_SETFL, O_NONBLOCK);
	//Datagram to represent the packet
	char datagram[4096];	
	
	//IP头
	struct iphdr *iph = (struct iphdr *) datagram;
	
	//TCP头
	struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
	
	struct sockaddr_in  dest;
	struct pseudo_header psh;
	char *target = (char*)h.c_str();
	dest_ip.s_addr = inet_addr( target );
	//if( inet_addr( target ) != -1){
	//	dest_ip.s_addr = inet_addr( target );
	//}else{
	//	char *ip = hostname_to_ip(target);
	//	if(ip != NULL){
	//		cout<<target<<" 解析-> "<<ip<<endl;
	//		//转换域名为IP地址
	//		dest_ip.s_addr = inet_addr( hostname_to_ip(target) );
	//	}else{
	//		cout<<"不能解析域名 : "<<target<<endl;
	//	}
	//}
	
	int source_port = 43591;
	char source_ip[20];
	get_local_ip( source_ip );
	
	//cout<<"本地IP地址:"<<source_ip<<endl;
	
	memset (datagram, 0, 4096);	/* zero out the buffer */
	
	//填充IP头部信息
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof (struct ip) + sizeof (struct tcphdr);
	iph->id = htons (54321);	//Id of this packet
	iph->frag_off = htons(16384);
	iph->ttl = 64;
	iph->protocol = IPPROTO_TCP;
	iph->check = 0;		//Set to 0 before calculating checksum
	iph->saddr = inet_addr ( source_ip );	//Spoof the source ip address
	iph->daddr = dest_ip.s_addr;
	
	iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
	
	//填充TCP头部信息
	tcph->source = htons ( source_port );
	tcph->dest = htons (80);
	tcph->seq = htonl(1105024978);
	tcph->ack_seq = 0;
	tcph->doff = sizeof(struct tcphdr) / 4;		//Size of tcp header
	tcph->fin=0;
	tcph->syn=1;
	tcph->rst=0;
	tcph->psh=0;
	tcph->ack=0;
	tcph->urg=0;
	tcph->window = htons ( 14600 );	// maximum allowed window size
	tcph->check = 0;
	tcph->urg_ptr = 0;
	
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	setsockopt (sock, IPPROTO_IP, IP_HDRINCL, &tv, sizeof(tv));

	//cout<<"开始发送syn pakcage"<<endl;
	
	
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = dest_ip.s_addr;
	
	
	int port=SYN_PORT;
	tcph->dest = htons ( port );
	tcph->check = 0;
	
	psh.source_address = inet_addr( source_ip );
	psh.dest_address = dest.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_TCP;
	psh.tcp_length = htons( sizeof(struct tcphdr) );
	
	memcpy(&psh.tcp , tcph , sizeof (struct tcphdr));
	
	tcph->check = csum( (unsigned short*) &psh , sizeof (struct pseudo_header));

	//Send the packet
	if ( sendto (sock, datagram , sizeof(struct iphdr) + sizeof(struct tcphdr) , 0 , (struct sockaddr *) &dest, sizeof (dest)) < 0){
		cout<<"发送syn package失败"<<strerror(errno)<<endl;
	}
	//close(sock);
}

/*
	Method to sniff incoming packets and look for Ack replies
*/
void task(){
	int sock_raw;
	
	int data_size;
        socklen_t saddr_size;
	struct sockaddr saddr;
	
	unsigned char *buffer = (unsigned char *)malloc(65536); //Its Big!
	
	//cout<<"嗅探开始..."<<endl;
	
	//Create a raw socket that shall sniff
	sock_raw = socket(AF_INET , SOCK_RAW , IPPROTO_TCP);
	
	saddr_size = sizeof(saddr);
	
	int br = 1;
	struct timeval tv;
	tv.tv_sec = SYN_TIMEOUT;//超时
	tv.tv_usec = 0;
	setsockopt(sock_raw,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));//设置接收package超时
	while(br){
		//接收包
		sleep(0.1);
		data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , &saddr_size);
	
		if(data_size < 0 ){
			cout<<"获取PACKAGE失败"<<endl;
			//br = 0;
			//pthread_mutex_lock(&mutex);
		       	//--threadCount;
		       	//pthread_mutex_unlock(&mutex);
			//close(sock_raw);
		}else{
			if(process_packet(buffer , data_size)){
				//br = 0;
				//pthread_mutex_lock(&mutex);
			       	//--threadCount;
			       	//pthread_mutex_unlock(&mutex);
				//close(sock_raw);
			}
		}
	}
	close(sock_raw);
	//cout<<"扫描完成"<<endl;
}

string uint_to_ip(unsigned int ip){
    ostringstream ii;
    ii<<((ip>>0)&0xFF);
    ii<<".";
    ii<<((ip>>8)&0xFF);
    ii<<".";
    ii<<((ip>>16)&0xFF);
    ii<<".";
    ii<<((ip>>24)&0xFF);
    return ii.str();
}
bool process_packet(unsigned char* buffer, int size)
{
	//Get the IP Header part of this packet
	struct iphdr *iph = (struct iphdr*)buffer;
	struct sockaddr_in source,dest;
	unsigned short iphdrlen;
	
	if(iph->protocol == 6){
		struct iphdr *iph = (struct iphdr *)buffer;
		iphdrlen = iph->ihl*4;
	
		struct tcphdr *tcph=(struct tcphdr*)(buffer + iphdrlen);
			
		memset(&source, 0, sizeof(source));
		source.sin_addr.s_addr = iph->saddr;
	
		memset(&dest, 0, sizeof(dest));
		dest.sin_addr.s_addr = iph->daddr;
		
		if(tcph->syn == 1 && tcph->ack == 1 && source.sin_addr.s_addr == dest_ip.s_addr ){
			cout<<uint_to_ip(iph->saddr)<<"端口 "<<ntohs(tcph->source)<<" open "<<endl;
			return true;
		}
	}
	return false;
}

/*
 Checksums - IP and TCP
 */
unsigned short csum(unsigned short *ptr,int nbytes) {
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}

/*
	Get ip from domain name
 */
char* hostname_to_ip(char * hostname){
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
		
	if ( (he = gethostbyname( hostname ) ) == NULL) {
		// get the host info
		herror("gethostbyname");
		return NULL;
	}

	addr_list = (struct in_addr **) he->h_addr_list;
	
	for(i = 0; addr_list[i] != NULL; i++) {
		//Return the first one;
		return inet_ntoa(*addr_list[i]) ;
	}
	
	return NULL;
}

/*
 获取本机系统IP地址 , like 192.168.0.6 or 192.168.1.2 通过连接外界DNS反查
 */

void get_local_ip ( char * buffer){
	int sock = socket ( AF_INET, SOCK_DGRAM, 0);

	const char* kGoogleDnsIp = "8.8.8.8";
	int dns_port = 53;

	struct sockaddr_in serv;

	memset( &serv, 0, sizeof(serv) );
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
	serv.sin_port = htons( dns_port );

	int err = connect( sock , (const struct sockaddr*) &serv , sizeof(serv) );

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*) &name, &namelen);

	const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

	close(sock);
}
