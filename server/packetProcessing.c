#include <stdio.h>
#include <stdlib.h>

#include "mystdlib.h"
#include "packetPrototype.h"
#include "packetProcessing.h"
#ifdef M3_TEST	 	   
	#include "utils/lwiplib.h"
	#include "lwip/sockets.h"
#endif

#include <time.h>

extern char mysql_join[256];
extern int amcount,amset,TCPperiod;
int jbsize=0;

DateTime RTCResponse2DateTime(RTCResponsePonter pkt)
{
	DateTime date;

	date.years = 	pkt->years + 2000;
	date.months = 	pkt->months;
	date.date = 	pkt->date;
	date.hours = 	pkt->hours;
	date.minutes = pkt->minutes;

	return date;
}

AMJoinRequestPonter generateAMJoin()
{
	AMJoinRequestPonter ptr;
	int i;

	
	ptr = (AMJoinRequestPonter)l_malloc(SIZE_OF_AM_JOIN_REQUEST_PACKET);
	ptr->MAC[0] = 0xA8;
	ptr->MAC[1] = 0x00;
	ptr->MAC[2] = 0x00;
	ptr->MAC[3] = 0x00;
	ptr->MAC[4] = 0x00;
	ptr->MAC[5] = 0x02;
	ptr->MAC[6] = 0xde;
	ptr->MAC[7] = 0xff;

	ptr->portNumber = 8000;
	for (i = 0; i < 200; i++) {
		ptr->comfirmedCode[i] = i;
	}

	return ptr;
}

AMJoinResponsePointer generateAMJoinResponse()
{
    AMJoinResponsePointer ptr;
     
	ptr = (AMJoinResponsePointer)l_malloc(SIZE_OF_AM_JOIN_RESPONSE_PACKET);
	if(mysql_join[0]!=0)
		ptr->ack = mysql_join[0];
	return ptr;
}

PeriodicDataPointer generatePeriodicData(int size)
{
	JunctionBoxPointer ptr = NULL;
	PeriodicDataPointer pData;
	int i, j;
	
	pData = (PeriodicDataPointer)l_malloc(SIZE_OF_PERIODIC_DATA_PACKET);
	pData->stamp.date = 15;
	pData->stamp.hours = 18;
	pData->stamp.minutes = 11;
	pData->stamp.seconds = 40;
	
	pData->updatePeriod = 10;
	pData->nJunctionBoxes = size;
	
	ptr = (JunctionBoxPointer)l_malloc(size * SIZE_OF_JUNCTIONBOX_PACKET);
	pData->payload = (uint8 *)ptr;
	for (i = 0; i < size; i++) {
		for (j = 0; j < 8; j++) {
			ptr->MAC[j] = i * 8 + j;
		}
		ptr->diodeTemperature = (15 + i) * 100 + (18 + i);
		ptr->voltage = (100 + i) * 100;
		ptr->current = (i) * 100 + i;
		ptr->power = ptr->voltage * ptr->current / 100;
		ptr->state = GOOD;
		ptr = (JunctionBoxPointer)((int)ptr + SIZE_OF_JUNCTIONBOX_PACKET);
	}
	
	
	return pData;
}

