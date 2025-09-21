/**
 * @file libEcoMcp2515CanCtrl.h
 * @brief Biblioteca para controlador CAN MCP2515 do EcoMauá.
 * @details Esta biblioteca foi desenvolvida para gerenciar o controlador CAN MCP2515 no veículo da equipe EcoMauá.
 * 
 * O formato da mensagem CAN para a telemetria da equipe EcoMauá é apresentado abaixo:
 *   - CAN Identifier (1 byte):
 *     -- Tatget Address (1 byte)
 *   - CAN Data (8 bytes):
 *     -- Message Type (1 byte)
 *     -- Source Address (1 byte)
 *     -- Data Identification (1 byte)
 *     -- Data Byte #0 (1 byte)
 *     -- Data Byte #1 (1 byte)
 *     -- Data Byte #2 (1 byte)
 *     -- Data Byte #3 (1 byte)
 *     -- CRC (1 byte)
 * 
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2024-05-05
 * @license MIT
 */

#ifndef LIBECOMCP2515CANCTRL_H
#define LIBECOMCP2515CANCTRL_H

#include <stdio.h>
#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

/**
 * @class CEcoMcp2515CanCtrl
 * @brief Classe para controlador CAN MCP2515 do EcoMauá
 */
class CEcoMcp2515CanCtrl {
public:

  /**
   * @brief Definição do tipo para enumeração dos dispositivos na rede CAN do EcoMauá
   */
  typedef enum EcoCanDevicesIds {
    E_MOTOR_CONTROLLER_DEVID = 0, /* Controlador do motor principal */
    E_PILOT_SCREEN_DEVID,         /* Tela de informações do piloto */
    E_DEVICES_ID_COUNT
  } TEcoCanDevicesIds;

  /**
   * @brief Definição do tipo para enumeração dos tipos de mensagem CAN do EcoMauá
   */
  typedef enum EcoCanMessageTypes {
    E_TELEMETRY_FLOAT_DATA_MSG = 0, /* Mensagem de telemetria com dados float (4-bytes) */
    E_MESSAGES_TYPE_COUNT
  } TEcoCanMessageTypes;

  /**
   * @brief Definição do tipo para enumeração dos identificadores de dados por CAN do EcoMauá
   */
  typedef enum EcoCanDataIds {
    E_THROTTLE_PERCENT_ID = 0, /* Throttle [%] */
    E_MOTOR_SPEED_RPM_ID,      /* Motor Speed [RPM] */
    E_VEHICLE_SPEED_KMH_ID,    /* Vehicle Speed [km/h] */
    E_DISTANCE_M_ID,           /* Distance [m] */
    E_VOLTAGE_V_ID,            /* Voltage [V] */
    E_CURRENT_A_ID,            /* Current [A] */
    E_POWER_W_ID,              /* Power [W] */
    E_ENERGY_J_ID,             /* Energy [J] */
    E_AUTONOMY_KMKWH_ID,       /* Automony [km/kWh] */
    E_DATA_ID_COUNT
  } TEcoCanDataIds;

  /**
   * @brief Definição do tipo para recebimento de dados de telemetria por CAN do EcoMauá
   */
  typedef struct EcoCanTelemetryData {
    float fThrottlePercent; /* Throttle [%] */
    float fMotorSpeedRpm;   /* Motor Speed [RPM] */
    float fVehicleSpeedkmH; /* Vehicle Speed [km/h] */
    float fDistanceM;       /* Distance [m] */
    float fVoltageV;        /* Voltage [V] */
    float fCurrentA;        /* Current [A] */
    float fPowerW;          /* Power [W] */
    float fEnergyJ;         /* Energy [J] */
    float fAutonomyKmKwH;   /* Automony [km/kWh] */
  } TEcoCanTelemetryData;

  /**
   * @brief Construtor
   * @param CU16_DEVICE_CAN_ADDR Endereço do dispositivo na rede CAN
   * @param CU8_MCP2515_CS_PIN Pino de Chip Select do modulo MCP2515
   * @param CU8_MCP2515_INT_PIN Pino de Interrupção do modulo MCP2515
   */
  explicit CEcoMcp2515CanCtrl(const uint16_t CU16_DEVICE_CAN_ADDR, const uint8_t CU8_MCP2515_CS_PIN, const uint8_t CU8_MCP2515_INT_PIN, SPIClass *pxSpi = nullptr);

  /**
   * @brief Construtor de cópia
   * @param CX_OTHER Outro objeto da classe CEcoMcp2515CanCtrl
   */
  CEcoMcp2515CanCtrl(const CEcoMcp2515CanCtrl& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CEcoMcp2515CanCtrl();

  /**
   * @brief Inicializa e configura o modulo CAN MCP2515
   * @param eCanSpeed Velocidade para as mensagens na rede CAN
   */
  void begin(enum CAN_SPEED eCanSpeed);

  /**
   * @brief Escreve uma mensagem CAN com dados do tipo float (4-bytes)
   * @param u16TargetCanAddr Endereço do dispositivo alvo na rede CAN
   * @param u8DataId Identificados do dado da mensagem CAN
   * @param fMsgData Dado da mensagem CAN
   */
  inline void vWriteCanMsgDataFloat(uint16_t u16TargetCanAddr, uint8_t u8DataId, float fMsgData);

  /**
   * @brief Realiza o recebimento e parsing de uma mensagem CAN recebida pelo dispositivo
   * @param pxEcoCanTelemetryData Ponteiro para a estrutura de dados de telemetria por CAN do EcoMauá
   * @return True se recebeu um dado válido, Falso caso o contrário
   */
  inline bool bReceiveCanMsgData(TEcoCanTelemetryData* pxEcoCanTelemetryData);

private:

  union unFloatBytes {
    float fValue;
    uint8_t u8Bytes[sizeof(float)];
  }; /**< Union para converter float em bytes e vice-versa */

  inline void _vCalculateCanMessageCrc(struct can_frame *pxCanMsg); /**< Calcula o CRC da mensagem CAN */

  static CEcoMcp2515CanCtrl* _pxInstance; /**< Instância única da classe */

  const uint16_t _CU16_DEVICE_CAN_ADDR; /**< Endereço do dispositivo na rede CAN (11-bits)*/
  const uint8_t _CU8_MCP2515_INT_PIN;   /**< Pino de Interrupção do modulo MCP2515 */

  MCP2515 xMcp2515;           /**< Instância da classe MCP2515 para o controlador CAN */
  struct can_frame xTxCanMsg; /**< Struct para transmissão de mensagem CAN */
  struct can_frame xRxCanMsg; /**< Struct para recebimento de mensagem CAN */
};

#endif
