/**
 * @file libUnidirEncoderAlt.h
 * @brief Biblioteca para encoder unidirecional.
 * @details Esta biblioteca foi desenvolvida para lidar com um encoder unidirecional usando interrupções em uma plataforma Arduino.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-06
 * @license MIT
 */

#ifndef LIBUNIDIRENCODERALT_H
#define LIBUNIDIRENCODERALT_H

#include <Arduino.h>

/**
 * @class CUnidirEncoderAlt
 * @brief Classe para operar um encoder unidirecional
 */
class CUnidirEncoderAlt {
public:

  /**
   * @brief Definição do tipo para o delta de pulsos e tempo
   */
  typedef struct PulsesDelta {
    uint16_t u16EncPulsesDeltaCnt;    /* Delta de pulsos */
    uint32_t u32EncPulsesDeltaTimeUs; /* Delta de tempo [us] */
  } TPulsesDelta;

  /**
   * @brief Construtor
   * @param CU8_ENCODER_PIN Pino de entrada do encoder
   * @param CI8_INTERRUPT_MODE Modo da interrupção do encoder
   * @param CF_PULSES_PER_REV Pulsos por revolução do encoder
   */
  explicit CUnidirEncoderAlt(const uint8_t CU8_ENCODER_PIN, const int8_t CI8_INTERRUPT_MODE, const float CF_PULSES_PER_REV);

  /**
   * @brief Construtor de cópia
   * @param CX_OTHER Outro objeto da classe CUnidirEncoderAlt
   */
  CUnidirEncoderAlt(const CUnidirEncoderAlt& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CUnidirEncoderAlt();

  /**
   * @brief Inicializa e configura o encoder
   */
  void begin(void);

  /**
   * @brief Habilita o encoder e suas interrupções
   */
  void enableEncoder(void);

  /**
   * @brief Desabilita o encoder e suas interrupções
   */
  void disableEncoder(void);

  /**
   * @brief Retorna a contagem atual de pulsos do encoder
   * @return Contagem de pulsos
   */
  inline uint16_t u16GetEncPulsesCnt(void) const;

  /**
   * @brief Retorna a diferença de pulsos desde a última chamada (opcionalmente atualiza a contagem)
   * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
   * @return Diferença de pulsos e tempo
   */
  TPulsesDelta xGetEncPulsesDelta(bool bUpdateLastCnt = true) const;

  /**
   * @brief Retorna a contagem de revoluções
   * @return Contagem de revoluções
   */
  inline float fGetRevolutionCount(void) const;

  /**
   * @brief Retorna o ângulo em radianos
   * @return Ângulo em radianos
   */
  inline float fGetAngleRad(void) const;

  /**
   * @brief Retorna a velocidade em RPM (rotações por minuto)
   * @param fDeltaTimeS Intervalo de tempo em segundos
   * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
   * @return Velocidade em RPM
   */
  float fGetSpeedRpm(float fDeltaTimeS, bool bUpdateLastCnt = true) const;

  /**
   * @brief Retorna a velocidade em radianos por segundo
   * @param fDeltaTimeS Intervalo de tempo em segundos
   * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
   * @return Velocidade em radianos por segundo
   */
  float fGetSpeedRadS(float fDeltaTimeS, bool bUpdateLastCnt = true) const;

private:
  inline void _clearEncPulsesCnts(void);              /**< @brief Limpa as contagens de pulsos do encoder */
  static void _encPulsesIsr(void);                    /**< @brief Função de interrupção para pulsos do encoder */

  const uint8_t _CU8_ENCODER_PIN;                     /**< @brief Pino de entrada do encoder */
  const int8_t _CI8_INTERRUPT_MODE;                   /**< @brief Modo da interrupção do encoder */
  const float _CF_PULSES_PER_REV;                     /**< @brief Pulsos por revolução do encoder */
  
  mutable volatile uint16_t _vu16EncPulsesCnt;        /**< @brief Contador atual de pulsos do encoder */
  mutable volatile uint16_t _vu16EncPulsesLastCnt;    /**< @brief Contador anterior de pulsos do encoder */
  mutable volatile uint32_t _vu32EncPulsesTimeUs;     /**< @brief Tempo atual de pulsos do encoder [us] */
  mutable volatile uint32_t _vu32EncPulsesLastTimeUs; /**< @brief Tempo anterior de pulsos do encoder [us] */

  static CUnidirEncoderAlt* _pxInstance;              /**< @brief Instância única da classe */
};

#endif
