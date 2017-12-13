#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mystdlib.h"
#include "packetPrototype.h"
#include "packetProcessing.h"

#include "mysql_sum.h"

#include <time.h>

#include "/usr/include/mysql/mysql.h"

#include <semaphore.h>
extern sem_t sem2;
extern int pf;

char query[5000]="";//,query2[1000]="";
char mysql_join[256];
int am_id,block_id,pos;
int amcount,amset,TCPperiod;
char jbmac[6];

int block[256]={0};
char JB_mac[256][5]={0};
int JB_block[256][3]={0};//temp power state

MYSQL mysql;
char host[100]="localhost",user[100]="pvweb",pwd[100]="pvweb",dbname[100]="PVWEB";

void SJBInfo_UpdateFirmeareV(JBJoinInfoPonter jbInfoptr);

void mysql_connect(void){
	mysql_close(&mysql);
	if(!mysql_init(&mysql)){
		printf("Initialization fails.\n");
		exit(1);
	}
	
	if(!mysql_real_connect(&mysql,host,user,pwd,dbname,0,NULL,0)){
		printf("%s\nmysql connect fail!!\n",mysql_error(&mysql));
		exit(1);
	}
}

void mysql_ArrayManager_Join(SunPonter pkt,char client_ip[]){
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	
	AMJoinRequestPonter amReqPtr = NULL;
	amReqPtr = (AMJoinRequestPonter)pkt->payload;
	
	int t,i,count;
	strsub(query,strlen(query));
	strcat(query,"SELECT * from ArrayManager WHERE `mac` = '");
	for(i=1;i<6;i++)
		sql_char(amReqPtr->MAC[i]);
	strcat(query,"'");// && `register_time` IS NULL");
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	
	count=0;
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		printf("\n");
		//AM_Join
		if(row[AM_register_time]!=NULL){
			sql_AMjoin(0,client_ip,amReqPtr);
			printf("Already Register and ArrayManager Join!!\n");
		}
		else{
			sql_AMjoin(1,client_ip,amReqPtr);
			printf("new Register and ArrayManager Join!!\n");
		}
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
		//new block
		if(row[AM_register_time]==NULL){
			sql_block(row[AM_am_id]);
			t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
			if(t)
				printf("Error making query: %s\n",mysql_error(&mysql));
		}
		count++;
	}
	mysql_free_result(res);
	res=NULL;
	
	if(count!=0){
		mysql_join[0]=0x02;
		printf("Join is Successful.\n");
	}
	else{
		mysql_join[0]=0x03;
		printf("MAC Doesn't Exist.\n");
	}
}

void mysql_JunctionBox_Join(SunPonter pkt){
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	
	JBJoinRequestPonter jbReqPtr = NULL;
	JBJoinInfoPonter jbInfoPtr = NULL;
	
	jbReqPtr = (JBJoinRequestPonter)pkt->payload;
	jbInfoPtr = (JBJoinInfoPonter)jbReqPtr->payload;
	
	int t,i,j;
	
	//read am_id
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM ArrayManager WHERE `mac` = '");
	for(i=1;i<6;i++)
		sql_char(jbReqPtr->AM_MAC[i]);
	strcat(query,"'");
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		am_id=atoi(row[AM_am_id]); 
	}
	mysql_free_result(res);
	res=NULL;
	//read block
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM Block WHERE `am_id` = ");
	sql_int(am_id);
	strcat(query," ORDER BY block_id LIMIT 1");
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		block_id=atoi(row[Block_block_id]); 
	}
	mysql_free_result(res);
	res=NULL;
	//read pos max 
	
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM JunctionBox WHERE `block_id` = ");
	sql_int(block_id);
	strcat(query," ORDER BY pos DESC LIMIT 1");
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	pos=0;
	while(row = mysql_fetch_row(res)){
		pos=atoi(row[JB_pos])+1; 
	}
	if(pos==0)
		pos=1;
	mysql_free_result(res);
	res=NULL;
	//printf("%d \t",pos);
	//Jbjoin

	for(i=0;i<jbReqPtr->nJunctionBoxes;i++){
		sql_JBjoin(jbInfoPtr);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		for(j=0;j<6;j++)
			printf("%02X ",jbInfoPtr->JB_MAC[j]);
		if(t){
			//printf("Error making query: %s\n",mysql_error(&mysql));
			printf("JunctionBox Mac is Exist.\n");
			
			SJBInfo_UpdateFirmeareV(jbInfoPtr);
                        

		}
		else{
			printf("JunctionBox Join is Successful.\n");
			pos++;
		}
		mysql_join[i]=0x02;
		jbInfoPtr = (JBJoinInfoPonter)((int)jbInfoPtr + SIZE_OF_JB_JOIN_INFO_PACKET);
	}
	
}

