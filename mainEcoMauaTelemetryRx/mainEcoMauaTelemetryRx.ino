/****************************************************************************************************
 * Programa base para recebimento da telemetria do veiculo Caixão (Carbonasso) do EcoMauá
 ****************************************************************************************************/

/****************************************************************************************************
 * Includes
 ****************************************************************************************************/

/* Geral */
#include <stdio.h>
#include <stdlib.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* Tasks */
//#include <FreeRTOS.h>

/* LoRa E32-433T30D */
#include "EBYTE.h"

/* CAN MCP2515 */
#include "libEcoMcp2515CanCtrl.h"

/* RTC DS3231 */
#include "RTClib.h"

/****************************************************************************************************
 * Defines
 ****************************************************************************************************/

/* Geral */
#define DEBUG_ON     1
#define LORA_ON      1
#define BLUETOOTH_ON 1

#define SERIAL_DEBUG      Serial
#define SERIAL_DEBUG_BAUD 9600

#define SERIAL_LORA      Serial2
#define SERIAL_LORA_BAUD 9600

#define SERIAL_BT      xSerialBt
#define SERIAL_BT_NAME "EcoRxTelemetryBT"

/* LCD 20x4 I2C Display */
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_I2C_ADDR 0x27  // Confirm correct address

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

/* Telemetry Message */
#define TM_ENCRIPTATION_KEY 0xC1
#define TM_START_OF_PACKET  '<'
#define TM_END_OF_PACKET    '>'
#define TM_BUFFER_SIZE      256

/* Tasks */
#define TM_TASK_STACKSIZE 8192
#define TM_TASK_PRIORITY  1
#define TM_TASK_CORE      1
#define TM_TASK_SLEEP_MS  10

#define ADC_TASK_STACKSIZE 8192
#define ADC_TASK_PRIORITY  2
#define ADC_TASK_CORE      1
#define ADC_TASK_SLEEP_MS  100

/* LoRa E32-433T30D */
#define LORA_E32_M0_PIN  32
#define LORA_E32_M1_PIN  33
#define LORA_E32_AUX_PIN 27

#define LORA_E32_ADDRESS 1
#define LORA_E32_CHANNEL 23

/* CAN MCP2515 */

/* RTC DS3231 */

/* ADC ESP32 */
#define AIN_PIN                34           // GPIO34 (input-only)
#define ADC_SAMPLES            10           // média simples

/****************************************************************************************************
 * Variaveis, Constantes e Objetos Globais
 ****************************************************************************************************/

/* Geral */
BluetoothSerial xSerialBt;
const char strDebugHeader[] = ">Datetime [YYYY/MM/DD hh:mm:ss] | Throttle [%] | Motor Speed [RPM] | Vehicle Speed [km/h] | Distance [m] | Voltage [V] | Current [A] | Power [W] | Energy [J] | Automony [km/kWh]\n\r";
char strWorkBuffer[64];
bool bValidTmData = false;
bool bWorkValidTmData = false;
uint8_t u8RxTmDataBuffer[TM_BUFFER_SIZE];
uint8_t u8RxTmBufferIndex = 0;
bool bTmPacketStarted = false;
typedef struct TmData {
  DateTime xDatetime;
  CEcoMcp2515CanCtrl::TEcoCanTelemetryData xEcoCanTelemetryData;
} TTmData;
TTmData xTmData;
TTmData xWorkTmData;
// Previously displayed values
TTmData lastDisplayedData;
bool isFirstLcdUpdate = true;

/* Tasks */
TaskHandle_t xTaskTxTmHandle;
TaskHandle_t xTaskBattAdcHandle;
SemaphoreHandle_t xTmDataMutex;
SemaphoreHandle_t xLcdMutex;

/* LoRa E32-433T30D */
//EBYTE xLoraRadio(&SERIAL_LORA, LORA_E32_M0_PIN, LORA_E32_M1_PIN, LORA_E32_AUX_PIN);

/* CAN MCP2515 */

/* RTC DS3231 */

/* ADC ESP32 */

/****************************************************************************************************
 * Prototipos de Funções Auxiliares
 ****************************************************************************************************/

