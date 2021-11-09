/*************************************************************************************************************************************
 *  is74_workflow.h - is a main include file for IS74 programming team                                                               *
 *  2019 Intersviyaz (*)> quack quack                                                                                                *
 *************************************************************************************************************************************/

#ifndef __IS74_WORKFLOW_H__
#define __IS74_WORKFLOW_H__



/*************************************************************************************************************************************
 *                                                         Includes                                                                  *
 *************************************************************************************************************************************/
#include "hw.h"
#include "bsp.h"
#include <time.h>
   

/*************************************************************************************************************************************
 *                                                          Defines                                                                  *
 *************************************************************************************************************************************/
#define SIZE_TXRX      256                                   // Response string size (Max request string of SPT-943.1 = 68 data bytes)
#define HOUR           3600000
#define MINUTE         60000
   
/* Unixtime constants */
#define UNIX_YEAR      31556926
#define UNIX_MONTH     2629743 
#define UNIX_DAY       86400
#define UNIX_HOUR      3600
#define UNIX_MINUTE    300
   
/* Delay between commands */
#define DEL_COM_ENR    50
#define DEL_COM_MER    10
   
/* Addresses in EEPROM  */
#define EEPR_DELAY     0x00
#define EEPR_SEND_M    0x04
#define EPPR_SEND_D    0x08
#define EEPR_COM_PORT  0x0C
#define EEPR_ADDR_NET  0x20
#define EEPROM_L_MONTH 0x40
#define EEPROM_L_DAY   0x60
#define EEPROM_SERIAL  0x80



/*************************************************************************************************************************************
 *                                                Is74_state_machine_definition                                                      *
 *************************************************************************************************************************************/
typedef enum __is74_mainstate {
   IS74_STATE_IDLE,
   IS74_STATE_WAIT_FOR_RESPONSE,
   IS74_STATE_WAIT_FOR_JOIN,
   IS74_STATE_RS485_CONNECT,
   IS74_STATE_NO_LORAWAN_NET,
   
}is74_mainstate;



/*************************************************************************************************************************************
 *                                                   Structres and variables                                                         *
 *************************************************************************************************************************************/
struct Connections {
  /* Target device */
  uint8_t whoami;                                           // Energymera = 1, mercury = 2.
  
  /* Address and flag of request */
  uint8_t netAddr;
  uint8_t desc_integral;
  
  /* Net addresses and addr counter */
  uint8_t net_addr_count;
  uint8_t netAddrDev[6];
  
  /* Request flag */
  bool request_04;
  bool request_05;
  
  /* Flags. Day and month 
     of last shipment */
  uint32_t l_day[5];
  uint32_t l_month[5];
  bool send_day;
  bool send_month;
  
  /* Others  var */
  uint8_t isOpen;                                    
  uint8_t index_com;                                        // Command counter
  uint8_t is_send;                                          // While sending data to LoRa server (is true)
  uint8_t complete;
  uint32_t delay;                                           // This variable is defined delay between requests to device
  uint32_t t;
  uint32_t delta_t;
  uint8_t error;
  uint32_t delta_delay;
  
  /* For relay */
  bool flag_relay_status;                                   // This variable is needed for the relay strategy
  uint8_t relay[4];                                         // Relay number
};

struct RelayStrats {
  /* Time of the on/off for different relays */
   uint32_t strat_relay1[2];
   uint32_t strat_relay2[2];
   uint32_t strat_relay3[2];
   uint32_t strat_relay4[2];
   
   /* Offset factor list. Time */
   int8_t arr_coef_on[72];
   int8_t arr_coef_off[72];
};

typedef enum __is74_whoami { 
   energy_mera = 1,
   mercury,    

}is74_whoami;

typedef struct Connections Connect;
typedef struct RelayStrats RelayStrat;



/*************************************************************************************************************************************
 *                                                     Uart configuration                                                            *
 *************************************************************************************************************************************/
typedef struct __is74_UART_Init_Variable {
  uint32_t BaudRate;                                     /*!< This member configures the UART communication baud rate. */
  uint32_t WordLength;                                   /*!< Specifies the number of data bits transmitted or received in a frame.
                                                              This parameter can be a value of @ref UARTEx_Word_Length. */
  uint32_t StopBits;                                     /*!< Specifies the number of stop bits transmitted.
                                                              This parameter can be a value of @ref UART_Stop_Bits. */
  uint32_t Parity;                                       /*!< Specifies the parity mode.*/

}is74_UART_Init_Variable;



