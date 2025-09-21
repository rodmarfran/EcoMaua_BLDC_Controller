/**
 * @file libJy01BrushlessCtrl.ino
 * @brief Implementação da biblioteca para o controlador de motores brushless JY01.
 * @details Esta implementação inclui as funções necessárias para controlar motores brushless usando o controlador JY01 com a biblioteca libJy01BrushlessCtrl.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-06
 * @license MIT License
 */

#include "libJy01BrushlessCtrl.h"

/**
 * @brief Construtor para a classe CJy01BrushlessCtrl
 * @param CU8_EL_PIN Pino de enable do controlador JY01
 * @param CU8_ZF_PIN Pino de direção do controlador JY01
 * @param CU8_M_PIN Pino do encoder de velocidade do controlador JY01
 * @param CF_PULSES_PER_REV Pulses por revolução do encoder
 */
CJy01BrushlessCtrl::CJy01BrushlessCtrl(const uint8_t CU8_EL_PIN, const uint8_t CU8_ZF_PIN, const uint8_t CU8_M_PIN, const float CF_PULSES_PER_REV)
  : _CU8_EL_PIN(CU8_EL_PIN), _CU8_ZF_PIN(CU8_ZF_PIN), xMEncoder(CU8_M_PIN, RISING, CF_PULSES_PER_REV) {}

/**
 * @brief Construtor de cópia para a classe CJy01BrushlessCtrl
 * @param CX_OTHER Outro objeto da classe CJy01BrushlessCtrl
 */
CJy01BrushlessCtrl::CJy01BrushlessCtrl(const CJy01BrushlessCtrl& CX_OTHER)
  : _CU8_EL_PIN(CX_OTHER._CU8_EL_PIN), _CU8_ZF_PIN(CX_OTHER._CU8_ZF_PIN), xMEncoder(CX_OTHER.xMEncoder) {}

/**
 * @brief Destrutor para a classe CJy01BrushlessCtrl
 */
CJy01BrushlessCtrl::~CJy01BrushlessCtrl() {
  /* Desabilita o controle do motor ao destruir o objeto */
  disableDrive();
}

/**
 * @brief Inicializa o controlador do motor JY01
 * @param u8DacI2cAddr Endereço I2C do DAC MCP4725
 */
void CJy01BrushlessCtrl::begin(uint8_t u8DacI2cAddr) {
  pinMode(_CU8_EL_PIN, OUTPUT);
  pinMode(_CU8_ZF_PIN, OUTPUT);
  xMEncoder.begin();
  _xMcp4725Dac.begin(u8DacI2cAddr);
  _xMcp4725Dac.setVoltage(0, true, _CU32_DAC_I2C_FREQ);
  enableDrive();
  setFowardDir();
}

/**
 * @brief Ativa o controle do motor brushless
 */
void CJy01BrushlessCtrl::enableDrive(void) {
  setControlRaw(0u);
  xMEncoder.enableEncoder();
  digitalWrite(_CU8_EL_PIN, HIGH);
}

/**
 * @brief Desativa o controle do motor brushless
 */
void CJy01BrushlessCtrl::disableDrive(void) {
  digitalWrite(_CU8_EL_PIN, LOW);
  xMEncoder.disableEncoder();
  setControlRaw(0u);
}

/**
 * @brief Define a direção do motor como para frente
 */
inline void CJy01BrushlessCtrl::setFowardDir(void) const {
  digitalWrite(_CU8_ZF_PIN, HIGH);
}

/**
 * @brief Define a direção do motor como para trás
 */
inline void CJy01BrushlessCtrl::setReverseDir(void) const {
  digitalWrite(_CU8_ZF_PIN, LOW);
}

/**
 * @brief Ajusta o controle do motor com um valor bruto (0-4095)
 * @param u16ControlRaw Valor bruto de controle do motor
 */
void CJy01BrushlessCtrl::setControlRaw(uint16_t u16ControlRaw) {

  /* Se o valor de controle excede o limite máximo (4095), limita o valor de controle a 4095 */
  if (u16ControlRaw > _CU16_MAX_CTRL_RAW_VALUE) {
    _u16ControlRaw = _CU16_MAX_CTRL_RAW_VALUE;
  } else {
    /* Caso contrário, atualiza o valor de controle com o valor fornecido */
    _u16ControlRaw = u16ControlRaw;
  }

  /* Define a tensão do DAC MCP4725 com base no valor de controle */
  _xMcp4725Dac.setVoltage(_u16ControlRaw, false, _CU32_DAC_I2C_FREQ);
}

/**
 * @brief Ajusta o controle do motor com um valor percentual (0%-100%)
 * @param fControlPercent Valor percentual de controle do motor
 */
void CJy01BrushlessCtrl::setControlPercent(float fControlPercent) {

  /* Se o valor de controle excede o limite máximo (1.0), limita o valor de controle a 1.0 */
  if (fControlPercent > 1.0f) {
    fControlPercent = 1.0f;
    /* Caso contrário, se o valor de controle é menor que o limite mínimo (0.0), limita o valor de controle a 0.0 */
  } else if (fControlPercent < 0.0f) {
    fControlPercent = 0.0f;
  }

  /* Converte o valor percentual para um valor bruto (0-4095) e define a tensão do DAC MCP4725 */
  _u16ControlRaw = static_cast<uint16_t>(fControlPercent * 4095.0f);
  _xMcp4725Dac.setVoltage(_u16ControlRaw, false, _CU32_DAC_I2C_FREQ);
}
