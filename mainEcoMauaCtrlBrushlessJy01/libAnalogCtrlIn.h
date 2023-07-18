/**
 * @file libAnalogCtrlIn.h
 * @brief Biblioteca para leitura de entradas analógicas de controles.
 * @details Essa biblioteca permite a leitura de entradas analógicas de controles, com opções de limiares inferior e superior, e resolução de saída ajustável.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-03
 * @license MIT License
 */

#ifndef LIBANALOGCTRLIN_H
#define LIBANALOGCTRLIN_H

#include <Arduino.h>

/** 
 * @class CAnalogCtrlIn
 * @brief Classe para leitura de entrada analógica de controles
 */
class CAnalogCtrlIn {
public:
  /** 
   * @brief Construtor
   * @param CU8_ANALOG_PIN Pino analógico do controle
   * @param CU16_LOWER_THRESHOLD Limiar inferior (default 0)
   * @param CU16_UPPER_THRESHOLD Limiar superior (default 1023)
   * @param CU8_OUTPUT_BITS Quantidade de bits na saída (default 10)
   */
  explicit CAnalogCtrlIn(const uint8_t CU8_ANALOG_PIN, const uint16_t CU16_LOWER_THRESHOLD = 0u, const uint16_t CU16_UPPER_THRESHOLD = 1023u, const uint8_t CU8_OUTPUT_BITS = 10u);

  /** 
   * @brief Construtor de cópia
   * @param CX_OTHER Objeto CAnalogCtrlIn a ser copiado
   */
  CAnalogCtrlIn(const CAnalogCtrlIn& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CAnalogCtrlIn();

  /**
   * @brief Inicializa o pino do controle
   */
  void begin(void);

  /**
   * @brief Retorna a leitura bruta do controle
   * @return Valor bruto do controle analógico
   */
  uint16_t u16GetControlRaw(void) const;

  /**
   * @brief Retorna a porcentagem de ativação do controle
   * @return Porcentagem de ativação do controle analógico
   */
  float fGetControlPercent(void) const;

private:
  static const uint8_t _CU8_DEFAULT_ADC_BITS = 10u; /**< Quantidade padrão da resolução do ADC interno */
  const uint8_t _CU8_ANALOG_PIN;                    /**< Pino analógico do controle */
  const uint16_t _CU16_LOWER_THRESHOLD;             /**< Limiar inferior */
  const uint16_t _CU16_UPPER_THRESHOLD;             /**< Limiar superior */
  const uint8_t _CU8_DECIMATION_FACTOR;             /**< Fator de decimação para aumentar a resolução da leitura */
};

#endif
