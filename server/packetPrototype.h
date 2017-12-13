#ifndef PACKET_PROTOTYPE_H
#define PACKET_PROTOTYPE_H

#ifndef uint32
	#define uint32 unsigned int
#endif
#ifndef uint16
	#define uint16 unsigned short int
#endif
#ifndef uint8
	#define uint8 unsigned char
#endif

#define HEADER_CODE 0x58
#define TAIL_CODE 0x62

enum PacketType {AM_JOIN_REQUEST = 0x03, AM_JOIN_RESPONSE = 0x04, RTC_UPDATE_REQUEST = 0x05, RTC_UPDATE_RESPONSE = 0x06, PERIODIC_DATA = 0x07, JB_JOIN_REQUEST = 0x08, JB_JOIN_RESPNOSE = 0x09,TCP_PERIODIC_LINK = 0x0a, ARRAY_MANGER_SETTING = 0x0b };
enum JoinCondition {JOIN_ACK = 0x02, JOIN_NACK_MAC = 0x03, JOIN_NACK = 0x04};
enum RTCCondition {CMD_RTC_UPDATE_REQUEST = 0x0C};
enum TCPArraySettingCondition {UPDATE_RTC = 0x00, UPADTE_PERIOD = 0x01, TCP_LINK_PERIOD = 0x02, RESET_ARRAY_MANGER = 0x03, RESET_JUNCTION_BOX = 0x04};

enum JunctionBoxState {GOOD = 0x00, OFFLINE = 0x01 << 0, OVER_HEAT = 0x01 << 1, OVER_VOLTAGE = 0x01 << 2, OVER_CURRENT  = 0x01 << 3};

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct SunPacket {
	uint8 headerCode;
	uint16 length;
	uint8 sequenceNumber;
	uint8 commandType;
	uint8 nPayload;
	uint8 *payload;
	uint8 tailCode;
} SunPacket;
typedef SunPacket *SunPonter;
#define SIZE_OF_SUN_PACKET (sizeof(SunPacket))
//
//AM_JOIN_REQUEST---------------------------------------------------------
typedef struct AMJoinRequestPacket {
	uint8 MAC[6];
	uint8 reserved[4];
	uint16 portNumber;
	uint8 comfirmedCode[200];
} AMJoinRequestPacket;
typedef AMJoinRequestPacket *AMJoinRequestPonter;
#define SIZE_OF_AM_JOIN_REQUEST_PACKET (sizeof(AMJoinRequestPacket))
//------------------------------------------------------------------------
//
//AM_JOIN_RESPONSE---------------------------------------------------------
typedef struct AMJoinResponsePacket {
	uint8 ack;
} AMJoinResponsePacket;
typedef AMJoinResponsePacket *AMJoinResponsePointer;
#define SIZE_OF_AM_JOIN_RESPONSE_PACKET (sizeof(AMJoinResponsePacket))
//------------------------------------------------------------------------
//
//RTC_UPDATE_REQUEST---------------------------------------------------------
typedef struct RTCRequestPacket {
	uint8 type;
} RTCRequestPacket;
typedef RTCRequestPacket *RTCRequestPonter;
#define SIZE_OF_RTC_REQUEST_PACKET (sizeof(RTCRequestPacket))
//------------------------------------------------------------------------
//
//RTC_UPDATE_RESPONSE---------------------------------------------------------
typedef struct RTCResponsePacket {
	uint8 years;
	uint8 months;
	uint8 date;
	uint8 hours;
	uint8 minutes;
	uint8 second;
} RTCResponsePacket;
typedef RTCResponsePacket *RTCResponsePonter;
#define SIZE_OF_RTC_RESPONSE_PACKET (sizeof(RTCResponsePacket))
//------------------------------------------------------------------------
//
//PERIODIC_DATA---------------------------------------------------------
typedef struct TimeStamp {
	uint8 date;
	uint8 hours;
	uint8 minutes;
	uint8 seconds;
} TimeStamp;
typedef TimeStamp *TimeStampPonter;
#define SIZE_OF_TIME_STAMP (sizeof(TimeStamp))

typedef struct JunctionBoxPacket {
	uint8 MAC[6];
	uint8 diodeTemperature;
	uint16 voltage;
	uint16 current;
	uint32 power;
	uint32 state;
} JunctionBoxPacket;
typedef JunctionBoxPacket *JunctionBoxPointer;
#define SIZE_OF_JUNCTIONBOX_PACKET (sizeof(JunctionBoxPacket))

