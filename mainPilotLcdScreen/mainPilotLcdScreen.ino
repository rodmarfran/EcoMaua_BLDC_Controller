#include <stdio.h>
#include <stdlib.h>

#include "libEcoMcp2515CanCtrl.h"

CEcoMcp2515CanCtrl xEcoMcp2515CanCtrl(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, 10, 2);

CEcoMcp2515CanCtrl::TEcoCanTelemetryData xEcoCanTelemetryData;

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

  xEcoMcp2515CanCtrl.bReceiveCanMsgData(&xEcoCanTelemetryData);

  u64CurrentTimeMs = millis();
  if (u64CurrentTimeMs - u64PreviousTimeMs >= CU64_TIME_INTERVAL_MS) {
    u64PreviousTimeMs = u64CurrentTimeMs;

    Serial.print('>');
    Serial.print("T:");  Serial.print(xEcoCanTelemetryData.fThrottlePercent, 4); /* Throttle [%] */
    Serial.print("|M:"); Serial.print(xEcoCanTelemetryData.fMotorSpeedRpm  , 4); /* Motor Speed [RPM] */
    Serial.print("|S:"); Serial.print(xEcoCanTelemetryData.fVehicleSpeedkmH, 4); /* Vehicle Speed [km/h] */
    Serial.print("|D:"); Serial.print(xEcoCanTelemetryData.fDistanceM      , 4); /* Distance [m] */
    Serial.print("|V:"); Serial.print(xEcoCanTelemetryData.fVoltageV       , 4); /* Voltage [V] */
    Serial.print("|I:"); Serial.print(xEcoCanTelemetryData.fCurrentA       , 4); /* Current [A] */
    Serial.print("|P:"); Serial.print(xEcoCanTelemetryData.fPowerW         , 4); /* Power [W] */
    Serial.print("|E:"); Serial.print(xEcoCanTelemetryData.fEnergyJ        , 4); /* Energy [J] */
    Serial.print("|A:"); Serial.print(xEcoCanTelemetryData.fAutonomyKmKwH  , 4); /* Automony [km/kWh] */
    Serial.print('\n');

  }

}
