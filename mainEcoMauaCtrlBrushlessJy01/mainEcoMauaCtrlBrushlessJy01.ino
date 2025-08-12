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

/* Tabela de testes (id, beta, vi, N^B).
   OBS: Armazenamos também N (inteiro) para controle de iteração e calculamos N^B no setup, 
   mas mantemos o campo NpowB para cumprir seu formato e permitir múltiplas entradas. */
typedef struct {
  TOperationalStatus id;
  float beta;
  float vi;     // 0..1
  uint16_t N;   // número de iterações
  float NpowB;  // N^beta (precomputado)
} TAccTestCfg;

/* Exemplo de múltiplos testes: ajuste/adicione conforme necessário */
TAccTestCfg gxAccTests[] = {
  { 25, 0.5f, 0.65f, 150, 0.0f },
  { 26, 0.5f, 0.65f, 300, 0.0f },
  { 27, 0.5f, 0.65f, 450, 0.0f },
  { 28, 0.5f, 0.65f, 600, 0.0f },
  { 29, 0.7f, 0.65f, 150, 0.0f },
  { 30, 0.7f, 0.65f, 300, 0.0f },
  { 31, 0.7f, 0.65f, 450, 0.0f },
  { 32, 0.7f, 0.65f, 600, 0.0f },
  { 33, 0.9f, 0.65f, 150, 0.0f },
  { 34, 0.9f, 0.65f, 300, 0.0f },
  { 35, 0.9f, 0.65f, 450, 0.0f },
  { 36, 0.9f, 0.65f, 600, 0.0f },
};
const uint8_t GU8_TEST_CNT = sizeof(gxAccTests) / sizeof(gxAccTests[0]);

/* Estado global do executor de testes */
enum TTestPhase : uint8_t {
  E_TP_IDLE_WAIT_V0 = 0,
  E_TP_PRE_SLEEP,
  E_TP_GO_TO_VI,
  E_TP_VI_STABILIZE,
  E_TP_CUT_ACCEL_WAIT,
  E_TP_RAMP,
  E_TP_HOLD_MAX,
  E_TP_SHUTDOWN,
  E_TP_DONE
};

volatile bool    gbTestsRunning = false;
uint8_t          gu8CurrentTestIdx = 0;
TTestPhase       gePhase = E_TP_IDLE_WAIT_V0;

/* Timers/contadores internos */
uint32_t gu32PhaseStartMs = 0;
uint32_t gu32LastStepMs   = 0;
uint16_t gu16K            = 0;     // iteração corrente (1..N)

/* Variáveis da curva */
float gB = 0.0f;        // beta
float gVi = 0.0f;       // Vi
float gNpowB = 1.0f;    // N^B
float gPrev_k_pow_B = 0.0f; // (k-1)^B armazenado para próxima iteração
float gUp = 0.0f;       // up(k)

/* ================== Auto-teste de aceleração: configuração e estado ================== */

