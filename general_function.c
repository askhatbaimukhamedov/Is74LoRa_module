/*************************************************************************************************************************************
 *                                                  Realization General functions 
 *************************************************************************************************************************************/

#include "is74_workflow.h"


/* Extern struct variables */
extern is74_UART_Init_Variable  is74_InitParam;
extern dm_to_is74_main_Variable dm_to_is74_main_var;

/* For relay */
extern output_pin_Variable output_ok_1;
extern output_pin_Variable output_ok_2;
extern output_pin_Variable output_ok_3;
extern output_pin_Variable output_ok_4;

/* Common structures */
extern RelayStrat strat;
extern Connect Common;                                                                

/* Clipboards and other variables */
extern uint8_t UART2_TX_buff[SIZE_TXRX];                                                             // Buffer to send the commands to UART
extern uint8_t UART2_RX_buff[SIZE_TXRX];                                                             // Buffer to recive the response from UART
extern uint8_t tmp[268];                                                                             // Temp buffer  



/* StartUp UART (RS485) Inicialisation, Run from main() at startup */
void is74__UART_Init(void)
{
  is74_InitParam.BaudRate = 9600;
  
  /* #define UART_WORDLENGTH_7B, #define UART_WORDLENGTH_8B, #define UART_WORDLENGTH_9B */                
  is74_InitParam.WordLength = UART_WORDLENGTH_7B;
  
  /* #define UART_STOPBITS_1, #define UART_STOPBITS_1_5, #define UART_STOPBITS_2 */           
  is74_InitParam.StopBits = UART_STOPBITS_1;
  
  /* #define UART_PARITY_NONE, #define UART_PARITY_EVEN, #define UART_PARITY_ODD */
  is74_InitParam.Parity = UART_PARITY_EVEN;
}

/* OneTime Run function after all initialization 
   is done and before main loop */
void is74__OneTime(void)
{ 
  /* Configuration initialization */
  setup_config(); 
  
  /* Connetion to LoRa */
  dmAPI_LoRaWAN_StartJoin();
  dm_to_is74_main_var.State  = IS74_STATE_WAIT_FOR_JOIN;
  dm_to_is74_main_var.LoRaTxTimerFlag = true;
  dm_to_is74_main_var.LoRaTxComplete = true;
 
}

/* The function setup inital configuration */
void setup_config(void)
{
  /* Target device init */
  Common.whoami = mercury;
  
  /* Relay status flag init */
  Common.flag_relay_status = false; 
  
  /* Request flags init */
  Common.request_04 = false; 
  Common.request_05 = false;
  
  /* Flags. Day and month 
     of last shipment. Init */
  Common.send_month = false;
  Common.send_day = false;
  
  /* Others  variables init */    
  Common.delta_delay = 0x00;
  Common.net_addr_count = 0x01;
  Common.index_com = 0; 
  Common.is_send = 0;                                                                         
  Common.complete = 0;
  Common.delay = 0;
  Common.isOpen = 0;
  Common.t = 0;
  Common.delta_t = 0;
  Common.error = 0;
  
  /* Installation of the starting 
     configuration from the EEPROM */
  uint32_t eeprom[12] = {0};
  
  /* Read delay from eeprom */
  dm_API_ReastoreFromEEPROM(EEPR_DELAY, eeprom, 0x01);
  Common.delay = (eeprom[0]) ? eeprom[0] : HOUR;
  
  /* Read target device and config com-port from eeprom */
  dm_API_ReastoreFromEEPROM(EEPR_COM_PORT, eeprom, 0x05);
  Common.whoami = eeprom[0];
  is74_InitParam.BaudRate = eeprom[1];
  is74_InitParam.WordLength = eeprom[2];
  is74_InitParam.StopBits = eeprom[3];
  is74_InitParam.Parity = eeprom[4];
  
  dmAPI_UART_ReInit();
  
  /* Read send_month and send_day from eeprom */
  dm_API_ReastoreFromEEPROM(EEPR_SEND_M, eeprom, 0x01);
  Common.send_month = eeprom[0];
  dm_API_ReastoreFromEEPROM(EPPR_SEND_D, eeprom, 0x01);
  Common.send_day = eeprom[0];
  
  /* Read net addr devices and other staff like that from eeprom */
  dm_API_ReastoreFromEEPROM(EEPR_ADDR_NET, eeprom, 
                            sizeof(Common.netAddrDev)/sizeof(uint8_t));
  for(uint8_t i = 0; i < sizeof(Common.netAddrDev)/sizeof(uint8_t); i++)
    Common.netAddrDev[i] = eeprom[i];
}

