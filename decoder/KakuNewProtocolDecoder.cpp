#include "KakuNewProtocolDecoder.h"
#include "../common/Ninja.h"

#define CHANGES_LENGTH_NODIM	132 //2start, 4x26 Adres, 4x1 Group, 4x1 On/Off/Dim, 4x4 Unit
#define CHANGES_LENGTH_DIM		148 //2start, 4x26 Adres, 4x1 Group, 4x1 On/Off/Dim, 4x4 Unit, 4x4 Dimvalue
// !!!!!!!!!!!!!! ALSO USED IN DECODER !!!!!!!!!!!!!!!!!!!!
#define BIT_LENGTH_NODIM		32  //(132-4[startstopbits])/4[changes_per_bit]
#define BIT_LENGTH_DIM_ORIG		36 
#define BIT_LENGTH_DIM_CUSTOM 	37 
#define BIT_DIM_POSITION		28
// !!!!!!!!!!!!!! ALSO USED IN DECODER !!!!!!!!!!!!!!!!!!!!

//Original New KAKU protocol:  http://www.circuitsonline.net/forum/view/message/1181410#1181410
//26 Adres, 1 Group, 1 On/Off/Dim, 4 Unit, [4] Dim (optional)
// '0':	| |_| |____	(T,T,T,3T)
//       _      _
// '1':	| |____| |_	(T,3T,T,T)
//       _   _
// dim:	| |_| |_	(T,T,T,T) 
// short period = 275 µs (or 375, also works)
// long period = 3 of 4*T (works both)

// TESTING: 3T = actually 4,5T: 1250, min1125, max1375
//	       1T = actually: 280, min180, max380.
//         Startbit time:Low:2640
//		   Dimbit not received, instead a 1 is send by real senders..

//Custom protocol for remembering Dim bit in code.
//Kaku protcol (+1 extra bit for On/Off/Dim)
//  [10011001100110011001100110] [0]    [11]        [0011] | [0011]
//     Adress                    Group  On/Off/Dim   Unit    [optional]Dim
//
// On/Off/Dim:
// [11]
//  | \_ Checked when Normal bit (One/Zero).
//   \_  Added bit, Tells what next bit is: 0=Real Dim bit  1=Normal Bit

KakuNewProtocolDecoder::KakuNewProtocolDecoder()
{
	m_nPulseLength_T = 0;
}

boolean KakuNewProtocolDecoder::decode(RFPacket* pPacket)
{
	switch (pPacket->getSize())
	{
		case CHANGES_LENGTH_NODIM:
			nLengthBits = BIT_LENGTH_NODIM; 
			break;
			
		case CHANGES_LENGTH_DIM:
			nLengthBits = BIT_LENGTH_DIM_ORIG; 
			break;
			
		default:
			return false;
	}
		
	m_nCode = 0;
	word nHighPulse = 0;
	word nLowPulse = 0;
	word nHighPulse2 = 0;
	word nLowPulse2 = 0;

	//Set timings
	m_nPulseLength_T = 280;
	word nShortMin = 100; //180;
	word nShortMax = 460; //380;
	word nLongMin = 1000; //Long=1250
	word nLongMax = 1475;
	
	//Skip Start pulse. Expect timing: High time:T, Low time:4,5T.
	pPacket->next();
	pPacket->next();
	
//	Serial.print(F("KAKU b"));
	//Get address 26bit, skip Group 1bit, Get On/Off/Dim 1bit, Dim 4bit
	bool bDimBit = false;
	for(int i = 1; i <= nLengthBits; i++)
	{
		//Grab bit (4 changes)
		nHighPulse = pPacket->next(); //always 1T
		nLowPulse = pPacket->next();
		nHighPulse2 = pPacket->next(); //always 1T
		nLowPulse2 = pPacket->next();
		
		//Check outer limits
		//For valid bits 0/1/dim high is always 1. Low bits 1T or 4,5T
		if ((nHighPulse < nShortMin) || (nHighPulse > nShortMax))
			return false;
		if ((nHighPulse2 < nShortMin) || (nHighPulse2 > nShortMax))
			return false;
		if ((nLowPulse < nShortMin) || (nLowPulse > nLongMax))
			return false;
		if ((nLowPulse2 < nShortMin) || (nLowPulse2 > nLongMax))
			return false;
		
		//Check for bit 0/1/Dim
		if(nLowPulse > nLongMin)
			m_nCode |= 1; //Valid 1
		else if(nLowPulse < nShortMax)
		{
			if(nLowPulse2 < nShortMax)
				bDimBit = true; //Dimbit, check valid later. Leave bit 0.
			else if(nLowPulse2 > nLongMin)
				; //Valid 0. Leave bit 0.
			else
				return false; //Time cannot be between 1T and 4,5T.
		}
		else
			return false; //Time cannot be between 1T and 4,5T.

		//Check Valid dim bit. Add 2 bits for this position. 
		if(i == BIT_DIM_POSITION)
		{
			if(bDimBit)
			{
				bDimBit = false;
				if(nLengthBits == BIT_LENGTH_DIM_ORIG)
				{
					//Valid Dim bit. Add extra 0 bit,  encoder will send a Dim bit. BIT_LENGTH_DIM_ORIG=>BIT_LENGTH_DIM_CUSTOM
					m_nCode <<= 1; //00, but 01 would also work.
				}
				else
				{
					//Should not happen! DimBit only valid on fixed position. Just set 1.
					m_nCode |= 1;
				}
			}
			else if(nLengthBits == BIT_LENGTH_DIM_ORIG)
			{
				//Not a DimBit received! Just leave the One/Zero intact. Original senders do send One..
				if(m_nCode&1)
				{
					//One received. Add extra 1 bit, encoder will send a One bit. BIT_LENGTH_DIM_ORIG=>BIT_LENGTH_DIM_CUSTOM
					m_nCode <<= 1;
					m_nCode |= 1;   //11
				}
				else
				{
					//One received. Add extra 1 bit, encoder will send a One bit. BIT_LENGTH_DIM_ORIG=>BIT_LENGTH_DIM_CUSTOM
					m_nCode |= 1;
					m_nCode <<= 1;  //10
				}
			}
		}
		
		m_nCode <<= 1;		
	}

	//Undo last, as for loop exits
	m_nCode >>= 1;	
	
	return (m_nCode != 0);
}

void KakuNewProtocolDecoder::fillPacket(NinjaPacket* pPacket)
{
	pPacket->setEncoding(ENCODING_KAKUNEW);
	pPacket->setTiming(m_nPulseLength_T);
	pPacket->setType(TYPE_DEVICE);
	pPacket->setGuid(0);
	pPacket->setDevice(ID_ONBOARD_RF);
	pPacket->setData(m_nCode); //32 or 36bits
}