/* Protótipo */
float fAutoAccelCtrl(void);

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
  //xSystemScheduler.setTaskEnable(false, CSystemScheduler::E_CAN_TX_TASK);

  pinMode(CBoardPins::CU8_COUPLING_MOTOR_ENGAGE_DO_PIN, OUTPUT);
  digitalWrite(CBoardPins::CU8_COUPLING_MOTOR_ENGAGE_DO_PIN, LOW);

  /* Precompute N^B para toda a tabela (caso queira iniciar já com valores) */
  for (uint8_t i = 0; i < GU8_TEST_CNT; ++i) {
    gxAccTests[i].NpowB = powf((float)gxAccTests[i].N, gxAccTests[i].beta);
  }
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

    float fThrottlePercentLast = fThrottlePercent;
    /* Verifica se o veículo está na faixa de operação de corrente máxima */
    if (CF_MOTOR_MAX_CURRENT > fCurrentA) {
      /* Se em auto-teste de rampa, delega para a função isolada; senão, usa o acelerador normal */
      if (xOperationalStatus == E_AUTOTEST_RAMP_OPERATION) {
        fThrottlePercent = fAutoAccelCtrl();
        if ((fThrottlePercent - fThrottlePercentLast) > CF_THROTTLE_MAX_STEP) {
          fThrottlePercent = fThrottlePercentLast + CF_THROTTLE_MAX_STEP;
        }
      } else {
        fThrottlePercent = xThrottle.fGetControlPercent();
      }
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
    xExtSerial.print("|t:");
    xExtSerial.print(CF_THROTTLE_MAX_STEP); /* Operational Status */
    xExtSerial.print("|i:");
    xExtSerial.print(gxAccTests[gu8CurrentTestIdx].id); /* Operational Status */
    xExtSerial.print("|B:");
    xExtSerial.print(gxAccTests[gu8CurrentTestIdx].beta); /* Operational Status */
    xExtSerial.print("|v:");
    xExtSerial.print(gxAccTests[gu8CurrentTestIdx].vi); /* Operational Status */
    xExtSerial.print("|T:");
    xExtSerial.print(fThrottlePercent, 4); /* Throttle [%] */
    //xExtSerial.print("|M:");
    //xExtSerial.print(fMotorSpeedRpm, 4); /* Motor Speed [RPM] */
    xExtSerial.print("|S:");
    xExtSerial.print(fVehicleSpeedkmH, 4); /* Vehicle Speed [km/h] */
    xExtSerial.print("|D:");
    xExtSerial.print(fDistanceM, 4); /* Distance [m] */
    xExtSerial.print("|V:");
    xExtSerial.print(fVoltageV, 4); /* Voltage [V] */
    xExtSerial.print("|I:");
    xExtSerial.print(fCurrentA, 4); /* Current [A] */
    //xExtSerial.print("|P:");
    //xExtSerial.print(fPowerW, 4); /* Power [W] */
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

    // /* Execute CAN Transmission */
    // switch (u8CanTxCnt) {
    //   case 0: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_THROTTLE_PERCENT_ID, fThrottlePercent); break;  /* Throttle [%] */
    //   case 1: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_MOTOR_SPEED_RPM_ID, fMotorSpeedRpm); break;     /* Motor Speed [RPM] */
    //   case 2: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_VEHICLE_SPEED_KMH_ID, fVehicleSpeedkmH); break; /* Vehicle Speed [km/h] */
    //   case 3: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_DISTANCE_M_ID, fDistanceM); break;              /* Distance [m] */
    //   case 4: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_VOLTAGE_V_ID, fVoltageV); break;                /* Voltage [V] */
    //   case 5: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_CURRENT_A_ID, fCurrentA); break;                /* Current [A] */
    //   case 6: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_POWER_W_ID, fPowerW); break;                    /* Power [W] */
    //   case 7: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_ENERGY_J_ID, fEnergyJ); break;                  /* Energy [J] */
    //   case 8: xEcoMcp2515CanCtrl.vWriteCanMsgDataFloat(CEcoMcp2515CanCtrl::E_PILOT_SCREEN_DEVID, CEcoMcp2515CanCtrl::E_AUTONOMY_KMKWH_ID, fAutonomyKmKwH); break;      /* Automony [km/kWh] */
    //   default:
    //     u8CanTxCnt = 0;
    //     /* Disable CAN transmission task */
    //     xSystemScheduler.setTaskEnable(false, CSystemScheduler::E_CAN_TX_TASK);
    // }
    // u8CanTxCnt++;

  } /* Fim da execução da tarefa de transmissão CAN */
}

/* Retorna o valor de throttle desejado pelo auto-teste (0..1). 
   Faz toda a gestão de tempo e fases internamente. */
