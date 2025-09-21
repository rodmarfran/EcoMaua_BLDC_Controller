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
 * @param CU8_ANALOG_PIN Pino analógico do controle
 * @param CU16_LOWER_THRESHOLD Limiar inferior (default 0)
 * @param CU16_UPPER_THRESHOLD Limiar superior (default 1023)
 * @param CF_CHANGE_RATE_PERCENT Taxa de variação percentual máxima do controle (default 1.0)
 * @param CF_MAX_CTRL_PERCENT Valor percentual máximo do controle (default 1.0)
 * @param CU8_OUTPUT_BITS Quantidade de bits na saída (default 10)
 */
CAnalogCtrlIn::CAnalogCtrlIn(const uint8_t CU8_ANALOG_PIN, const uint16_t CU16_LOWER_THRESHOLD, const uint16_t CU16_UPPER_THRESHOLD, const float CF_CHANGE_RATE_PERCENT, const float CF_MAX_CTRL_PERCENT, const uint8_t CU8_OUTPUT_BITS)
  : _CU8_ANALOG_PIN(CU8_ANALOG_PIN), _CU16_LOWER_THRESHOLD(CU16_LOWER_THRESHOLD), _CU16_UPPER_THRESHOLD(CU16_UPPER_THRESHOLD),
    _CU16_CHANGE_RATE_RAW(static_cast<uint16_t>(((1u << (CU8_OUTPUT_BITS)) - 1u) * CF_CHANGE_RATE_PERCENT)), _CU16_MAX_CTRL_RAW(static_cast<uint16_t>(((1u << (CU8_OUTPUT_BITS)) - 1u) * CF_MAX_CTRL_PERCENT)), 
    _CU8_DECIMATION_FACTOR(CU8_OUTPUT_BITS > _CU8_DEFAULT_ADC_BITS ? static_cast<uint8_t>(min(CU8_OUTPUT_BITS - _CU8_DEFAULT_ADC_BITS, 6u)) : 0u) {}

/**
 * @brief Construtor de cópia para a classe CAnalogCtrlIn
 */
CAnalogCtrlIn::CAnalogCtrlIn(const CAnalogCtrlIn& CX_OTHER)
  : _CU8_ANALOG_PIN(CX_OTHER._CU8_ANALOG_PIN), _CU16_LOWER_THRESHOLD(CX_OTHER._CU16_LOWER_THRESHOLD), _CU16_UPPER_THRESHOLD(CX_OTHER._CU16_UPPER_THRESHOLD),
    _CU16_CHANGE_RATE_RAW(CX_OTHER._CU16_CHANGE_RATE_RAW), _CU16_MAX_CTRL_RAW(CX_OTHER._CU16_MAX_CTRL_RAW), _CU8_DECIMATION_FACTOR(CX_OTHER._CU8_DECIMATION_FACTOR) {}

/**
 * @brief Destrutor para a classe CAnalogCtrlIn
 */
CAnalogCtrlIn::~CAnalogCtrlIn() {}

/**
 * @brief Inicializa o pino de entrada analógica
 */
void CAnalogCtrlIn::begin(void) {
  pinMode(_CU8_ANALOG_PIN, INPUT);
  u16LastControlRaw = 0;
}

/**
 * @brief Obtém o valor bruto do controle analógico
 * @return Valor bruto do controle analógico
 */
uint16_t CAnalogCtrlIn::u16GetControlRaw(void) {
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

  /* Modifica o controle de acordo com as restrições do sistema, exceto se o controle for 0 */
  if (u16AnalogReadDecimated != 0u) {

    /* Aplica a taxa máxima de variação do controle */
    int16_t i16ControlDiff = u16AnalogReadDecimated - u16LastControlRaw;
    if (i16ControlDiff > (int16_t)(_CU16_CHANGE_RATE_RAW)) {
      u16AnalogReadDecimated = u16LastControlRaw + _CU16_CHANGE_RATE_RAW;
    } else if (i16ControlDiff < (int16_t)(-_CU16_CHANGE_RATE_RAW)) {
      u16AnalogReadDecimated = u16LastControlRaw - _CU16_CHANGE_RATE_RAW;
    }

    /* Atualiza o último controle com o atual */
    u16LastControlRaw = u16AnalogReadDecimated;

    /* Ajusta a saída de acordo com o valor de controle máximo */
    u16AnalogReadDecimated = map(u16AnalogReadDecimated, 0u, (1u << (_CU8_DEFAULT_ADC_BITS + _CU8_DECIMATION_FACTOR)) - 1u, 0u, _CU16_MAX_CTRL_RAW);

  } else {
    u16LastControlRaw = 0;
  }

  return (u16AnalogReadDecimated);
}

/**
 * @brief Retorna a porcentagem de controle do valor analógico lido
 * @return Porcentagem de controle do valor analógico lido
 */
float CAnalogCtrlIn::fGetControlPercent(void) {
  uint16_t u16AnalogControlRaw;
  float fAnalogPercent;

  /* Obtém o valor bruto do controle analógico */
  u16AnalogControlRaw = u16GetControlRaw();

  /* Calcula a porcentagem do controle analógico */
  fAnalogPercent = static_cast<float>(u16AnalogControlRaw) / static_cast<float>((1u << (_CU8_DEFAULT_ADC_BITS + _CU8_DECIMATION_FACTOR)) - 1u);

  return (fAnalogPercent);
}


