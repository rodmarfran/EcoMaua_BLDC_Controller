/**
 * @file libEcoMcp2515CanCtrl.ino
 * @brief Implementação da biblioteca para controlador CAN MCP2515 do EcoMauá.
 * @details Esta implementação fornece a lógica necessária para gerenciar o controlador CAN MCP2515 no veículo da equipe EcoMauá.
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

#include "libEcoMcp2515CanCtrl.h"

/**
 * @brief Inicializa a instância única da classe como nullptr
 */
CEcoMcp2515CanCtrl* CEcoMcp2515CanCtrl::_pxInstance = nullptr;

/**
 * @brief Construtor
 * @param CU16_DEVICE_CAN_ADDR Endereço do dispositivo na rede CAN
 * @param CU8_MCP2515_CS_PIN Pino de Chip Select do modulo MCP2515
 * @param CU8_MCP2515_INT_PIN Pino de Interrupção do modulo MCP2515
 */
CEcoMcp2515CanCtrl::CEcoMcp2515CanCtrl(const uint16_t CU16_DEVICE_CAN_ADDR, const uint8_t CU8_MCP2515_CS_PIN, const uint8_t CU8_MCP2515_INT_PIN)
  : _CU16_DEVICE_CAN_ADDR(CU16_DEVICE_CAN_ADDR), _CU8_MCP2515_INT_PIN(CU8_MCP2515_INT_PIN), xMcp2515(CU8_MCP2515_CS_PIN) {}

/**
 * @brief Construtor de cópia
 * @param CX_OTHER Outro objeto da classe CEcoMcp2515CanCtrl
 */
CEcoMcp2515CanCtrl::CEcoMcp2515CanCtrl(const CEcoMcp2515CanCtrl& CX_OTHER)
  : _CU16_DEVICE_CAN_ADDR(CX_OTHER._CU16_DEVICE_CAN_ADDR), _CU8_MCP2515_INT_PIN(CX_OTHER._CU8_MCP2515_INT_PIN), xMcp2515(CX_OTHER.xMcp2515) {}

/**
 * @brief Destrutor
 */
CEcoMcp2515CanCtrl::~CEcoMcp2515CanCtrl() {
  _pxInstance = nullptr;
}

/**
 * @brief Inicializa e configura o modulo CAN MCP2515
 * @param eCanSpeed Velocidade para as mensagens na rede CAN
 */
void CEcoMcp2515CanCtrl::begin(enum CAN_SPEED eCanSpeed) {
  	
  xMcp2515.reset();
  xMcp2515.setBitrate(eCanSpeed);
  xMcp2515.setNormalMode();
  _pxInstance = this;
}

/**
 * @brief Escreve uma mensagem CAN com dados do tipo float (4-bytes)
 * @param u16TargetCanAddr Endereço do dispositivo alvo na rede CAN
 * @param u8DataId Identificados do dado da mensagem CAN
 * @param fMsgData Dado da mensagem CAN
 */
void CEcoMcp2515CanCtrl::vWriteCanMsgDataFloat(uint16_t u16TargetCanAddr, uint8_t u8DataId, float fMsgData) {

  /*
   * - CAN Identifier (1 byte):
   *   -- Tatget Address (1 byte)
   * - CAN Data (8 bytes):
   *   -- Message Type (1 byte)
   *   -- Source Address (1 byte)
   *   -- Data Identification (1 byte)
   *   -- Data Byte #0 (1 byte)
   *   -- Data Byte #1 (1 byte)
   *   -- Data Byte #2 (1 byte)
   *   -- Data Byte #3 (1 byte)
   *   -- CRC (1 byte)
   */
 
  union unFloatBytes uxFloatBytes;
  uxFloatBytes.fValue = fMsgData;
	
  xTxCanMsg.can_id  = u16TargetCanAddr;
  
  xTxCanMsg.can_dlc = 8;
  xTxCanMsg.data[0] = static_cast<uint8_t>(E_TELEMETRY_FLOAT_DATA_MSG);
  xTxCanMsg.data[1] = _CU16_DEVICE_CAN_ADDR;
  xTxCanMsg.data[2] = u8DataId;
  xTxCanMsg.data[3] = uxFloatBytes.u8Bytes[0];
  xTxCanMsg.data[4] = uxFloatBytes.u8Bytes[1];
  xTxCanMsg.data[5] = uxFloatBytes.u8Bytes[2];
  xTxCanMsg.data[6] = uxFloatBytes.u8Bytes[3];
  
  _vCalculateCanMessageCrc(&xTxCanMsg);
  
  xMcp2515.sendMessage(&xTxCanMsg);
  
}

/**
 * @brief Realiza o recebimento e parsing de uma mensagem CAN recebida pelo dispositivo
 * @param pxEcoCanTelemetryData Ponteiro para a estrutura de dados de telemetria por CAN do EcoMauá
 * @return True se recebeu um dado válido, Falso caso o contrário
 */
