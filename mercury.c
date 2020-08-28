/*************************************************************************************************************************************
 *                                                    Realization functions MERCURY
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
extern Connect Common;
                                                                  
/* Device information structure */  
extern ConnectionMER  MerCon;                                                                        // Mercury

/* Clipboards and other variables */
extern uint8_t UART2_TX_buff[SIZE_TXRX];                                                             // Buffer to send the commands to UART
extern uint8_t UART2_RX_buff[SIZE_TXRX];                                                             // Buffer to recive the response from UART
extern uint8_t tmp[268];                                                                             // Temp buffer  



/* The function of connection request */
void connect_mercury(void)
{
  static uint8_t count = 0;
  set_net_addr((Common.request_04 || Common.request_05) 
                ? Common.netAddr : Common.netAddrDev[Common.net_addr_count]);
  
  /* Open connetion with Mercury */
  request_mercury(OpenConnectMer, sizeof(OpenConnectMer)/sizeof(uint8_t), 4);               
  
  while(true)
  { 
    if(dm_to_is74_main_var.RxComplete)
    { 
      for(uint8_t i = 0; i < 4; i++)
        if(UART2_RX_buff[i] != 0x00)
        {
          Common.isOpen = 0x01;
          count = 0;
        }
      
      /* If the port (RS-485) is busy */
      if(UART2_RX_buff[0] == 0x50 && UART2_RX_buff[1] == 0x6f &&
         UART2_RX_buff[2] == 0x72 && UART2_RX_buff[3] == 0x74   )
        Common.isOpen = 0x00;
      
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
      
      clear_ans();                                                                          // Interval inits and answer line deductio
      Common.complete = 0;
      break;
    }       
  }
}

/* Initialization service information Mercury */
void init_mercury(void)
{
  uint32_t tmp_serial[4] = {0x00};
  
  for(uint8_t i = 0; i < sizeof(MerCon.serviceInfo)/sizeof(uint8_t); i++)
  {
    MerCon.serviceInfo[i] = UART2_RX_buff[i+1];
    tmp_serial[i] = UART2_RX_buff[i+1];
  }
  
  /* Save serial number to eeprom */
  dm_API_SaveToEEPROM(EEPROM_SERIAL, tmp_serial, 0x04);
}

/* The function reads current date/time and saves them */
void init_time_date_mercury(void)
{
  for(uint8_t i = 0; i < sizeof(MerCon.dateTime)/sizeof(uint8_t); i++)                      // Consists of Season (01 = Winter, 00 = Summer),
    MerCon.dateTime[i] = UART2_RX_buff[i+1];                                                // Year, Month, Day of the week, Day, Hour,
  
  /* Read serial num from eeprom */
  uint32_t tmp_serial[4] = {0x00};
  dm_API_ReastoreFromEEPROM(EEPROM_SERIAL, tmp_serial, 0x04);
  
  bool other_dev = false;
  
  for(uint8_t i = 0; i < sizeof(MerCon.serviceInfo)/sizeof(uint8_t); i++)
  {
    if(tmp_serial[i] != MerCon.serviceInfo[i])
      other_dev = true;
  }
  
  /* If true send data for the month */
  dm_API_ReastoreFromEEPROM(EEPROM_L_MONTH, Common.l_month, 0x05);
  
  if((Common.l_month[Common.net_addr_count-1] != MerCon.dateTime[5] 
      && 0x00 != MerCon.dateTime[5]) || other_dev)
  {
    Common.send_month = true;
    Common.l_month[Common.net_addr_count-1] = MerCon.dateTime[5];
    dm_API_SaveToEEPROM(EEPROM_L_MONTH, Common.l_month, 0x05);
  }
  
  /* If true send data for the day */
  dm_API_ReastoreFromEEPROM(EEPROM_L_DAY, Common.l_day, 0x05);
  
  if((Common.l_day[Common.net_addr_count-1] != MerCon.dateTime[4] 
      && 0x00 != MerCon.dateTime[4]) || other_dev)
  {
    Common.send_day = true;
    Common.l_day[Common.net_addr_count-1] = MerCon.dateTime[4];  
    dm_API_SaveToEEPROM(EEPROM_L_DAY, Common.l_day, 0x05);
  }
}