void SJBInfo_UpdateFirmeareV(JBJoinInfoPonter jbInfoPtr) 
{ 
			
		int j;	
		int i;
		int t;	
			 strsub(query,strlen(query));
                         strcat(query, "UPDATE `JunctionBox` SET `firmware_vs`= 0x");
                         i=0;
                         while(jbInfoPtr->FirmwareVersion[i]!='\0')
                         sql_char(jbInfoPtr->FirmwareVersion[i++]);
 
                         strcat(query," WHERE `jb_mac` = 0x");
                         for( j=1;j<6;j++)
                         sql_char(jbInfoPtr->JB_MAC[j]);/////////////////////alex add for firmware update 
                         t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
                         if(t)
                         printf("Error making query: %s\n",mysql_error(&mysql));



}


void mysql_PeriodicData(SunPonter pkt,int flag){
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	
	int n,i,j,t;
	
	JunctionBoxPointer jBoxPtr = NULL;
	PeriodicDataPointer pdcPtr = NULL;
	TimeStampPonter timeStampPtr = NULL;
	pdcPtr = (PeriodicDataPointer)pkt->payload;
	timeStampPtr = &(pdcPtr->stamp);
	jBoxPtr = (JunctionBoxPointer)pdcPtr->payload;
	n = pdcPtr->nJunctionBoxes;

	//updata junctionbox
	for (i = 0; i < n; i++) {
		sql_PD(pdcPtr,i,0);
		//printf("%s\n",query);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
	}
	
	if(flag){

	
		//insert paneldata
		sql_PD(pdcPtr,n,1);
		//printf("%s\n",query);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));

		//insert blockdata		
		strsub(query,strlen(query));
		strcat(query,"SELECT `block_id` , SUM(`power`)`power` FROM `JunctionBox` GROUP BY `block_id`");
		//printf("%s\n",query);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
		res = mysql_store_result(&mysql);

		strsub(query,strlen(query));
		strcat(query, "INSERT `BlockData` (`block_id` , `power` , `time`) VALUES ");
		while(row = mysql_fetch_row(res)){
			if(row[1]!=0){
				strcat(query,"(");
				strcat(query,row[0]);
				strcat(query," , ");
				strcat(query,row[1]);
				strcat(query, " , '");
				//sql_time();
				sql_devicetime(timeStampPtr);
				strcat(query, "'");
				strcat(query,"),");
			}
		}
		mysql_free_result(res);
		res=NULL;
		strsub(query,1);
		//printf("%s\n",query);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
		printf("JunctionBox Update and PanelData Insert and BlockDate Insert Success!!\n");

#if 0
		
		CHECK_IF_DUMYZERO_EXIST(pdcPtr,n,1);

		Insert_Dummy_Zero2Pannel_Block_Tab(pdcPtr,n,1);
#endif		
	}
	else
		printf("JunctionBox Update Success!!\n");
}

void mysql_TCPLink(SunPonter pkt){
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	int n,i,j,t;
	char RTC=0x01,UDP=0x02,TCP=0x04,CH=0x08,RST=0x10;
	
	time_t timep;
	struct tm *p;
	
	TCPPeriodicLinkPointer tcplinkPtr = NULL;
	tcplinkPtr = (TCPPeriodicLinkPointer)pkt->payload;

	time(&timep);
	p=localtime(&timep);
	
	//block_id
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM JunctionBox WHERE `jb_mac` = 0x");
	for(i=1;i<6;i++)
		sql_char(jbmac[i]);
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		block_id=atoi(row[JB_block_id]);
		//printf("%d\n",block_id);
	}
	mysql_free_result(res);
	res=NULL;
	//am_id
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM Block WHERE `block_id` = ");
	sql_int(block_id);
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		am_id=atoi(row[Block_am_id]);
		//printf("%d\n",am_id);
	}
	mysql_free_result(res);
	res=NULL;
	
	//
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM  ArrayManager WHERE `am_id` = ");
	sql_int(am_id);
	//printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		TCPperiod=atoi(row[AM_period]);
		//printf("%d\n",TCPperiod);
	}
	mysql_free_result(res);
	res=NULL;
	//
	
	amcount=amset=0;
	if(p->tm_min != tcplinkPtr->AMRTCTime[4]){
		amcount++;
		amset=RTC;
	}
	if(tcplinkPtr->TCPLinkPeriod != TCPperiod){
		amcount++;
		amset|=TCP;
	}
	
	printf("ARRAY_MANGER_SETTING Success!!\n");
}

