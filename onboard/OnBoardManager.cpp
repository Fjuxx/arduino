#include "OnBoardManager.h"

#include "../common/Ninja.h"
#include "../common/Heartbeat.h"

#include "NinjaLED.h"

#include "../decoder/CommonProtocolDecoder.h"
#include "../encoder/CommonProtocolEncoder.h"

#include "../decoder/WT450ProtocolDecoder.h"

#include "../decoder/arlecProtocolDecoder.h"
#include "../encoder/arlecProtocolEncoder.h"

#include "../decoder/KakuNewProtocolDecoder.h"
#include "../encoder/KakuNewProtocolEncoder.h"

#include "../decoder/HE330v2ProtocolDecoder.h"
#include "../encoder/HE330v2ProtocolEncoder.h"

#include "../decoder/OSv2ProtocolDecoder.h"
#include "../encoder/OSv2ProtocolEncoder.h"

#include "../decoder/bInDProtocolDecoder.h"
#include "../encoder/bInDProtocolEncoder.h"

#include "../decoder/bOutDProtocolDecoder.h"
#include "../encoder/bOutDProtocolEncoder.h"

extern NinjaLED leds;

OnBoardManager::OnBoardManager()
{

}

void OnBoardManager::setup()
{
	m_Transmitter.setup(TRANSMIT_PIN);

	m_Receiver.start();
}

void OnBoardManager::check()
{
	RFPacket* pReceivedPacket = m_Receiver.getPacket();
	if(pReceivedPacket != NULL)
	{
		m_Receiver.stop();
		bool bDecodeSuccessful = false;

		for(int i = 1; i <= 8; i++)
		{
			switch (i)
			{
				case 1:
				m_Decoder = new CommonProtocolDecoder(); // 50bits
				break;
				
				case 2:
				m_Decoder = new WT450ProtocolDecoder();  // >=64bits
				break;

				case 3:
				m_Decoder = new KakuNewProtocolDecoder();  // 132/148bits
				break;

				case 4:
				m_Decoder = new arlecProtocolDecoder(); // 26bits
				break;

				case 5:
				m_Decoder = new HE330v2ProtocolDecoder(); //132 bits (TODO: test KakuNewProtocolDecoder does not conflict)
				break;

				case 6:
				m_Decoder = new OSv2ProtocolDecoder(); //>=136bits
				break;

				case 7:
				m_Decoder = new bInDProtocolDecoder(); // 50bits
				break;			
			
				case 8:
				m_Decoder = new bOutDProtocolDecoder(); //96bits
				break;			
			}
			
			if(m_Decoder->decode(pReceivedPacket))
			{
				bDecodeSuccessful = true;
				leds.blinkStat(); // Blink stat LED to show activity

				NinjaPacket packet;
				m_Decoder->fillPacket(&packet);
				packet.printToSerial();	
				
				delete m_Decoder;
				break;
			}
			pReceivedPacket->rewind();
			delete m_Decoder;
		}
		
		m_Receiver.purge();
		m_Receiver.start();
	}

	// Check if heartbeat expired
	if(heartbeat.isExpired())
	{
		m_Receiver.stop();

		NinjaPacket packet;

		packet.setType(TYPE_DEVICE);
		packet.setGuid(0);
		packet.setDevice(ID_STATUS_LED);
		packet.setData(leds.getStatColor());

		packet.printToSerial();
		
		packet.setDevice(ID_NINJA_EYES);
		packet.setData(leds.getEyesColor());

		packet.printToSerial();
		
		m_Receiver.start();
	}
}

void OnBoardManager::handle(NinjaPacket* pPacket)
{
	if(pPacket->getGuid() != 0)
		return;

	if(pPacket->getDevice() == ID_STATUS_LED)
		leds.setStatColor(pPacket->getData());
	else if(pPacket->getDevice() == ID_NINJA_EYES)
		leds.setEyesColor(pPacket->getData());
	else if(pPacket->getDevice() == ID_ONBOARD_RF)
	{
		m_Receiver.stop();
		
		char encoding = pPacket->getEncoding();
		switch (encoding)
		{
			case ENCODING_COMMON:
				m_encoder = new CommonProtocolEncoder(pPacket->getTiming());
				break;
			case ENCODING_KAKUNEW:
				m_encoder = new KakuNewProtocolEncoder(pPacket->getTiming());
				break;
			case ENCODING_ARLEC:
				m_encoder = new arlecProtocolEncoder(pPacket->getTiming());
				break;
			case ENCODING_HE330:
				m_encoder = new HE330v2ProtocolEncoder(pPacket->getTiming());
				break;
			case ENCODING_OSV2:
				m_encoder = new OSv2ProtocolEncoder(pPacket->getTiming());
				break;
			case ENCODING_BIND:
				m_encoder = new bInDProtocolEncoder(pPacket->getTiming());
				break;
			case ENCODING_BOUTD:
				m_encoder = new bOutDProtocolEncoder(pPacket->getTiming());
				break;								
			default:
				m_encoder = new CommonProtocolEncoder(pPacket->getTiming());
				break;

		}
		
		if(pPacket->isDataInArray())
			m_encoder->setCode(pPacket->getDataArray(), pPacket->getArraySize());
		else 
			m_encoder->setCode(pPacket->getData());
			
		m_encoder->encode(&m_PacketTransmit);
		
		m_Transmitter.send(&m_PacketTransmit, 5);
		delete m_encoder;
	}

	m_Receiver.stop(); //prevent double printing to serial.

	pPacket->setType(TYPE_ACK);
	pPacket->printToSerial();

	m_Receiver.start();

}