/* The function forms and send the message for UART */
void request_mercury(uint8_t *req, uint8_t len, uint16_t size_rec)                          // Forms response command
{ 
  /* Bytes of data */
  for(int i = 0; i < len; i++)                                                             
    UART2_TX_buff[i] = req[i];
  
  /* Checksum bytes */
  uint16_t crc = Crc16MudBus(req, len);                                                     
  UART2_TX_buff[len] = crc & 0xFF;
  UART2_TX_buff[len+1] = (crc >> 8) & 0xFF;  
  
  /* Send command   */
  dmAPI_UART_Transaction(UART2_TX_buff, (len+2), UART2_RX_buff, size_rec, 1000);            
}

/* The function to change commands. (Select addr Mercury ) */
void set_net_addr(uint8_t addr)
{
    /* Service commands */
    OpenConnectMer[0] = addr;
    CloseConnectMer[0] = addr;
    CurrDateTimeMer[0] = addr;
    ServiceInfoMer[0] = addr;
    
    /* Integral data. (Month) */
    StoreEnrMerM[0] = addr;
    StoreEnrMerBegM[0] = addr;
    
    /* Integral data. (Day) */
    StoreEnrMerLD[0] = addr;
    StoreEnrMerBegD[0] = addr;
    
    /* Losses */
    LossesEnrM[0] = addr;
    LossesEnrD[0] = addr;
    
    /* Current data*/
    CurrentData[0] = addr;
    EnergySinceRes[0] = addr;
    
    /* Profile of power */
    ProfileOfPower[0] = addr;
    GetConfProOfPow[0] = addr;
}

/* The function return address for stored energy */
uint16_t  addr_return(uint8_t arg)
{
  uint16_t res[] = {0, 682, 767, 852, 937, 1022, 1107, 
                    1192, 1277, 1362, 1447, 1532, 1617};
  return res[arg];
}

/* The function init profile of power */
void init_propow_mercury(void)
{
  /* Save the config module of power */
  for(uint8_t i = 0; i < sizeof(MerCon.config_pro_pow)/sizeof(uint8_t); i++)
    MerCon.config_pro_pow[i] = UART2_RX_buff[i+1];
  
  /* Make the request */
  ProfileOfPower[3] = MerCon.config_pro_pow[0];
  ProfileOfPower[4] = MerCon.config_pro_pow[1];
}

