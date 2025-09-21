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

// TOperationalStatus xOperationalStatus = E_NORMAL_OPERATION;
TOperationalStatus xOperationalStatus = E_AUTOTEST_RAMP_OPERATION;

bool bStartOfPacket = false;
bool bEndOfPacket = false;
uint8_t u8CmdRxIndex = 0;

static uint32_t u32TimeStepMs = 0;

const float CF_WSPD_TO_MRPM_CONV = 46.11f; /* Converte velocidade da roda [km/h] para velocidade do motor [RPM] */
const float CF_MAX_WSPD_KMH = 30.00f;      /* Velocidade máxima do veículo [km/h] */

const float CF_VEHICLE_STARTING_THROTTLE = 0.15f; /* Valor do acelerador de partida do motor quando o veículo está parado */
const float CF_WSPD_TO_MOTV_CONV = 1.15f;         /* Valor de conversão entre velocidade [km/h] e tensão no motor [V] */

const float CF_MOTOR_MAX_CURRENT = 8.5f; /* Corrente máxima do motor em operação */

const float CF_THROTTLE_MAX_STEP = 0.02f; /* Mudança limite do acelerador */

uint32_t u32InitTime = 0;
uint32_t u32FinalTime = 0;
uint32_t u32TimeDiff = 0;
// u32InitTime = micros();
// u32FinalTime = micros() - 4;
// u32TimeDiff = (uint32_t) (u32FinalTime - u32InitTime);
// Serial.print("Time Diff:");
// Serial.println(u32TimeDiff);
// Serial.println("------");

/* ================== Auto-teste de aceleração: configuração e estado ================== */

/* Constantes de tempo (em ms) – ajuste conforme necessário */
const uint32_t CF_TEST_START_WAIT_MS   = 5000;  // espera após atingir 0 km/h
const uint32_t CF_VI_STABILIZE_MS      = 5000;  // estabilizar em Vi
const uint32_t CF_POST_CUT_STAB_MS     = 100;   // estabilizar após cortar aceleração
const uint32_t CF_HOLD_AT_MAX_MS       = 5000;   // manter 100% antes de encerrar
const uint32_t CF_STEP_MS              = 100;    // agendador chama a cada 100ms

/* Limiar para considerar o veículo parado */
const float    CF_VEHICLE_STOP_KMH     = 0.10f;

const float CF_RAMP_UP_VI = 0.15f;
const float CF_RAMP_UP_BETA = 0.75f;
const uint16_t CU16_RAMP_UP_N = 150;

float fRampUpVi = 0.15f;

/* ================== Rampa de aceleração (estado mínimo) ================== */
static uint16_t gRamp_k = 0;              // iteração atual (1..N)
static uint16_t gRamp_N = 0;              // N da rampa corrente
static float    gRamp_B = 0.0f;           // beta da rampa corrente
static float    gRamp_Vi = 0.0f;          // Vi (0..1) da rampa corrente
static float    gRamp_NpowB = 1.0f;       // N^B precomputado
static float    gRamp_prev_k_pow_B = 0.0f;// (k-1)^B armazenado
static float    gRamp_up = 0.0f;          // up(k)
static bool     gRamp_active = false;     // rampa em execução
static uint32_t gRamp_lastStepMs = 0;     // timestamp do último passo

/* Clamp utilitário */
static inline float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

/* True se a rampa terminou (atingiu k>=N) */
bool bAccelRampFinished() {
  return (gRamp_active == false) && (gRamp_k != 0);
}

/* Zera/encerra rampa imediatamente */
void vAccelRampAbort() {
  gRamp_active = false;
  gRamp_k = 0;
  gRamp_up = 0.0f;
  gRamp_prev_k_pow_B = 0.0f;
  gRamp_NpowB = 1.0f;
  gRamp_Vi = 0.0f;
  gRamp_B = 0.0f;
  gRamp_N = 0;
  gRamp_lastStepMs = millis();
}

/* Passo da rampa:
   - Vi:     ponto inicial do throttle (0..1)
   - N:      número de iterações da rampa (>=1)
   - beta:   expoente da curva (>0)
   - bReset: quando true, inicia/zera a rampa com estes parâmetros
   - stepMs: resolução temporal entre iterações (típico 100 ms)

   Retorna o throttle sugerido (0..1) para aplicar agora. */
float fAccelRampStep(float Vi, uint16_t N, float beta, bool bReset, uint16_t stepMs) {
  const uint32_t nowMs = millis();

  /* Reset/Inicialização sob demanda */
  if (bReset || !gRamp_active) {
    gRamp_active = true;
    gRamp_k = 0;
    gRamp_up = 0.0f;
    gRamp_prev_k_pow_B = 0.0f;

    gRamp_Vi = clamp01(Vi);
    gRamp_N = (N == 0 ? 1 : N);
    gRamp_B = (beta <= 0.0f ? 1.0f : beta);
    gRamp_NpowB = powf((float)gRamp_N, gRamp_B);

    gRamp_lastStepMs = nowMs;
  }

  /* Avança as iterações conforme o tempo decorrido */
  //while (gRamp_active && (nowMs - gRamp_lastStepMs) >= stepMs && gRamp_k < gRamp_N) {
  if (gRamp_active && (gRamp_k < gRamp_N)) {
    gRamp_lastStepMs += stepMs;
    gRamp_k++;                               // k = 1..N

    float k_pow_B = powf((float)gRamp_k, gRamp_B);                 // k^B
    float dup = (1.0f - gRamp_Vi) * (k_pow_B - gRamp_prev_k_pow_B) // (1 - Vi)*(k^B - (k-1)^B)/N^B
                / gRamp_NpowB;
    gRamp_up += dup;                          // up(k) = up(k-1) + dup(k)
    gRamp_prev_k_pow_B = k_pow_B;
  }

  /* Se terminou as iterações, mantenha 1.0 e desative */
  if (gRamp_k >= gRamp_N) {
    gRamp_active = false; // terminou
    return 1.0f;
  }

  /* Throttle atual: Vi + up(k) (com clamp numérico) */
  float f = gRamp_Vi + gRamp_up;
  return clamp01(f);
}