float fAutoAccelCtrl(void) {
  const uint32_t nowMs = millis();

  /* Se não estamos em modo de auto-teste, garantir reset de estado e devolver 0 */
  if (xOperationalStatus != E_AUTOTEST_RAMP_OPERATION) {
    gbTestsRunning = false;
    gePhase = E_TP_IDLE_WAIT_V0;
    gu8CurrentTestIdx = 0;
    gUp = 0.0f; gPrev_k_pow_B = 0.0f; gu16K = 0;
    return 0.0f;
  }

  /* Início do(s) teste(s) */
  if (!gbTestsRunning) {
    gbTestsRunning = true;
    gePhase = E_TP_IDLE_WAIT_V0;
    gu8CurrentTestIdx = 0;
    gu32PhaseStartMs = nowMs;
    gu32LastStepMs = nowMs;
    gUp = 0.0f; gPrev_k_pow_B = 0.0f; gu16K = 0;

    /* Carregar parâmetros do primeiro teste */
    gB  = gxAccTests[gu8CurrentTestIdx].beta;
    gVi = gxAccTests[gu8CurrentTestIdx].vi;
    gNpowB = gxAccTests[gu8CurrentTestIdx].NpowB;

  }

  /* ===== Máquina de estados do procedimento ===== */
  switch (gePhase) {
    case E_TP_IDLE_WAIT_V0:
      /* Espera veículo parado */
      if (fVehicleSpeedkmH <= CF_VEHICLE_STOP_KMH) {
        gePhase = E_TP_PRE_SLEEP;
        gu32PhaseStartMs = nowMs;
        return 0.0f;
      }
      return 0.0f;

    case E_TP_PRE_SLEEP:
      /* Dorme 15s antes de iniciar o teste */
      if (nowMs - gu32PhaseStartMs >= CF_TEST_START_WAIT_MS) {
        gePhase = E_TP_GO_TO_VI;
        gu32PhaseStartMs = nowMs;
        xWheelEnc.clearWhellDistance();
        fEnergyJ = 0.0f;
      }
      return 0.0f;

    case E_TP_GO_TO_VI:
    static float viStep = 0.0f;         // valor incremental
    static uint32_t reachViTimeMs = 0;  // momento em que gVi foi atingido

    // Se ainda não atingiu gVi, vai incrementando
    if (viStep < gVi) {
        viStep += 0.01f;
        if (viStep >= gVi) {
            viStep = gVi;
            reachViTimeMs = nowMs; // marca o momento que atingiu gVi
        }
        return viStep;
    }

    // Já atingiu gVi: verifica tempo de estabilização
    if (nowMs - reachViTimeMs >= CF_VI_STABILIZE_MS) {
        gePhase = E_TP_CUT_ACCEL_WAIT;
        gu32PhaseStartMs = nowMs;
        viStep = 0.0f;        // reseta para a próxima vez
        reachViTimeMs = 0;    // limpa marcador
        return 0.0f;
    }

    return gVi;

    case E_TP_CUT_ACCEL_WAIT:
      /* Controlador para de acelerar (0%) e espera estabilizar 2s */
      if (nowMs - gu32PhaseStartMs >= CF_POST_CUT_STAB_MS) {
        gePhase = E_TP_RAMP;
        gu32PhaseStartMs = nowMs;
        gu32LastStepMs = nowMs;
        gu16K = 0;
        gUp = 0.0f;
        gPrev_k_pow_B = 0.0f; // (k-1)^B com k=0 => 0
      }
      return 0.0f;

    case E_TP_RAMP: {
      /* Executa N iterações conforme dup/up; passo a cada CF_STEP_MS */
      uint16_t N = gxAccTests[gu8CurrentTestIdx].N;

      /* Avança iterações em passos de CF_STEP_MS (recupera eventuais atrasos) */
      while ((nowMs - gu32LastStepMs) >= CF_STEP_MS && gu16K < N) {
        gu32LastStepMs += CF_STEP_MS;
        gu16K++;                                 // k = 1..N
        float k_pow_B = powf((float)gu16K, gB);  // k^B
        float dup = (1.0f - gVi) * (k_pow_B - gPrev_k_pow_B) / gNpowB;
        gUp += dup;                               // up(k) = up(k-1) + dup(k)
        gPrev_k_pow_B = k_pow_B;                  // vira (k-1)^B da próxima
      }

      if (gu16K >= N) {
        /* Chegou ao final da curva: throttle deve ter chegado em 1.0 (com numerics) */
        gePhase = E_TP_HOLD_MAX;
        gu32PhaseStartMs = nowMs;
        return 1.0f;
      }

      /* Throttle durante a rampa: Vi + up(k) */
      float f = gVi + gUp;
      if (f > 1.0f) f = 1.0f;
      if (f < 0.0f) f = 0.0f;
      return f;
    }

    case E_TP_HOLD_MAX:
      /* Mantém 100% por CF_HOLD_AT_MAX_MS */
      if (nowMs - gu32PhaseStartMs >= CF_HOLD_AT_MAX_MS) {
        gePhase = E_TP_SHUTDOWN;
        gu32PhaseStartMs = nowMs;
      }
      return 1.0f;

    case E_TP_SHUTDOWN:
      /* Encerra teste: motor desligado (0%), prepara próximo teste */
      gePhase = E_TP_DONE;
      return 0.0f;

    case E_TP_DONE:
      /* Avança para o próximo teste na tabela, se houver */
      if ((gu8CurrentTestIdx + 1) < GU8_TEST_CNT) {
        gu8CurrentTestIdx++;
        /* Carrega parâmetros do próximo teste */
        gB  = gxAccTests[gu8CurrentTestIdx].beta;
        gVi = gxAccTests[gu8CurrentTestIdx].vi;
        gxAccTests[gu8CurrentTestIdx].NpowB = powf((float)gxAccTests[gu8CurrentTestIdx].N, gB);
        gNpowB = gxAccTests[gu8CurrentTestIdx].NpowB;

        /* Reinicia estado para o próximo ciclo */
        gePhase = E_TP_IDLE_WAIT_V0;
        gu32PhaseStartMs = nowMs;
        gu32LastStepMs = nowMs;
        gu16K = 0; gUp = 0.0f; gPrev_k_pow_B = 0.0f;

        return 0.0f;
      } else {
        /* Sem mais testes: finaliza rotina e volta para operação normal */
        gbTestsRunning = false;
        xOperationalStatus = E_NORMAL_OPERATION; // opcional: volte ao modo normal
        return 0.0f;
      }
  }

  /* Fallback de segurança */
  return 0.0f;
}