SunPonter generateSunPacket(uint8 commandType)
{
	SunPonter pkt;
	PeriodicDataPointer pdata;
	RTCResponsePonter rtcrsp;
	
	JBJoinRequestPonter jbdata;


	

	pkt = (SunPonter)l_malloc(SIZE_OF_SUN_PACKET);
	pkt->headerCode = HEADER_CODE;
	pkt->length = SIZE_OF_SUN_PACKET;
	pkt->sequenceNumber = 0;
	pkt->commandType = commandType;
	pkt->nPayload = 0;
	pkt->payload = NULL;
	pkt->tailCode = TAIL_CODE;
	
	switch(commandType) {
	case AM_JOIN_REQUEST:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)generateAMJoin();
		pkt->length = AM_JOIN_WHOLE_SIZE;
	break;
	case AM_JOIN_RESPONSE:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)generateAMJoinResponse();
		pkt->length = AM_JRSP_WHOLE_SIZE;
	break;
	case RTC_UPDATE_REQUEST:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)l_malloc(SIZE_OF_RTC_REQUEST_PACKET);
		pkt->length = RTC_REQ_WHOLE_SIZE;
		*(pkt->payload) = CMD_RTC_UPDATE_REQUEST;
	break;
	case RTC_UPDATE_RESPONSE:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)l_malloc(SIZE_OF_RTC_RESPONSE_PACKET);
		pkt->length = RTC_RSP_WHOLE_SIZE;
		rtcrsp = (RTCResponsePonter)pkt->payload;
		rtcrsp->years = 12;
		rtcrsp->months = 11;
		rtcrsp->date = 12;
		rtcrsp->hours = 17;
		rtcrsp->minutes = 44;
     break;
	case PERIODIC_DATA:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)generatePeriodicData(100);
		pdata = (PeriodicDataPointer)pkt->payload;
		pkt->length = PRD_DTA_WHOLE_SIZE(pdata->nJunctionBoxes);
	break;
	case JB_JOIN_REQUEST:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)generateJBJoin(10);
		jbdata = (JBJoinRequestPonter)pkt->payload;
		pkt->length = JB_JOIN_WHOLE_SIZE(10);
		printf("JB_JOIN_REQUEST\n");
	break;
	case JB_JOIN_RESPNOSE:
		//pkt->nPayload = 1;
		//pkt->payload = (uint8 *)generateJBoinResponse();
		//pkt->length = JB_JRSP_WHOLE_SIZE();
	break;
	case TCP_PERIODIC_LINK:
		pkt->nPayload = 1;
		pkt->payload = (uint8 *)generateTCPLink();
		pkt->length = TCP_LINK_WHOLE_SIZE;
	break;
	case ARRAY_MANGER_SETTING:
		//pkt->nPayload = amcount;
		//pkt->payload = (uint8 *)generateAMSetting();
		//pkt->length = AM_SETTING_WHOLE_SIZE(amcount);
	break;
	}


	return pkt;
}

uint8 *packet2Array(SunPonter pkt)
{	
	uint8 *array, *ptr;
	int i, n;
	PeriodicDataPointer pDataPointer;
	JBJoinRequestPonter jbJoinPointer;
	printf("packet2Array start ,pkt=%x pkt->length=%d \n",pkt,pkt->length);
	if (pkt == NULL)
		return NULL;
	array = (uint8 *)l_malloc(pkt->length + 4);
	printf("array=%x \n",array);
	//Head
	ptr = (uint8 *)pkt;
	for (i = 0, n = 0; i < SIZE_BEFORE_PAYLOAD; i++, n++) {
		array[n] = ptr[i];
	}

	printf("commandType=%d \n",pkt->commandType );
	//Payload
	if (pkt->commandType == PERIODIC_DATA) {
		ptr = (uint8 *)pkt->payload;
		for (i = 0; i < SIZE_OF_PERIODIC_DATA_PACKET_LESS; i++, n++) {
			array[n] = ptr[i];
		}
		pDataPointer = (PeriodicDataPointer)pkt->payload;
		ptr = (uint8 *)pDataPointer->payload;
		for (i = 0; i < payLoadSize(pkt) - SIZE_OF_PERIODIC_DATA_PACKET_LESS; i++, n++) {
			array[n] = ptr[i];
		}
	}
	else if (pkt->commandType == JB_JOIN_REQUEST) {///////////////////
		
		ptr = (uint8 *)pkt->payload;
		for (i = 0; i < JB_JOIN_REQUEST_PACKET_LESS; i++, n++) {
			array[n] = ptr[i];
		}
		jbJoinPointer = (JBJoinRequestPonter)pkt->payload;
		ptr = (uint8 *)jbJoinPointer->payload;
		for (i = 0; i < payLoadSize(pkt) - JB_JOIN_REQUEST_PACKET_LESS; i++, n++) {
			array[n] = ptr[i];
		}
	}
	else {
		printf("packet2Array res \n");
		ptr = (uint8 *)pkt->payload;
		for (i = 0; i < payLoadSize(pkt); i++, n++) {
			array[n] = ptr[i];
		}
	}

	//Tail
	array[n++] = pkt->tailCode;
	array[n] = 0;
	if (pkt->length + 4 != n) {
		printf("\n\nERROR size not match: %d / %d\n\n", n, pkt->length + 4);
	}
	printf("packet2Array END \n");	
	return array;
}

