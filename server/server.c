#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <winsock.h>
//#include <windows.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define M3_DEBUG

#include "packetPrototype.h"
#include "mystdlib.h"
#include "packetProcessing.h"
#include "mysql_sum.h"

#include <pthread.h>

#include <semaphore.h>

#include <sys/time.h>
#include <time.h>

sem_t sem;
int fc=1,ft=1;


int time_compare(int timef){
	time_t timep;
	struct tm *p;
	
	time(&timep);
	p=localtime(&timep);
	//printf("%d %d\n",p->tm_sec,timef);
	if(p->tm_sec<5 && timef>55)
		if(abs((p->tm_sec)+60-timef)>4)
			return p->tm_sec;
		else
			return timef;
	else if(abs((p->tm_sec)-timef)>4)
		return p->tm_sec;
	else
		return timef;
	//sprintf(s,"%d-%d-%d %d:%d:%d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
}

void udp_thread(void){
	char	RxBUF[4096];
	int		RxSocket;
	struct	sockaddr_in	server_address, client_address;
	int		addrlen = sizeof(server_address);
	int		c=0,cn,nRecvBuf=65536;
	
	int time=0;
	int timereg=0;
	
	struct timeval tv_out;
    tv_out.tv_sec = 0;///chang 
    tv_out.tv_usec = 100;
	
	SunPonter pkt_in;
	/* create socket , same as client */
	RxSocket = socket(AF_INET, SOCK_DGRAM, 0);
	/* initialize structure server_address */
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(5000);
	/* this line is different from client */
	server_address.sin_addr.s_addr = INADDR_ANY;
	/* Assign a port number to socket */
	setsockopt(RxSocket,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	setsockopt(RxSocket, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	bind(RxSocket, (struct sockaddr*)&server_address, sizeof(server_address));


	while(1){
		fc=0;printf("abc\n");
		bzero(&RxBUF, sizeof(RxBUF));
		cn=recvfrom(RxSocket, RxBUF, sizeof(RxBUF) , 0, (struct sockaddr *) &client_address, &addrlen);
		if(cn != 0 && cn !=-1){
			pkt_in = array2Packet(RxBUF);
			if (pkt_in != NULL) {
				showSunPacketInfo(pkt_in);
				if (pkt_in->commandType == PERIODIC_DATA) {
					while(!ft);
					mysql_PeriodicData(pkt_in,0);
				}
				releasePacket(pkt_in);
			}
			printf("I'm UDP PERIODIC_DATA\n");
			printf("Server is waiting......\n");
		}
		else{
			timereg=time_compare(time);
			if(timereg != time){
				printf("%d %d\n",timereg,time);
				printf("I'm Compare!! %d\n",fc);
				time = timereg;
				while(!ft);
				mysql_compare();
				printf("UServer is waiting......\n");
			}
		}
		fc=1;printf("abc2\n");
		sem_wait(&sem);
	}
	close(RxSocket);
}

int main()
{
	int serverFd, clientFd;
	int serverLen, clientLen;
	struct sockaddr_in serverAddr, clientAddr;
	char str[150000], *array;
    SunPonter pkt_out, pkt_in;
	int i, len;

	//DWORD t1,t2;//clock_t t1,t2;
	//int time_f=0;
	
	struct timeval tv_out;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
   
	mysql_connect();
   
   	pthread_t p1;
	sem_init(&sem,0,0);
	pthread_create(&p1,NULL,(void *)udp_thread,NULL);
   
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1) {
		printf("Socket Error");
		system("pause");
		exit(1);
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(9966);
	serverLen = sizeof(serverAddr);
   
	setsockopt(serverFd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
   
	if (bind(serverFd, (struct sockaddr *)&serverAddr, serverLen) < 0) {
		printf("Bind Error");
		system("pause");
		exit(1);
	}
   
	if (listen(serverFd, 5) < 0) {
		printf("Listen Error");
		system("pause");
		exit(1);
	}

/////test by alex wang start test for memory malloc and free
	char* testptr;
	char* testptr1;
	testptr=l_malloc(100);
	printf("testptr=%X",testptr);
	l_free(testptr);
	testptr1=l_malloc(50);
	printf("testptr1=%X",testptr1);

//// test by alex wang end
	
   
	clientLen = sizeof(clientAddr);


	
	while(1) {
		printf("MServer is waiting......\n");
		while(1){
				
			if(fc){
				printf("fc=%d\n",fc);
				//printf("Server is waiting......\n");
				clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
				printf("clientFd=%d\n",clientFd);
				if (clientFd != 0 && clientFd != -1){
					while(!fc);
					break;
				}
				else{
					printf("post\n");
					sem_post(&sem);
					printf("afterpost\n");
				}
			}
		}
		ft=0;
		printf("get from IP %s port %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		while (1){
			len = recv(clientFd, str, 4096, 0);
			if (len == 0 || len == -1)
				break;
			pkt_in = array2Packet(str);
			if (pkt_in != NULL) {
				showSunPacketInfo(pkt_in);
				if (pkt_in->commandType == AM_JOIN_REQUEST) {
					mysql_ArrayManager_Join(pkt_in,inet_ntoa(clientAddr.sin_addr));//
					pkt_out = generateSunPacket(AM_JOIN_RESPONSE);
					array = packet2Array(pkt_out);
					send(clientFd, array, pkt_out->length+4, 0);
					l_free(array);
					releasePacket(pkt_out);
				}
				else if (pkt_in->commandType == RTC_UPDATE_REQUEST) {
					pkt_out = generateSunPacket(RTC_UPDATE_RESPONSE);
					RTCtime(pkt_out);//
					array = packet2Array(pkt_out);
					send(clientFd, array, pkt_out->length+4, 0);
					l_free(array);
					releasePacket(pkt_out);
				}
				else if (pkt_in->commandType == PERIODIC_DATA) {
					mysql_PeriodicData(pkt_in,1);
				}
				else if (pkt_in->commandType == JB_JOIN_REQUEST) {
					mysql_JunctionBox_Join(pkt_in);
					//pkt_out = generateSunPacket(JB_JOIN_RESPNOSE);
					pkt_out = generateSunPacketJB(pkt_in);//generateSunPacket(JB_JOIN_RESPNOSE);
					//showSunPacketInfo(pkt_out);
					array = packet2Array(pkt_out);
					printf("packet2Array end \n");
					send(clientFd, array, pkt_out->length+4, 0);
					l_free(array);
					releasePacket(pkt_out);
				}
				else if (pkt_in->commandType == TCP_PERIODIC_LINK){
					mysql_TCPLink(pkt_in);
					//pkt_out=generateSunPacket(ARRAY_MANGER_SETTING);
					pkt_out=generateAMSetting();
					//showSunPacketInfo(pkt_out);
					array = packet2Array(pkt_out);
					send(clientFd, array, pkt_out->length+4, 0);
					l_free(array);
					releasePacket(pkt_out);
				}
				releasePacket(pkt_in);
			}
		}
		ft=1;
		close(clientFd);
		//closesocket(clientFd);
    }
   
   system("pause");
   return 0;
}

