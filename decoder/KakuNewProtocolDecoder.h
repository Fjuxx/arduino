#ifndef KAKUNEWPROTOCOLDECODER_H
#define KAKUNEWPROTOCOLDECODER_H

#include "Decoder.h"

class KakuNewProtocolDecoder :
	public Decoder
{
private:
	int 			m_nPulseLength_T;
	unsigned long long	m_nCode;
	int 			nLengthBits;

public:
	KakuNewProtocolDecoder();

	boolean decode(RFPacket* pPacket);
	void	fillPacket(NinjaPacket* pPacket);
};

#endif