SunPonter array2Packet(uint8 * array)
{	
	uint8 *ptr;
	SunPonter pkt;
	int i, n;
	PeriodicDataPointer pDataPointer;
	JunctionBoxPointer ppp;
	
	JBJoinRequestPonter jbJoinPointer;

	if (array == NULL)
		return NULL;
	if (array[0] != HEADER_CODE)
		return NULL;
	pkt = (SunPonter)l_malloc(SIZE_OF_SUN_PACKET);
	//Head
	ptr = (uint8 *)pkt;
	for (i = 0, n = 0; i < SIZE_BEFORE_PAYLOAD; i++, n++) {
		ptr[i] = array[n];
	}
	
	//Payload
	if (pkt->commandType == PERIODIC_DATA) {
		//First stage payload=====================
		pkt->payload = (uint8 *)l_malloc(SIZE_OF_PERIODIC_DATA_PACKET_LESS);

		ptr = (uint8 *)pkt->payload;
		for (i = 0; i < SIZE_OF_PERIODIC_DATA_PACKET_LESS; i++, n++) {
			ptr[i] = array[n];
		}
		
		//Second stage payload=====================
		pDataPointer = (PeriodicDataPointer)pkt->payload;
		pDataPointer->payload = (uint8 *)l_malloc(payLoadSize(pkt) - SIZE_OF_PERIODIC_DATA_PACKET_LESS);
		ptr = (uint8 *)pDataPointer->payload;
		for (i = 0; i < payLoadSize(pkt) - SIZE_OF_PERIODIC_DATA_PACKET_LESS; i++, n++) {
			ptr[i] = array[n];
		}
	}
	else if (pkt->commandType == JB_JOIN_REQUEST) {/////////////////
		//First stage payload=====================
		pkt->payload = (uint8 *)l_malloc(JB_JOIN_REQUEST_PACKET_LESS);

		ptr = (uint8 *)pkt->payload;
		for (i = 0; i < JB_JOIN_REQUEST_PACKET_LESS; i++, n++) {
			ptr[i] = array[n];
		}
		
		//Second stage payload=====================
		jbJoinPointer = (JBJoinRequestPonter)pkt->payload;
		jbJoinPointer->payload = (uint8 *)l_malloc(payLoadSize(pkt) - JB_JOIN_REQUEST_PACKET_LESS);
		ptr = (uint8 *)jbJoinPointer->payload;
		for (i = 0; i < payLoadSize(pkt) - JB_JOIN_REQUEST_PACKET_LESS; i++, n++) {
			ptr[i] = array[n];
		}
	}
	else {
		pkt->payload = (uint8 *)l_malloc(payLoadSize(pkt));
		ptr = (uint8 *)pkt->payload;
		for (i = 0; i < payLoadSize(pkt); i++, n++) {
			ptr[i] = array[n];
		}
	}
	
	//Tail
	pkt->tailCode = array[n++];

	//Check packet length
	if ((pkt->length + 4) != n || pkt->tailCode != TAIL_CODE) {
		l_free(pkt->payload);
		l_free(pkt);
		
		printf("\n\nERROR size not match: %d / %d or tail code %X\n\n", n, pkt->length + 4, pkt->tailCode);
		return NULL;
	}
	
	return pkt;
}

void AM_Join(int Remotefd)
{
	SunPonter pkt;
	uint8 *array;

	pkt = generateSunPacket(AM_JOIN_REQUEST);
	array = packet2Array(pkt);
	send( Remotefd, array, pkt->length+5,0);
	
	l_free(pkt->payload);
	l_free(pkt);
	l_free(array);
	
	array = (uint8 *)l_malloc(512);
	recv(Remotefd, array, 512, 0);
	pkt = array2Packet(array);
	showSunPacketInfo(pkt);
	l_free(array);
	if (pkt != NULL) {
		if (pkt->commandType == AM_JOIN_RESPONSE) {
			if (*(pkt->payload) != JOIN_ACK) {
				printf("NACK\n");
			}
			else {
				printf("ACK\n");
			}
		}
		else
			printf("commandType error\n");
		
		l_free(pkt->payload);
		l_free(pkt);
	}
	else {
		printf("Packet Crushed\n");
	}
}

