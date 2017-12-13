//void mysql_connect(MYSQL *mysql);
void mysql_connect(void);
void mysql_ArrayManager_Join(SunPonter pkt,char client_ip[]);
void mysql_JunctionBox_Join(SunPonter pkt);
void mysql_PeriodicData(SunPonter pkt,int flag);
void mysql_TCPLink(SunPonter pkt);
void mysql_compare(void);
void sql_AMjoin(char flag,char client_ip[],AMJoinRequestPonter amReqPtr);
void sql_JBjoin(JBJoinInfoPonter jbInfoPtr);
void sql_PD(PeriodicDataPointer pdcPtr,int n,int flag);
void sql_block(char am_id[]);
void sql_char(char temp);//char->char[]
void sql_int(int temp);//int->char[]
void sql_short(short temp);//short->char[]
void sql_time(void);//get now time
void sql_devicetime(TimeStampPonter timeStampPtr);
void sql_float(float temp);
	
void strsub(char *s1, int sub);

void RTCtime(SunPonter pkt);

int state_timecompare(char timef[]);

#define AM_am_id 			0
#define AM_mac 				1
#define AM_row 				2
#define AM_col 				3
#define AM_area_id 			4
#define AM_ip 				5
#define AM_port 			6
#define AM_period 			7
#define AM_insert_time 		8
#define AM_register_time 	9
#define AM_update_time 		10

#define Block_block_id		0
#define Block_am_id			1
#define Block_row			2
#define Block_col			3

#define JB_jb_mac 			0
#define JB_sn 				1
#define JB_firmware_vs 		2
#define JB_hardware_vs 		3
#define JB_device_spec 		4
#define JB_manufacture_date	5
#define JB_pos 				6
#define JB_block_id 		7
#define JB_insert_time 		8
#define JB_state2 			9
#define JB_state 			10
#define JB_voltage 			11
#define JB_current 			12
#define JB_temp 			13
#define JB_power 			14
#define JB_update_time 		15