/* Geral */
void inline printSerial(const char *strData);
void inline printSerial(const char ccData);
bool isDifferent(float a, float b, float threshold);
void lcdPrintMask();
void lcdUpdateTelemetry(TTmData *data);
bool bParseTmMessage(uint8_t *pu8Buffer, uint8_t u8BufferSize, TTmData *pxTelemetryData, const uint8_t cu8EncriptionKey = 0);

/* Tasks */
void rxTmTask(void *pParameter);
void battAdcTask(void *pParameter);

/* LoRa E32-433T30D */

/* RTC DS3231 */

/* ADC ESP32 */

/****************************************************************************************************
 * Setup Inicial
 ****************************************************************************************************/

void setup() {

/* Set LoRa Mode Pins as Outputs and High */
pinMode(LORA_E32_M0_PIN, OUTPUT);
pinMode(LORA_E32_M1_PIN, OUTPUT);

digitalWrite(LORA_E32_M0_PIN, HIGH);
digitalWrite(LORA_E32_M1_PIN, HIGH);

/* Geral */
#if DEBUG_ON
  SERIAL_DEBUG.begin(SERIAL_DEBUG_BAUD);
#endif
#if BLUETOOTH_ON
  SERIAL_BT.begin(SERIAL_BT_NAME);
#endif

  /* Tasks */
  xTmDataMutex = xSemaphoreCreateMutex();
  if (xTmDataMutex == NULL) {
    printSerial("Mutex creation failed!");
    while (1) {}
  }

  xLcdMutex = xSemaphoreCreateMutex();
  if (xLcdMutex == NULL) {
    printSerial("Mutex creation failed!");
    while (1) {}
  }

  /* LoRa E32-433T30D */
#if LORA_ON
  SERIAL_LORA.begin(SERIAL_LORA_BAUD);
  //xLoraRadio.init();
  //xLoraRadio.Reset();
  //xLoraRadio.SetAirDataRate(ADR_1K);
  //xLoraRadio.SetAddress(LORA_E32_ADDRESS);
  //xLoraRadio.SetChannel(LORA_E32_CHANNEL);
  //xLoraRadio.SaveParameters(TEMPORARY);
  //xLoraRadio.PrintParameters();
  //xLoraRadio.SetMode(MODE_NORMAL);
#endif

  /* ADC ESP32 */
  // Largura 12 bits (0-4095) e atenuação de 11 dB (~0–3,3 V)
  analogSetWidth(12);
  analogSetPinAttenuation(AIN_PIN, ADC_11db);
  // (Opcional) "prender" o pino ao ADC para estabilidade
  adcAttachPin(AIN_PIN);

  lcd.init();
  lcd.backlight();
  lcdPrintMask(); // Print fixed mask initially
  //lcd.setCursor(0,0);
  //lcd.print("EcoMaua Telemetry");
  //lcd.setCursor(0,1);
  //lcd.print("Waiting Data...");

  /* Write Headers */
  printSerial(strDebugHeader);

  /* Start Tasks */
  xTaskCreatePinnedToCore(
    rxTmTask,
    "rxTmTask",
    TM_TASK_STACKSIZE,
    NULL,
    TM_TASK_PRIORITY,
    &xTaskTxTmHandle,
    TM_TASK_CORE);

  xTaskCreatePinnedToCore(
    battAdcTask,
    "battAdcTask",
    ADC_TASK_STACKSIZE,
    NULL,
    ADC_TASK_PRIORITY,
    &xTaskBattAdcHandle,
    ADC_TASK_CORE);
}

/****************************************************************************************************
 * Loop Infinito
 ****************************************************************************************************/