DateTime RTC_Request(int Remotefd)
{
	SunPonter pkt;
	uint8 *array;
	DateTime date;
	
	pkt = generateSunPacket(RTC_UPDATE_REQUEST);
	array = packet2Array(pkt);
	send(Remotefd, array, pkt->length+5,0);
	
	if (pkt != NULL) {
		l_free(pkt->payload);
		l_free(pkt);
		l_free(array);
	}
	
	array = (uint8 *)l_malloc(512);
	recv(Remotefd, array, 512, 0);
	pkt = array2Packet(array);
	if (pkt != NULL) {
		showSunPacketInfo(pkt);
		if (pkt->commandType == RTC_UPDATE_RESPONSE) {
			date = RTCResponse2DateTime((RTCResponsePonter)pkt->payload);
			//TODO Deal with the "date".
		}
		else
			printf("commandType error\n");
		
		l_free(pkt->payload);
		l_free(pkt);
		l_free(array);
	}
	else {
		printf("Packet Crushed\n");
	}
	
	return date;
}

void sendPeriodicData(int Remotefd)
{
	SunPonter pkt;
	uint8 *array;
	DateTime date;
	PeriodicDataPointer p;
	
	pkt = generateSunPacket(PERIODIC_DATA);
	array = packet2Array(pkt);
	send(Remotefd, array, pkt->length+5,0);
	
	if (pkt != NULL) {
		p = (PeriodicDataPointer)pkt->payload;
		l_free(p->payload);
		l_free(pkt->payload);
		l_free(pkt);
		l_free(array);
	}
	
	return ;
}