/*************************************************************************************************************************************
 *                                           ENERGYMERA (102, 303)  | INTERFACE RS-485                                               *
 *************************************************************************************************************************************/
/********************************************* Connection structure with ENERGYMERA **************************************************/
 
struct ConnectionENR {
  /* Service information */
  char model[15];
  char serialNum[16];
  char currentDate[22];
  char currentTime[22];
  
  /* Stored energy. Month beginning Month */
  //char total[200];
  char storeEnrMonth[100];
  char storeEnrBegMonth[100];
  
  /* Stored energy. Day beginning Day */
  char storeEnrDay[250];
  char storeEnrLDay[250];
  
  /* Current data */
  char volta[51];
  char curre[51];
  char power[51];
  char cos[51];
  char freq[20];
};

typedef struct ConnectionENR ConnectionENR;


/***************************************************** Commands ENERGYMERA ***********************************************************/

/* Open/Close connection with Energymera */
static uint8_t OpenConnectEnr[]  = {0x2F, 0x3F, 0x21, 0x0D, 0x0A};
static uint8_t CloseConnectEnr[] = {0x01, 0x42, 0x30, 0x03, 0x75};

/* Service information */
static uint8_t SerialNumEnr[]    = {0x06, 0x30, 0x35, 0x31, 0x0D, 0x0A};
static uint8_t CurDateEnr[]      = {0x01, 0x52, 0x31, 0x02, 0x44, 0x41, 0x54, 0x45, 0x5F, 0x28, 0x29, 0x03, 0x56};
static uint8_t CurTimeEnr[]      = {0x01, 0x52, 0x31, 0x02, 0x54, 0x49, 0x4D, 0x45, 0x5F, 0x28, 0x29, 0x03, 0x67};

/* Integral data. (Month) */
static uint8_t StoreEnrMEnr[]    = {0x01, 0x52, 0x31, 0x02, 0x45, 0x41, 0x4D, 0x50, 0x45, 0x28, 0x00, 0x2E, 0x00, 0x00, 0x29, 0x03};
static uint8_t StoreEnrBegM[]    = {0x01, 0x52, 0x31, 0x02, 0x45, 0x4E, 0x4D, 0x50, 0x45, 0x28, 0x00, 0x2E, 0x00, 0x00, 0x29, 0x03};

/* Integral data. (Days) */
static uint8_t StoreEnrD[]       = {0x01, 0x52, 0x31, 0x02, 0x45, 0x41, 0x44, 0x50, 0x45, 0x28, 
                                          0x35, 0x2E, 0x36, 0x2E, 0x31, 0x39, 0x29, 0x03, 0x69};               
static uint8_t StoreEnrLD[]      = {0x01, 0x52, 0x31, 0x02, 0x45, 0x4E, 0x44, 0x50, 0x45, 0x28, 
                                    0x32, 0x35, 0x2E, 0x36, 0x2E, 0x31, 0x39, 0x29, 0x03, 0x28};

/* Current data*/
static uint8_t GetVoltaEnr[]     = {0x01, 0x52, 0x31, 0x02, 0x56, 0x4F, 0x4C, 0x54, 0x41, 0x28, 0x29, 0x03, 0x5F};
static uint8_t GetCurreEnr[]     = {0x01, 0x52, 0x31, 0x02, 0x43, 0x55, 0x52, 0x52, 0x45, 0x28, 0x29, 0x03, 0x5A};
static uint8_t GetPowerEnr[]     = {0x01, 0x52, 0x31, 0x02, 0x50, 0x4F, 0x57, 0x45, 0x50, 0x28, 0x29, 0x03, 0x64};
static uint8_t GetCosEnr[]       = {0x01, 0x52, 0x31, 0x02, 0x43, 0x4F, 0x53, 0x5F, 0x66, 0x28, 0x29, 0x03, 0x03};
static uint8_t GetFreqEnr[]      = {0x01, 0x52, 0x31, 0x02, 0x46, 0x52, 0x45, 0x51, 0x55, 0x28, 0x29, 0x03, 0x5C};

//static uint8_t GetTotalEnr[]     = {0x06, 0x30, 0x35, 0x36, 0x0D, 0x0A};




