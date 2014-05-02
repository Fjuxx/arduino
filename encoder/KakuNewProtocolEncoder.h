#ifndef KAKUNEWPROTOCOLENCODER_H
#define KAKUNEWPROTOCOLENCODER_H

#include "Encoder.h"

class KakuNewProtocolEncoder :
	public Encoder
{
private:
	unsigned long long	m_nCode;
	unsigned long long	m_nTopBit;
	
	
	int				m_nPulseLengthShort;
	int				m_nPulseLengthLong;
	void 			encodeZero(RFPacket* pPacket);
	void 			encodeOne(RFPacket* pPacket);
	void 			encodeDim(RFPacket* pPacket);

public:
	KakuNewProtocolEncoder(int nPulseLength);

	void	setCode(unsigned long long nCode);
	void	encode(RFPacket* pPacket);
	void 	setCode(byte* dataPointer, byte pos);
};

#endif