void showSunPacketInfo(SunPonter pkt){
	AMJoinRequestPonter amReqPtr = NULL;
	AMJoinResponsePointer amRspPtr = NULL;
	RTCRequestPonter rtcReqPtr = NULL;
	RTCResponsePonter rtcRspPtr = NULL;
	TimeStampPonter timeStampPtr = NULL;
	JunctionBoxPointer jBoxPtr = NULL;
	PeriodicDataPointer pdcPtr = NULL;
	
	JBJoinRequestPonter jbReqPtr = NULL;
	JBJoinInfoPonter jbInfoPtr = NULL;
	JBJoinResponsePointer jbRspPtr = NULL;
	
	TCPPeriodicLinkPointer tcplinkPtr = NULL;
	AMSettingPointer amsetPtr = NULL;
	
	int i, j, n;
	
	printf("==========Sun Packet==========\n\r");
	printf("Header Code : 0x%X\n\r", pkt->headerCode);
	printf("Packet Len. : %d\n\r", pkt->length);
	printf("Sequence #. : %d\n\r", pkt->sequenceNumber);
	printf("Packet Type : 0x%X\n\r", pkt->commandType);
	printf("Header Code : %d\n\r", pkt->nPayload);
	printf("Payload Adr : 0x%X\n\r", (int)(pkt->payload));
	printf("Tail Code   : 0x%X\n\r", pkt->tailCode);
	printf("----------");
	switch(pkt->commandType) {
	case AM_JOIN_REQUEST:
		printf("AMJOIN_REQ----------\n\r");
		amReqPtr = (AMJoinRequestPonter)pkt->payload;
		printf("MAC Address  : %02X:%02X:%02X:%02X:%02X:%02X\n\r", amReqPtr->MAC[0], amReqPtr->MAC[1], amReqPtr->MAC[2], amReqPtr->MAC[3], 
																amReqPtr->MAC[4], amReqPtr->MAC[5]);
		printf("Port Number  : %d\n\r", amReqPtr->portNumber);
		printf("Confirm Code : \n\r");
		for (i = 0; i < 200; i++) {
			if (i % 16 == 0) {
				printf("\n%02X: ", i / 16);
			}
			printf("%02X, ", amReqPtr->comfirmedCode[i] & 0x00ff);
		}
		printf("\n\r");
	break;
	case AM_JOIN_RESPONSE:
		printf("AMJOIN_RSP----------\n\r");
		amRspPtr = (AMJoinResponsePointer)pkt->payload;
		printf("Acknowledge  : ");
		switch(amRspPtr->ack) {
		case JOIN_ACK:
			printf("Join is Successful.\n\r");
			break;
		case JOIN_NACK_MAC:
			printf("MAC Doesn't Exist.\n\r");
			break;
		case JOIN_NACK:
			printf("Retry on Next Time Period.\n\r");
			break;
		default:
			printf("ERROR: Unknown type\n\r");
			break;
		}
	break;
	case RTC_UPDATE_REQUEST:
		rtcReqPtr = (RTCRequestPonter)pkt->payload;
		printf("RTC_UP_REQ----------\n\r");
		
		if (rtcReqPtr->type == CMD_RTC_UPDATE_REQUEST)
			printf("RTCupdate req: Request\n\r");
		else
			printf("RTCupdate req: Unknown type\n\r");
	break;
	case RTC_UPDATE_RESPONSE:
		printf("RTC_UP_RSP----------\n\r");
		rtcRspPtr = (RTCResponsePonter)pkt->payload;
		
		printf("RTCupdate rsp: %d, %d/%d, %02d'%02d''\n\r", (int)(rtcRspPtr->years) + 2000, rtcRspPtr->months, rtcRspPtr->date, rtcRspPtr->hours, rtcRspPtr->minutes);
     break;
	case PERIODIC_DATA:
		printf("PERIODIC_D----------\n\r");
		pdcPtr = (PeriodicDataPointer)pkt->payload;
		timeStampPtr = &(pdcPtr->stamp);
		jBoxPtr = (JunctionBoxPointer)pdcPtr->payload;
		n = pdcPtr->nJunctionBoxes;
		
		printf("time stamp   : %d, %02d:%02d:%02d\n\r", timeStampPtr->date, timeStampPtr->hours, timeStampPtr->minutes, timeStampPtr->seconds);
		printf("update period: %d\n\r", pdcPtr->updatePeriod);
		printf("# of J. boxes: %d\n\r", pdcPtr->nJunctionBoxes);
		printf("J. boxes data:\n\r");
		for (i = 0; i < n; i++) {
			printf("\tMAC Address  : %02X:%02X:%02X:%02X:%02X:%02X\n\r", jBoxPtr->MAC[0], jBoxPtr->MAC[1], jBoxPtr->MAC[2], jBoxPtr->MAC[3], 
																jBoxPtr->MAC[4], jBoxPtr->MAC[5]);
			printf("\tD. tempture  : %d\n\r", jBoxPtr->diodeTemperature);
			printf("\tVoltage      : %4.2f\n\r", jBoxPtr->voltage/100.0);// / 100.0);
			printf("\tCurrent      : %4.2f\n\r", jBoxPtr->current/100.0);// / 100.0);
			printf("\tPower        : %4.2f\n\r", jBoxPtr->power/100.0);// / 100.0);
			printf("\tStatus       : %04X\n\n\r", jBoxPtr->state);
			jBoxPtr = (JunctionBoxPointer)((int)jBoxPtr + SIZE_OF_JUNCTIONBOX_PACKET);
		}
	break;
	case JB_JOIN_REQUEST:
		printf("JBJOIN_REQ----------\n\r");
		jbReqPtr = (JBJoinRequestPonter)pkt->payload;
		printf("AM_MAC Address      : %02X:%02X:%02X:%02X:%02X:%02X\n\r", jbReqPtr->AM_MAC[0], jbReqPtr->AM_MAC[1], jbReqPtr->AM_MAC[2], jbReqPtr->AM_MAC[3], 
																jbReqPtr->AM_MAC[4], jbReqPtr->AM_MAC[5]);
		printf("Number JunctionBox  : %d\n\r", jbReqPtr->nJunctionBoxes);
		n = jbReqPtr->nJunctionBoxes;
		jbsize=n;
		jbInfoPtr = (JBJoinInfoPonter)jbReqPtr->payload;
		for (j = 0; j < n; j++){
			//jbInfoPtr = (JBJoinInfoPonter)jbReqPtr->payload;
			printf("JB_MAC Address      : %02X:%02X:%02X:%02X:%02X:%02X\n\r", jbInfoPtr->JB_MAC[0], jbInfoPtr->JB_MAC[1], jbInfoPtr->JB_MAC[2], jbInfoPtr->JB_MAC[3], 
																	jbInfoPtr->JB_MAC[4], jbInfoPtr->JB_MAC[5]);

			printf("SerialNumber        : ");
			printf("%s",jbInfoPtr->SerialNumber);
			printf("\n\r");
			printf("FirmwareVersion     : ");
			printf("%s",jbInfoPtr->FirmwareVersion);
			printf("\n\r");
			printf("HardwareVersion     : ");
			printf("%s",jbInfoPtr->HardwareVersion);
			printf("\n\r");
			printf("DeviceSpecification : ");
			printf("%s",jbInfoPtr->DeviceSpecification);
			printf("\n\r");
			printf("ManufactureData     : ");
			printf("%s",jbInfoPtr->ManufactureData);
			printf("\n\n\r");
			jbInfoPtr = (JBJoinInfoPonter)((int)jbInfoPtr + SIZE_OF_JB_JOIN_INFO_PACKET);
		}
		printf("\n\r");
	break;
	case JB_JOIN_RESPNOSE:
		printf("JBJOIN_RSP----------\n\r");
		jbRspPtr = (JBJoinResponsePointer)pkt->payload;
		for(i=0;i<jbsize;i++){
			printf("JB_MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n\r", jbRspPtr->JB_MAC[0], jbRspPtr->JB_MAC[1], jbRspPtr->JB_MAC[2], jbRspPtr->JB_MAC[3], 
																	jbRspPtr->JB_MAC[4], jbRspPtr->JB_MAC[5]);
			printf("Acknowledge    : ");
			switch(jbRspPtr->ack) {
			case 0x02:
				printf("Join is Successful.\n\r");
				break;
			case 0x03:
				printf("MAC Doesn't Exist.\n\r");
				break;
			case 0x04:
				printf("Retry on Next Time Period.\n\r");
				break;
			default:
				printf("ERROR: Unknown type\n\r");
				break;
			}
			jbRspPtr = (JBJoinResponsePointer)((int)jbRspPtr + SIZE_OF_JB_JOIN_RESPONSE_PACKET);
        }
	break;
	case TCP_PERIODIC_LINK:
		printf("TCP_LINK------------\n\r");
		tcplinkPtr = (TCPPeriodicLinkPointer)pkt->payload;
		printf("AMRTCTime     :20%d/%d/%d %d:%d:%d\n",tcplinkPtr->AMRTCTime[0],tcplinkPtr->AMRTCTime[1],tcplinkPtr->AMRTCTime[2],tcplinkPtr->AMRTCTime[3],tcplinkPtr->AMRTCTime[4],tcplinkPtr->AMRTCTime[5]);
		printf("UpPeriod      :%d\n",tcplinkPtr->UpPeriod);
		printf("TCPLinkPeriod :%d\n",tcplinkPtr->TCPLinkPeriod);
	break;
	case ARRAY_MANGER_SETTING:
		printf("AM_ARRAY------------\n\r");
		amsetPtr = (AMSettingPointer)pkt->payload;
		for(i=0;i<pkt->nPayload;i++){
            printf("SettingType :%d\n",amsetPtr->SettingDate[0]);
			switch(amsetPtr->SettingDate[0]){
			case UPDATE_RTC :
				printf("SettingDate :20%d/%d/%d %d:%d:%d\n",amsetPtr->SettingDate[1],amsetPtr->SettingDate[2],amsetPtr->SettingDate[3],amsetPtr->SettingDate[4],amsetPtr->SettingDate[5],amsetPtr->SettingDate[6]);
				printf("----------UPDATE_RTC----------\n\r");
				amsetPtr = (AMSettingPointer)((int)amsetPtr + 7);
			break;
			case UPADTE_PERIOD :
				printf("SettingDate :%d\n",(amsetPtr->SettingDate[1]<<8)|amsetPtr->SettingDate[2]);
				printf("---------UPADTE_PERIOD--------\n\r");
				amsetPtr = (AMSettingPointer)((int)amsetPtr + 3);
			break;
			case TCP_LINK_PERIOD :
				printf("SettingDate :%d\n",(amsetPtr->SettingDate[1]<<8)|amsetPtr->SettingDate[2]);
				printf("--------TCP_LINK_PERIOD-------\n\r");
				amsetPtr = (AMSettingPointer)((int)amsetPtr + 3);
			break;
			case RESET_ARRAY_MANGER :
				printf("SettingDate :%02X %02X %02X %02X\n",amsetPtr->SettingDate[1],amsetPtr->SettingDate[2],amsetPtr->SettingDate[3],amsetPtr->SettingDate[4]);
				printf("------RESET_ARRAY_MANGER------\n\r");
				amsetPtr = (AMSettingPointer)((int)amsetPtr + 5);
			break;
			case RESET_JUNCTION_BOX :
				printf("SettingDate :%02X %02X %02X %02X %02X %02X\n",amsetPtr->SettingDate[1],amsetPtr->SettingDate[2],amsetPtr->SettingDate[3],amsetPtr->SettingDate[4],amsetPtr->SettingDate[5],amsetPtr->SettingDate[6]);
				printf("------RESET_JUNCTION_BOX------\n\r");
				amsetPtr = (AMSettingPointer)((int)amsetPtr + 7);
			break;		
			}
        }
	break;
	}
	printf("==========End Packet==========\n\r");
}

