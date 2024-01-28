/**
 * @file incSystemConstants.h
 * @brief Include com a definição das constantes de sistema para o projeto do controlador JY01 (equipe EcoMauá).
 * @details Este arquivo contém a definição constantes de sistema para serem usados em no projeto do controlador JY01 (equipe EcoMauá), 
 *          incluindo constantes de controle, do motor, entre outros.
 * @author Rodrigo França
 * @version 1.0.0
 * @date 2023-04-08
 * @license MIT License
 */

#ifndef INCSYSTEMCONSTANTS_H
#define INCSYSTEMCONSTANTS_H

#include <Arduino.h>

/**
 * @class CSystemConstants
 * @brief Classe para definição das constantes de sistema para o projeto do controlador JY01 (equipe EcoMauá)
 */
class CSystemConstants {
public:
  static constexpr float CF_BLDC_MOTOR_POLE_PAIRS = 8.0f; /**< Quantidade de pares de polos do motor BLDC utilizado */
};

#endif