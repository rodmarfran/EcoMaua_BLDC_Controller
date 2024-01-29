#include <stdio.h>
#include <stdlib.h>
#include <AltSoftSerial.h>

#include "libAnalogCtrlIn.h"
#include "libAnalogSensor.h"
#include "libWheelUniEncoder.h"
#include "libPIController.h"
#include "libJy01BrushlessCtrl.h"
#include "incBoardPins.h"
#include "incSystemConstants.h"
#include "incSystemScheduler.h"

CAnalogSensor xSystemVoltage(CBoardPins::CU8_VIN_SENSE_AN_PIN, 9.2775f, 0.0f, 12, 5.0f);
CAnalogSensor xMotorCurrent(CBoardPins::CU8_IM_SENSE_AN_PIN, 10.0f, 0.0f, 12, 5.0f);

// CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 750, 3100, 0.01f, 0.70f, 12);
// CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 750, 3100, 1.0f, 1.0f, 12); /* Acelerador pequeno */
CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 850, 3100, 1.0f, 0.80f, 12); /* Acelerador grande */

CWheelUniEncoder xWheelEnc(CBoardPins::CU8_ENC_0_DI_IRQ_PIN, FALLING, 1.0f, 2.010f/TWO_PI);

CJy01BrushlessCtrl xJy01MotorCtrl(CBoardPins::CU8_JY01_CTRL_EL_DO_PIN, CBoardPins::CU8_JY01_CTRL_ZF_DO_PIN, CBoardPins::CU8_JY01_CTRL_M_DI_IRQ_PIN, CSystemConstants::CF_BRUSHED_MOTOR_ENCODER_PPR);

#define xExtSerial Serial
// AltSoftSerial xExtSerial(CBoardPins::CU8_EXT_UART_RX_PIN, CBoardPins::CU8_EXT_UART_TX_PIN); // RX, TX

CSystemScheduler xSystemScheduler;
constexpr uint32_t CSystemScheduler::_U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TASKS_COUNT];
uint32_t u32CurrentTimeMs = 0;

float fThrottlePercent = 0.0f;
float fMotorSpeedRpm = 0.0f;
float fWheelSpeedRpm = 0.0f;
float fVehicleSpeedkmH = 0.0f;
float fDistanceM = 0.0f;
float fVoltageV = 0.0f;
float fAvgVoltageV = 0.0f;
float fCurrentA = 0.0f;
float fAvgCurrentA = 0.0f;
float fPreviousPowerW = 0.0f;
float fPowerW = 0.0f;
float fEnergyJ = 0.0f;
float fEnergyWh = 0.0f;
float fAutonomyKmKwH = 0.0f;

bool bDriveActivated = false;

char strCmdRxBuffer[64];
uint32_t u32CmdRxNumber = 0;

typedef enum OperationalStatus {
  E_NORMAL_OPERATION = 0,    /* Operação padrão do sistema */
  E_AUTOTEST_RAMP_OPERATION, /* Operação de auto-teste de rampa */
  E_AUTOTEST_AVGS_OPERATION  /* Operação de auto-teste de velocidade média */
} TOperationalStatus;

TOperationalStatus xOperationalStatus = E_NORMAL_OPERATION;

bool bStartOfPacket = false;
bool bEndOfPacket = false;
uint8_t u8CmdRxIndex = 0;

static uint32_t u32TimeStepMs = 0;

uint32_t u32TestDurationMs = 0;
float fTestInitialValue = 0.0f;
float fTestFinalValue = 0.0f;
uint32_t u32TestSettlingTimeMs = 0;

float fTestFinalDistanceM = 0.0f;
float fTestMinSpeedKmH = 0.0f;
float fTestMaxSpeedKmH = 0.0f;
float fTestAvgSpeedKmH = 0.0f;

const float CF_WSPD_TO_MRPM_CONV = 46.11f; /* Converte velocidade da roda [km/h] para velocidade do motor [RPM] */
const float CF_MAX_WSPD_KMH = 30.00f; /* Velocidade máxima do veículo [km/h] */

const float CF_VEHICLE_STARTING_THROTTLE = 0.20f; /* Valor do acelerador de partida do motor quando o veículo está parado */
const float CF_WSPD_TO_MOTV_CONV = 1.15f; /* Valor de conversão entre velocidade [km/h] e tensão no motor [V] */