typedef struct PeriodicDataPacket {
	TimeStamp stamp;
	uint16 updatePeriod;
	uint8 nJunctionBoxes;
	uint8 *payload;
} PeriodicDataPacket;
typedef PeriodicDataPacket *PeriodicDataPointer;
#define SIZE_OF_PERIODIC_DATA_PACKET (sizeof(PeriodicDataPacket))
//------------------------------------------------------------------------
//JB_JOIN_REQUEST---------------------------------------------------------
typedef struct JBJoinRequestPacket {
	uint8 AM_MAC[6];
	uint8 nJunctionBoxes;
	uint8 *payload;
} JBJoinRequestPacket;
typedef JBJoinRequestPacket *JBJoinRequestPonter;
#define SIZE_OF_JB_JOIN_REQUEST_PACKET (sizeof(JBJoinRequestPacket))
//------------------------------------------------------------------------]
//JB_JOIN_INFO---------------------------------------------------------
typedef struct JBJoinInfoPacket {
	uint8 JB_MAC[6];
	uint8 SerialNumber[24];
	uint8 FirmwareVersion[24];
	uint8 HardwareVersion[24];
	uint8 DeviceSpecification[24];
	uint8 ManufactureData[8];
} JBJoinInfoPacket;
typedef JBJoinInfoPacket *JBJoinInfoPonter;
#define SIZE_OF_JB_JOIN_INFO_PACKET (sizeof(JBJoinInfoPacket))
//------------------------------------------------------------------------
//AM_JOIN_RESPONSE---------------------------------------------------------
typedef struct JBJoinResponsePacket {
	uint8 ack;
	uint8 JB_MAC[6];
} JBJoinResponsePacket;
typedef JBJoinResponsePacket *JBJoinResponsePointer;
#define SIZE_OF_JB_JOIN_RESPONSE_PACKET (sizeof(JBJoinResponsePacket))
//------------------------------------------------------------------------
//TCP_PERIODIC_LINK-------------------------------------------------------
typedef struct TCPPeriodicLinkPacket {
	uint8  AMRTCTime[6];
	uint16 UpPeriod;
	uint16 TCPLinkPeriod;
} TCPPeriodicLinkPacket;
typedef TCPPeriodicLinkPacket *TCPPeriodicLinkPointer;
#define SIZE_OF_TCP_PERIODIC_LINK_PACKET (sizeof(TCPPeriodicLinkPacket))
//------------------------------------------------------------------------
//TCP_PERIODIC_LINK-------------------------------------------------------
typedef struct AMSettingPacket {
	uint8  SettingDate[255];
} AMSettingPacket;
typedef AMSettingPacket *AMSettingPointer;
#define SIZE_OF_AM_SETTING_PACKET (sizeof(AMSettingPacket))
//------------------------------------------------------------------------
#pragma pack(pop) /* restore original alignment from stack */

typedef struct DateTime {
	uint32 years;
	uint32 months;
	uint32 date;
	uint32 hours;
	uint32 minutes;
} DateTime;

#define SIZE_OF_SUN_PACKET_LESS (SIZE_OF_SUN_PACKET - 8)
#define SIZE_OF_PERIODIC_DATA_PACKET_LESS (sizeof(PeriodicDataPacket) - 4)
#define AM_JOIN_WHOLE_SIZE (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_AM_JOIN_REQUEST_PACKET)
#define AM_JRSP_WHOLE_SIZE (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_AM_JOIN_RESPONSE_PACKET)
#define RTC_REQ_WHOLE_SIZE (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_RTC_REQUEST_PACKET)
#define RTC_RSP_WHOLE_SIZE (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_RTC_RESPONSE_PACKET)
#define PRD_DTA_WHOLE_SIZE(n) (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_PERIODIC_DATA_PACKET_LESS + n * SIZE_OF_JUNCTIONBOX_PACKET)
#define SIZE_BEFORE_PAYLOAD 6

#define payLoadSize(pkt) (pkt->length - SIZE_OF_SUN_PACKET_LESS)

//#define JB_JOIN_WHOLE_SIZE (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_JB_JOIN_REQUEST_PACKET)//JB
#define JB_JOIN_WHOLE_SIZE(n) (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_JB_JOIN_REQUEST_PACKET + n * SIZE_OF_JB_JOIN_INFO_PACKET)
#define JB_JOIN_REQUEST_PACKET_LESS (sizeof(JBJoinRequestPacket) - 4)
#define JB_JRSP_WHOLE_SIZE(n) (SIZE_OF_SUN_PACKET_LESS + n * SIZE_OF_JB_JOIN_RESPONSE_PACKET)

#define TCP_LINK_WHOLE_SIZE (SIZE_OF_SUN_PACKET_LESS + SIZE_OF_TCP_PERIODIC_LINK_PACKET)
#define AM_SETTING_WHOLE_SIZE(n) (SIZE_OF_SUN_PACKET_LESS + n)

#endif
