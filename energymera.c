/*************************************************************************************************************************************
 *                                                  Realization functions ENERGYMERA
 *************************************************************************************************************************************/

#include "is74_workflow.h"


/* Extern struct variables */
extern is74_UART_Init_Variable  is74_InitParam;
extern dm_to_is74_main_Variable dm_to_is74_main_var;

/* Common structures */
extern Connect Common;
                                                                  
/* Device information structure */  
extern ConnectionENR  EnrCon;                                                                        // EnergyMera

/* Clipboards and other variables */
extern uint8_t UART2_TX_buff[SIZE_TXRX];                                                             // Buffer to send the commands to UART
extern uint8_t UART2_RX_buff[SIZE_TXRX];                                                             // Buffer to recive the response from UART
extern uint8_t tmp[268];                                                                             // Temp buffer  



/* The function of connection request */
void connect_energy_mera(void)
{
  static uint8_t count = 0;
  
  /* Open connetion with EnergyMera */
  request_enr(OpenConnectEnr, sizeof(OpenConnectEnr)/sizeof(uint8_t), 16); 
  
  while(true)
  {
    if(dm_to_is74_main_var.RxComplete)
    {
      for(uint8_t i = 0; i < 16; i++)
        if(UART2_RX_buff[i] != 0x00)
        {
          Common.isOpen = 0x01;
          break;
        }
      
      /* If the port (RS-485) is busy */
      if(EnrCon.model[2] == 'E' && EnrCon.model[3] == 'R' && EnrCon.model[4] == 'R')
        Common.isOpen = 0x00;
      
      /* Read the model of device */
      if(Common.isOpen)
        to_string_req_enr(EnrCon.model);
      
      /* If the device does not respond more 
         than 20 times go to the next */
      if(count > 10)
      {
        Common.isOpen = 0x01;
        Common.error = 0x01;
        count = 0;
      }
      else 
        count++;

      /* Interval inits and answer line deductio */
      clear_ans();   
      Common.complete = 0;
      break;
    }  
  }
}

/* The function converts answer from byte to string */
void to_string_req_enr(char *arg)
{  
  uint16_t count = 0; 
  
  while( UART2_RX_buff[count] != 0x03)
    count++;
  
  for(uint16_t i = 0; i < count+1; i++)
    arg[i] = (char)UART2_RX_buff[i];
}

/* The function forms and send the query string for UART */
void request_enr(uint8_t *req, uint8_t len, uint16_t size_rec)
{
  for(int i = 0; i < len; i++)                                                              // Bytes of data
    UART2_TX_buff[i] = req[i] & 0x7F;                                                       // Convert 8N1 to 7E1                                                                                                                          

  dmAPI_UART_Transaction(UART2_TX_buff, len, UART2_RX_buff, size_rec, 2000);                // Send command             
}

/* The main function EnergyMera */
void run_energy_mera(void)
{
  
}

void send_lora_energy_mera(void)
{
  if(Common.error)
  {
    // There are some errors
  }
  else
  {
    // There aren't any errors
  }
}