void loop() {

#if LORA_ON
  if (SERIAL_LORA.available()) {
    uint8_t u8ReceivedByte = SERIAL_LORA.read();

#if DEBUG_ON
    //SERIAL_DEBUG.print("Received byte: 0x");
    //if (u8ReceivedByte < 0x10) SERIAL_DEBUG.print('0'); // ensures leading zero for single-digit hex
    //SERIAL_DEBUG.println(u8ReceivedByte, HEX);
#endif

    if (!bTmPacketStarted) {
      /* Wait for Start of Packet */
      if (u8ReceivedByte == TM_START_OF_PACKET) {
        bTmPacketStarted = true;
        u8RxTmBufferIndex = 0;
        u8RxTmDataBuffer[u8RxTmBufferIndex++] = u8ReceivedByte;
      }
    } else {
      /* Accumulate data */
      u8RxTmDataBuffer[u8RxTmBufferIndex++] = u8ReceivedByte;

      /* Check for buffer overrun */
      if (u8RxTmBufferIndex >= TM_BUFFER_SIZE) {
        bTmPacketStarted = false;
        u8RxTmBufferIndex = 0;
        //printSerial("Buffer overrun, discarding data.\n");
      } else {

        /* Check for Start of Packet again */
        if (u8ReceivedByte == TM_START_OF_PACKET) {
          bTmPacketStarted = true;
          u8RxTmBufferIndex = 0;
          u8RxTmDataBuffer[u8RxTmBufferIndex++] = u8ReceivedByte;
          //printSerial("New SOP detected, restarting packet accumulation.\n");
        }

        /* Check for End of Packet */
        if (u8ReceivedByte == TM_END_OF_PACKET) {
          /* Parse the packet */
          if (xSemaphoreTake(xTmDataMutex, portMAX_DELAY) == pdTRUE) {
            bValidTmData = parseTelemetryMessage(u8RxTmDataBuffer, u8RxTmBufferIndex, &xTmData, TM_ENCRIPTATION_KEY);
            xSemaphoreGive(xTmDataMutex);

            if (bValidTmData) {
              //printSerial("Valid packet received and parsed.\n");
              /* Process telemetryData here */
            }
          } else {
            //printSerial("Invalid packet received.\n");
          }

          /* Reset for next packet */
          bTmPacketStarted = false;
          u8RxTmBufferIndex = 0;
        }
      }
    }
  }
#endif
}


/****************************************************************************************************
 * Corpo de Funções Auxiliares
 ****************************************************************************************************/

/* Geral */
void inline printSerial(const char *strData) {
#if DEBUG_ON
  SERIAL_DEBUG.print(strData);
#endif
#if BLUETOOTH_ON
  SERIAL_BT.print(strData);
#endif
}

void inline printSerial(const char ccData) {
#if DEBUG_ON
  SERIAL_DEBUG.print(ccData);
#endif
#if BLUETOOTH_ON
  SERIAL_BT.print(ccData);
#endif
}

bool isDifferent(float a, float b, float threshold = 0.01f) {
  return fabs(a - b) >= threshold;
}

void lcdPrintMask() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  /  /        :  :  ");  // Date and Time placeholders
  lcd.setCursor(0,1);
  lcd.print("S:      |D:        ");  // Speed and Distance placeholders
  lcd.setCursor(0,2);
  lcd.print("V:      |C:        ");  // Voltage and Current placeholders
  lcd.setCursor(0,3);
  lcd.print("A:      |B:        ");  // Autonomy and Battery placeholders
}

