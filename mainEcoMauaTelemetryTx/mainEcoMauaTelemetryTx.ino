/****************************************************************************************************
 * Programa base para tranmissão da telemetria do veiculo Caixão (Carbonasso) do EcoMauá
 ****************************************************************************************************/

/****************************************************************************************************
 * Includes
 ****************************************************************************************************/

/* Geral */
#include <stdio.h>
#include <stdlib.h>
#include <BluetoothSerial.h>

/* Tasks */
//#include <FreeRTOS.h>

/* LoRa E32-433T30D */
#include "EBYTE.h"

/* CAN MCP2515 */
#include "libEcoMcp2515CanCtrl.h"

/* RTC DS3231 */
#include "RTClib.h"

/* SD Card */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

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
#define SERIAL_BT_NAME "EcoTxTelemetryBT"

/* Telemetry Message */
#define TM_ENCRIPTATION_KEY 0xC1
#define TM_START_OF_PACKET  '<'
#define TM_END_OF_PACKET    '>'

/* Tasks */
#define TM_TASK_STACKSIZE 8192
#define TM_TASK_PRIORITY  1
#define TM_TASK_CORE      1
#define TM_TASK_SLEEP_MS  1000

/* LoRa E32-433T30D */
#define LORA_E32_M0_PIN  32
#define LORA_E32_M1_PIN  33
#define LORA_E32_AUX_PIN 27

#define LORA_E32_ADDRESS 1
#define LORA_E32_CHANNEL 23

/* CAN MCP2515 */
#define MCP2515_CS_PIN 26

/* RTC DS3231 */

/* SD Card */
#define LOG_FILENAME "/data.csv"
/* SPI H */
#define HSPI_MOSI_PIN  13
#define HSPI_MISO_PIN  34
#define HSPI_SCK_PIN   14
#define SD_CARD_CS_PIN 25

/****************************************************************************************************
 * Variaveis, Constantes e Objetos Globais
 ****************************************************************************************************/

/* Geral */
BluetoothSerial xSerialBt;
const char strDebugHeader[] = ">Datetime [YYYY/MM/DD hh:mm:ss] | Throttle [%] | Motor Speed [RPM] | Vehicle Speed [km/h] | Distance [m] | Voltage [V] | Current [A] | Power [W] | Energy [J] | Automony [km/kWh]\n\r";
const char strCsvHeader[] = "Datetime [YYYY/MM/DD hh:mm:ss],Throttle [%],Motor Speed [RPM],Vehicle Speed [km/h],Distance [m],Voltage [V],Current [A],Power [W],Energy [J],Automony [km/kWh]\n\r";
char strWorkBuffer[64];
typedef struct TmData {
  DateTime xDatetime;
  CEcoMcp2515CanCtrl::TEcoCanTelemetryData xEcoCanTelemetryData;
} TTmData;
TTmData xTmData;

/* Tasks */
TaskHandle_t xTaskTxTmHandle;
SemaphoreHandle_t xTmDataMutex;

/* LoRa E32-433T30D */
//EBYTE xLoraRadio(&SERIAL_LORA, LORA_E32_M0_PIN, LORA_E32_M1_PIN, LORA_E32_AUX_PIN);

/* SD Card */
SPIClass xHSPI(HSPI);
File xDataCsvFile;

/* CAN MCP2515 */
CEcoMcp2515CanCtrl xEcoMcp2515CanCtrl(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, MCP2515_CS_PIN, 26, &xHSPI);
CEcoMcp2515CanCtrl::TEcoCanTelemetryData xEcoCanTelemetryData;

/* RTC DS3231 */
RTC_DS3231 xDs3132Rtc;

/****************************************************************************************************
 * Prototipos de Funções Auxiliares
 ****************************************************************************************************/

/* Geral */
void inline printSerial(const char *strData);
void inline printSerial(const char ccData);
void sendTmMessage(TTmData *pxTelemetryData, const uint8_t cu8EncriptionKey = 0);

/* Tasks */
void txTmTask(void *pParameter);

/* LoRa E32-433T30D */

/* CAN MCP2515 */

/* RTC DS3231 */

/* SD Card */

/****************************************************************************************************
 * Setup Inicial
 ****************************************************************************************************/