/*************************************************************************************************************************************
 *                                                 MERCURY  | INTERFACE RS-485                                                       *
 *************************************************************************************************************************************/
/********************************************** Connection structure with MERCURY ****************************************************/
 
struct ConnectionMER {
  /* Service information */
  uint8_t serviceInfo[4];
  uint8_t dateTime[8];
  
  /* Stored energy. Month beginning Month */
  uint8_t storeEnrMonth[96];
  uint8_t storeEnrBegMonth[80];
  uint8_t energySinceRes[96];
  
  /* Stored energy. Day beginning Day */
  uint8_t storeEnrDay[80];
  uint8_t storeEnrLDay[96];
  
  /* Current data */
  uint8_t currentDataMer[78];
  
  /* Losses */
  uint8_t lossesEnrM[16];
  uint8_t lossesEnrD[16];
  
  /* Profile of power */
  uint8_t config_pro_pow[9];
  uint8_t profile_pow[15];
};

typedef struct ConnectionMER ConnectionMER;



/******************************************************* Commands MERCURY ************************************************************/

/* Open/Close connection with Mercury*/
static uint8_t OpenConnectMer[]  = {0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
static uint8_t CloseConnectMer[] = {0x00, 0x02};

/* Service information */
static uint8_t CurrDateTimeMer[] = {0x00, 0x04, 0x00};
static uint8_t ServiceInfoMer[]  = {0x00, 0x08, 0x01};

/* Integral data. (Month) */
static uint8_t StoreEnrMerM[]    = {0x00, 0x05, 0x30, 0x06};                           // Stored energy for last month
static uint8_t StoreEnrMerBegM[] = {0x00, 0x06, 0x02, 0x02, 0xaa, 0x50};               // Stored energy for beginning current month

/* Integral data. (Days) */
static uint8_t StoreEnrMerBegD[] = {0x00, 0x06, 0x02, 0x06, 0xa6, 0x50};               // Stored energy for beginning current day
static uint8_t StoreEnrMerLD[]   = {0x00, 0x05, 0x50, 0x06};                           // Stored energy for last day

/* Losses */
static uint8_t LossesEnrM[]      = {0x00, 0x06, 0x02, 0x07, 0x72, 0x10};               // Month
static uint8_t LossesEnrD[]      = {0x00, 0x06, 0x02, 0x08, 0x3e, 0x10};               // Days

/* Current data */
static uint8_t CurrentData[]     = {0x00, 0x08, 0x16, 0xa0};
static uint8_t EnergySinceRes[]  = {0x00, 0x05, 0x00, 0x06};

/* Profile of power */
static uint8_t GetConfProOfPow[] = {0x00, 0x08, 0x13};
static uint8_t ProfileOfPower[]  = {0x00, 0x06, 0x03, 0x00, 0x00, 0x0F};




/*************************************************************************************************************************************
 *                                                      Function prototype                                                           *
 *************************************************************************************************************************************/

/* General functions */
void is74__UART_Init(void);
void is74__OneTime(void);
void setup_config(void);
void is74_PrepareTx(void);
void is74_mainloop(void);
uint16_t Crc16MudBus(uint8_t *, uint8_t);
uint16_t Crc8Calc(uint8_t *, uint8_t);
void clear_ans(void);
void is74_delay(uint8_t, uint8_t);
void wrapper_crc(uint8_t *, uint8_t);
void request_handler(uint8_t *, uint8_t);
void send_lora_request(void);
struct tm *epoche_to_date();
uint32_t date_to_epoch(uint8_t *, uint8_t);
void onoff_relay(uint32_t);
uint8_t is_empty_metrika();

/* functions EnergyMera */
void connect_energy_mera(void);
void to_string_req_enr(char *);
void request_enr(uint8_t *, uint8_t, uint16_t);
void run_energy_mera(void);
void send_lora_energy_mera(void);

/* functions Mercury */
void connect_mercury(void);
void init_mercury(void);
void init_time_date_mercury(void);
void request_mercury(uint8_t *, uint8_t, uint16_t);
void set_net_addr(uint8_t);
uint16_t  addr_return(uint8_t arg);
void init_propow_mercury(void);
void run_mercury(void);
void send_lora_mercury(void);

#endif // __IS74_WORKFLOW_H__