void lcdUpdateTelemetry(TTmData *data) {

  // Update Date individually
  if(isFirstLcdUpdate || lastDisplayedData.xDatetime.day() != data->xDatetime.day()) {
    lcd.setCursor(0,0);
    char buff[16];
    sprintf(buff,"%02d", data->xDatetime.day());
    lcd.print(buff);
  }

  if(isFirstLcdUpdate || lastDisplayedData.xDatetime.month() != data->xDatetime.month()) {
    lcd.setCursor(3,0);
    char buff[16];
    sprintf(buff,"%02d", data->xDatetime.month());
    lcd.print(buff);
  }

  if(isFirstLcdUpdate || lastDisplayedData.xDatetime.year() != data->xDatetime.year()) {
    lcd.setCursor(6,0);
    char buff[16];
    sprintf(buff,"%04d", data->xDatetime.year());
    lcd.print(buff);
  }

  // Update Time individually
  if(isFirstLcdUpdate || lastDisplayedData.xDatetime.hour() != data->xDatetime.hour()) {
    lcd.setCursor(12,0);
    char buff[16];
    sprintf(buff,"%02d", data->xDatetime.hour());
    lcd.print(buff);
  }

  if(isFirstLcdUpdate || lastDisplayedData.xDatetime.minute() != data->xDatetime.minute()) {
    lcd.setCursor(15,0);
    char buff[16];
    sprintf(buff,"%02d", data->xDatetime.minute());
    lcd.print(buff);
  }

  if(isFirstLcdUpdate || lastDisplayedData.xDatetime.second() != data->xDatetime.second()) {
    lcd.setCursor(18,0);
    char buff[16];
    sprintf(buff,"%02d", data->xDatetime.second());
    lcd.print(buff);
  }

  // Speed
  if(isFirstLcdUpdate || isDifferent(lastDisplayedData.xEcoCanTelemetryData.fVehicleSpeedkmH, data->xEcoCanTelemetryData.fVehicleSpeedkmH)) {
    lcd.setCursor(3,1);
    char buff[16];
    //sprintf(buff, "%02.2f", data->xEcoCanTelemetryData.fVehicleSpeedkmH);
    dtostrf(data->xEcoCanTelemetryData.fVehicleSpeedkmH, 5, 2, buff);
    lcd.print(buff);
  }

  // Distance
  if(isFirstLcdUpdate || isDifferent(lastDisplayedData.xEcoCanTelemetryData.fDistanceM, data->xEcoCanTelemetryData.fDistanceM)) {
    lcd.setCursor(11,1);
    char buff[16];
    //sprintf(buff, "%08.2f", data->xEcoCanTelemetryData.fDistanceM);
    dtostrf(data->xEcoCanTelemetryData.fDistanceM, 9, 2, buff);
    lcd.print(buff);
  }

  // Voltage
  if(isFirstLcdUpdate || isDifferent(lastDisplayedData.xEcoCanTelemetryData.fVoltageV, data->xEcoCanTelemetryData.fVoltageV)) {
    lcd.setCursor(3,2);
    char buff[16];
    //sprintf(buff, "%05.2f", data->xEcoCanTelemetryData.fVoltageV);
    dtostrf(data->xEcoCanTelemetryData.fVoltageV, 5, 2, buff);
    lcd.print(buff);
  }

  // Current
  if(isFirstLcdUpdate || isDifferent(lastDisplayedData.xEcoCanTelemetryData.fCurrentA, data->xEcoCanTelemetryData.fCurrentA)) {
    lcd.setCursor(15,2);
    char buff[16];
    //sprintf(buff, "%02.2f", data->xEcoCanTelemetryData.fCurrentA);
    dtostrf(data->xEcoCanTelemetryData.fCurrentA, 5, 2, buff);
    lcd.print(buff);
  }

  // Autonomy
  if(isFirstLcdUpdate || isDifferent(lastDisplayedData.xEcoCanTelemetryData.fAutonomyKmKwH, data->xEcoCanTelemetryData.fAutonomyKmKwH)) {
    lcd.setCursor(2,3);
    char buff[16];
    //sprintf(buff, "%03.2f", data->xEcoCanTelemetryData.fAutonomyKmKwH);
    dtostrf(data->xEcoCanTelemetryData.fAutonomyKmKwH, 6, 2, buff);
    lcd.print(buff);
  }

  // // Energy
  // if(isFirstLcdUpdate || isDifferent(lastDisplayedData.xEcoCanTelemetryData.fEnergyJ, data->xEcoCanTelemetryData.fEnergyJ)) {
  //   lcd.setCursor(11,3);
  //   char buff[16];
  //   //sprintf(buff, "%08.2f", data->xEcoCanTelemetryData.fEnergyJ);
  //   dtostrf(data->xEcoCanTelemetryData.fEnergyJ, 9, 2, buff);
  //   lcd.print(buff);
  // }

  // Update lastDisplayedData
  lastDisplayedData = *data;
  isFirstLcdUpdate = false;
}