void releasePacket(SunPonter pkt){
	if (pkt->commandType == PERIODIC_DATA) {
		l_free(((PeriodicDataPointer)(pkt->payload))->payload);
	}
	else if (pkt->commandType == JB_JOIN_REQUEST) {
		l_free(((JBJoinRequestPonter)(pkt->payload))->payload);
	}
	l_free(pkt->payload);
	l_free(pkt);
}

void JB_Join(int Remotefd)
{
	SunPonter pkt;
	uint8 *array;

	pkt = generateSunPacket(JB_JOIN_REQUEST);
	array = packet2Array(pkt);
	send( Remotefd, array, pkt->length+5,0);
	//showSunPacketInfo(array2Packet(array));
	//printf("%d\n",pkt->length);
	
	l_free(pkt->payload);
	l_free(pkt);
	l_free(array);
	
	array = (uint8 *)l_malloc(512);
	recv(Remotefd, array, 512, 0);
	pkt = array2Packet(array);
	showSunPacketInfo(pkt);
	l_free(array);
	
}

JBJoinRequestPonter generateJBJoin(int number){
	JBJoinInfoPonter ptr = NULL;
	JBJoinRequestPonter jbData;
	int i;
	
	jbData = (JBJoinRequestPonter)l_malloc(SIZE_OF_JB_JOIN_REQUEST_PACKET);
	
	jbData->AM_MAC[0] = 0xA8;
	jbData->AM_MAC[1] = 0x00;
	jbData->AM_MAC[2] = 0x00;
	jbData->AM_MAC[3] = 0x00;
	jbData->AM_MAC[4] = 0x00;
	jbData->AM_MAC[5] = 0x02;
	jbData->nJunctionBoxes = number;
	
	ptr = (JBJoinInfoPonter)l_malloc(number*SIZE_OF_JB_JOIN_INFO_PACKET);////modify by alex number*
	jbData->payload = (uint8 *)ptr;
	for (i = 0; i < 6; i++) 
		ptr->JB_MAC[i] =i;
	
	for (i = 0; i < 24; i++){
		ptr->SerialNumber[i]=i;
		ptr->FirmwareVersion[i]=23-i;
		ptr->HardwareVersion[i]=i*2;
		ptr->DeviceSpecification[i]=i*4;
	}
	
	for (i = 0; i < 8; i++)
		ptr->ManufactureData[i]=i+2;
	
	return jbData;
}

