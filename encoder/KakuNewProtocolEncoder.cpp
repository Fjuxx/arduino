#include "KakuNewProtocolEncoder.h"

//Original New KAKU protocol:  
//26 Adres, 1 Group, 1 On/Off/Dim, 4 Unit, [4] Dim (optional)

//Custom protocol for remembering Dim bit in code.
//Kaku protcol (+1 extra bit for On/Off/Dim)
//  [10011001100110011001100110] [0]    [11]        [0011] | [0011]
//     Adress                    Group  On/Off/Dim   Unit    [optional]Dim
//
// On/Off/Dim:
// [11]
//  | \_ Checked when Normal bit (One/Zero).
//   \_  Added bit, Tells what next bit is: 0=Real Dim bit  1=Normal Bit

// !!!!!!!!!!!!!! ALSO USED IN DECODER !!!!!!!!!!!!!!!!!!!!
#define BIT_LENGTH_NODIM		32  //(132-4[startstopbits])/4[changes_per_bit]
#define BIT_LENGTH_DIM_ORIG		36 
#define BIT_LENGTH_DIM_CUSTOM 	37 
#define BIT_DIM_POSITION		28
// !!!!!!!!!!!!!! ALSO USED IN DECODER !!!!!!!!!!!!!!!!!!!!

#define SIZE_LONG_LONG		64


//For protocol see KakuProtocolDecoder.cpp
KakuNewProtocolEncoder::KakuNewProtocolEncoder(int nPulseLength)
{
	m_nCode = 0;
	m_nTopBit = 1;
	m_nTopBit <<= (SIZE_LONG_LONG-1); //64th bit.
	
	m_nPulseLengthShort = 280;
	m_nPulseLengthLong = 1250;
}

void KakuNewProtocolEncoder::setCode(unsigned long long nCode)
{
	m_nCode = nCode;
}

void KakuNewProtocolEncoder::setCode(byte* dataPointer, byte pos)
{
	return;
}


void KakuNewProtocolEncoder::encode(RFPacket* pPacket)
{
	pPacket->reset();
	
	//Start bit
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort*9); //actually should be 9,5

	//With Dim=36bit, Normal=32bits. Set i to skip unused bits of 64bits total.
	int i;
	int DimBit = 0;
	if(m_nCode>>BIT_LENGTH_NODIM)
	{
		i = SIZE_LONG_LONG-BIT_LENGTH_DIM_CUSTOM; //with Dim.  Skip 64-37=27 bits.
		DimBit = i + BIT_DIM_POSITION-1; // Skip + Adres + group. Next bit = On/Off/Dim bit
	}
	else
		i = SIZE_LONG_LONG-BIT_LENGTH_NODIM; //Normal code, Skip 64-32=32 bits.		
	
	// Add data
//	Serial.print(F("KAKU: Startbit:"));
//	Serial.print(i);
//	Serial.print(F(" Code: "));
//	Serial.print((unsigned long)(m_nCode>>32), HEX);
//	Serial.print((unsigned long)m_nCode, HEX);
//	Serial.print(F(" Encoding:"));
	m_nCode <<= i;
	for (; i < SIZE_LONG_LONG; i++)
	{
		if(i==DimBit)
		{
			//Dimbit position is 2bits.
			if((m_nCode & m_nTopBit) == 0)
			{
				//Valid Dim
				encodeDim(pPacket);
				//Ignore next bit.
				m_nCode <<= 1;
				i += 1;
			}
			else
			{
				//No dim. Ignore bit, next bit decides if One/Zero.
			}
		}
		else if(m_nCode & m_nTopBit)
		{
			//Real one, or fake dimbit so just send one.
			encodeOne(pPacket);			
		}
		else
			encodeZero(pPacket);

		m_nCode <<= 1;
	}

//	Serial.println(F(" send")); 

	// Finish off with one short  (high) pulse  followed by a gap of 10ms 
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(10000);

	pPacket->rewind();
}

void KakuNewProtocolEncoder::encodeOne(RFPacket* pPacket)
{
//	Serial.print(F("1"));
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthLong);
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort);
}

void KakuNewProtocolEncoder::encodeZero(RFPacket* pPacket)
{
//	Serial.print(F("0"));
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthLong);
}

void KakuNewProtocolEncoder::encodeDim(RFPacket* pPacket)
{
//	Serial.print(F("D"));
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort);
	pPacket->append(m_nPulseLengthShort);
}