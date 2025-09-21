#include <stdio.h>
#include <stdlib.h>
#include <Wire.h>

#include "libEcoMcp2515CanCtrl.h"
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd_pilot(0x27, 20, 4);

CEcoMcp2515CanCtrl xEcoMcp2515CanCtrl(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, 10, 2);

CEcoMcp2515CanCtrl::TEcoCanTelemetryData xEcoCanTelemetryData;
CEcoMcp2515CanCtrl::TEcoCanTelemetryData xEcoCanTelemetryLastData;

uint64_t u64PreviousTmTimeMs = 0;
uint64_t u64PreviousBtnTimeMs = 0;
uint64_t u64CurrentTimeMs = 0;
uint64_t u64TempoTentativaMs = 0;
uint64_t u64BtnPressStartTimeMs = 0;

float fDistanciaM = 0.0;
float fVelocidadeMediaKmH = 0.0;
bool bBtnPressLong = false;
bool bBtnResetState = false;

const uint64_t CU64_TM_TIME_INTERVAL_MS = 1000;
const uint64_t CU64_BTN_TIME_INTERVAL_MS = 10;

int iBotaoVoltasPin = A3;
int cntVoltas = 0;
int cntVoltasAnt = 0;
int iEstadoBotaoVoltas = HIGH;
int iEstadoBotaoVoltasAnt = HIGH;

bool bAtualizaLcd = true;

uint8_t u8TempoTentativaS = 0;
uint8_t u8TempoTentativaM = 0;
uint8_t u8TempoTentativaH = 0;
uint8_t u8TempoTentativaSAnt = 0;

float fDistanceOffsetM = 0.0;

const char* padToTwoDigits(uint8_t value) {
  static char buffer[3];  // 2 digits + null terminator

  // Cap the value at 99
  if (value > 99) {
    value = 99;
  }

  // Format the value to 2 digits, ensuring it's padded with a leading zero if necessary
  snprintf(buffer, sizeof(buffer), "%02u", value);

  return buffer;
}

void setup() {
  Serial.begin(9600);
  pinMode(iBotaoVoltasPin, INPUT_PULLUP);

  xEcoMcp2515CanCtrl.begin(CAN_20KBPS);

  lcd_pilot.init();
  lcd_pilot.setCursor(0, 0);
  lcd_pilot.print("V:       / VM:       ");
  lcd_pilot.setCursor(0, 1);
  lcd_pilot.print("D:         / L:   ");
  lcd_pilot.setCursor(0, 2);
  lcd_pilot.print("T: 00:00:00 ");

  lcd_pilot.setBacklight(120);
}

void loop() {
  xEcoMcp2515CanCtrl.bReceiveCanMsgData(&xEcoCanTelemetryData);

  u64CurrentTimeMs = millis();

  // Atualiza o tempo da tentativa e a distância percorrida
  u64TempoTentativaMs += (u64CurrentTimeMs - u64PreviousTmTimeMs);
  u64PreviousTmTimeMs = u64CurrentTimeMs;
  fDistanciaM = xEcoCanTelemetryData.fDistanceM - fDistanceOffsetM;

  // Calcula a velocidade média
  fVelocidadeMediaKmH = (fDistanciaM / 1000.0) / (u64TempoTentativaMs / 3600000.0);

  if (bBtnResetState) {
    u64TempoTentativaMs = 0;
    fDistanceOffsetM = xEcoCanTelemetryData.fDistanceM;
    fDistanciaM = 0.0;
    fVelocidadeMediaKmH = 0.0;
    cntVoltas = 0;
  }

  u8TempoTentativaH = (uint8_t)(u64TempoTentativaMs / 3600000);
  u8TempoTentativaM = (uint8_t)((u64TempoTentativaMs / 60000) % 60);
  u8TempoTentativaS = (uint8_t)((u64TempoTentativaMs / 1000) % 60);

  if (u64CurrentTimeMs - u64PreviousBtnTimeMs >= CU64_BTN_TIME_INTERVAL_MS) {
    u64PreviousBtnTimeMs = u64CurrentTimeMs;

    iEstadoBotaoVoltas = digitalRead(iBotaoVoltasPin);
    if ((iEstadoBotaoVoltas == HIGH) && (iEstadoBotaoVoltasAnt == LOW)) {
      bBtnResetState = false;
    } else if ((iEstadoBotaoVoltas == LOW) && (iEstadoBotaoVoltasAnt == HIGH)) {
      cntVoltas++;
      u64BtnPressStartTimeMs = u64CurrentTimeMs;
      bBtnPressLong = false;
    } else if (iEstadoBotaoVoltas == LOW && !bBtnPressLong && (u64CurrentTimeMs - u64BtnPressStartTimeMs >= 5000)) {
      // Botão pressionado por mais de 10 segundos, reinicia os parâmetros
      u64TempoTentativaMs = 0;
      fDistanceOffsetM = xEcoCanTelemetryData.fDistanceM;
      fDistanciaM = 0.0;
      fVelocidadeMediaKmH = 0.0;
      cntVoltas = 0;
      bBtnPressLong = true;
      bBtnResetState = true;

      lcd_pilot.setCursor(0, 0);
      lcd_pilot.print("V:       / VM:       ");
      lcd_pilot.setCursor(0, 1);
      lcd_pilot.print("D:         / L:   ");
      lcd_pilot.setCursor(0, 2);
      lcd_pilot.print("T: 00:00:00 ");
    }
    iEstadoBotaoVoltasAnt = iEstadoBotaoVoltas;
  }

  /* Verifica se os dados do LCD foram atualizados (aumenta a velocidade de atualização) */
  bAtualizaLcd = ((xEcoCanTelemetryData.fVehicleSpeedkmH != xEcoCanTelemetryLastData.fVehicleSpeedkmH)
                  || (xEcoCanTelemetryData.fDistanceM != xEcoCanTelemetryLastData.fDistanceM)
                  || (cntVoltas != cntVoltasAnt)
                  || (u8TempoTentativaS != u8TempoTentativaSAnt));


  // Atualiza o display imediatamente se houver mudanças nos valores
  if (bAtualizaLcd) {

    lcd_pilot.setCursor(3, 0);
    lcd_pilot.print(xEcoCanTelemetryData.fVehicleSpeedkmH, 2); /* Velocidade instantanea [km/h] */
    lcd_pilot.setCursor(15, 0);
    lcd_pilot.print(fVelocidadeMediaKmH, 2); /* Velocidade média [km/h] */

    lcd_pilot.setCursor(3, 1);
    lcd_pilot.print(fDistanciaM, 2); /* Distância [m] */
    lcd_pilot.setCursor(16, 1);
    lcd_pilot.print(cntVoltas); /* Quantidade de voltas */

    lcd_pilot.setCursor(3, 2);
    lcd_pilot.print(padToTwoDigits(u8TempoTentativaH));  // Horas
    lcd_pilot.print(":");
    lcd_pilot.print(padToTwoDigits(u8TempoTentativaM));  // Minutos
    lcd_pilot.print(":");
    lcd_pilot.print(padToTwoDigits(u8TempoTentativaS));  // Segundos
  }

  // Armazena os últimos valores para comparação na próxima iteração
  xEcoCanTelemetryLastData = xEcoCanTelemetryData;
  cntVoltasAnt = cntVoltas;
  u8TempoTentativaSAnt = u8TempoTentativaS;
}
