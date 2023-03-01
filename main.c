#include "stm32f4xx_hal.h"
#include "led_init.h"

//#include "led_init.h"
//#include "delay.h"
//#include "systick.h"
#include "usart.h"

//#include "FreeRTOS.h"
//#include "task.h"
#include "stdio.h"
//#include "trace_freertos.h"
//#include "semphr.h"
//#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include "math.h"
#include "string.h"

#include "amcom.h"
#include "amcom_packets.h"
#define M_PI 3.1415926

AMCOM_FoodState food_list[200];


AMCOM_IdentifyRequestPayload identifyRequest;

AMCOM_NewGameRequestPayload newGameRequest;

AMCOM_PlayerUpdateRequestPayload PlayerUpdateRequest;

AMCOM_FoodUpdateRequestPayload foodUpdateRequest;

AMCOM_MoveRequestPayload moveRequest;

AMCOM_MoveResponsePayload moveResponse;

//-------------------------------------------------------

float calculate_angle(float x1, float y1, float x2, float y2){

  float angle;
  angle = atan2(y2 - y1, x2 - x1);
  
  if (angle<0){
    angle+=2*M_PI;
  }

  return angle;
}

float calculate_distance(float x1, float y1, float x2, float y2){
  return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}



float brain(){
  float minDist = 99999;//1414.2137;
  uint16_t minDistIndex = 199;
  float safe_radius = 100;
  float nearOppDist = 1414.2137;
  uint8_t nearOppIndex = 7;//0-7
  float angle = 0;
  
  //scan if someone is in safe radius
  for(int i = 0; i < newGameRequest.numberOfPlayers; i++){
    //check if it is not our number && hp is not 0 && is closer than others 
    if(PlayerUpdateRequest.playerState[i].playerNo != newGameRequest.playerNumber && PlayerUpdateRequest.playerState[i].hp > 0 && calculate_distance(moveRequest.x, moveRequest.y, PlayerUpdateRequest.playerState[i].x, PlayerUpdateRequest.playerState[i].y) < nearOppDist){
      nearOppDist = calculate_distance(moveRequest.x, moveRequest.y, PlayerUpdateRequest.playerState[i].x, PlayerUpdateRequest.playerState[i].y);
      nearOppIndex = PlayerUpdateRequest.playerState[i].playerNo;
    }
  }
  
  //scan for nearest food
  for(int i = 0; i<199; i++){
    if(food_list[i].state == 1 && (calculate_distance(moveRequest.x, moveRequest.y, food_list[i].x, food_list[i].y) < minDist)){
      minDist = calculate_distance(moveRequest.x, moveRequest.y, food_list[i].x, food_list[i].y);
      minDistIndex = food_list[i].foodNo;//(i)
    }
  }

  //someone in radius
  if(nearOppDist < safe_radius){
    //calculate angle to opponent
    angle = calculate_angle(moveRequest.x, moveRequest.y, PlayerUpdateRequest.playerState[nearOppIndex].x, PlayerUpdateRequest.playerState[nearOppIndex].y);

    //check if closest opponent HP is bigger than ours
    if(PlayerUpdateRequest.playerState[nearOppIndex].hp > PlayerUpdateRequest.playerState[newGameRequest.playerNumber].hp){
      //change to opposite angle - run away!!
      angle = fmod((angle + M_PI), (2 * M_PI));
    }
    else if(PlayerUpdateRequest.playerState[nearOppIndex].hp == PlayerUpdateRequest.playerState[newGameRequest.playerNumber].hp){
      angle = calculate_angle(moveRequest.x, moveRequest.y, food_list[minDistIndex].x, food_list[minDistIndex].y);
    }
    //remainder(); <- can also be used
  }

  //safe radius is clear - if food exist - go to it
  else if(minDist<99999){
    angle = calculate_angle(moveRequest.x, moveRequest.y, food_list[minDistIndex].x, food_list[minDistIndex].y);
  }

  //no one in safe radius and no food - checking if nearest opponent is weaker)
  else if (nearOppDist<1414 && PlayerUpdateRequest.playerState[nearOppIndex].hp < PlayerUpdateRequest.playerState[newGameRequest.playerNumber].hp){
    angle = calculate_angle(moveRequest.x, moveRequest.y, PlayerUpdateRequest.playerState[nearOppIndex].x, PlayerUpdateRequest.playerState[nearOppIndex].y);
  }

  //no one in safe radius and no food - checking if nearest opponent is stronger)
  else if (nearOppDist<1414 && PlayerUpdateRequest.playerState[nearOppIndex].hp > PlayerUpdateRequest.playerState[newGameRequest.playerNumber].hp){
    angle = calculate_angle(moveRequest.x, moveRequest.y, PlayerUpdateRequest.playerState[nearOppIndex].x, PlayerUpdateRequest.playerState[nearOppIndex].y);
    angle = fmod((angle + M_PI), (2 * M_PI));
  }

  //no opponents or equal hp
  else{
    static uint8_t direction = 1;
    if(direction == 0){
      angle = 0;
      direction = 1;
    }
    else{
      angle = M_PI;
      direction = 0;
    }
  }
  
  return angle;
  
}