/* The main function Mercury */
void run_mercury(void)
{
  /* If connection is open continue request */
  if(Common.isOpen && Common.netAddrDev[0] != 0x00)
  {   
    /********************************** SERVICE DATA ************************************/
    if(Common.complete && Common.index_com == 1)
    {
      /* Read service information */
      request_mercury(ServiceInfoMer, sizeof(ServiceInfoMer)/sizeof(uint8_t), 20);  
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 2)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      init_mercury();
      clear_ans();                                                                          // Interval inits and answer line deductio
      request_mercury(CurrDateTimeMer, sizeof(CurrDateTimeMer)/sizeof(uint8_t), 12);        // Read current time
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 3)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      init_time_date_mercury();
      clear_ans();                                                                          // Interval inits and answer line deductio
      
      /* Select the desired month  */
      if(MerCon.dateTime[5] == 0x01)
        StoreEnrMerM[2] = 0x0c;
      else
        StoreEnrMerM[2] += (MerCon.dateTime[5] - 0x01);   
      
      /* Read stored energy (sum) */
      request_mercury(StoreEnrMerM, sizeof(StoreEnrMerM)/sizeof(uint8_t), 100);              
      StoreEnrMerM[2] = 0x30;
      Common.complete = 0;
    }
    
    /************************* SORED ENERGY (INTEGRAL DATA) *****************************/
    if(Common.complete && Common.index_com == 4)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.storeEnrMonth)/sizeof(uint8_t); i++)
        MerCon.storeEnrMonth[i] = UART2_RX_buff[i+1];
      clear_ans();                                                                          
      
      /* Get the address for the desired month */
      uint16_t addr = addr_return(MerCon.dateTime[5]);
      StoreEnrMerBegM[3] = addr >> 8;
      StoreEnrMerBegM[4] = addr;
      
      request_mercury(StoreEnrMerBegM, sizeof(StoreEnrMerBegM)/sizeof(uint8_t), 85);        // Read stored energy (For beginn-g Month)
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 5)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.storeEnrBegMonth)/sizeof(uint8_t); i++)
        MerCon.storeEnrBegMonth[i] = UART2_RX_buff[i+1];
      clear_ans();                                                                          // Interval inits and answer line deductio
      request_mercury(StoreEnrMerBegD, sizeof(StoreEnrMerBegD)/sizeof(uint8_t), 85);        // Read stored energy (For beginn-g current day)
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 6)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.storeEnrDay)/sizeof(uint8_t); i++)
        MerCon.storeEnrDay[i] = UART2_RX_buff[i+1];
      clear_ans();                                                                          // Interval inits and answer line deductio
      request_mercury(LossesEnrM, sizeof(LossesEnrM )/sizeof(uint8_t), 20);                 // Read losses for month
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 7)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.lossesEnrM)/sizeof(uint8_t); i++)
        MerCon.lossesEnrM[i] = UART2_RX_buff[i+1];
      clear_ans();  
      request_mercury(StoreEnrMerLD, sizeof(StoreEnrMerLD)/sizeof(uint8_t), 100);            // Read stored energy (For last day)
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 8)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.storeEnrLDay)/sizeof(uint8_t); i++)
        MerCon.storeEnrLDay[i] = UART2_RX_buff[i+1];
      clear_ans();                                                                           // Interval inits and answer line deductio
      request_mercury(LossesEnrD, sizeof(LossesEnrD)/sizeof(uint8_t), 20);                       
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 9)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.lossesEnrD)/sizeof(uint8_t); i++)
        MerCon.lossesEnrD[i] = UART2_RX_buff[i+1];
      clear_ans();                                                                           // Interval inits and answer line deductio
      request_mercury(EnergySinceRes, sizeof(EnergySinceRes)/sizeof(uint8_t), 100);                        
      Common.complete = 0;
    }
    
    /******************************** CURRENT DATA *************************************/
    if(Common.complete && Common.index_com == 10)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.energySinceRes)/sizeof(uint8_t); i++)
        MerCon.energySinceRes[i] = UART2_RX_buff[i+1];
      
      clear_ans();  
      request_mercury(CurrentData, sizeof(CurrentData)/sizeof(uint8_t), 85);
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 11)
    {
      /* Is empty metrika */
      Common.error = (Common.error) ? Common.error : is_empty_metrika();
      
      for(uint8_t i = 0; i < sizeof(MerCon.currentDataMer)/sizeof(uint8_t); i++)
        MerCon.currentDataMer[i] = UART2_RX_buff[i+1];
      clear_ans();                                                                          
      
      /* Read config profile of power */
      request_mercury(GetConfProOfPow, sizeof(GetConfProOfPow)/sizeof(uint8_t), 20);
      Common.complete = 0;
    }
    
    /******************************* PROFILE OF POWER ***********************************/
    if(Common.complete && Common.index_com == 12)
    {
       /* Init the profile of power */
       init_propow_mercury(); 
       clear_ans();     
          
       /* Read the profile of power */
       request_mercury(ProfileOfPower, sizeof(ProfileOfPower)/sizeof(uint8_t), 20);          
       Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 13)
    {
      for(uint8_t i = 0; i < 15; i++)
        MerCon.profile_pow[i] = UART2_RX_buff[i+1];
          
      clear_ans();
          
      ProfileOfPower[3] = 0x00;
      ProfileOfPower[4] = 0x00;                      
          
      /* Close connection with mercury */  
      request_mercury(CloseConnectMer, sizeof(CloseConnectMer)/sizeof(uint8_t), 5);               
      Common.complete = 0;
    }
    if(Common.complete && Common.index_com == 14)
    {
      clear_ans();                                                                          
      dm_to_is74_main_var.RxComplete = true;
      Common.is_send = 1;
      Common.index_com = 0;
      Common.isOpen = 0;
    }
  }
  else
  {
    connect_mercury();
    Common.index_com = 0;
  } 
}

