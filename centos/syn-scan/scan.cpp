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

#define SYN_TIMEOUT 1
void task();
bool process_packet(unsigned char* , int);
unsigned short csum(unsigned short * , int );
char * hostname_to_ip(char * );
void get_local_ip (char *);

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
	if(argc<2){
		cout<<"请输入扫描的IP或域名"<<endl;
		return 1;
	}
	
	//创建socket套接字
	int s = socket (AF_INET, SOCK_RAW , IPPROTO_TCP);
		
	//Datagram to represent the packet
	char datagram[4096];	
	
	//IP头
	struct iphdr *iph = (struct iphdr *) datagram;
	
	//TCP头
	struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
	
	struct sockaddr_in  dest;
	struct pseudo_header psh;
	char *target = argv[1];
	
	if( inet_addr( target ) != -1){
		dest_ip.s_addr = inet_addr( target );
	}else{
		char *ip = hostname_to_ip(target);
		if(ip != NULL){
			cout<<target<<" 解析-> "<<ip<<endl;
			//转换域名为IP地址
			dest_ip.s_addr = inet_addr( hostname_to_ip(target) );
		}else{
			cout<<"不能解析域名 : "<<target<<endl;
			return 1;
		}
	}
	
	int source_port = 43591;
	char source_ip[20];
	get_local_ip( source_ip );
	
	cout<<"本地IP地址:"<<source_ip<<endl;
	
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
	
	int one = 1;
	
	setsockopt (s, IPPROTO_IP, IP_HDRINCL, &one, sizeof (one));
	
	cout<<"启动嗅探线程..."<<endl;

	thread *tt = new thread(task);

	cout<<"开始发送syn pakcage"<<endl;
	
	
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = dest_ip.s_addr;
	
	
	int port=80;
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
	if ( sendto (s, datagram , sizeof(struct iphdr) + sizeof(struct tcphdr) , 0 , (struct sockaddr *) &dest, sizeof (dest)) < 0){
		cout<<"发送syn package失败"<<endl;
	}
	
	tt->join();
	
	return 0;
}

/*
	Method to sniff incoming packets and look for Ack replies
*/
void task(){
	int sock_raw;
	
	int data_size;
        socklen_t saddr_size;
	struct sockaddr saddr;
	
	unsigned char *buffer = (unsigned char *)malloc(100); //Its Big!
	
	cout<<"嗅探开始..."<<endl;
	
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
		data_size = recvfrom(sock_raw , buffer , 100 , 0 , &saddr , &saddr_size);
	
		if(data_size < 0 ){
			cout<<"获取PACKAGE失败"<<endl;
			br = 0;
		}else{
			if(process_packet(buffer , data_size)){
				br = 0;
			}
		}
	}

	free(buffer);
	close(sock_raw);
	cout<<"扫描完成"<<endl;
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
			cout<<"端口 "<<ntohs(tcph->source)<<" 打开 "<<endl;
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