bool CEcoMcp2515CanCtrl::bReceiveCanMsgData(TEcoCanTelemetryData *pxEcoCanTelemetryData) {
	
  /*
   * - CAN Identifier (1 byte):
   *   -- Tatget Address (1 byte)
   * - CAN Data (8 bytes):
   *   -- Message Type (1 byte)
   *   -- Source Address (1 byte)
   *   -- Data Identification (1 byte)
   *   -- Data Byte #0 (1 byte)
   *   -- Data Byte #1 (1 byte)
   *   -- Data Byte #2 (1 byte)
   *   -- Data Byte #3 (1 byte)
   *   -- CRC (1 byte)
   */

  bool bCanMessageValid = false;
  uint8_t u8RxCanMessageCrc = 0;
  union unFloatBytes uxFloatBytes;

  /* Verifica se existe alguma mensagem CAN no buffer do MCP2515 */
  if (MCP2515::ERROR_OK == xMcp2515.readMessage(&xRxCanMsg)) {
	  
	/* Verifica se a mensagem era para esse dispositivo */
    if (_CU16_DEVICE_CAN_ADDR == xRxCanMsg.can_id) {
		
	  /* Verifica o CRC da mensagem CAN recebida */
	  u8RxCanMessageCrc = xRxCanMsg.data[7];
	  _vCalculateCanMessageCrc(&xRxCanMsg);
	  if (u8RxCanMessageCrc == xRxCanMsg.data[7]) {
        bCanMessageValid = true;
	  
	    /* Tratamento para os diferentes tipos de mensagens CAN */
	    switch (xRxCanMsg.data[0]) {
          
		  /* Mensagem de telemetria com dados float (4-bytes) */
          case (E_TELEMETRY_FLOAT_DATA_MSG):
            uxFloatBytes.u8Bytes[0] = xRxCanMsg.data[3];
            uxFloatBytes.u8Bytes[1] = xRxCanMsg.data[4];
            uxFloatBytes.u8Bytes[2] = xRxCanMsg.data[5];
            uxFloatBytes.u8Bytes[3] = xRxCanMsg.data[6];
            /* Tratamento para os diferentes tipos de dados de telemetria por CAN */
            switch (xRxCanMsg.data[2]) {
              case (E_THROTTLE_PERCENT_ID):  pxEcoCanTelemetryData->fThrottlePercent = uxFloatBytes.fValue; break; /* Throttle [%] */
              case (E_MOTOR_SPEED_RPM_ID):   pxEcoCanTelemetryData->fMotorSpeedRpm   = uxFloatBytes.fValue; break; /* Motor Speed [RPM] */
              case (E_VEHICLE_SPEED_KMH_ID): pxEcoCanTelemetryData->fVehicleSpeedkmH = uxFloatBytes.fValue; break; /* Vehicle Speed [km/h] */
              case (E_DISTANCE_M_ID):        pxEcoCanTelemetryData->fDistanceM       = uxFloatBytes.fValue; break; /* Distance [m] */
              case (E_VOLTAGE_V_ID):         pxEcoCanTelemetryData->fVoltageV        = uxFloatBytes.fValue; break; /* Voltage [V] */
              case (E_CURRENT_A_ID):         pxEcoCanTelemetryData->fCurrentA        = uxFloatBytes.fValue; break; /* Current [A] */
              case (E_POWER_W_ID):           pxEcoCanTelemetryData->fPowerW          = uxFloatBytes.fValue; break; /* Power [W] */
              case (E_ENERGY_J_ID):          pxEcoCanTelemetryData->fEnergyJ         = uxFloatBytes.fValue; break; /* Energy [J] */
              case (E_AUTONOMY_KMKWH_ID):    pxEcoCanTelemetryData->fAutonomyKmKwH   = uxFloatBytes.fValue; break; /* Automony [km/kWh] */
		  	  default: bCanMessageValid = false;
            }
		    break;
		  
          default:
		    bCanMessageValid = false;
	    }
	  }	 
	}
  }

  return (bCanMessageValid);
}

/**
 * @brief Calcula o CRC da mensagem CAN
 * @param pxCanMsg Ponteiro para a Mensagem CAN
 */
void CEcoMcp2515CanCtrl::_vCalculateCanMessageCrc(struct can_frame *pxCanMsg) {
  uint8_t u8CanMsgCrc = 0;
  u8CanMsgCrc = (pxCanMsg->data[0]) ^ (pxCanMsg->data[1]) ^ (pxCanMsg->data[2]) ^ (pxCanMsg->data[3]) ^ (pxCanMsg->data[4]) ^ (pxCanMsg->data[5]) ^ (pxCanMsg->data[6]);
  pxCanMsg->data[7] = u8CanMsgCrc;
}