void setup() {

/* Set LoRa Mode Pins as Outputs and High */
pinMode(LORA_E32_M0_PIN, OUTPUT);
pinMode(LORA_E32_M1_PIN, OUTPUT);

digitalWrite(LORA_E32_M0_PIN, HIGH);
digitalWrite(LORA_E32_M1_PIN, HIGH);

#if DEBUG_ON
  SERIAL_DEBUG.begin(SERIAL_DEBUG_BAUD);
#endif
#if BLUETOOTH_ON
  SERIAL_BT.begin(SERIAL_BT_NAME);
#endif

  /* Tasks */
  xTmDataMutex = xSemaphoreCreateMutex();
  if (xTmDataMutex == NULL) {
    printSerial("Mutex creation failed!\n");
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

  /* CAN MCP2515 */
  xHSPI.begin(HSPI_SCK_PIN, HSPI_MISO_PIN, HSPI_MOSI_PIN);
  xEcoMcp2515CanCtrl.begin(CAN_20KBPS);

  /* RTC DS3231 */
  if (!xDs3132Rtc.begin()) {
    printSerial("Couldn't find RTC\n");
    //while (1) {};
  }

  if (xDs3132Rtc.lostPower()) {
    printSerial("RTC lost power, let's set the time!\n");
    /* When time needs to be set on a new device, or after a power loss, the following line sets the RTC to the date & time this sketch was compiled */
    xDs3132Rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //xDs3132Rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  /* SD Card */
  //xHSPI.begin(HSPI_SCK_PIN, HSPI_MISO_PIN, HSPI_MOSI_PIN);
  //if (!SD.begin(SD_CARD_CS_PIN, xHSPI)) {
  if (!SD.begin(SD_CARD_CS_PIN)) {
    printSerial("Card Mount Failed\n");
    return;
  }

  /* Write Headers */
  SERIAL_DEBUG.print(strDebugHeader);
  xDataCsvFile = SD.open(LOG_FILENAME, FILE_APPEND);
  if (xDataCsvFile) {
    xDataCsvFile.print(strCsvHeader);
    xDataCsvFile.flush();
    xDataCsvFile.close();
  }

  /* Start Tasks */
  xTaskCreatePinnedToCore(
    txTmTask,
    "txTmTask",
    TM_TASK_STACKSIZE,
    NULL,
    TM_TASK_PRIORITY,
    &xTaskTxTmHandle,
    TM_TASK_CORE);
}


/****************************************************************************************************
 * Loop Infinito
 ****************************************************************************************************/

void loop() {

  if (xSemaphoreTake(xTmDataMutex, portMAX_DELAY) == pdTRUE) {
    xEcoMcp2515CanCtrl.bReceiveCanMsgData(&xEcoCanTelemetryData);
    xSemaphoreGive(xTmDataMutex);
  }

  // SERIAL_DEBUG.print("Temperature: ");
  // SERIAL_DEBUG.print(xDs3132Rtc.getTemperature());
  // SERIAL_DEBUG.println(" C");
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

void sendTmMessage(TTmData *pxTelemetryData, const uint8_t cu8EncriptionKey) {

  /* 
  * Telemetry Message Format:
  * -- SOP
  * -- LEN
  * -- DAT
  * -- CRC
  * -- EOP
  */

  uint8_t *pu8TelemetryDataPrt = (uint8_t *)(pxTelemetryData);
  uint8_t u8TelemetryData = 0;
  const uint8_t cu8TelemetrySize = sizeof(TTmData);
  uint8_t u8TelemetryCnt = 0;
  uint8_t u8CalculatedCrc = 0;

  /* Send SOP */
  SERIAL_LORA.write(TM_START_OF_PACKET);

  /* Send LEN */
  SERIAL_LORA.write(cu8TelemetrySize);

  /* Send DAT */
  for (u8TelemetryCnt = 0; u8TelemetryCnt < cu8TelemetrySize; u8TelemetryCnt++) {
    u8TelemetryData = *(pu8TelemetryDataPrt + u8TelemetryCnt);
    u8CalculatedCrc ^= u8TelemetryData;
    SERIAL_LORA.write(u8TelemetryData ^ cu8EncriptionKey);
  }

  /* Send CRC */
  SERIAL_LORA.write(u8CalculatedCrc);

  /* Send EOP */
  SERIAL_LORA.write(TM_END_OF_PACKET);
}

/* Tasks */
void txTmTask(void *pParameter) {
  for (;;) {

    xTmData.xDatetime = xDs3132Rtc.now();
    if (xSemaphoreTake(xTmDataMutex, portMAX_DELAY) == pdTRUE) {
      xTmData.xEcoCanTelemetryData = xEcoCanTelemetryData;
      xSemaphoreGive(xTmDataMutex);
    }

    /* Debug Telemetry - Bluetooth Short Range */

    /* Start of line */
    printSerial('>');
    /* Date and Time */
    sprintf(strWorkBuffer, "%04d/%02d/%02d %02d:%02d:%02d",
            xTmData.xDatetime.year(), xTmData.xDatetime.month(), xTmData.xDatetime.day(),
            xTmData.xDatetime.hour(), xTmData.xDatetime.minute(), xTmData.xDatetime.second());
    printSerial(strWorkBuffer); /* YYYY/MM/DD hh:mm:ss */
    /* Telemetry Data */
    sprintf(strWorkBuffer, "|T:%.4f", xTmData.xEcoCanTelemetryData.fThrottlePercent); printSerial(strWorkBuffer); /* Throttle [%] */
    sprintf(strWorkBuffer, "|M:%.4f", xTmData.xEcoCanTelemetryData.fMotorSpeedRpm);   printSerial(strWorkBuffer); /* Motor Speed [RPM] */
    sprintf(strWorkBuffer, "|S:%.4f", xTmData.xEcoCanTelemetryData.fVehicleSpeedkmH); printSerial(strWorkBuffer); /* Vehicle Speed [km/h] */
    sprintf(strWorkBuffer, "|D:%.4f", xTmData.xEcoCanTelemetryData.fDistanceM);       printSerial(strWorkBuffer); /* Distance [m] */
    sprintf(strWorkBuffer, "|V:%.4f", xTmData.xEcoCanTelemetryData.fVoltageV);        printSerial(strWorkBuffer); /* Voltage [V] */
    sprintf(strWorkBuffer, "|I:%.4f", xTmData.xEcoCanTelemetryData.fCurrentA);        printSerial(strWorkBuffer); /* Current [A] */
    sprintf(strWorkBuffer, "|P:%.4f", xTmData.xEcoCanTelemetryData.fPowerW);          printSerial(strWorkBuffer); /* Power [W] */
    sprintf(strWorkBuffer, "|E:%.4f", xTmData.xEcoCanTelemetryData.fEnergyJ);         printSerial(strWorkBuffer); /* Energy [J] */
    sprintf(strWorkBuffer, "|A:%.4f", xTmData.xEcoCanTelemetryData.fAutonomyKmKwH);   printSerial(strWorkBuffer); /* Automony [km/kWh] */
    /* End of line */
    printSerial("\n");

    /* Storage Telemetry - SD card */

    xDataCsvFile = SD.open(LOG_FILENAME, FILE_APPEND);
    if (xDataCsvFile) {
      /* Start of line */
      /* Date and Time */
      sprintf(strWorkBuffer, "%04d/%02d/%02d %02d:%02d:%02d",
              xTmData.xDatetime.year(), xTmData.xDatetime.month(), xTmData.xDatetime.day(),
              xTmData.xDatetime.hour(), xTmData.xDatetime.minute(), xTmData.xDatetime.second());
      xDataCsvFile.print(strWorkBuffer); /* Datetime [YYYY/MM/DD hh:mm:ss] */
      /* Telemetry Data */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fThrottlePercent); xDataCsvFile.print(strWorkBuffer); /* Throttle [%] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fMotorSpeedRpm);   xDataCsvFile.print(strWorkBuffer); /* Motor Speed [RPM] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fVehicleSpeedkmH); xDataCsvFile.print(strWorkBuffer); /* Vehicle Speed [km/h] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fDistanceM);       xDataCsvFile.print(strWorkBuffer); /* Distance [m] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fVoltageV);        xDataCsvFile.print(strWorkBuffer); /* Voltage [V] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fCurrentA);        xDataCsvFile.print(strWorkBuffer); /* Current [A] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fPowerW);          xDataCsvFile.print(strWorkBuffer); /* Power [W] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fEnergyJ);         xDataCsvFile.print(strWorkBuffer); /* Energy [J] */
      sprintf(strWorkBuffer, ",%.4f", xTmData.xEcoCanTelemetryData.fAutonomyKmKwH);   xDataCsvFile.print(strWorkBuffer); /* Automony [km/kWh] */
      /* End of line */
      xDataCsvFile.print("\n");
      xDataCsvFile.flush();
      xDataCsvFile.close();
    }

    /* Real-Time Telemetry - LoRa long range */
    sendTmMessage(&xTmData, TM_ENCRIPTATION_KEY);

    /* Put task to sleep */
    vTaskDelay(TM_TASK_SLEEP_MS / portTICK_PERIOD_MS);
  }
}

/* LoRa E32-433T30D */

/* CAN MCP2515 */

/* RTC DS3231 */

/* SD Card */
