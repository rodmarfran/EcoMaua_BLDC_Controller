#include <stdio.h>
#include <stdlib.h>

#include "libEcoMcp2515CanCtrl.h"

CEcoMcp2515CanCtrl xEcoMcp2515CanCtrl(CEcoMcp2515CanCtrl::E_MOTOR_CONTROLLER_DEVID, 53, 2);

uint64_t u64PreviousTimeMs = 0;
uint64_t u64CurrentTimeMs = 0;

const uint64_t CU64_TIME_INTERVAL_MS = 1000;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  xEcoMcp2515CanCtrl.begin(CAN_500KBPS);

}

void loop() {
  // put your main code here, to run repeatedly:

  u64CurrentTimeMs = millis();
  if (u64CurrentTimeMs - u64PreviousTimeMs >= CU64_TIME_INTERVAL_MS) {
    u64PreviousTimeMs = u64CurrentTimeMs;

    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_THROTTLE_PERCENT_ID , 0.1f); /* Throttle [%] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_MOTOR_SPEED_RPM_ID  , 0.2f); /* Motor Speed [RPM] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_VEHICLE_SPEED_KMH_ID, 0.3f); /* Vehicle Speed [km/h] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_DISTANCE_M_ID       , 0.4f); /* Distance [m] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_VOLTAGE_V_ID        , 0.5f); /* Voltage [V] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_CURRENT_A_ID        , 0.6f); /* Current [A] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_POWER_W_ID          , 0.7f); /* Power [W] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_ENERGY_J_ID         , 0.8f); /* Energy [J] */
    delay(2);
    xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_AUTONOMY_KMKWH_ID   , 0.9f); /* Automony [km/kWh] */

  }

}