SunPonter generateSunPacketJB(SunPonter pkt){
	int i,j;
	SunPonter pkt2;
	
	JBJoinRequestPonter jbReqPtr = NULL;
	JBJoinInfoPonter jbInfoPtr = NULL;
	JBJoinResponsePointer jbRspPtr = NULL;
	
	jbReqPtr = (JBJoinRequestPonter)pkt->payload;
	jbInfoPtr = (JBJoinInfoPonter)jbReqPtr->payload;
	
	pkt2 = (SunPonter)l_malloc(SIZE_OF_SUN_PACKET);
	pkt2->headerCode = HEADER_CODE;
	pkt2->length = SIZE_OF_SUN_PACKET;
	pkt2->sequenceNumber = 0;
	pkt2->commandType = JB_JOIN_RESPNOSE;
	pkt2->nPayload = jbReqPtr->nJunctionBoxes;
	pkt2->payload = NULL;
	pkt2->tailCode = TAIL_CODE;
	printf("generateSunPacketJB \n");
	printf("generateSunPacketJB Mem=%d \n",jbReqPtr->nJunctionBoxes * SIZE_OF_JB_JOIN_RESPONSE_PACKET);
	jbRspPtr = (JBJoinResponsePointer)l_malloc((jbReqPtr->nJunctionBoxes * SIZE_OF_JB_JOIN_RESPONSE_PACKET));
	pkt2->payload = (uint8 *)jbRspPtr;
	printf("jbRspPtr =%x \n",jbRspPtr);
	for(j=0;j<jbReqPtr->nJunctionBoxes;j++){
		for(i=0;i<6;i++)
			jbRspPtr->JB_MAC[i]=jbInfoPtr->JB_MAC[i];
		if(mysql_join[j]!=0)
			jbRspPtr->ack = mysql_join[j];
		jbRspPtr = (JBJoinResponsePointer)((int)jbRspPtr + SIZE_OF_JB_JOIN_RESPONSE_PACKET);
		jbInfoPtr = (JBJoinInfoPonter)((int)jbInfoPtr + SIZE_OF_JB_JOIN_INFO_PACKET);
	}
	pkt2->length = JB_JRSP_WHOLE_SIZE(jbReqPtr->nJunctionBoxes);
	
	return pkt2;
}