bool parseTelemetryMessage(uint8_t *pu8Buffer, uint8_t u8BufferSize, TTmData *pxTelemetryData, const uint8_t cu8EncriptionKey) {

  /* 
  * Telemetry Message Format:
  * -- SOP
  * -- LEN
  * -- DAT
  * -- CRC
  * -- EOP
  */

  bool bIsValid = true;
  uint8_t u8TelemetrySize = 0;
  uint8_t u8CalculatedCrc = 0;
  uint8_t u8TelemetryCnt = 0;
  uint8_t u8EncryptedData = 0;
  uint8_t u8DecryptedData = 0;

  TTmData xTempTmData;
  uint8_t *pu8TelemetryDataPrt = (uint8_t *)&xTempTmData;

  if (u8BufferSize < 5) { /* Minimum length check (SOP + LEN + CRC + EOP + at least 1 data byte) */
    bIsValid = false;
  }

  u8TelemetrySize = pu8Buffer[1]; /* Length byte */
  if (bIsValid && u8TelemetrySize != sizeof(TTmData)) {
    bIsValid = false; /* Length mismatch */
  }

  if (bIsValid && u8BufferSize != (u8TelemetrySize + 4)) { /* SOP, LEN, CRC, EOP + data size */
    bIsValid = false;                                      /* Size mismatch */
  }

  /* Check SOP */
  if (bIsValid && pu8Buffer[0] != TM_START_OF_PACKET) {
    bIsValid = false; /* Start of packet mismatch */
  }

  /* Extract and decrypt data, calculate CRC */
  for (u8TelemetryCnt = 0; bIsValid && u8TelemetryCnt < u8TelemetrySize; u8TelemetryCnt++) {
    u8EncryptedData = pu8Buffer[2 + u8TelemetryCnt];
    u8DecryptedData = u8EncryptedData ^ cu8EncriptionKey;
    pu8TelemetryDataPrt[u8TelemetryCnt] = u8DecryptedData;
    u8CalculatedCrc ^= u8DecryptedData;
  }

  /* Check CRC */
  if (bIsValid && pu8Buffer[2 + u8TelemetrySize] != u8CalculatedCrc) {
    bIsValid = false; /* CRC mismatch */
  }

  /* Check EOP */
  if (bIsValid && pu8Buffer[3 + u8TelemetrySize] != TM_END_OF_PACKET) {
    bIsValid = false; /* End of packet mismatch */
  }

  /* Copy the data to the provided pointer only if the message was valid */
  if (bIsValid) {
    *pxTelemetryData = xTempTmData;
  }

  return (bIsValid);
}

