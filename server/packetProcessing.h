#ifndef PACKET_PROCESSING_H
#define PACKET_PROCESSING_H

#define l_malloc(size) testmalloc(size)
#define l_free(ptr) testfree(ptr)
#ifdef M3_TEST
	#define recv(a,b,c,d)         lwip_recv(a,b,c,d)
#endif


DateTime RTCResponse2DateTime(RTCResponsePonter pkt);
AMJoinRequestPonter generateAMJoin(void);
AMJoinResponsePointer generateAMJoinResponse(void);
PeriodicDataPointer generatePeriodicData(int size);
SunPonter generateSunPacket(uint8 commandType);
uint8 *packet2Array(SunPonter pkt);
SunPonter array2Packet(uint8 * array);
void AM_Join(int Remotefd);
DateTime RTC_Request(int Remotefd);
void showSunPacketInfo(SunPonter pkt);
void releasePacket(SunPonter pkt);

void JB_Join(int Remotefd);
JBJoinRequestPonter generateJBJoin(int size);
//JBJoinResponsePointer generateJBoinResponse(JBJoinInfoPonter jbInfoPtr,int size);

void TCP_Link(int Remotefd);
TCPPeriodicLinkPointer generateTCPLink(void);
SunPonter generateAMSetting(void);

SunPonter generateSunPacketJB(SunPonter pkt);

#endif