void mysql_compare(void){
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	int t,i,j,index;
	int SetSJBZero=0;
	int updateAllBlock=0;
	int count=0;
	char SJB_State[50];
	

	char HIGH=0x10,LOW=0x08,HOT=0x04,COLD=0x02,OPEN=0x01;
	//block data number
	strsub(query,strlen(query));
	strcat(query,"SELECT * FROM Block ORDER BY block_id");
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		block[count++]=atoi(row[Block_block_id]);
		//printf("%d\n",block[count-1]);
	}
	mysql_free_result(res);
	res=NULL;
	//
	for(i=0;i<count;i++){
		strsub(query,strlen(query));
		strcat(query,"SELECT * FROM JunctionBox WHERE `block_id` =");
		sql_int(block[i]);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
		res = mysql_store_result(&mysql);
		index=0;
		//jb_mac time power temp
		while(row = mysql_fetch_row(res)){
			//printf("row[JB_state]=%s \n",(row[JB_state]));//
			
			if(!row[JB_state])///check if string = 0
				{
				strcpy(SJB_State,"NULL");		
				}
			else{
				strcpy(SJB_State,row[JB_state]);				
				}
			if(atoi(row[JB_power]) != 0 || (strcmp(SJB_State,"offline") !=0)){

			//printf("row[JB_state]=%s \n",(row[JB_state]));//
			//printf("row[JB_power]=%d \n",atoi(row[JB_power]));

			for(j=0;j<5;j++){
				JB_mac[index][j]=row[JB_jb_mac][j];
				//printf("%02X ",(unsigned char)JB_mac[index][j]);
			}
			
			if(!TimeCompare(row[JB_update_time])  )
				{
				JB_block[index][2]=0;
				printf("Time Clear SJB \n");//
				}
			else if ( atoi(row[JB_voltage]) ==0  )
				{
				//printf("row[JB_state]=%s \n",(row[JB_state]));
				printf("Voltage Clear SJB \n");//
				JB_block[index][2]=0;
				}
			else
				{
				JB_block[index][2]=OPEN;
				}

			JB_block[index][1]=atoi(row[JB_power]);
			JB_block[index++][0]=atoi(row[JB_temp]);
			}
			

			

		
			//printf("%d \t%d \t%d\n",JB_block[index][0],JB_block[index][1],JB_block[index][2]);
		}
		mysql_free_result(res);
		res=NULL;
		//printf("\n");
		//
		int temp[2],max[2],min[2];//temp power
		max[0]=max[1]=0x80000000;
		min[0]=min[1]=0x7FFFFFFF;
		temp[0]=temp[1]=0;
		for(j=0;j<index;j++){
			temp[0]+=JB_block[j][0];
			if(JB_block[j][0]>max[0]) max[0]=JB_block[j][0];
			if(JB_block[j][0]<min[0]) min[0]=JB_block[j][0];
			temp[1]+=JB_block[j][1];
			if(JB_block[j][1]>max[1]) max[1]=JB_block[j][1];
			if(JB_block[j][1]<min[1]) min[1]=JB_block[j][1];
			//printf("%d \t%d \t%d\n",JB_block[j][0],JB_block[j][1],JB_block[j][2]);
		}
		
		if(index != 0){
			if(index>5){
				temp[0]=temp[0]-max[0]-min[0];
				temp[1]=temp[1]-max[1]-min[1];
				//printf("%d %d\n",temp[0],temp[1]);
				temp[0]/=(index-2);
				temp[1]/=(index-2);
				//printf("avg temp power:%d \t%d\n",temp[0],temp[1]);
			}
			else{
				temp[0]/=index;
				temp[1]/=index;
				//printf("avg temp power:%d \t%d\n",temp[0],temp[1]);
			}
		}
		
		//printf("temp min max:%d \t%d\n",(int)(temp[0]*0.9),(int)(temp[0]*1.1));
		//printf("powe min max:%d \t%d\n",(int)(temp[1]*0.9),(int)(temp[1]*1.1));
		for(j=0;j<index;j++){
			 SetSJBZero=0;
			
			if(temp[0] < 0){
				if((temp[0]*0.9) < JB_block[j][0])
					JB_block[j][2]|=HOT;
				else if((temp[0]*1.1) > JB_block[j][0])
					JB_block[j][2]|=COLD;
			}
			else{
				if((temp[0]*0.9) > JB_block[j][0])
					JB_block[j][2]|=COLD;
				else if((temp[0]*1.1) < JB_block[j][0])
					JB_block[j][2]|=HOT;
			}
			if((temp[1]*0.9) > JB_block[j][1])
				JB_block[j][2]|=LOW;
			else if((temp[1]*1.1) < JB_block[j][1])
				JB_block[j][2]|=HIGH;
				
			if((JB_block[j][2]&OPEN)==1){
				strsub(query,strlen(query));
				strcat(query,"UPDATE `JunctionBox` SET `state2` = ");
				if(JB_block[j][2]==1)
					strcat(query,"1");
				else
					strcat(query,"2");
				//sql_int(JB_block[j][2]);
				strcat(query," WHERE `jb_mac` = 0x");
				for(t=0;t<5;t++)
					sql_char(JB_mac[j][t]);
				//printf("%s\n",query);
				t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
				if(t)
					printf("Error making query: %s\n",mysql_error(&mysql));
			}
			
			strsub(query,strlen(query));
			strcat(query,"UPDATE `JunctionBox` SET `state` ='");
			if((JB_block[j][2]&OPEN)==0){
				
				SetSJBZero=1;

				strcat(query,"offline'");
				
			}
			else{
				if(JB_block[j][2]==1){
					strsub(query,1);
					strcat(query,"NULL");
				}
				else{
					/*if((JB_block[j][2]&OPEN)==0)
						strcat(query,"offline,");*/
					if((JB_block[j][2]&HIGH)==HIGH)
						strcat(query,"overpower,");
					else if((JB_block[j][2]&LOW)==LOW)
						strcat(query,"underpower,");
					if((JB_block[j][2]&HOT)==HOT)
						strcat(query,"overtemperature,");
					else if((JB_block[j][2]&COLD)==COLD)
						strcat(query,"undertemperature,");
					strsub(query,1);
					strcat(query,"'");
				}
			}
			strcat(query," WHERE `jb_mac` = 0x");
			for(t=0;t<5;t++)
				sql_char(JB_mac[j][t]);
			//printf("%s\n",query);
			t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
			if(t)
				printf("Error making query: %s\n",mysql_error(&mysql));
			//printf("%d \t%d \t",JB_block[j][0],JB_block[j][1]);
			//printf("%d%d%d%d%d %02X %d\n",JB_block[j][2]>>4,(JB_block[j][2]>>3)&0x01,(JB_block[j][2]>>2)&0x01,(JB_block[j][2]>>1)&0x01,JB_block[j][2]&0x01,JB_block[j][2],JB_block[j][2]);

			////// update the junciton box table in my sql  if offline 
			
				if(SetSJBZero)	
				{
					strsub(query,strlen(query));
					Set_SJB_ALLZero(JB_mac[j]);
					//printf("%s\n",query);
					t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
					if(t)
					printf("Error making query: %s\n",mysql_error(&mysql));

					INSERT_SJB_PANNEL_Zero(JB_mac[j]);
					//printf("%s\n",query);
					t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
					if(t)
					printf("Error making query: %s\n",mysql_error(&mysql));
					
					SetSJBZero=0;
					updateAllBlock=1;
					
				}

			
			//////update the junciton box table in my sql  end 
		}
	}

	if(updateAllBlock)
		{
			updateAllBlock=0;
			//////update all block 
			INSERT_ALL_BLOCKDATA();
			strsub(query,1);
			//printf("%s\n",query);
			t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
			if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
							
		}

	
	printf("state compare OK\n");
}