void TCP_Link(int Remotefd){
	SunPonter pkt;
	uint8 *array;

	pkt = generateSunPacket(TCP_PERIODIC_LINK);
	array = packet2Array(pkt);
	//send( Remotefd, array, pkt->length+5,0);
	//showSunPacketInfo(array2Packet(array));
	//printf("%d\n",pkt->length);
}

TCPPeriodicLinkPointer generateTCPLink(){
	TCPPeriodicLinkPointer ptr;
	int i;
	
	ptr = (TCPPeriodicLinkPointer)l_malloc(SIZE_OF_TCP_PERIODIC_LINK_PACKET);
	ptr->AMRTCTime[0] = 0x0c;
	ptr->AMRTCTime[1] = 0x03;
	ptr->AMRTCTime[2] = 0x07;
	ptr->AMRTCTime[3] = 0x0e;
	ptr->AMRTCTime[4] = 0x14;
	ptr->AMRTCTime[5] = 0x0f;
	ptr->UpPeriod = 0x0014;
	ptr->TCPLinkPeriod = 0x003c;

	return ptr;
}

SunPonter generateAMSetting(){
	
	SunPonter pkt;
	AMSettingPointer ptr;
	int i;

	pkt = (SunPonter)l_malloc(SIZE_OF_SUN_PACKET);
	pkt->headerCode = HEADER_CODE;
	pkt->sequenceNumber = 0;
	pkt->commandType = ARRAY_MANGER_SETTING;
	pkt->nPayload = amcount;
	pkt->payload = NULL;
	pkt->tailCode = TAIL_CODE;
	
	char RTC=0x01,UDP=0x02,TCP=0x04,CH=0x08,RST=0x10;
	char tempspace=0;
	if((amset&RTC)==RTC)
		tempspace=7;
	if((amset&TCP)==TCP)
		tempspace+=3;
	pkt->length = AM_SETTING_WHOLE_SIZE(tempspace);
	
	time_t timep;
	struct tm *p;
	time(&timep);
	p=localtime(&timep);
	
	ptr = (AMSettingPointer)l_malloc(tempspace);
	pkt->payload = (uint8 *)ptr;
	if((amset&RTC)==RTC){
		ptr->SettingDate[0] = 0x00;
		ptr->SettingDate[1] = p->tm_year-100;
		ptr->SettingDate[2] = p->tm_mon+1;
		ptr->SettingDate[3] = p->tm_mday;
		ptr->SettingDate[4] = p->tm_hour;
		ptr->SettingDate[5] = p->tm_min;
		ptr->SettingDate[6] = p->tm_sec;
		ptr = (AMSettingPointer)((int)ptr + 7);
	}
	if((amset&TCP)==TCP){
		ptr->SettingDate[0] = 0x02;
		ptr->SettingDate[1] = (TCPperiod&0x0000FF00)>>8;
		ptr->SettingDate[2] = (TCPperiod&0x000000FF);
		ptr = (AMSettingPointer)((int)ptr + 3);
	}

	return pkt;
}



int testmalloc(int size)
{
	char*bufptr;
	//bufptr=malloc(size);
	bufptr=testmy_malloc(size);
	printf("testmalloc%x\n",bufptr);	
	return (int)bufptr;
}


void testfree(char*bufptr)
{
	printf("testfree%x\n",bufptr);
	//free(bufptr);
	testmy_free(bufptr);
	printf("testfree end\n");
}

