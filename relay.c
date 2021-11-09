/*************************************************************************************************************************************
 *                                                    Realization functions RELAY                                                    *
 *************************************************************************************************************************************/

#include "is74_workflow.h"


/* For relay */
extern output_pin_Variable output_ok_1;
extern output_pin_Variable output_ok_2;
extern output_pin_Variable output_ok_3;
extern output_pin_Variable output_ok_4;

/* Common structures */
extern RelayStrat strat;
extern Connect Common;



/* The function to on/off relay */
void onoff_relay(uint32_t time) {
  bool flag = true;
  /* Run on the strategy of on and off */
  for(uint8_t i = 0; i < 2; i++) {
    if(time == strat.strat_relay1[i]) {
      /* Turn on / off relay 1 */
      dmAPI_OutputSetState(&output_ok_1, (i) ? OUTPUT_OFF : OUTPUT_ON); 
      static uint8_t count = 0;
      
      /* Shift to the next strategy time */
      strat.strat_relay1[i] += (UNIX_MINUTE + strat.arr_coef_on[count]*UNIX_MINUTE); 
      (count == 71) ? count = 0 : count++;
      
      /* Send status event, relay 1 */
      uint8_t status_answer[] = {0x01, !i};
      dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x01);
    }
    else if(time == strat.strat_relay2[i]) {
      /* Turn on / off relay 2 */
      dmAPI_OutputSetState(&output_ok_2, (i) ? OUTPUT_OFF : OUTPUT_ON);
      
      /* Shift to the next strategy time */
      strat.strat_relay2[i] += UNIX_MINUTE;
      
      /* Send status event, relay 2 */
      uint8_t status_answer[] = {0x02, !i};
      dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x01);
    }
    else if(time == strat.strat_relay3[i]) {
      /* Turn on / off relay 3 */
      dmAPI_OutputSetState(&output_ok_3, (i) ? OUTPUT_OFF : OUTPUT_ON);
      
      /* Shift to the next strategy time */
      strat.strat_relay3[i] += UNIX_MINUTE;
      
      /* Send status event, relay 3 */
      uint8_t status_answer[] = {0x03, !i};
      dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x01);
    }
    else if(time == strat.strat_relay4[i]) {
      /* Turn on / off relay 4 */
      dmAPI_OutputSetState(&output_ok_4, (i) ? OUTPUT_OFF : OUTPUT_ON); 
      
      /* Shift to the next strategy time */
      strat.strat_relay4[i] += UNIX_MINUTE;
      
      /* Send status event, relay 4 */
      uint8_t status_answer[] = {0x04, !i};
      dmAPI_LoRaWAN_Send(status_answer, sizeof(status_answer)/sizeof(uint8_t), 0x00, 0x01);
    }
    else 
      flag = false;
  }
  if(flag) {
    Common.flag_relay_status = true;
    HAL_Delay(10000);
  } 
}