void sql_AMjoin(char flag,char client_ip[],AMJoinRequestPonter amReqPtr){
	int i;

	strsub(query,strlen(query));
	strcat(query, "UPDATE `ArrayManager` SET `ip` = '");
	strcat(query,client_ip);
	strcat(query,"' ,`port` = ");
	sql_short(amReqPtr->portNumber);
	if(flag){
		strcat(query," ,`register_time` = '");
		sql_time();
		strcat(query,"'");
	}
	strcat(query," ,`update_time` = '");
	sql_time();
	strcat(query,"'");
	strcat(query," WHERE `mac` = '");
	for(i=1;i<6;i++)
		sql_char(amReqPtr->MAC[i]);
	strcat(query,"' LIMIT 1");
}

void sql_JBjoin(JBJoinInfoPonter jbInfoPtr){
	int i;
	
	strsub(query,strlen(query));
	strcat(query, "INSERT `JunctionBox` (`jb_mac` , `s/n` , `firmware_vs` ,`hardware_vs` , `device_spec` ,`manufacture_date` , `pos` , `block_id`) VALUES (0x");
	for(i=1;i<6;i++)
		sql_char(jbInfoPtr->JB_MAC[i]);
	strcat(query," , 0x");
	i=0;
	while(jbInfoPtr->SerialNumber[i]!='\0')
		sql_char(jbInfoPtr->SerialNumber[i++]);// chang by alex wang for fixed SN
		//sql_char("1");
		//sql_char("2");
	strcat(query," , 0x");
	i=0;
	while(jbInfoPtr->FirmwareVersion[i]!='\0')
		sql_char(jbInfoPtr->FirmwareVersion[i++]);
	strcat(query," , 0x");
	i=0;
	while(jbInfoPtr->HardwareVersion[i]!='\0')
		sql_char(jbInfoPtr->HardwareVersion[i++]);
	strcat(query," , 0x");
	i=0;
	while(jbInfoPtr->DeviceSpecification[i]!='\0')
		sql_char(jbInfoPtr->DeviceSpecification[i++]);
	strcat(query," , 0x");
	//i=0;
	for(i=0;i<8;i++)
		sql_char(jbInfoPtr->ManufactureData[i]);
	strcat(query," , ");
	sql_int(pos);
	strcat(query," , ");
	sql_int(block_id);
	strcat(query, ")");
	//printf("%s\n",query);
}

