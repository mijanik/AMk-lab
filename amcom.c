#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "amcom.h"

/// Start of packet character
const uint8_t  AMCOM_SOP         = 0xA1;
const uint16_t AMCOM_INITIAL_CRC = 0xFFFF;

static uint16_t AMCOM_UpdateCRC(uint8_t byte, uint16_t crc)
{
	byte ^= (uint8_t)(crc & 0x00ff);
	byte ^= (uint8_t)(byte << 4);
	return ((((uint16_t)byte << 8) | (uint8_t)(crc >> 8)) ^ (uint8_t)(byte >> 4) ^ ((uint16_t)byte << 3));
}


void AMCOM_InitReceiver(AMCOM_Receiver* receiver, AMCOM_PacketHandler packetHandlerCallback, void* userContext) {
	// TODO

  receiver->receivedPacket.header.crc = 0;
  receiver->receivedPacket.header.length = 0;
  receiver->receivedPacket.header.type = 0;
  receiver->receivedPacket.header.sop = 0xA1;
  
  for(uint8_t i = 0; i<200; i++) receiver->receivedPacket.payload[i] = 0;

  receiver->payloadCounter = 0;
  receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY ;
  
  receiver->packetHandler = packetHandlerCallback;
  
  receiver->userContext = userContext;
  
}

size_t AMCOM_Serialize(uint8_t packetType, const void* payload, size_t payloadSize, uint8_t* destinationBuffer) {
	// TODO

  uint8_t bytes_written = 0;

  uint8_t CRC_high_order;
  uint8_t CRC_low_order;

  uint16_t CRC = AMCOM_INITIAL_CRC;

  uint8_t length = (uint8_t) payloadSize;

  unsigned const char * ladunek = (unsigned const char *) payload;
  
  //calculating CRC
  CRC = AMCOM_UpdateCRC(packetType, CRC);
  CRC = AMCOM_UpdateCRC(length, CRC);

  for(uint8_t i = 0; i<length; i++){
    CRC = AMCOM_UpdateCRC(ladunek[i], CRC);
  }

  destinationBuffer[bytes_written] = AMCOM_SOP;
  bytes_written++;

  destinationBuffer[bytes_written] = packetType;
  bytes_written++;

  destinationBuffer[bytes_written] = payloadSize;
  bytes_written++;

  CRC_high_order = CRC>>8;
  CRC_low_order = CRC & 0x00FF;

  destinationBuffer[bytes_written] = CRC_low_order;
  bytes_written++;
  destinationBuffer[bytes_written] = CRC_high_order;
  bytes_written++;

  uint8_t bytes_written_before_payload = bytes_written;
  for(; bytes_written < payloadSize + bytes_written_before_payload; bytes_written++){
    destinationBuffer[bytes_written] = ladunek[bytes_written-bytes_written_before_payload];
  }
  
	return bytes_written;
}

void state_get_SOP(){
  
}


void AMCOM_Deserialize(AMCOM_Receiver* receiver, const void* data, size_t dataSize) {

  uint8_t* dane = (uint8_t*)data;
  
  for(int i=0; i< dataSize; i++){ //checking every data byte

    switch(receiver->receivedPacketState){
      
      //get SOP --------------------
      case AMCOM_PACKET_STATE_EMPTY:
        {
        if(*(dane+i) == AMCOM_SOP){
          receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_SOP;
          receiver->receivedPacket.header.sop = AMCOM_SOP;
          break; //SOP catched! - go to next data byte
        }
        else{
          break; //SOP not found, check next byte
        }
        }


      //get TYPE --------------------
      case AMCOM_PACKET_STATE_GOT_SOP:
        {        
        receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_TYPE;
        receiver->receivedPacket.header.type = *(dane+i);
        break; //TYPE read, go to next data byte
        }

      
      //get LENGTH --------------------
      case AMCOM_PACKET_STATE_GOT_TYPE:
        {
        if(*(dane+i) <=200 && *(dane+i) >= 0){ //check if length have correct value
          receiver->receivedPacket.header.length = *(dane+i);
          receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_LENGTH;
          break; //LENGTH read, go to next byte
        }
        else{ //LENGTH ERROR
          receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY; //go ot state 0
          break; //go to next data byte and search again for SOP
        }
        }

      
      //get low CRC --------------------
      case AMCOM_PACKET_STATE_GOT_LENGTH:
        {
        receiver->receivedPacket.header.crc = *(dane+i); //catch low CRC
        receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_CRC_LO;
        break; //CRC low byte read, go to next data byte
        }

      
      //get high CRC and check LENGTH value --------------------
      case AMCOM_PACKET_STATE_GOT_CRC_LO:
        {
        receiver->receivedPacket.header.crc |= *(dane+i) << 8;
      
        if(receiver->receivedPacket.header.length == 0){ //empty packet
          receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_WHOLE_PACKET; //got whole (empty) packet
          break;
        }
      
        if(receiver->receivedPacket.header.length > 0){ //non-empty packet
          receiver->receivedPacketState = AMCOM_PACKET_STATE_GETTING_PAYLOAD;
          receiver->payloadCounter = 0; //count from 0 to length
          break; //starting payload read - go to next data byte
        }
        }

      
      //getting payload (if packet not empty) (length times) --------------------
      case AMCOM_PACKET_STATE_GETTING_PAYLOAD:
        {
        if(receiver->payloadCounter < receiver->receivedPacket.header.length-1){ //read data without last payload byte (nie chcemy zmieniac bajtu po odczycie ostatniego)
          receiver->receivedPacket.payload[receiver->payloadCounter] = *(dane+i); //read payload and write to payload table
          receiver->payloadCounter++;
          break;
        }
        receiver->receivedPacket.payload[receiver->payloadCounter] = *(dane+i);//read last payload byte
        receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_WHOLE_PACKET; //payload finished - got whole packet (we do NOT move to next byte)
        break;
        }

      
    }//end Switch case

    
      //got whole packet - checking CRC --------------------
    if(receiver->receivedPacketState == AMCOM_PACKET_STATE_GOT_WHOLE_PACKET){
      uint16_t crc = AMCOM_INITIAL_CRC;
      crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.type, crc);
      crc = AMCOM_UpdateCRC(receiver->receivedPacket.header.length, crc);
      
      if(receiver->receivedPacket.header.length >0){
        for(uint8_t j = 0; j< receiver->receivedPacket.header.length; j++){
        	crc = AMCOM_UpdateCRC(receiver->receivedPacket.payload[j], crc);
        }   
      }
      
      if(crc == receiver->receivedPacket.header.crc){ //CRC OK?
        receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY; //reset state to 0
        (*receiver->packetHandler)(&receiver->receivedPacket, receiver->userContext);
      }
    }

    
  } //end for
} //end Deserialize
