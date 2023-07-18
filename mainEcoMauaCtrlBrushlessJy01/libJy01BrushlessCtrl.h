/**
 * @file libJy01BrushlessCtrl.h
 * @brief Biblioteca para o controlador de motores brushless JY01.
 * @details Esta biblioteca simplifica o controle de motores brushless usando o controlador JY01, permitindo ajustar a velocidade e direção do motor de forma fácil e eficiente.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-06
 * @license MIT License
 */

#ifndef LIBJY01BRUSHLESSCTRL_H
#define LIBJY01BRUSHLESSCTRL_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include "libUnidirEncoder.h"

/**
 * @class CJy01BrushlessCtrl
 * @brief Classe para utilização do controlador de motores brushless JY01
 */
class CJy01BrushlessCtrl {
public:
  /**
   * @brief Construtor
   * @param CU8_EL_PIN Pino de enable do controlador JY01
   * @param CU8_ZF_PIN Pino de direção do controlador JY01
   * @param CU8_M_PIN Pino do encoder de velocidade do controlador JY01
   * @param CF_PULSES_PER_REV Pulses por revolução do encoder
   */
  explicit CJy01BrushlessCtrl(const uint8_t CU8_EL_PIN, const uint8_t CU8_ZF_PIN, const uint8_t CU8_M_PIN, const float CF_PULSES_PER_REV);

  /**
   * @brief Construtor de cópia
   * @param CX_OTHER Outro objeto da classe CJy01BrushlessCtrl
   */
  CJy01BrushlessCtrl(const CJy01BrushlessCtrl& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CJy01BrushlessCtrl();

  /**
   * @brief Inicializa os pinos e bibliotecas necessários
   * @param u8DacI2cAddr Endereço I2C do DAC MCP4725 (padrão: 0x60)
   */
  void begin(uint8_t u8DacI2cAddr = 0x60u);

  /**
   * @brief Habilita o motor brushless e o encoder de velocidade
   */
  void enableDrive(void);

  /**
   * @brief Desabilita o motor brushless e o encoder de velocidade
   */
  void disableDrive(void);

  /**
   * @brief Define a direção do motor como para a frente
   */
  inline void setFowardDir(void) const;

  /**
   * @brief Define a direção do motor como para trás
   */
  inline void setReverseDir(void) const;

  /**
   * @brief Ajusta o controle do motor com um valor bruto (0-4095)
   * @param u16ControlRaw Valor bruto de controle do motor
   */
  void setControlRaw(uint16_t u16ControlRaw);

  /**
   * @brief Ajusta o controle do motor com um valor percentual (0%-100%)
   * @param fControlPercent Valor percentual de controle do motor
   */
  void setControlPercent(float fControlPercent);

  /**
   * @brief Instância da classe CUnidirEncoder para acesso ao encoder de velocidade
   */
  CUnidirEncoder xMEncoder;

private:
  static const uint16_t _CU16_MAX_CTRL_RAW_VALUE = 4095u; /**< Valor máximo para o controle bruto do motor (resolução do DAC MCP4725) */
  static const uint32_t _CU32_DAC_I2C_FREQ = 100000ul;    /**< Frequência I2C para comunicação com o DAC MCP4725 */
  const uint8_t _CU8_EL_PIN;                              /**< Pino de enable do controlador JY01 */
  const uint8_t _CU8_ZF_PIN;                              /**< Pino de direção do controlador JY01 */
  uint16_t _u16ControlRaw;                                /**< Valor bruto de controle do motor atual */
  Adafruit_MCP4725 _xMcp4725Dac;                          /**< Instância da classe Adafruit_MCP4725 para controlar o DAC */
};

#endif