/* Tasks */
void rxTmTask(void *pParameter) {
  for (;;) {

    if (xSemaphoreTake(xTmDataMutex, portMAX_DELAY) == pdTRUE) {
      bWorkValidTmData = bValidTmData;
      bValidTmData = false;
      xWorkTmData = xTmData;
      xSemaphoreGive(xTmDataMutex);
    }

    if (bWorkValidTmData) {
    
      /* Debug Telemetry - Bluetooth Short Range */

      /* Start of line */
      printSerial('>');
      /* Date and Time */
      sprintf(strWorkBuffer, "%04d/%02d/%02d %02d:%02d:%02d",
              xWorkTmData.xDatetime.year(), xWorkTmData.xDatetime.month(), xWorkTmData.xDatetime.day(),
              xWorkTmData.xDatetime.hour(), xWorkTmData.xDatetime.minute(), xWorkTmData.xDatetime.second());
      printSerial(strWorkBuffer); /* YYYY/MM/DD hh:mm:ss */
      /* Telemetry Data */
      sprintf(strWorkBuffer, "|T:%.4f", xWorkTmData.xEcoCanTelemetryData.fThrottlePercent); printSerial(strWorkBuffer); /* Throttle [%] */
      sprintf(strWorkBuffer, "|M:%.4f", xWorkTmData.xEcoCanTelemetryData.fMotorSpeedRpm);   printSerial(strWorkBuffer); /* Motor Speed [RPM] */
      sprintf(strWorkBuffer, "|S:%.4f", xWorkTmData.xEcoCanTelemetryData.fVehicleSpeedkmH); printSerial(strWorkBuffer); /* Vehicle Speed [km/h] */
      sprintf(strWorkBuffer, "|D:%.4f", xWorkTmData.xEcoCanTelemetryData.fDistanceM);       printSerial(strWorkBuffer); /* Distance [m] */
      sprintf(strWorkBuffer, "|V:%.4f", xWorkTmData.xEcoCanTelemetryData.fVoltageV);        printSerial(strWorkBuffer); /* Voltage [V] */
      sprintf(strWorkBuffer, "|I:%.4f", xWorkTmData.xEcoCanTelemetryData.fCurrentA);        printSerial(strWorkBuffer); /* Current [A] */
      sprintf(strWorkBuffer, "|P:%.4f", xWorkTmData.xEcoCanTelemetryData.fPowerW);          printSerial(strWorkBuffer); /* Power [W] */
      sprintf(strWorkBuffer, "|E:%.4f", xWorkTmData.xEcoCanTelemetryData.fEnergyJ);         printSerial(strWorkBuffer); /* Energy [J] */
      sprintf(strWorkBuffer, "|A:%.4f", xWorkTmData.xEcoCanTelemetryData.fAutonomyKmKwH);   printSerial(strWorkBuffer); /* Automony [km/kWh] */
      /* End of line */
      printSerial("\n");

      if (xSemaphoreTake(xLcdMutex, portMAX_DELAY) == pdTRUE) {
        /* Update the display */
        lcdUpdateTelemetry(&xWorkTmData);
        xSemaphoreGive(xLcdMutex);
      }

      bWorkValidTmData = false;
    }

    /* Put task to sleep */
    vTaskDelay(TM_TASK_SLEEP_MS / portTICK_PERIOD_MS);
  }
}

void battAdcTask(void *pParameter) {

    // if (xSemaphoreTake(xTmDataMutex, portMAX_DELAY) == pdTRUE) {
    //   bWorkValidTmData = bValidTmData;
    //   bValidTmData = false;
    //   xWorkTmData = xTmData;
    //   xSemaphoreGive(xTmDataMutex);
    // }

  static uint32_t acc = 0;
  static uint16_t cnt = 0;
  static bool update_lcd = false;
  uint16_t avgRaw = 0;

  for (;;) {

    // --- exactly one ADC read per iteration ---
    uint16_t raw = analogRead(AIN_PIN);
    acc += raw;
    cnt++;

    if (cnt >= ADC_SAMPLES) {
      avgRaw = (uint16_t)(acc / cnt);

      float battvoltage = (4.185f / 2560.00f) * (float)avgRaw;

      float battpercent = (-344.6374f * battvoltage * battvoltage * battvoltage) + (3947.7404f * battvoltage * battvoltage) - (14903.3718f * battvoltage) + 18587.9621f;
      if (battpercent < 0.0f){
        battpercent = 0.0f;
      } else if (battpercent > 100.0f){
        battpercent = 100.0f;
      }

      // // Print once per block of ADC_SAMPLES
      // SERIAL_DEBUG.print("Battery Voltage: ");
      // SERIAL_DEBUG.print(battvoltage);
      // SERIAL_DEBUG.print(" | Battery Percent ");
      // SERIAL_DEBUG.println(battpercent);

      // reset for next window
      acc = 0;
      cnt = 0;
      update_lcd = true;

      if ((xSemaphoreTake(xLcdMutex, portMAX_DELAY) == pdTRUE) && (update_lcd)) {
        lcd.setCursor(11,3);
        char buff[16];
        //sprintf(buff, "%08.2f", data->xEcoCanTelemetryData.fEnergyJ);
        dtostrf(battpercent, 8, 2, buff);
        buff[8] = '%%';
        lcd.print(buff);
        xSemaphoreGive(xLcdMutex);
        update_lcd = false;
      }

    }

    /* Put task to sleep */
    vTaskDelay(ADC_TASK_SLEEP_MS / portTICK_PERIOD_MS);
  }
}

/* LoRa E32-433T30D */

/* CAN MCP2515 */

/* RTC DS3231 */

/* ADC ESP32 */