/* Function run before LoRaWAN send */
void is74_PrepareTx(void){
  // There are will be something (idk) (*)> quack quack ...
}

/* The function calculates the checksum of a sequence 
   of len bytes, specified by msg. 16bit */
uint16_t Crc16MudBus(uint8_t *msg, uint8_t len)
{
  uint16_t crc = 0xFFFF;
  for(uint16_t pos = 0; pos < len; pos++)
  {
    crc ^= (uint16_t)msg[pos];                                                              // XOR byte into least sig. byte of crc
    for (uint16_t i = 8; i != 0; i--)                                                       // Loop over each bit
    {                                                      
      if((crc & 0x0001) != 0)                                                               // If the LSB is set
      {                                                              
        crc >>= 1;                                                                          // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else crc >>= 1;                                                                       // Else LSB is not set | Just shift right
    }
  } return crc;
}

/* The function calculates the checksum of a sequence 
   of len bytes, specified by msg. 8bit */
uint16_t Crc8Calc(uint8_t *msg, uint8_t len)
{
  uint8_t sum = 0;
  while(len > 0)
  {
      sum += *msg++;
      len--;
  }
  sum ^= 0xff;   
  return sum;
}

/* The function clears the array 
   of responses after each use */
void clear_ans()
{
  for(int i = 0; i < SIZE_TXRX; i++)
    UART2_RX_buff[i] = UART2_TX_buff[i] = 0x00;
}

/* The function realization delay */
void is74_delay(uint8_t d_enr, uint8_t d_mer)
{
  switch(Common.whoami)
  {
    /* Request processing time for different devices: */
    case energy_mera: dmAPI_SetTimer(d_enr); break;                                           // EnergyMera
    case mercury: dmAPI_SetTimer(d_mer); break;                                               // Mercury
  }
}

/* The function adds a crc to the message */
void wrapper_crc(uint8_t *arr, uint8_t size)
{
  /* Check sum */
  uint16_t crc = Crc16MudBus(arr, size - 2);
  uint8_t cr1 = crc & 0xFF, cr2 = (crc >> 8) & 0xFF;
  arr[size-2] = cr1; 
  arr[size-1] = cr2;
}

/* The function to convert from human readable date to epoch */
uint32_t date_to_epoch(uint8_t *arr, uint8_t i)
{
  struct tm t;
  
  /* date --> epoch */
  t.tm_year = ((arr[i] << 8) + arr[i+1]) - 0x076C;
  t.tm_mon = (arr[i+2] - 0x01);
  t.tm_mday = arr[i+3];
  t.tm_hour = arr[i+4];
  t.tm_min = arr[i+5];
  t.tm_sec = 0x00;
  
  /* Is DST on? 1 = yes, 0 = no,
     -1 = unknown */
  t.tm_isdst = -1;        
  
  return  mktime(&t);
}

/* The function to convert from epoch to human readable date*/
struct tm *epoche_to_date()
{
  struct tm *res = localtime(&dm_to_is74_main_var.UnixTime);
  return res;
}

/* The function checks if the device does not respond. */
uint8_t is_empty_metrika()
{
  for(int i = 0; i < SIZE_TXRX; i++)
    if(0x00 != UART2_RX_buff[i])
      return 0x00;
  return 0x02;
}

/* The function for handling device commands */
void request_handler(uint8_t *buf, uint8_t len)
{
  switch(*buf)
  {
    /* To pick target device and configuration com-port */
    case 0x02: 
    {
      if(0x07 == len)
      {
        /* Target device */
        Common.whoami = buf[1];
        
        /* BaudRate. Possible values: 4800, 9600, ... */
        is74_InitParam.BaudRate = (buf[2] << 8) + buf[3];        
        
        /* WordLenght. Possible values: 7, 8, 9 bits */
        if(0x07 == buf[4]) is74_InitParam.WordLength = UART_WORDLENGTH_7B;
        else if(0x08 == buf[4]) is74_InitParam.WordLength = UART_WORDLENGTH_8B;
        else if(0x09 == buf[4]) is74_InitParam.WordLength = UART_WORDLENGTH_9B; 
        
        /* StopBits. Possible values: 1, 1.5, 2 bits */
        if(0x10 == buf[5]) is74_InitParam.StopBits = UART_STOPBITS_1;
        else if(0x15 == buf[5]) is74_InitParam.StopBits = UART_STOPBITS_1_5;
        else if(0x20 == buf[5]) is74_InitParam.StopBits = UART_STOPBITS_2;   
        
        /* Parity. Possible values: none, even, odd */
        if(0x00 == buf[6]) is74_InitParam.Parity = UART_PARITY_NONE;
        else if(0x01 == buf[6]) is74_InitParam.Parity = UART_PARITY_ODD;
        else if(0x02 == buf[6]) is74_InitParam.Parity = UART_PARITY_EVEN; 
        
        /* Initialize buffer to write to EEPROM */
        uint32_t eepr_com_port[] = {Common.whoami, is74_InitParam.BaudRate, is74_InitParam.WordLength, 
                                                       is74_InitParam.StopBits, is74_InitParam.Parity};
        
        /* Save whoami and com-port configuration to EEPROM */
        dm_API_SaveToEEPROM(EEPR_COM_PORT, eepr_com_port, sizeof(eepr_com_port)/sizeof(uint32_t));
       
        /* Answer + Check sum */  
        uint8_t status_answer[] = {buf[1], 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send status of handler to LoRa. If successful */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]);  
        
        /* UART re-initialization */
        dmAPI_UART_ReInit();
        is74__OneTime();
      }
      else
      {   
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x02); 
      }
    } break;   
    
    /* Setting the device scanning schedule. Set delay */
    case 0x03:
    {
      if(0x03 == len && (buf[1]*0x03c + buf[2]) >= 0x0a)
      {
        Common.delay = HOUR * buf[1] + MINUTE * buf[2];
        uint32_t eepr_delay[] = {Common.delay};
        
        /* Save delay to EEPROM */
        dm_API_SaveToEEPROM(EEPR_DELAY, eepr_delay, sizeof(eepr_delay)/sizeof(uint32_t));
        
        /* Set timer */
        dm_API_SetLoRaTxTimer(Common.delay);  
         
        /* Answer + Check sum */  
        uint8_t status_answer[] = {buf[1], buf[2], 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send set delay to LoRa. If successful  */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]);
      }
      else
      {
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x03);
      }
    } break;
    
    /* Is the relay turned on? */
    case 0x04: 
    {
      if(0x02 == len)
      {
        Common.request_04 = true; 
        Common.netAddr = buf[1];
      }
      else
      {   
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x04);
      }
    } break;
    
    /* Response on request */
    case 0x05: 
    {
      if(0x03 == len && (buf[1] == 0x81 || 
         buf[1] == 0x82 || buf[1] == 0x84))
      {
        Common.request_05 = true;
        Common.desc_integral = buf[1]; 
        Common.netAddr = buf[2];
      }
      else
      {
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x05);
      }
    } break;
    
     /* Setting the time to module  */
    case 0x06:
    {     
      if(0x06 == len)
      {
        uint32_t time = 0;
        
        for(uint8_t i = 1; i <= 4; i++)
        {
          time += buf[i];
          if(i != 4) time <<= 8;
        }
        time += buf[5] * UNIX_HOUR;
        dm_API_StartUnixTimer(time);
        
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, buf[1], buf[2], buf[3], buf[4], 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send set time to LoRa. If successful  */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]);
      }
      else
      {
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x06);
      }
    } break;
    
    /* Set network addresses */
    case 0x07:
    {
      uint8_t num = buf[1];
      if(num + 2 == len && num < 5)
      {
        for(uint8_t i = 0; i < sizeof(Common.netAddrDev)/sizeof(uint8_t); i++)
          Common.netAddrDev[i] = 0x00;
        for(uint8_t i = 0; i < num+1; i++)
          Common.netAddrDev[i] = buf[i+1];
        
        /* Initialize buffer to write to EEPROM */
        uint32_t eepr_addr_net[11] = {0x00};
        for(uint8_t i = 0; i < sizeof(Common.netAddrDev)/sizeof(uint8_t); i++)
          eepr_addr_net[i] = Common.netAddrDev[i];
        
        /* Save net addr devices configuration to EEPROM */
        dm_API_SaveToEEPROM(EEPR_ADDR_NET, eepr_addr_net, sizeof(Common.netAddrDev)/sizeof(uint8_t)+2);
         
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));

        /* Send set net addresses for scanning to LoRa. If successful */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]);
        
        /* Restart the programm */
        is74__OneTime();
      }
      else
      {  
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x07);
      }
    } break;
    
    /* Relay operation strategy */
    case 0x08:
    {
      if(0x0e == len)
      {  
        /* Convert from human readable date to epoch */        
        uint32_t relay_on  = date_to_epoch(buf, 0x02);
        uint32_t relay_off = date_to_epoch(buf, 0x08);
        
        switch(buf[1])
        {
          case 0x01: {strat.strat_relay1[0] = relay_on; strat.strat_relay1[1] = relay_off;} break;
          case 0x02: {strat.strat_relay2[0] = relay_on; strat.strat_relay2[1] = relay_off;} break;
          case 0x03: {strat.strat_relay3[0] = relay_on; strat.strat_relay3[1] = relay_off;} break;
          case 0x04: {strat.strat_relay4[0] = relay_on; strat.strat_relay4[1] = relay_off;} break;
        }   
        
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send status of handler to LoRa. If successful */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]);
      }
      else
      {   
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x08);
      }
    } break;
    
     /* Relay control */
    case 0x09:
    {
      if(0x03 == len)
      {
        uint8_t output_on_off = 0x00, which_relay = 0x00;
        output_pin_Variable num_relay;
        
        /* Choosing a relay */
        switch(buf[1])
        {
          case 0x01: num_relay = output_ok_1; which_relay = 0x01; break;
          case 0x02: num_relay = output_ok_2; which_relay = 0x02; break;
          case 0x03: num_relay = output_ok_3; which_relay = 0x03; break;
          case 0x04: num_relay = output_ok_4; which_relay = 0x04; break;
        }
        
        /* Choosing an action */
        if(buf[2]) output_on_off = OUTPUT_ON;
        else output_on_off = OUTPUT_OFF;
        
        /* An action */
        dmAPI_OutputSetState(&num_relay, output_on_off);
        
        /* Answer + Check sum */  
        uint8_t status_answer[] = {which_relay, output_on_off, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send the relay number and action to LoRa. If successful */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]);
      }
      else
      {
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x09);
      }
    } break;
    
    /* Time ratios array */
    case 0x0a:
    {
      if(0x01 == buf[2])
      {
        if(0x01 == buf[3])
          for(uint8_t i = 0; i < 36; i++)
            strat.arr_coef_on[i] = buf[i+4];
        if(0x02 == buf[3])
          for(uint8_t i = 36; i < 72; i++)
            strat.arr_coef_on[i] = buf[i-32];
      }
      if(0x00 == buf[2])
      {
        if(0x01 == buf[3])
          for(uint8_t i = 0; i < 36; i++)
            strat.arr_coef_off[i] = buf[i+4];
        if(0x02 == buf[3])
          for(uint8_t i = 36; i < 72; i++)
            strat.arr_coef_off[i] = buf[i+4];
      }
      
      /* Answer + Check sum */  
      uint8_t status_answer[] = {buf[1], buf[2], buf[3], 0x00, 0x00};
      wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
      
      /* Send error message to LoRa. If failure */
      dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]); 
    } break;
    
    /* Clear the EPPROM */
    case 0x0b:
    {
      if(0x01 == len)
      {
        uint32_t clr[5] = {0x00};
        
        /* Clear the EEPROM */
        dm_API_SaveToEEPROM(EEPR_SEND_M, clr, 1);
        dm_API_SaveToEEPROM(EPPR_SEND_D, clr, 1);
        dm_API_SaveToEEPROM(EEPR_ADDR_NET, clr, 0x05);
        dm_API_SaveToEEPROM(EEPROM_L_MONTH, clr, 0x05);
        dm_API_SaveToEEPROM(EEPROM_L_DAY, clr, 0x05);
        
        /* Answer + Check sum */  
        uint8_t status_answer[] = {0x01, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send the status of handler to LoRa. If successful */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, buf[0]); 
        
        /* Restart the programm */
        is74__OneTime();
      }
      else
      {
        /* Answer + Check summ */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x0b); 
      }
    } break;
    
    case 0x0c:
    {
      if(len == 0x01)
      {
        /* Answer + Check summ */  
        uint8_t mas[] = {dm_to_is74_main_var.LoRaTxTimerFlag, Common.delay, Common.delta_delay, Common.delta_t, 0x00, 0x00};        
        wrapper_crc(mas, sizeof(mas)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(mas, sizeof(mas)/sizeof(uint8_t), 0x00, 0x0c); 
      }
      else
      {
        /* Answer + Check summ */  
        uint8_t status_answer[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
        wrapper_crc(status_answer, sizeof(status_answer)/sizeof(uint8_t));
        
        /* Send error message to LoRa. If failure */
        dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x0b); 
      }
    } break;
  }
  dm_to_is74_main_var.LoRaRxCompleteFlag = false;
}