void sql_PD(PeriodicDataPointer pdcPtr,int n,int flag){
	int i,j;
	JunctionBoxPointer jBoxPtr = NULL;
	TimeStampPonter timeStampPtr = NULL;
	timeStampPtr = &(pdcPtr->stamp);
	jBoxPtr = (JunctionBoxPointer)pdcPtr->payload;

	if(!flag)
	  jBoxPtr = (JunctionBoxPointer)((int)jBoxPtr + n * SIZE_OF_JUNCTIONBOX_PACKET);
	
	strsub(query,strlen(query));
	if(flag==0){
		strcat(query, "UPDATE `JunctionBox` SET ");
		if(jBoxPtr->state == 0){
			strcat(query, "`state2` = ");
			sql_int(jBoxPtr->state);
			strcat(query, " , `voltage` = ");
		}
		else
			strcat(query, "`voltage` = ");
		sql_int(jBoxPtr->voltage);
		strcat(query, " , `current` = ");
		sql_int(jBoxPtr->current);//*100
		strcat(query, " , `temp` = ");
		sql_int(jBoxPtr->diodeTemperature);
		strcat(query, " , `power` = ");
		sql_float(jBoxPtr->power*1);///100);
		//sql_float(2.23);
		strcat(query," ,`update_time` = '");
		//sql_time();
		sql_devicetime(timeStampPtr);
		strcat(query,"'");
		strcat(query," WHERE `jb_mac` = 0x");
		for(i=1;i<6;i++)
			sql_char(jBoxPtr->MAC[i]);
		//printf("%s\n",query);
	}
	else{
		strcat(query, "INSERT `PanelData` (`jb_mac` , `power`, `temp` , `time`) VALUES");
		for(j=0;j<6;j++)
			jbmac[j]=jBoxPtr->MAC[j];
		for(j=0;j<n;j++){
			//if(jBoxPtr->state!=0x00){  ///alex by alex wang
				strcat(query,"(0x");
				for(i=1;i<6;i++)
					sql_char(jBoxPtr->MAC[i]);
				strcat(query, " , ");
				sql_int(jBoxPtr->power);///100);
				strcat(query, " , ");
				sql_int(jBoxPtr->diodeTemperature);
				strcat(query, " , '");
				//sql_time();
				sql_devicetime(timeStampPtr);
				strcat(query, "'");
				strcat(query, "),");
			//}
			jBoxPtr = (JunctionBoxPointer)((int)jBoxPtr + SIZE_OF_JUNCTIONBOX_PACKET);
		}
		strsub(query,1);
		/*strcat(query, "INSERT `PanelData` (`jb_mac` , `power`, `temp` , `time`) VALUES(0x");
		for(i=1;i<6;i++)
			sql_char(jBoxPtr->MAC[i]);
		strcat(query, " , ");
		sql_int(jBoxPtr->power/100);
		strcat(query, " , ");
		sql_int(jBoxPtr->diodeTemperature-50);
		strcat(query, " , '");
		//sql_time();
		sql_devicetime(timeStampPtr);
		strcat(query, "'");
		strcat(query, ")");
		//printf("%s\n",query);*/
	}
	//printf("n=%d:%s\n",n,query);
}

void sql_block(char am_id[]){
	strsub(query,strlen(query));
	strcat(query, "INSERT INTO `Block` (`am_id`) VALUES(");
	strcat(query, am_id);
	strcat(query, ")");
}