uint32_t u32InitTime = 0;
uint32_t u32FinalTime = 0;
uint32_t u32TimeDiff = 0;
// u32InitTime = micros();
// u32FinalTime = micros() - 4;
// u32TimeDiff = (uint32_t) (u32FinalTime - u32InitTime);
// Serial.print("Time Diff:");
// Serial.println(u32TimeDiff);
// Serial.println("------");

float fCalculateTestANextStep(uint32_t u32TestDurationMs, float fTestInitialValue, float fTestFinalValue, uint32_t u32TestSettlingTimeMs, uint32_t u32DeltaTimeMs) {
    float fTestSignal = 0.0f;

    if (u32TimeStepMs == 0) {

      fTestSignal = fTestInitialValue;
      u32TimeStepMs = u32DeltaTimeMs;

    } else if(u32TimeStepMs <= u32TestDurationMs) {

      fTestSignal = fTestInitialValue + (float)((float)u32TimeStepMs / (float)u32TestSettlingTimeMs);
      if (fTestSignal < 0.0f) {
        fTestSignal = 0.0f;
      } else if (fTestSignal > fTestFinalValue) {
        fTestSignal = fTestFinalValue;
      }
      u32TimeStepMs += u32DeltaTimeMs;

    } else {

      fTestSignal = 0.0f;
      u32TimeStepMs = 0;
      xOperationalStatus = E_NORMAL_OPERATION;

    }

    return (fTestSignal);
}

float fCalculateTestBNextStep(float fTestFinalDistanceM, float fTestMinSpeedKmH, float fTestMaxSpeedKmH, float fTestAvgSpeedKmH, float fTestCurrentSpeedKmH, uint32_t u32DeltaTimeMs) {
  static bool bFirstRun = true;
  static bool bRampUp = true;
  static float fTestTotalDistanceM = 0.0f;
  static float fIretationDistanceM = 0.0f;
  float fDistanceStepM = 0.0f;
  float fIterationAvgSpeedKhM = 0.0f;
  float fTestSpeedKmH = 0.0f;

  if (u32TimeStepMs == 0) {

    bFirstRun = true;
    bRampUp = true;
    fTestTotalDistanceM = 0.0f;
    fIretationDistanceM = 0.0f;
    fTestSpeedKmH = CF_MAX_WSPD_KMH;
    u32TimeStepMs = u32DeltaTimeMs;

  } else if (fTestTotalDistanceM < fTestFinalDistanceM) {
    
    fDistanceStepM = fTestCurrentSpeedKmH  * (1.0f/3600.0f) * (float)u32DeltaTimeMs;
    fTestTotalDistanceM += fDistanceStepM;
    fIretationDistanceM += fDistanceStepM;
    fIterationAvgSpeedKhM = fIretationDistanceM * 3600.0f / (float)u32TimeStepMs;
  
    if (bRampUp == true) {

      /* Ramp-Up */
      if (fTestCurrentSpeedKmH < fTestMaxSpeedKmH) {
        fTestSpeedKmH = CF_MAX_WSPD_KMH;
      } else {
        bRampUp = false;
        fTestSpeedKmH = fTestMinSpeedKmH;
      }

      /* First Run */
      if (bFirstRun == true) {
        if (fTestCurrentSpeedKmH >= fTestAvgSpeedKmH) {
          bFirstRun = false;
          fIretationDistanceM = 0.0f;
          u32TimeStepMs = 0;
        }
      }
    
    } else {

      /* Ramp-Down */
      if (fIterationAvgSpeedKhM > fTestAvgSpeedKmH) {
        fTestSpeedKmH = fTestMinSpeedKmH;
      } else {
        bRampUp = true;
        fIretationDistanceM = 0.0f;
        u32TimeStepMs = 0;
        fTestSpeedKmH = CF_MAX_WSPD_KMH;
      }
    
    }
    
    u32TimeStepMs += u32DeltaTimeMs;
  
  } else {

    bFirstRun = true;
    bRampUp = true;
    fTestSpeedKmH = 0.0f;
    fTestTotalDistanceM = 0.0f;
    fIretationDistanceM = 0.0f;
    u32TimeStepMs = 0;
    xOperationalStatus = E_NORMAL_OPERATION;

  }

  return (fTestSpeedKmH);
}

