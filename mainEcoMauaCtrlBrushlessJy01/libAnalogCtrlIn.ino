/**
 * @file libAnalogCtrlIn.ino
 * @brief Implementação da biblioteca para leitura de entradas analógicas de controles.
 * @details Essa implementação inclui as funções para leitura e processamento de entradas analógicas de controles, permitindo o acesso aos dados brutos e à porcentagem de ativação.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-03
 * @license MIT License
 */

#include "libAnalogCtrlIn.h"

/**
 * @brief Construtor para a classe CAnalogCtrlIn
 */
CAnalogCtrlIn::CAnalogCtrlIn(const uint8_t CU8_ANALOG_PIN, const uint16_t CU16_LOWER_THRESHOLD, const uint16_t CU16_UPPER_THRESHOLD, const uint8_t CU8_OUTPUT_BITS)
  : _CU8_ANALOG_PIN(CU8_ANALOG_PIN), _CU16_LOWER_THRESHOLD(CU16_LOWER_THRESHOLD), _CU16_UPPER_THRESHOLD(CU16_UPPER_THRESHOLD),
    _CU8_DECIMATION_FACTOR(CU8_OUTPUT_BITS > _CU8_DEFAULT_ADC_BITS ? static_cast<uint8_t>(min(CU8_OUTPUT_BITS - _CU8_DEFAULT_ADC_BITS, 6u)) : 0u) {}

/**
 * @brief Construtor de cópia para a classe CAnalogCtrlIn
 */
CAnalogCtrlIn::CAnalogCtrlIn(const CAnalogCtrlIn& CX_OTHER)
  : _CU8_ANALOG_PIN(CX_OTHER._CU8_ANALOG_PIN), _CU16_LOWER_THRESHOLD(CX_OTHER._CU16_LOWER_THRESHOLD), _CU16_UPPER_THRESHOLD(CX_OTHER._CU16_UPPER_THRESHOLD),
    _CU8_DECIMATION_FACTOR(CX_OTHER._CU8_DECIMATION_FACTOR) {}

/**
 * @brief Destrutor para a classe CAnalogCtrlIn
 */
CAnalogCtrlIn::~CAnalogCtrlIn() {}

/**
 * @brief Inicializa o pino de entrada analógica
 */
void CAnalogCtrlIn::begin(void) {
  pinMode(_CU8_ANALOG_PIN, INPUT);
}

/**
 * @brief Obtém o valor bruto do controle analógico
 * @return Valor bruto do controle analógico
 */
uint16_t CAnalogCtrlIn::u16GetControlRaw(void) const {
  uint32_t u32AnalogReadAcumulator = 0;
  uint16_t u16AnalogReadDecimated;
  uint16_t u16AnalogReadCnt;

  /* Se o fator de decimação é 0, faz uma leitura direta */
  if (_CU8_DECIMATION_FACTOR == 0u) {
    u16AnalogReadDecimated = static_cast<uint16_t>(analogRead(_CU8_ANALOG_PIN));
  } else {
    /* Caso contrário, acumula múltiplas leituras e aplica a decimação */
    for (u16AnalogReadCnt = 0u; u16AnalogReadCnt < (1u << (2u * _CU8_DECIMATION_FACTOR)); u16AnalogReadCnt++) {
      u32AnalogReadAcumulator += static_cast<uint32_t>(analogRead(_CU8_ANALOG_PIN));
    }
    u16AnalogReadDecimated = static_cast<uint16_t>(u32AnalogReadAcumulator >> _CU8_DECIMATION_FACTOR);
  }

  /* Ajusta os limites e mapeia a leitura para o intervalo desejado */
  if (u16AnalogReadDecimated < _CU16_LOWER_THRESHOLD) {
    u16AnalogReadDecimated = 0u;
  } else if (u16AnalogReadDecimated > _CU16_UPPER_THRESHOLD) {
    u16AnalogReadDecimated = static_cast<uint16_t>((1u << (_CU8_DEFAULT_ADC_BITS + _CU8_DECIMATION_FACTOR)) - 1u);
  } else {
    u16AnalogReadDecimated = map(u16AnalogReadDecimated, _CU16_LOWER_THRESHOLD, _CU16_UPPER_THRESHOLD, 0u, (1u << (_CU8_DEFAULT_ADC_BITS + _CU8_DECIMATION_FACTOR)) - 1u);
  }

  return (u16AnalogReadDecimated);
}

/**
 * @brief Retorna a porcentagem de controle do valor analógico lido
 * @return Porcentagem de controle do valor analógico lido
 */
float CAnalogCtrlIn::fGetControlPercent(void) const {
  uint16_t u16AnalogControlRaw;
  float fAnalogPercent;

  /* Obtém o valor bruto do controle analógico */
  u16AnalogControlRaw = u16GetControlRaw();

  /* Calcula a porcentagem do controle analógico */
  fAnalogPercent = static_cast<float>(u16AnalogControlRaw) / static_cast<float>((1u << (_CU8_DEFAULT_ADC_BITS + _CU8_DECIMATION_FACTOR)) - 1u);

  return (fAnalogPercent);
}