void sql_char(char temp){
	char i,j,number[16][2]={"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};
	strcat(query, number[(temp&0xF0)>>4]);
	strcat(query, number[temp&0x0F]);
}

void sql_short(short temp){
	char num[]="";
	sprintf(num,"%d",temp);
	strcat(query,num);
}

void sql_int(int temp){
	char num[]="";
	sprintf(num,"%d",temp);
	strcat(query,num);
}
#if 1
void sql_float(float temp){
	char num[]="";
	sprintf(num,"%4.2f",temp);
	//printf("%s\n",num);
	strcat(query,num);
}
#endif
void sql_time(void){
	char s[30]="";
	time_t timep;
	struct tm *p;
	
	strsub(s,strlen(s));
	time(&timep);
	p=localtime(&timep);
	sprintf(s,"%d-%d-%d %d:%d:%d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
	strcat(query,s);
}
void Sql_ServerTime()
{
	char s[30]="";
	time_t timep;
	struct tm *p;

	strsub(s,strlen(s));
	time(&timep);
	p=localtime(&timep);
	sprintf(s,"%d-%d-%d %d:%d:%d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour,p->tm_min, p->tm_sec);
	strcat(query,s);

}


void sql_devicetime(TimeStampPonter timeStampPtr){
	char s[30]="";
	time_t timep;
	struct tm *p;

	strsub(s,strlen(s));
	time(&timep);
	p=localtime(&timep);
	//sprintf(s,"%d-%d-%d %d:%d:00",(1900+p->tm_year), (1+p->tm_mon),timeStampPtr->date,timeStampPtr->hours, timeStampPtr->minutes);//, timeStampPtr->seconds);
	sprintf(s,"%d-%d-%d %d:%d:%d",(1900+p->tm_year), (1+p->tm_mon),timeStampPtr->date,timeStampPtr->hours, timeStampPtr->minutes, timeStampPtr->seconds);
	//sec promble
	strcat(query,s);
}

void RTCtime(SunPonter pkt){
	RTCResponsePonter rtcrsp;
	
	time_t timep;
	struct tm *p;
	time(&timep);
	p=localtime(&timep);

	rtcrsp = (RTCResponsePonter)pkt->payload;
	rtcrsp->years = p->tm_year-100;
	rtcrsp->months =p->tm_mon+1;
	rtcrsp->date = p->tm_mday;
	rtcrsp->hours = p->tm_hour;
	rtcrsp->minutes = p->tm_min;
	rtcrsp->second = p->tm_sec;


	printf("RTCtime\n");
	printf("RTCtime\n");
	printf("RTCtime\n");
	printf("RTCtime\n");
	printf("Year:%d M:%d Date:%d H:%d Min:%d\n",rtcrsp->years,rtcrsp->months,rtcrsp->date ,rtcrsp->hours,rtcrsp->minutes);

	
}

void strsub(char *s1, int sub) {
    int i,temp=strlen(s1);
    for (i = temp;i>=(temp-sub); i--)
		s1[i] = '\0';
}

#define  SJBDisconect_Time      3
int TimeCompare(char timef[])
{
	////// we only compare the date

	int year,mon,date,hour,min,sec;
	unsigned int ServerTotalMin=0;
	unsigned int SJBTotalMin=0;
	int ServerDate;
	int SJBDate;

	char s[25];
	//dateTabl[31,28,31,30,31,30,31,31,30,31,30,31];
	int TotalDateTable[]={0,0,31,59,90,120,151,181,212,243,273,304,334};
	time_t timep;
	struct tm *p;

	
	year=mon=date=hour=min=sec=0;
	
	strsub(s,strlen(s));
	time(&timep);
	p=localtime(&timep);


	sprintf(s,"%c%c%c%c",timef[0],timef[1],timef[2],timef[3]);
	year=atoi(s);
	sprintf(s,"%c%c",timef[5],timef[6]);
	mon=atoi(s);
	sprintf(s,"%c%c",timef[8],timef[9]);
	date=atoi(s);
	sprintf(s,"%c%c",timef[11],timef[12]);
	hour=atoi(s);
	sprintf(s,"%c%c",timef[14],timef[15]);
	min=atoi(s);


	ServerDate=TotalDateTable[1+p->tm_mon]+(1900+p->tm_year-2010)*365;
	SJBDate=TotalDateTable[mon]+(year-2010)*365;

	
	ServerTotalMin=p->tm_min+60*(p->tm_hour)+24*60*((p->tm_mday)+ServerDate);

	
	SJBTotalMin=min+60*hour+(24*60)*(date+SJBDate);

		
	if(ServerTotalMin-SJBTotalMin>SJBDisconect_Time) return 0;

	return 1;


}




int state_timecompare(char timef[]){
	int year,mon,data,hour,min,sec;
	char s[25];
	
	time_t timep;
	struct tm *p;
	
	year=mon=data=hour=min=sec=0;
	
	strsub(s,strlen(s));
	time(&timep);
	p=localtime(&timep);
	
	//sprintf(s,"%d-%d-%d %d:%d:%d",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
	//printf("%s # %s\n",timef,s);
	
	sprintf(s,"%c%c%c%c",timef[0],timef[1],timef[2],timef[3]);
	year=atoi(s);
	if((1900+p->tm_year)!=year)
		return 0;
	sprintf(s,"%c%c",timef[5],timef[6]);
	mon=atoi(s);
	if((1+p->tm_mon)!=mon)
		return 0;
	sprintf(s,"%c%c",timef[8],timef[9]);
	data=atoi(s);
	if(p->tm_mday!=data)
		return 0;
	sprintf(s,"%c%c",timef[11],timef[12]);
	hour=atoi(s);
	if(p->tm_hour!=hour)
		return 0;
	sprintf(s,"%c%c",timef[14],timef[15]);
	min=atoi(s);	
	
	if(p->tm_min<SJBDisconect_Time && min>(60-SJBDisconect_Time))
		if(abs((p->tm_min)+60-min)>SJBDisconect_Time)
			return 0;
		else
			return 1;
	else if(abs((p->tm_min)-min)>SJBDisconect_Time)
		return 0;
	else
		return 1;
	//if(p->tm_min!=min)
	//	return 0;
	
	sprintf(s,"%c%c",timef[17],timef[18]);
	sec=atoi(s);
	
	/*if(p->tm_sec<20 && sec>40)
		if(abs((p->tm_sec)+60-sec)>20)
			return 0;
		else
			return 1;
	else if(abs((p->tm_sec)-sec)>20)
		return 0;
	else
		return 1;*/
	//if(p->tm_sec!=sec)
	//	return 0;
}

#define AXLE_DUMMY

#ifdef AXLE_DUMMY
///////////panelData
#define Dummy_power 1
#define   Dummy_TEMP	1
#define  PN_jb_mac     0
#define  PN_power     1
#define  PN_temp     2
#define  PN_time     3

#define DummyTimeOffset		1

#define BL_blockid	0
#define  BL_power	1
#define  BL_time		2

////*********************************************************************
////	function name:    CHECK_IF_DUMYZERO_EXIST
////
////	Description:	Delete  the dummy data in mysql ,pannel and block table, if we find them 
///					 	
////	
////
////	autor	Alex wang 
////**********************************************************************


int CHECK_IF_DUMYZERO_EXIST(PeriodicDataPointer pdcPtr,int n, int flag)
{
	
	int blockid;
	int PN_power_L;
	int i,j,t;
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	JunctionBoxPointer jBoxPtr = NULL;
	TimeStampPonter timeStampPtr = NULL;
	timeStampPtr = &(pdcPtr->stamp);
	jBoxPtr = (JunctionBoxPointer)pdcPtr->payload;


	////// delete the Block dummy data
	
	
	for( i=0;i<n;i++)
	{	

	////get block id 
	

	strsub(query,strlen(query));

	strcat(query,"SELECT * FROM JunctionBox WHERE `jb_mac` = 0x");
	for(j=1;j<6;j++)
		sql_char(jBoxPtr->MAC[j]);
	
	printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		blockid=atoi(row[JB_block_id]);
		printf("%d\n",blockid);
	}

	mysql_free_result(res);
	res=NULL;

	////delete the block dummy data 
	
	strsub(query,strlen(query));
	
	strcat(query, "SELECT * FROM  `BlockData` WHERE  `block_id` =  " );
	
	sql_int(blockid);
	
	strcat(query," ORDER BY  `BlockData`.`time` DESC  LIMIT 0 , 10");

	printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		
		PN_power_L=atoi(row[BL_power]);

		if(PN_power_L==0) break;

		if(PN_power_L==Dummy_power)
			{
				////////////we got dummy Zero  , delete it 
				printf("Got Block Dummy \n");
				strsub(query,strlen(query));
				strcat(query, "DELETE FROM `BlockData` WHERE `block_id`=  " );
				sql_int(blockid);
				strcat(query, " AND`power` = " );
				sql_char(Dummy_power);
				printf("%s\n",query);
				t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
				if(t)
				printf("Error making query: %s\n",mysql_error(&mysql));
				break;
				
			}
		

	}
		mysql_free_result(res);
		res=NULL;
		 jBoxPtr = (JunctionBoxPointer)((int)jBoxPtr + 1* SIZE_OF_JUNCTIONBOX_PACKET);
		}

	///////delete the pannel   dummy data

	jBoxPtr = (JunctionBoxPointer)pdcPtr->payload;


	for( i=0;i<n;i++)
	{
	
	strsub(query,strlen(query));
	
	strcat(query, "SELECT * FROM  `PanelData` WHERE  `jb_mac` = 0x" );
	for(j=1;j<6;j++)
		sql_char(jBoxPtr->MAC[j]);
	
	strcat(query," ORDER BY  `PanelData`.`time` DESC  LIMIT 0 , 10");
	printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		PN_power_L=atoi(row[PN_power]);

		if(PN_power_L==0) break;

		if(PN_power_L==Dummy_power)
			{
				////////////we got dummy Zero  , delete it 

				printf("Got Pannel Dummy \n");
				strsub(query,strlen(query));
				strcat(query, "DELETE FROM `PanelData` WHERE `jb_mac`= 0x" );
				for(j=1;j<6;j++)
				sql_char(jBoxPtr->MAC[j]);
				strcat(query, " AND`power` = " );
				sql_char(Dummy_power);
				printf("%s\n",query);
				t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
				if(t)
				printf("Error making query: %s\n",mysql_error(&mysql));
				
				break;
				
			}
		
		//printf("%d\n",PN_power_L);
	}

	mysql_free_result(res);
	res=NULL;
	  jBoxPtr = (JunctionBoxPointer)((int)jBoxPtr + 1* SIZE_OF_JUNCTIONBOX_PACKET);
		}

}

