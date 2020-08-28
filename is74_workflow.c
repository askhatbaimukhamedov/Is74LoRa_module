/***********************************************************************************************************************************************
 *  is74_workflow.c - is a main workpoint for IS74 programming team
 *  2019 Intersviyaz (*)> quack quack ...
 ***********************************************************************************************************************************************/

#include "is74_workflow.h"

         
/* Extern struct variables */
extern is74_UART_Init_Variable  is74_InitParam;
extern dm_to_is74_main_Variable dm_to_is74_main_var;

/* Common structures */
RelayStrat strat;
Connect Common;
                                                                  
/* Device information structures */  
ConnectionENR  EnrCon;                                                                        // EnergyMera
ConnectionMER  MerCon;                                                                        // Mercury

/* Clipboards and other variables */
uint8_t UART2_TX_buff[SIZE_TXRX] = {0};                                                       // Buffer to send the commands to UART
uint8_t UART2_RX_buff[SIZE_TXRX] = {0};                                                       // Buffer to recive the response from UART
uint8_t tmp[268]                 = {0};                                                       // Temp buffer  



/* Main cycle function for IS74 team, runs cyclically in the main loop */
void is74_mainloop(void)
{
   /* Response on request. Command handler */
   if(dm_to_is74_main_var.LoRaRxCompleteFlag)
     request_handler(dm_to_is74_main_var.LoRaRxBuff, dm_to_is74_main_var.LoRaRxBuffSize); 
  
   switch (dm_to_is74_main_var.State)
   { 
     case IS74_STATE_IDLE: break;
     
     /* The main case of the program */
     case IS74_STATE_WAIT_FOR_RESPONSE:
     {
        /* Strategy work relay */
       if(dm_to_is74_main_var.UnixTime != 0x00)
         onoff_relay(dm_to_is74_main_var.UnixTime);
       
        /* Scanning of target device */
       if(dm_to_is74_main_var.LoRaTxTimerFlag || Common.flag_relay_status 
                                || Common.request_04 || Common.request_05)
       {       
         /* Save the first request time */
         if(dm_to_is74_main_var.LoRaTxTimerFlag)
           Common.t = (!Common.t) ? dm_to_is74_main_var.UnixTime : Common.t;
         
         /* Send data to the LoRa server */
         if (dm_to_is74_main_var.LoRaTxComplete && Common.is_send)
         {
           Common.index_com++;           
           switch(Common.whoami)
           {     
             //case energy_mera: send_lora_energy_mera(); break;
             case mercury: send_lora_mercury(); break;
           }
         } 
         
         /* Scanning of target device */
         else if (dm_to_is74_main_var.RxComplete && !Common.is_send)
         {
           dm_to_is74_main_var.RxComplete = false;
           Common.complete = 1;
           
           /* Delay between commands to device */
           is74_delay(DEL_COM_ENR, DEL_COM_MER);                                             
           Common.index_com++;
         }       
         if (dm_to_is74_main_var.TimerAlarm && Common.complete)
         {
           dm_to_is74_main_var.TimerAlarm = false;       
           switch(Common.whoami)
           {     
             //case energy_mera: run_energy_mera(); break;
             case mercury: run_mercury(); break;          
             default: is74__OneTime();
           }
         }
       }
     } break; 
     
     /* Upon successful connection of the module to the 
        network variable state = IS74_STATE_RS485_CONNECT */
     case IS74_STATE_WAIT_FOR_JOIN:
     {
       if (dm_to_is74_main_var.LoRaJoinState == LORA_JOINST_JOINED)
       {
         /* Setting the start time */
         uint32_t time = 0;
         dm_API_StartUnixTimer(time);
         
         /* Request from the server, real time*/
         uint8_t req_time[] = {0x00, 0x00, 0x00, 0x00};
          
         /* Check sum and send request */
         wrapper_crc(req_time, sizeof(req_time)/sizeof(uint8_t));
         dmAPI_LoRaWAN_Send(req_time, sizeof(req_time)/sizeof(uint8_t), 0x00, 0x80);
          
         dm_to_is74_main_var.State = IS74_STATE_RS485_CONNECT;
       }
       else if (dm_to_is74_main_var.LoRaJoinState == LORA_JOINST_NOTJOINED)
         dm_to_is74_main_var.State = IS74_STATE_NO_LORAWAN_NET;        
     } break;
     
     /* Connect to the target device and transfer 
        control to case: IS74_STATE_WAIT_FOR_RESPONSE */
     case IS74_STATE_RS485_CONNECT:
     {
       switch(Common.whoami)
       {
         //case energy_mera: connect_energy_mera(); break;
         case mercury: connect_mercury(); break;
       }
       dm_to_is74_main_var.State = IS74_STATE_WAIT_FOR_RESPONSE;     
     } break; 
     
     /* If you here check LoRaWAN NET  */
     case IS74_STATE_NO_LORAWAN_NET:
     {
       is74__OneTime();
       break;                                                                                
     }
     default: break;
  } 
}

