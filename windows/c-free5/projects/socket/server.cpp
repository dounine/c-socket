#include <iostream>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;
void init_server(){	
	cout<<"server init!!!"<<endl;
	
	WSAData wsaData; 
    WSAStartup( MAKEWORD(2, 2), &wsaData);
	SOCKET serverSocket = socket(AF_INET,SOCK_STREAM,0);
	
	
	sockaddr_in sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockAddr.sin_port = htons(8888);
	
	if(bind(serverSocket,(SOCKADDR*)&sockAddr, sizeof(SOCKADDR))<0){
		cout<<"�˿ڰ�ʧ��"<<endl;
	}
	
	if(listen(serverSocket,10)<0){
		cout<<"����ʧ��"<<endl;
	}
	
	//���տͻ�������
    SOCKADDR clientAddr;
    int nSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket,(SOCKADDR*)&clientAddr,&nSize);
 	
 	cout<<"�пͻ�����������"<<endl;
 	
 	char *str = "helloj,ni hao";
 	send(clientSocket,str,strlen(str)+sizeof(char),NULL);
	
	
	closesocket(clientSocket);
	closesocket(serverSocket);
	
	WSACleanup();
}