////*********************************************************************
////	function name:    Insert_Dummy_Zero2Pannel_Block_Tab
////
////	Description:	insert the dummy zero to pannel and block tab after real data inserting 
///					 	
////	
////
////	autor	Alex wang 
////**********************************************************************
int Insert_Dummy_Zero2Pannel_Block_Tab(PeriodicDataPointer pdcPtr,int n, int flag)
{

	/////reset the AM response Data 

	int L_blockid,PN_power_L,i,j,t;
	int   GotGummy=0;
	MYSQL_RES *res=NULL;
	MYSQL_ROW row=NULL;
	JunctionBoxPointer jBoxPtr = NULL;
	TimeStampPonter timeStampPtr = NULL;
	timeStampPtr = &(pdcPtr->stamp);
	jBoxPtr = (JunctionBoxPointer)pdcPtr->payload;

		
	for( i=0;i<n;i++)
	{	

	////get block id 
	
	strsub(query,strlen(query));

	strcat(query,"SELECT * FROM JunctionBox WHERE `jb_mac` = 0x");
	for(j=1;j<6;j++)
		sql_char(jBoxPtr->MAC[j]);
	
	printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
	printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	while(row = mysql_fetch_row(res)){
		L_blockid=atoi(row[JB_block_id]);
		printf("%d\n",L_blockid);
	}

	mysql_free_result(res);
	res=NULL;

	////Inset  the block dummy data 

	strsub(query,strlen(query));
	
	strcat(query, "SELECT * FROM  `BlockData` WHERE  `block_id` =  " );
	
	sql_int(L_blockid);
	
	strcat(query," ORDER BY  `BlockData`.`time` DESC  LIMIT 0 , 5");

	printf("%s\n",query);
	t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
	if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
	res = mysql_store_result(&mysql);
	GotGummy=0;
	while(row = mysql_fetch_row(res)){
		
		PN_power_L=atoi(row[BL_power]);

		if(PN_power_L==Dummy_power)
			{
				GotGummy=1;
				break;
				
			}
		

		}
		
		mysql_free_result(res);
		res=NULL;
		
		if(!GotGummy)
		{
		strsub(query,strlen(query));
		strcat(query, "INSERT `BlockData` (`block_id` , `power` , `time`) VALUES ");
		strcat(query,"(");
		//strcat(query,blockid);
		sql_int(L_blockid);
		strcat(query," , ");
		sql_int(Dummy_power);
		strcat(query, " , '");
		//sql_time();
		sql_deviceDummytime(timeStampPtr);
		strcat(query, "'");
		strcat(query,")");

		printf("%s\n",query);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
		printf("Error making query: %s\n",mysql_error(&mysql));
		
		 
		res=NULL;
		 
		}


		//////insert the dummy Zero to pannel table
		strsub(query,strlen(query));
		strcat(query, "INSERT `PanelData` (`jb_mac` , `power`, `temp` , `time`) VALUES");
			
			
			strcat(query,"(0x");
			for(j=1;j<6;j++)
			sql_char(jBoxPtr->MAC[j]);
			strcat(query, " , ");
			sql_int(Dummy_power);///100);
			strcat(query, " , ");
			sql_int(Dummy_TEMP);
			strcat(query, " , '");
			//sql_time();
			sql_deviceDummytime(timeStampPtr);
			strcat(query, "'");
			strcat(query, ")");

			printf("%s\n",query);
			t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
			if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
			 
			res=NULL;
			
		//////insert the dummy Zero to pannel table END
		
		jBoxPtr = (JunctionBoxPointer)((int)jBoxPtr + 1* SIZE_OF_JUNCTIONBOX_PACKET);

		


	}

}
void sql_deviceDummytime(TimeStampPonter timeStampPtr){
	char s[30]="";
	time_t timep;
	struct tm *p;

	strsub(s,strlen(s));
	time(&timep);
	p=localtime(&timep);
	//sprintf(s,"%d-%d-%d %d:%d:00",(1900+p->tm_year), (1+p->tm_mon),timeStampPtr->date,timeStampPtr->hours, timeStampPtr->minutes);//, timeStampPtr->seconds);
	sprintf(s,"%d-%d-%d %d:%d:%d",(1900+p->tm_year), (1+p->tm_mon),timeStampPtr->date,timeStampPtr->hours, timeStampPtr->minutes, (timeStampPtr->seconds)+DummyTimeOffset);
	//sec promble
	strcat(query,s);
}

