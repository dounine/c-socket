#include <iostream>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib");

using namespace std;

void init_client(){
	WSAData wsaData;
	
	WSAStartup(MAKEWORD(2,2),&wsaData);
	
	SOCKET clientSocket = socket(AF_INET,SOCK_STREAM,0);
	
	sockaddr_in sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	sockAddr.sin_family=AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockAddr.sin_port = htons(8888);
	if(connect(clientSocket,(SOCKADDR*)&sockAddr,sizeof(sockAddr))<0){
		cout<<"连接失败"<<endl; 
		return;
	}
	
	char szBuffer[MAXBYTE] = {0}; 
	
	if(recv(clientSocket,szBuffer,MAXBYTE,NULL)<0){
		cout<<"接收失败"<<endl;
		return; 
	}
	
	cout<<"消息:"<<szBuffer<<endl;
	
	closesocket(clientSocket);
	
	WSACleanup(); 
}