void setup() {
  // put your setup code here, to run once:

  xExtSerial.begin(9600);

  xThrottle.begin();

  xWheelEnc.begin();

  xJy01MotorCtrl.begin(0x60);
  xJy01MotorCtrl.setReverseDir();
  //xJy01MotorCtrl.setFowardDir();

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
  xSystemScheduler.setTaskEnable(true, CSystemScheduler::E_CAN_TX_TASK);

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

    // float fThrottlePercentLast = fThrottlePercent;
    // /* Verifica se o veículo está na faixa de operação de corrente máxima */
    // if (CF_MOTOR_MAX_CURRENT > fCurrentA) {
    //   /* Se em auto-teste de rampa, delega para a função isolada; senão, usa o acelerador normal */
    //   if (xOperationalStatus == E_AUTOTEST_RAMP_OPERATION) {
    //     if (bAccelRampFinished()) {
    //       fThrottlePercent = 0.0f;
    //     } else {
    //       fThrottlePercent = fAccelRampStep(fRampUpVi, CU16_RAMP_UP_N, CF_RAMP_UP_BETA, /*bReset=*/false, /*stepMs=*/100);
    //       if ((fThrottlePercent - fThrottlePercentLast) > CF_THROTTLE_MAX_STEP) {
    //         fThrottlePercent = fThrottlePercentLast + CF_THROTTLE_MAX_STEP;
    //       }
    //     }
    //   } else {
        float fRealThrottlePercent = xThrottle.fGetControlPercent();
    //   }
    // }

    /* Gestão do acelerador */
    if (fRealThrottlePercent <= 0.05f) {
      if (bDriveActivated == true) {
        xJy01MotorCtrl.disableDrive();
        vAccelRampAbort(); // throttle sugerido voltará a 0 na próxima chamada com reset verdadeiro
        bDriveActivated = false;
      }
      fThrottlePercent = 0;
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
        fRampUpVi = 0.03520958f * fVehicleSpeedkmH - 0.00790419f;
        if (fRampUpVi > 1.0f) {
          fRampUpVi = 1.0f;
        } else if (fRampUpVi < CF_VEHICLE_STARTING_THROTTLE) {
          fRampUpVi = CF_VEHICLE_STARTING_THROTTLE;
        }
        fThrottlePercent = fAccelRampStep(fRampUpVi, CU16_RAMP_UP_N, CF_RAMP_UP_BETA, /*bReset=*/true, /*stepMs=*/100);
        // }
      } else {
        if (fRealThrottlePercent >= 0.40f) {
          float fThrottlePercentLast = fThrottlePercent;
          /* Verifica se o veículo está na faixa de operação de corrente máxima */
          if (CF_MOTOR_MAX_CURRENT > fCurrentA) {
            /* Se em auto-teste de rampa, delega para a função isolada; senão, usa o acelerador normal */
            if (xOperationalStatus == E_AUTOTEST_RAMP_OPERATION) {
              if (bAccelRampFinished()) {
                fThrottlePercent = 1.0f;
              } else {
                fThrottlePercent = fAccelRampStep(fRampUpVi, CU16_RAMP_UP_N, CF_RAMP_UP_BETA, /*bReset=*/false, /*stepMs=*/100);
                if ((fThrottlePercent - fThrottlePercentLast) > CF_THROTTLE_MAX_STEP) {
                  fThrottlePercent = fThrottlePercentLast + CF_THROTTLE_MAX_STEP;
                }
              }
            }
          } 
          xJy01MotorCtrl.setControlPercent(fThrottlePercent);
        } else {
          fThrottlePercent = fRealThrottlePercent;
          xJy01MotorCtrl.setControlPercent(fThrottlePercent);
        }
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
    // xExtSerial.print("|t:");
    // xExtSerial.print(CF_THROTTLE_MAX_STEP); /* Operational Status */
    // xExtSerial.print("|i:");
    // xExtSerial.print(gxAccTests[gu8CurrentTestIdx].id); /* Operational Status */
    // xExtSerial.print("|B:");
    // xExtSerial.print(gxAccTests[gu8CurrentTestIdx].beta); /* Operational Status */
    // xExtSerial.print("|v:");
    // xExtSerial.print(gxAccTests[gu8CurrentTestIdx].vi); /* Operational Status */
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
    xExtSerial.print("|E:"); xExtSerial.print(fEnergyWh       , 4); /* Energy [Wh] */
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