int  Set_SJB_ALLZero( char*SJBMAC)
{
		int i;


		strcat(query, "UPDATE `JunctionBox` SET ");
		strcat(query, "`voltage` = ");
		sql_int(0);
		strcat(query, " , `current` = ");
		sql_int(1);//*100
		strcat(query, " , `temp` = ");
		sql_int(0);
		strcat(query, " , `power` = ");
		sql_float(0);///100);
		strcat(query," ,`update_time` = '");
		Sql_ServerTime();
		strcat(query,"'");
		strcat(query," WHERE `jb_mac` = 0x");
		for(i=0;i<5;i++)
			sql_char(SJBMAC[i]);





}

int  INSERT_SJB_PANNEL_Zero( char*SJBMAC)
{
		int i;
	
		strsub(query,strlen(query));
		strcat(query, "INSERT `PanelData` (`jb_mac` , `power`, `temp` , `time`) VALUES");
	
		strcat(query,"(0x");
		for(i=0;i<5;i++)
			sql_char(SJBMAC[i]);
		strcat(query, " , ");
		sql_char(0);
		strcat(query, " , ");
		sql_char(1);	
		strcat(query, " , ");
		strcat(query, "'");
		Sql_ServerTime();
		strcat(query, "'");
		strcat(query, ")");
			

}

int  INSERT_ALL_BLOCKDATA()
{
		MYSQL_RES *res=NULL;
		MYSQL_ROW row=NULL;
		int t;
			//insert all blockdata		
		strsub(query,strlen(query));
		strcat(query,"SELECT `block_id` , SUM(`power`)`power` FROM `JunctionBox` GROUP BY `block_id`");
		//printf("%s\n",query);
		t = mysql_real_query(&mysql,query,(unsigned int) strlen(query));
		if(t)
			printf("Error making query: %s\n",mysql_error(&mysql));
		res = mysql_store_result(&mysql);
		
		strsub(query,strlen(query));
		strcat(query, "INSERT `BlockData` (`block_id` , `power` , `time`) VALUES ");
		while(row = mysql_fetch_row(res)){
				
				strcat(query,"(");
				strcat(query,row[0]);
				strcat(query," , ");
				strcat(query,row[1]);
				strcat(query, " , '");
				//sql_time();
				Sql_ServerTime();
				strcat(query, "'");
				strcat(query,"),");
		}


		mysql_free_result(res);
		

	return 0;
}
#endif




