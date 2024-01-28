/**
 * @file libWheelUniEncoder.h
 * @brief Biblioteca para encoder unidirecional montado em roda.
 * @details Esta biblioteca foi desenvolvida para lidar com um encoder unidirecional montado em roda usando interrupções em uma plataforma Arduino.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-07-20
 * @license MIT
 */

#ifndef LIBWHEELUNIENCODER_H
#define LIBWHEELUNIENCODER_H

#include <Arduino.h>
#include "libUnidirEncoderAlt.h"

/**
 * @class CWheelUniEncoder
 * @brief Classe para operar um encoder unidirecional montado em roda
 */
class CWheelUniEncoder {
public:

  /**
   * @brief Construtor
   * @param CU8_ENCODER_PIN Pino de entrada do encoder
   * @param CI8_INTERRUPT_MODE Modo da interrupção do encoder
   * @param CF_PULSES_PER_REV Pulsos por revolução do encoder
   * @param CF_WHEEL_RADIUS_M Raio da roda (em metros)
   */
  explicit CWheelUniEncoder(const uint8_t CU8_ENCODER_PIN, const int8_t CI8_INTERRUPT_MODE, const float CF_PULSES_PER_REV, const float CF_WHEEL_RADIUS_M);

  /**
   * @brief Construtor de cópia
   * @param CX_OTHER Outro objeto da classe CWheelUniEncoder
   */
  CWheelUniEncoder(const CWheelUniEncoder& CX_OTHER);

  /**
   * @brief Destrutor
   */
  ~CWheelUniEncoder();

  /**
   * @brief Inicializa e configura o encoder
   */
  void begin(void);

  /**
   * @brief Habilita o encoder e suas interrupções
   */
  inline void enableEncoder(void);

  /**
   * @brief Desabilita o encoder e suas interrupções
   */
  inline void disableEncoder(void);

  /**
   * @brief Limpa a distância atual percorrida pela roda
   */
  inline void clearWhellDistance(void);

  /**
   * @brief Retorna a distância percorrida pela roda em m (metros)
   * @return Distância atual percorrida pela roda em m (metros)
   */
  inline float fGetWhellDistanceM(void) const;
  
  /**
   * @brief Retorna a distância percorrida pela roda em Km (quilômetros)
   * @return Distância atual percorrida pela roda (em quilômetros)
   */
  inline float fGetWhellDistanceKm(void) const;

  /**
   * @brief Retorna a velocidade da roda em m/s (metros por segundo)
   * @param fDeltaTimeS Intervalo de tempo em segundos
   * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
   * @return Velocidade da roda em metros por segundo
   */
  inline float fGetWhellSpeedMS(float fDeltaTimeS, bool bUpdateLastCnt = true);
  
  /**
   * @brief Retorna a velocidade média da roda em m/s (metros por segundo)
   * @return Velocidade média da roda em metros por segundo
   */
  inline float fGetWhellAvgSpeedMS(void) const;

  /**
   * @brief Retorna a velocidade da roda em km/h (quilômetros por hora)
   * @param fDeltaTimeS Intervalo de tempo em segundos
   * @param bUpdateLastCnt Atualizar a contagem (padrão: true)
   * @return Velocidade da roda em quilômetros por hora
   */
  inline float fGetWhellSpeedKmH(float fDeltaTimeS, bool bUpdateLastCnt = true);

  /**
   * @brief Retorna a velocidade média da roda em km/h (quilômetros por hora)
   * @return Velocidade média da roda em quilômetros por hora
   */
  inline float fGetWhellAvgSpeedKmH(void) const;
  
  /**
   * @brief Instância da classe CUnidirEncoder para acesso ao encoder de velocidade
   */
  CUnidirEncoderAlt xUniDirEncoder;

private:

  const float _CF_WHEEL_RADIUS_M; /**< Raio da roda (em metros) */
  float _fDistanceRevOffset;      /**< Offset de revoluções para o comando de zerar a distância */
  float _fMeasureTotalTimeS;      /**< Tempo total de medição para cálculo da velocidade média */
  
};

#endif