/* The function send data to lora server */
void send_lora_mercury(void)
{
  if(Common.error)
  {
    uint8_t send_err[] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00};
    
    /* Error reading metrics */
    if(Common.error == 0x02)
         send_err[1] = 0x02;
    
    /* Check summ and send data */ 
    wrapper_crc(send_err, sizeof(send_err)/sizeof(uint8_t));
    dmAPI_LoRaWAN_Send(send_err, sizeof(send_err)/sizeof(uint8_t), (Common.request_05) ? 
                       Common.netAddr : Common.netAddrDev[Common.net_addr_count], Common.desc_integral);                                                             
    
    /* Reset session variables */
    dm_to_is74_main_var.RxComplete = true;
    Common.request_04 = false;
    Common.request_05 = false;
    Common.index_com = 0;
    
    if(Common.netAddrDev[0] <= Common.net_addr_count && !Common.desc_integral)
    {
      /* Calculate the increment of time from the beginning 
                     of the request to the end of sending */
      Common.delta_t = (dm_to_is74_main_var.UnixTime - Common.t);
      Common.delta_delay = Common.delay - Common.delta_t * 1000;
      dm_API_SetLoRaTxTimer(Common.delta_delay);
      
      Common.t = 0x00;
      Common.net_addr_count = 0x01;
    }
    else if(!Common.desc_integral)
      Common.net_addr_count++;
    
    /* Reset session variables */
    Common.netAddr = 0x00;
    Common.desc_integral = 0x00;
    Common.is_send = 0;
    Common.error = 0;
  }
  else
  {
    switch(Common.index_com)
    {
      /* Relay event response */
      case 1:
      { 
        if(Common.flag_relay_status)
        {
          /* Form a message to send */
          for(uint8_t i = 0; i < 21; i++)
          {
            /* Frequence */
             if(i < 3) tmp[i] = MerCon.currentDataMer[i+76];
             
             /* Voltage */
             if(i >= 3 && i < 12) tmp[i] = MerCon.currentDataMer[i+37]; 
             
             /* Amperage */
             if(i >= 12 && i < 21) tmp[i] = MerCon.currentDataMer[i+55];
          }
          
          /* Check summ and send data  */
          wrapper_crc(tmp, 23);         
          dmAPI_LoRaWAN_Send(tmp, 23, 0x00, 0x01);
          
          Common.flag_relay_status = false;
          Common.t = 0x00;
        }
      } break;
      
      /* Response on request 04 */
      case 2:
      { 
        if(Common.request_04)
        {
           struct tm *date = epoche_to_date();
          
           /* Relay status */
           tmp[0] = output_ok_1.PinFlag;
           tmp[1] = output_ok_2.PinFlag; 
           tmp[2] = output_ok_3.PinFlag;
           tmp[3] = output_ok_4.PinFlag; 
           
           /* Delay between requests */
           tmp[4] = Common.delay/HOUR; 
           
           /* Current date and time. LoRa-module */
           tmp[5] = date->tm_hour;
           tmp[6] = date->tm_min; 
           tmp[7] = date->tm_sec;
           
           /* Form a message to send */
           for(uint8_t i = 0; i < 21; i++)
           {
             /* Frequence */
             if(i < 3) tmp[i+8] = MerCon.currentDataMer[i+76];
             
             /* Voltage */
             if(i >= 3 && i < 12) tmp[i+8] = MerCon.currentDataMer[i+37]; 
             
             /* Amperage */
             if(i >= 12 && i < 21) tmp[i+8] = MerCon.currentDataMer[i+55];
           }
           
           /* Check summ and send data */
           wrapper_crc(tmp, 31);   
           dmAPI_LoRaWAN_Send(tmp, 31, Common.netAddr, 0x04);
                      
           /* Reset session variables */
           Common.request_04 = false;
           Common.netAddr = 0x00;
           Common.is_send = 0;  
           Common.t = 0x00;
        }
      } break;
      
      /* Send integral data (month) */
      case 3:
      { 
        if(Common.send_month || Common.desc_integral == 0x81)
        {      
          /* Form a message to send */
          for(uint8_t i = 0; i < 204; i++)
          {
            /* Serivce information */
            if(i < 4) tmp[i] = MerCon.serviceInfo[i];
            
            /* Current date and time */
            if(i >= 4 && i < 12) tmp[i] = MerCon.dateTime[i-4];
            
            /* Stored energy for sum and 1-4 tariffs. For month */
            if(i >= 12 && i < 108) tmp[i] = MerCon.storeEnrMonth[i-12];
            if(i >= 108 && i < 188) tmp[i] = MerCon.storeEnrBegMonth[i-108];
            if(i >= 188 && i < 204) tmp[i] = MerCon.lossesEnrM[i-188];
          }
          
          /* Check summ and send data */
          wrapper_crc(tmp, 206); 
          dmAPI_LoRaWAN_Send(tmp, 206, (Common.request_05) ? Common.netAddr : 
                                        Common.netAddrDev[Common.net_addr_count], 0x81);                                                               
          
          /* Save send_month to EEPROM */
          uint32_t tmp_month[] = {Common.send_month = false};
          dm_API_SaveToEEPROM(EEPR_SEND_M, tmp_month, 0x01);
                
          if(Common.desc_integral == 0x81)
          {
            /* Reset session variables */
            Common.request_05 = false;          
            Common.netAddr = 0x00;
            Common.desc_integral = 0x00;
            Common.is_send = 0;
            Common.t = 0x00;
          }
        }
      } break;
      
      /* Send integral data (day) */
      case 4:
      {
        if(Common.send_day || Common.desc_integral == 0x82)
        {  
          /* Form a message to send */
          for(uint8_t i = 0; i < 204; i++)
          {
            /* Serivce information */
            if(i < 4) tmp[i] = MerCon.serviceInfo[i];
            
            /* Current date and time */
            if(i >= 4 && i < 12) tmp[i] = MerCon.dateTime[i-4];
            
            /* Stored energy for sum and 1-4 tariffs. For day */
            if(i >= 12 && i < 92) tmp[i] = MerCon.storeEnrDay[i-12];
            if(i >= 92 && i < 108) tmp[i] = MerCon.lossesEnrD[i-92];
            if(i >= 108 && i < 204) tmp[i] = MerCon.storeEnrLDay[i-108];
          }
          
          /* Check summ and send data */
          wrapper_crc(tmp, 206); 
          dmAPI_LoRaWAN_Send(tmp, 206, (Common.request_05) ? Common.netAddr : 
                                        Common.netAddrDev[Common.net_addr_count], 0x82);    
          
          /* Save send_day to EEPROM */
          uint32_t tmp_day[] = {Common.send_day = false};
          dm_API_SaveToEEPROM(EPPR_SEND_D, tmp_day, 0x01);
                  
          if(Common.desc_integral == 0x82)
          {
            /* Reset session variables */
            Common.request_05 = false;
            Common.netAddr = 0x00;
            Common.desc_integral = 0x00;
            Common.is_send = 0;
            Common.t = 0x00;
          }
        }
      } break;
      
      /* Sessions response. Send current data */
      case 5:
      {
        if(dm_to_is74_main_var.LoRaTxTimerFlag || Common.desc_integral == 0x84)
        {        
          /* Form a message to send */
          for(uint16_t i = 0; i < 186; i++)
          {
            /* Serivce information */
            if(i < 4) tmp[i] = MerCon.serviceInfo[i];
            
            /* Current date and time */
            if(i >= 4 && i < 12) tmp[i] = MerCon.dateTime[i-4];
            
            /* Current data */
            if(i >= 12 && i < 90) tmp[i] = MerCon.currentDataMer[i-12];
              
            /* Power since Reset */
            if(i >= 90 && i < 186) tmp[i] = MerCon.energySinceRes[i-90];
            
            //if(i >= 186 && i < 201) tmp[i] = MerCon.profile_pow[i-186];
          }
          
          /* Check summ and send data */
          wrapper_crc(tmp, 188); 
          dmAPI_LoRaWAN_Send(tmp, 188, (Common.request_05) ? Common.netAddr : 
                             Common.netAddrDev[Common.net_addr_count], 0x84);                                                             
          
          dm_to_is74_main_var.RxComplete = true;
          Common.request_05 = false;
          Common.index_com = 0; 
          
          /* Set timer to delay */
          if(Common.netAddrDev[0] <= Common.net_addr_count && !Common.desc_integral)
          {
            /* Calculate the increment of time from the beginning 
                             of the request to the end of sending */
            Common.delta_t = (dm_to_is74_main_var.UnixTime - Common.t);
            Common.delta_delay = Common.delay - Common.delta_t * 1000;
            dm_API_SetLoRaTxTimer(Common.delta_delay);
            
            PRINTF("DELAY: %d \n\r", Common.delay); 
            PRINTF("DELTA_DELAY: %d \n\r", Common.delta_delay); 
            PRINTF("DELTA_T (mill): %d \n\r", Common.delta_t*1000); 
            PRINTF("DELTA_T: %d \n\r", Common.delta_t);

            Common.net_addr_count = 0x01;
          }
          else if(!Common.desc_integral)
            Common.net_addr_count++;
          
          /* Reset session variables */   
          Common.netAddr = 0x00;
          Common.desc_integral = 0x00;
          Common.is_send = 0;
          Common.t = 0;
        }
      } break;
    }
  }
}