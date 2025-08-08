#include <stdio.h>
#include <stdlib.h>

#include "libAnalogCtrlIn.h"
#include "libAnalogSensor.h"
#include "libWheelUniEncoder.h"
#include "libPIController.h"
#include "libJy01BrushlessCtrl.h"
#include "libEcoMcp2515CanCtrl.h"
#include "incBoardPins.h"
#include "incSystemConstants.h"
#include "incSystemScheduler.h"

CAnalogSensor xSystemVoltage(CBoardPins::CU8_VIN_SENSE_AN_PIN, 9.2775f, 0.0f, 12, 5.0f);
CAnalogSensor xMotorCurrent(CBoardPins::CU8_IM_SENSE_AN_PIN, 10.0f, 0.0f, 12, 5.0f);

// CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 750, 3100, 0.01f, 0.70f, 12);
// CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 750, 3100, 1.0f, 1.0f, 12); /* Acelerador pequeno */
CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 850, 3100, 1.0f, 1.00f, 12); /* Acelerador grande */

CWheelUniEncoder xWheelEnc(CBoardPins::CU8_ENC_0_DI_IRQ_PIN, FALLING, 1.0f, 1.420f / TWO_PI);

CJy01BrushlessCtrl xJy01MotorCtrl(CBoardPins::CU8_JY01_CTRL_EL_DO_PIN, CBoardPins::CU8_JY01_CTRL_ZF_DO_PIN, CBoardPins::CU8_JY01_CTRL_M_DI_IRQ_PIN, CSystemConstants::CF_BRUSHED_MOTOR_ENCODER_PPR);

CEcoMcp2515CanCtrl xEcoMcp2515CanCtrl(CEcoMcp2515CanCtrl::E_MOTOR_CONTROLLER_DEVID, CBoardPins::CU8_MCP2515_CS_DO_PIN, CBoardPins::CU8_MCP2515_INT_DI_IRQ_PIN);

#define xExtSerial Serial

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

uint8_t u8CanTxCnt = 0;

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

const float CF_WSPD_TO_MRPM_CONV = 46.11f; /* Converte velocidade da roda [km/h] para velocidade do motor [RPM] */
const float CF_MAX_WSPD_KMH = 30.00f;      /* Velocidade máxima do veículo [km/h] */

const float CF_VEHICLE_STARTING_THROTTLE = 0.20f; /* Valor do acelerador de partida do motor quando o veículo está parado */
const float CF_WSPD_TO_MOTV_CONV = 1.15f;         /* Valor de conversão entre velocidade [km/h] e tensão no motor [V] */

const float CF_MOTOR_MAX_CURRENT = 10.00f; /* Corrente máxima do motor em operação */

uint32_t u32InitTime = 0;
uint32_t u32FinalTime = 0;
uint32_t u32TimeDiff = 0;
// u32InitTime = micros();
// u32FinalTime = micros() - 4;
// u32TimeDiff = (uint32_t) (u32FinalTime - u32InitTime);
// Serial.print("Time Diff:");
// Serial.println(u32TimeDiff);
// Serial.println("------");