void setup() {
  // put your setup code here, to run once:

  xExtSerial.begin(9600);

  xThrottle.begin();

  xWheelEnc.begin();

  xJy01MotorCtrl.begin(0x60);
  // xJy01MotorCtrl.setReverseDir();
  xJy01MotorCtrl.setFowardDir();

  /* tempo para o JY01 inicializar */
  delay(1000);

  xSystemVoltage.begin();
  xMotorCurrent.begin();
  xMotorCurrent.sensorAutoZeroCalibration();

  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);

  xWheelEnc.enableEncoder();

  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_ENERGY_MEASURE_TASK);
  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_MOTOR_CONTROL_TASK);
  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_TELEMETRY_TASK);

}

void loop() {
  // put your main code here, to run repeatedly:

  if (xExtSerial.available()) {

    strCmdRxBuffer[u8CmdRxIndex] = xExtSerial.read();
    
    if (strCmdRxBuffer[u8CmdRxIndex] == '>') {
      bStartOfPacket = true;
      bEndOfPacket = false;
    } else if (strCmdRxBuffer[u8CmdRxIndex] == '\n') {
      bStartOfPacket = false;
      bEndOfPacket = true;
    }

    if (bStartOfPacket == true) {
      u8CmdRxIndex++;
    }

    if (bEndOfPacket == true) {
      bEndOfPacket = false;
      // xExtSerial.flushInput();

      // if ((strCmdRxBuffer[1] == 'S') && (strCmdRxBuffer[2] == ':')) {

      //   u32CmdRxNumber =  (strCmdRxBuffer[3] - '0') * 10000;
      //   u32CmdRxNumber += (strCmdRxBuffer[5] - '0') * 1000;
      //   u32CmdRxNumber += (strCmdRxBuffer[6] - '0') * 100;
      //   u32CmdRxNumber += (strCmdRxBuffer[7] - '0') * 10;
      //   u32CmdRxNumber += (strCmdRxBuffer[8] - '0') * 1;

      //   fThrottlePercent = (float)(u32CmdRxNumber / 10000.0f);

      // } else if ((strCmdRxBuffer[1] == 'T') && (strCmdRxBuffer[2] == ':')) {

      if ((strCmdRxBuffer[1] == 'T') && (strCmdRxBuffer[2] == ':')) {

        fThrottlePercent = 0.0f;
        xWheelEnc.clearWhellDistance();
        fAvgVoltageV = 0.0f;
        fAvgCurrentA = 0.0f;
        fPreviousPowerW = 0.0f;
        fEnergyJ = 0.0f;

        bDriveActivated = false;

        u32TimeStepMs = 0;

        /* Decode Delay Time [ms] */
        u32CmdRxNumber =  (strCmdRxBuffer[5] - '0') * 1000;
        u32CmdRxNumber += (strCmdRxBuffer[6] - '0') * 100;
        u32CmdRxNumber += (strCmdRxBuffer[7] - '0') * 10;
        u32CmdRxNumber += (strCmdRxBuffer[8] - '0') * 1;

        if (u32CmdRxNumber > 0) {
          delay(u32CmdRxNumber);
        }

        if (strCmdRxBuffer[3] == 'A') {
          xOperationalStatus = E_AUTOTEST_RAMP_OPERATION;
        } else if (strCmdRxBuffer[3] == 'M') {
          xOperationalStatus = E_AUTOTEST_RAMP_OPERATION;
          while (fThrottlePercent < 0.50f) {
            fThrottlePercent = xThrottle.fGetControlPercent();
            delay(100);
          }
          fThrottlePercent = 0.0f;
        } else if (strCmdRxBuffer[3] == 'B') {
          xOperationalStatus = E_AUTOTEST_AVGS_OPERATION;
        } else {
          xOperationalStatus = E_NORMAL_OPERATION;
        }

        /* Check if the command is for an automated ramp test */
        if ((xOperationalStatus == E_AUTOTEST_RAMP_OPERATION) && (u8CmdRxIndex >= 37)) {

          /* Command Example: ">T:A.2500|D:0120|I:0.00|F:1.00|R:0060\n" */

          /* Decode Test Duration [ms] */
          u32CmdRxNumber =  (strCmdRxBuffer[12] - '0') * 1000;
          u32CmdRxNumber += (strCmdRxBuffer[13] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[14] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[15] - '0') * 1;
          u32TestDurationMs = u32CmdRxNumber * 1000;

          /* Decode Initial Value [-] */
          u32CmdRxNumber = (strCmdRxBuffer[19] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[21] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[22] - '0') * 1;
          fTestInitialValue = (float)(u32CmdRxNumber / 100.0f);

          /* Decode Final Value [-] */
          u32CmdRxNumber = (strCmdRxBuffer[26] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[28] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[29] - '0') * 1;
          fTestFinalValue = (float)(u32CmdRxNumber / 100.0f);

          /* Decode Change Rate (Settling Time [ms]) */
          u32CmdRxNumber =  (strCmdRxBuffer[33] - '0') * 1000;
          u32CmdRxNumber += (strCmdRxBuffer[34] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[35] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[36] - '0') * 1;
          u32TestSettlingTimeMs = u32CmdRxNumber * 1000;

        /* Check if the command is for an automated avgs test */
        } else if ((xOperationalStatus == E_AUTOTEST_AVGS_OPERATION) && (u8CmdRxIndex >= 40)) {

          /* Command Example: ">T:B.2500|D:1000|I:00.00|F:30.00|A:20.00\n" */

          /* Decode Test Final Distance [m] */
          u32CmdRxNumber = (strCmdRxBuffer[12] - '0') * 1000;
          u32CmdRxNumber += (strCmdRxBuffer[13] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[14] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[15] - '0') * 1;
          fTestFinalDistanceM = (float)(u32CmdRxNumber);

          /* Decode Test Minimum Speed [km/h] */
          u32CmdRxNumber = (strCmdRxBuffer[19] - '0') * 1000;
          u32CmdRxNumber += (strCmdRxBuffer[20] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[22] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[23] - '0') * 1;
          fTestMinSpeedKmH = (float)(u32CmdRxNumber / 100.0f);

          /* Decode Test Maximum Speed [km/h] */
          u32CmdRxNumber = (strCmdRxBuffer[27] - '0') * 1000;
          u32CmdRxNumber += (strCmdRxBuffer[28] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[30] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[31] - '0') * 1;
          fTestMaxSpeedKmH = (float)(u32CmdRxNumber / 100.0f);

          /* Decode Test Average Speed [km/h] */
          u32CmdRxNumber = (strCmdRxBuffer[35] - '0') * 1000;
          u32CmdRxNumber += (strCmdRxBuffer[36] - '0') * 100;
          u32CmdRxNumber += (strCmdRxBuffer[38] - '0') * 10;
          u32CmdRxNumber += (strCmdRxBuffer[39] - '0') * 1;
          fTestAvgSpeedKmH = (float)(u32CmdRxNumber / 100.0f);

        } else {

          u32TestDurationMs = 0;
          fTestInitialValue = 0.0f;
          fTestFinalValue = 0.0f;
          u32TestSettlingTimeMs = 0;
          fTestFinalDistanceM = 0.0f;
          fTestMinSpeedKmH = 0.0f;
          fTestMaxSpeedKmH = 0.0f;
          fTestAvgSpeedKmH = 0.0f;

        }

        xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_ENERGY_MEASURE_TASK);
        xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_MOTOR_CONTROL_TASK);
        xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_TELEMETRY_TASK);

      }

      u8CmdRxIndex = 0;
    }
  }

  /* Execução da tarefa de medição de energia */
  if (xSystemScheduler.bGetTaskExecFlag(CSystemScheduler::E_ENERGY_MEASURE_TASK)) {

    fAvgVoltageV += xSystemVoltage.fGetSensorScaled();
    fAvgCurrentA += xMotorCurrent.fGetSensorAutoZeroScaled();

  }

  /* Execução da tarefa de controle do motor */
  if (xSystemScheduler.bGetTaskExecFlag(CSystemScheduler::E_MOTOR_CONTROL_TASK)) {

    fMotorSpeedRpm = xJy01MotorCtrl.xMEncoder.fGetSpeedRpm((float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_MOTOR_CONTROL_TASK]) / 1000.0f);

    if (xOperationalStatus == E_AUTOTEST_RAMP_OPERATION) {
      fThrottlePercent = fCalculateTestANextStep(u32TestDurationMs, fTestInitialValue, fTestFinalValue, u32TestSettlingTimeMs, xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_MOTOR_CONTROL_TASK]);
    } else if (xOperationalStatus == E_AUTOTEST_AVGS_OPERATION) {
      fThrottlePercent = fCalculateTestBNextStep(fTestFinalDistanceM, fTestMinSpeedKmH, fTestMaxSpeedKmH, fTestAvgSpeedKmH, fVehicleSpeedkmH, xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_MOTOR_CONTROL_TASK]) * (1.0f/CF_MAX_WSPD_KMH);
    } else {
      fThrottlePercent = xThrottle.fGetControlPercent();
    }

    /* Gestão do acelerador */
    if (fThrottlePercent <= 0.05f) {
      if (bDriveActivated == true) {
        xJy01MotorCtrl.disableDrive();
        bDriveActivated = false;
      }
      xJy01MotorCtrl.setControlRaw(0);
    } else {
      if (bDriveActivated == false) {
        xJy01MotorCtrl.enableDrive();
        bDriveActivated = true;
        /* Motor iniciando */
        /* Verifica se o veículo está parado */
        // if (fVehicleSpeedkmH == 0.0f) {
          /* Veículo parado, aciona o motor com um valor apropriado para sair da inercia */
          xJy01MotorCtrl.setControlPercent(CF_VEHICLE_STARTING_THROTTLE);
        // }
      } else {
        xJy01MotorCtrl.setControlPercent(fThrottlePercent);

      }
    }

  } /* Fim da execução da tarefa de controle do motor */

  /* Execução da tarefa de telemetria */
  if (xSystemScheduler.bGetTaskExecFlag(CSystemScheduler::E_TELEMETRY_TASK)) {

    // fMotorSpeedRpm = xJy01MotorCtrl.xMEncoder.fGetSpeedRpm((float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TELEMETRY_TASK]) / 1000.0f);
    fVehicleSpeedkmH = xWheelEnc.fGetWhellSpeedKmH((float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TELEMETRY_TASK]) / 1000.0f);
    // fVehicleSpeedkmH = xWheelEnc.fGetWhellAvgSpeedKmH();
    fDistanceM = xWheelEnc.fGetWhellDistanceM();

    fCurrentA = fAvgCurrentA / (float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TELEMETRY_TASK] / xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_ENERGY_MEASURE_TASK]);
    fAvgCurrentA = 0.0f;
    if (fCurrentA < 0.05f) {
      fCurrentA = 0.00f;
    }

    fVoltageV = fAvgVoltageV / (float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TELEMETRY_TASK] / xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_ENERGY_MEASURE_TASK]);
    fAvgVoltageV = 0.0f;

    fPowerW = fVoltageV * fCurrentA;
    fEnergyJ += (fPowerW + fPreviousPowerW) * (1.0f/(0.002f * (float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TELEMETRY_TASK])));
    fEnergyWh = fEnergyJ * (1.0f/3600.0f);
    fPreviousPowerW = fPowerW;
    
    if ((fDistanceM != 0.0f) && (fEnergyWh != 0.0f)) {
      fAutonomyKmKwH = fDistanceM / fEnergyWh;
    } else {
      fAutonomyKmKwH = 0.0f;
    }

    xExtSerial.print('>');
    xExtSerial.print("O:");  xExtSerial.print(xOperationalStatus);  /* Operational Status */
    xExtSerial.print("|T:"); xExtSerial.print(fThrottlePercent, 4); /* Throttle [%] */
    xExtSerial.print("|M:"); xExtSerial.print(fMotorSpeedRpm, 4);   /* Motor Speed [RPM] */
    xExtSerial.print("|S:"); xExtSerial.print(fVehicleSpeedkmH, 4); /* Vehicle Speed [km/h] */
    xExtSerial.print("|D:"); xExtSerial.print(fDistanceM, 4);       /* Distance [m] */
    xExtSerial.print("|V:"); xExtSerial.print(fVoltageV, 4);        /* Voltage [V] */
    xExtSerial.print("|I:"); xExtSerial.print(fCurrentA, 4);        /* Current [A] */
    xExtSerial.print("|P:"); xExtSerial.print(fPowerW, 4);          /* Power [W] */
    xExtSerial.print("|E:"); xExtSerial.print(fEnergyJ, 4);         /* Energy [J] */
    // xExtSerial.print("|E:"); xExtSerial.print(fEnergyWh, 4);        /* Energy [Wh] */
    xExtSerial.print("|A:"); xExtSerial.print(fAutonomyKmKwH, 4);   /* Automony [km/kWh] */
    xExtSerial.print('\n');
    // xExtSerial.flushOutput();

  } /* Fim da execução da tarefa de telemetria */

}