//---------------------------------------------------------





/**
 * This function will be called each time a valid AMCOM packet is received
 */
void amcomPacketHandler(const AMCOM_Packet* packet, void* userContext) {
	uint8_t amcomBuf[AMCOM_MAX_PACKET_SIZE];	// buffer used to serialize outgoing packets
	size_t bytesToSend = 0;						// size of the outgoing packet
	static int playerCounter;					// just a counter to distinguish player instances
	//SOCKET sock = (SOCKET)userContext;			// socket used for communication with the game client

	switch (packet->header.type) {
	
    case AMCOM_IDENTIFY_REQUEST:
    memcpy(&identifyRequest, &packet->payload, 4);
      
    printf("Got IDENTIFY.request. Responding with IDENTIFY.response\n");
		AMCOM_IdentifyResponsePayload identifyResponse;
		sprintf(identifyResponse.playerName, "MILONI%d", playerCounter++);
		bytesToSend = AMCOM_Serialize(AMCOM_IDENTIFY_RESPONSE, &identifyResponse, sizeof(identifyResponse), amcomBuf);
		break;


    
	case AMCOM_NEW_GAME_REQUEST:
		printf("Got NEW_GAME.request.\n");
    
    memcpy(&newGameRequest, &packet->payload, 10);
    
    // TODO: respond with NEW_GAME.confirmation
    bytesToSend = AMCOM_Serialize(AMCOM_NEW_GAME_RESPONSE, NULL, 0, amcomBuf);
	    break;

    
	case AMCOM_PLAYER_UPDATE_REQUEST:
		printf("Got PLAYER_UPDATE.request.\n");
		// TODO: use the received information
    memcpy(&PlayerUpdateRequest, &packet->payload, packet->header.length);
    
	  break;
	
    
  case AMCOM_FOOD_UPDATE_REQUEST:
		printf("Got FOOD_UPDATE.request.\n");
		// TODO: use the received information

    memcpy(&foodUpdateRequest, &packet->payload, packet->header.length);
    for(int i = 0; i<packet->header.length/11; i++){
      int food_id = foodUpdateRequest.foodState[i].foodNo;
      food_list[food_id].foodNo = foodUpdateRequest.foodState[i].foodNo;
      food_list[food_id].state = foodUpdateRequest.foodState[i].state;
      food_list[food_id].x = foodUpdateRequest.foodState[i].x;
      food_list[food_id].y = foodUpdateRequest.foodState[i].y;
    }
		break;
	
    
  case AMCOM_MOVE_REQUEST:
		printf("Got MOVE.request.\n");
		// TODO: respond with MOVE.confirmation
    memcpy(&moveRequest, &packet->payload, packet->header.length);


    moveResponse.angle = brain();
    
    bytesToSend = AMCOM_Serialize(AMCOM_MOVE_RESPONSE, &moveResponse, sizeof(identifyResponse), amcomBuf);
		break;
	}
  
	if (bytesToSend > 0) {
		int bytesSent = USART_WriteData((const char*)amcomBuf, bytesToSend);	
	}
}

void SysTick_Handler(void){
	//do nothing
} /* SysTick_Handler */

/** Application entry point */
int main(void)
{
  // Initialize STM32Cube HAL library
  HAL_Init();
	USART_Init();
	

	AMCOM_Receiver amcomReceiver;	// AMCOM receiver structure
	char buf[512];								// buffer for temporary data
	int receivedBytesCount;				// holds the number of bytes received via socket

	// Initialize AMCOM receiver
	AMCOM_InitReceiver(&amcomReceiver, amcomPacketHandler, NULL);

	// Receive data from socket until the peer shuts down the connection
	while(1) {
		// Fetch the bytes from socket into buf
		receivedBytesCount =  USART_ReadData(buf, sizeof(buf));
		if (receivedBytesCount > 0) {
			AMCOM_Deserialize(&amcomReceiver, buf, receivedBytesCount);
		}
		
	}
	
} /* main */