void setup() {
  // put your setup code here, to run once:

  xExtSerial.begin(9600);

  xThrottle.begin();

  xWheelEnc.begin();

  xJy01MotorCtrl.begin(0x60);
  xJy01MotorCtrl.setReverseDir();
  // xJy01MotorCtrl.setFowardDir();

  /* tempo para o JY01 inicializar */
  delay(1000);

  xEcoMcp2515CanCtrl.begin(CAN_20KBPS);

  xSystemVoltage.begin();
  xMotorCurrent.begin();
  xMotorCurrent.sensorAutoZeroCalibration();

  pinMode(CBoardPins::CU8_LED_0_BLUE_DO_PIN, OUTPUT);
  pinMode(CBoardPins::CU8_LED_1_RED_DO_PIN, OUTPUT);
  digitalWrite(CBoardPins::CU8_LED_0_BLUE_DO_PIN, HIGH);
  digitalWrite(CBoardPins::CU8_LED_1_RED_DO_PIN, HIGH);

  xWheelEnc.enableEncoder();

  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_ENERGY_MEASURE_TASK);
  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_MOTOR_CONTROL_TASK);
  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_TELEMETRY_TASK);
  xSystemScheduler.setTaskEnable(false, CSystemScheduler::E_CAN_TX_TASK);

  pinMode(CBoardPins::CU8_COUPLING_MOTOR_ENGAGE_DO_PIN, OUTPUT);
  digitalWrite(CBoardPins::CU8_COUPLING_MOTOR_ENGAGE_DO_PIN, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

  /* Execução da tarefa de medição de energia */
  if (xSystemScheduler.bGetTaskExecFlag(CSystemScheduler::E_ENERGY_MEASURE_TASK)) {

    fAvgVoltageV += xSystemVoltage.fGetSensorScaled();
    fAvgCurrentA += xMotorCurrent.fGetSensorAutoZeroScaled();
  }

  /* Execução da tarefa de controle do motor */
  if (xSystemScheduler.bGetTaskExecFlag(CSystemScheduler::E_MOTOR_CONTROL_TASK)) {

    fMotorSpeedRpm = xJy01MotorCtrl.xMEncoder.fGetSpeedRpm((float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_MOTOR_CONTROL_TASK]) / 1000.0f);

    /* Verifica se o veículo está na faixa de operação de corrente máxima */
    if (CF_MOTOR_MAX_CURRENT > fCurrentA) {
      /* Faixa de operaçãom respeitada, atualizar o acelerador */
      /* Caso contrario, manter o acelerador igual até o motor voltar para a faixa de operação */
      fThrottlePercent = xThrottle.fGetControlPercent();
    }

    /* Gestão do acelerador */
    if (fThrottlePercent <= 0.05f) {
      if (bDriveActivated == true) {
        xJy01MotorCtrl.disableDrive();
        bDriveActivated = false;
      }
      xJy01MotorCtrl.setControlRaw(0);
      digitalWrite(CBoardPins::CU8_COUPLING_MOTOR_ENGAGE_DO_PIN, LOW);
    } else {
      if (bDriveActivated == false) {
        xJy01MotorCtrl.enableDrive();
        bDriveActivated = true;
        /* Motor iniciando */
        /* Verifica se o veículo está parado */
        // if (fVehicleSpeedkmH == 0.0f) {
        digitalWrite(CBoardPins::CU8_COUPLING_MOTOR_ENGAGE_DO_PIN, HIGH);
        //delay(100);
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
    fEnergyJ += (fPowerW + fPreviousPowerW) * (1.0f / (0.002f * (float)(xSystemScheduler._U32_TASK_EXEC_TIME_MS[CSystemScheduler::E_TELEMETRY_TASK])));
    fEnergyWh = fEnergyJ * (1.0f / 3600.0f);
    fPreviousPowerW = fPowerW;

    if ((fDistanceM != 0.0f) && (fEnergyWh != 0.0f)) {
      fAutonomyKmKwH = fDistanceM / fEnergyWh;
    } else {
      fAutonomyKmKwH = 0.0f;
    }

    xExtSerial.print('>');
    xExtSerial.print("O:");
    xExtSerial.print(xOperationalStatus); /* Operational Status */
    xExtSerial.print("|T:");
    xExtSerial.print(fThrottlePercent, 4); /* Throttle [%] */
    xExtSerial.print("|M:");
    xExtSerial.print(fMotorSpeedRpm, 4); /* Motor Speed [RPM] */
    xExtSerial.print("|S:");
    xExtSerial.print(fVehicleSpeedkmH, 4); /* Vehicle Speed [km/h] */
    xExtSerial.print("|D:");
    xExtSerial.print(fDistanceM, 4); /* Distance [m] */
    xExtSerial.print("|V:");
    xExtSerial.print(fVoltageV, 4); /* Voltage [V] */
    xExtSerial.print("|I:");
    xExtSerial.print(fCurrentA, 4); /* Current [A] */
    xExtSerial.print("|P:");
    xExtSerial.print(fPowerW, 4); /* Power [W] */
    xExtSerial.print("|E:");
    xExtSerial.print(fEnergyJ, 4); /* Energy [J] */
    // xExtSerial.print("|E:"); xExtSerial.print(fEnergyWh       , 4); /* Energy [Wh] */
    xExtSerial.print("|A:");
    xExtSerial.print(fAutonomyKmKwH, 4); /* Automony [km/kWh] */
    xExtSerial.print('\n');

    /* Enable CAN transmission task */
    xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_CAN_TX_TASK);
    u8CanTxCnt = 0;

  } /* Fim da execução da tarefa de telemetria */

  /* Execução da tarefa de transmissão CAN */
  if (xSystemScheduler.bGetTaskExecFlag(CSystemScheduler::E_CAN_TX_TASK)) {

    /* Execute CAN Transmission */
    switch (u8CanTxCnt) {
      case 0: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_THROTTLE_PERCENT_ID, fThrottlePercent); break;  /* Throttle [%] */
      case 1: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_MOTOR_SPEED_RPM_ID, fMotorSpeedRpm); break;     /* Motor Speed [RPM] */
      case 2: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_VEHICLE_SPEED_KMH_ID, fVehicleSpeedkmH); break; /* Vehicle Speed [km/h] */
      case 3: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_DISTANCE_M_ID, fDistanceM); break;              /* Distance [m] */
      case 4: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_VOLTAGE_V_ID, fVoltageV); break;                /* Voltage [V] */
      case 5: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_CURRENT_A_ID, fCurrentA); break;                /* Current [A] */
      case 6: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_POWER_W_ID, fPowerW); break;                    /* Power [W] */
      case 7: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_ENERGY_J_ID, fEnergyJ); break;                  /* Energy [J] */
      case 8: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_AUTONOMY_KMKWH_ID, fAutonomyKmKwH); break;      /* Automony [km/kWh] */
      default:
        u8CanTxCnt = 0;
        /* Disable CAN transmission task */
        xSystemScheduler.setTaskEnable(false, CSystemScheduler::E_CAN_TX_TASK);
    }
    u8CanTxCnt++;

  } /* Fim da execução da tarefa de transmissão CAN */
}
