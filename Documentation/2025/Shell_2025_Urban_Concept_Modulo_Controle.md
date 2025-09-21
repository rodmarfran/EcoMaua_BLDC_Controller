---
title: Especificação e Guia de Desenvolvimento do Módulo de Controle para Veículo EcoMauá - Conceito Urbano
...

# Proposta Geral
Desenvolver um módulo de controle eletrônico para o veículo Conceito Urbano que participará da Shell Eco Marathon Brazil. O módulo será responsável por gerenciar funções elétricas e eletrônicas essenciais, garantindo eficiência, confiabilidade e integração com o veículo.

# Funcionalidades Principais

## Controle de Iluminação
- **Componentes controlados:**
  - 2 faróis dianteiros (branco)
  - 2 setas dianteiras (amarelo)
  - 2 setas traseiras (amarelo)
  - 2 luzes traseiras de freio (vermelho)
  - 2 luzes traseiras de operação (vermelho)
  - 2 luzes reservas
- **Especificações:**
  - Acionamento via saídas digitais com suporte a PWM (500 Hz).
  - Alimentação de 12V para LEDs e fitas LED.
  - Uso de MOSFETs de canal N para controle.

## Controle do Motor do Para-brisa
- **Especificações:**
  - Motor DC acionado por uma ponte-H completa.
  - Controle de velocidade via PWM (20 kHz).
  - Capacidade de inversão de sentido de rotação.
  - Dimensões e corrente do motor a serem definidas (TBD).

## Interface de Botões
- **Lista de botões disponíveis:**
  - **Seta Direita:** Liga/desliga.
  - **Seta Esquerda:** Liga/desliga.
  - **Emergência:** Liga/desliga.
  - **Freio:** Pushbutton.
  - **Buzina:** Pushbutton.
  - **Faróis:** Liga/desliga.
  - **Para-brisa:** Liga/desliga.
  - **Botões Reservados:** Duas entradas adicionais.
- **Especificações:**
  - Todas as entradas são digitais simples.

## Interface CAN
- **Hardware utilizado:**
  - MCP2515 com transceiver TJA1050 integrado.
- **Finalidade:**
  - Comunicação com outros módulos e sistemas do veículo.
  - Troca de informações como diagnósticos, acionamento remoto e status do sistema.
  - Especificações de mensagens a serem definidas (TBD).

## Reguladores de Tensão
- **Fontes de entrada e saída:**
  - Entrada de 12V.
  - Regulador chaveado para 5V.
  - Regulador linear para 3,3V.
- **Filtros:**
  - Capacitores de desacoplamento e filtros para ruído na alimentação.

## Microcontrolador
- **Opções:**
  - Preferência: Raspberry Pi Pico.
  - Alternativa: ESP32.
- **Linguagem de programação:**
  - C ou MicroPython (preferência pelo MicroPython).

# Especificações de Hardware

## MOSFETs para Controle de LEDs
- **Corrente máxima esperada:** 5A (máximo).
- **Critérios de seleção:**
  - Deve ser acionado diretamente pelo GPIO do microcontrolador (3,3V).
  - Modelo sugerido: **IRLZ44N** (MOSFET de canal N com baixo limiar de acionamento).
- **Dissipação:**
  - MOSFET deve operar sem a necessidade de dissipador de calor.

## Motor do Para-brisa
- **Especificações do motor:**
  - Potência e corrente máxima ainda a serem definidas (TBD).
- **Ponte-H:**
  - Seleção baseada na corrente máxima do motor.

## Proteções Elétricas
- **Fusíveis:**
  - Externos ao módulo (não integrados).
- **Proteção contra inversão de polaridade:**
  - Não necessária devido à montagem física.

## Reguladores de Tensão
- **12V para 5V:**
  - Regulador chaveado para eficiência energética.
- **5V para 3,3V:**
  - Regulador linear para maior estabilidade em sinais lógicos.

## Layout e Conexões
- **Integração com volante:**
  - Caso seja inviável, permitir conexão por fios utilizando conectores padrão (ex.: JST ou Molex).
- **Fixação:**
  - A ser definida (TBD).
- **Passagem de fiação:**
  - Prevê-se o uso de um chicote específico (TBD).

# Programação e Configuração

## Comunicação com Botões e Sensores
- **Método:**
  - GPIOs dedicados para cada botão/sensor.

## Controle PWM
- **Frequências:**
  - LEDs: 500 Hz.
  - Motor do para-brisa: 20 kHz.

## Prioridade de Interrupções
- **Configuração:**
  - Garantir alta prioridade para o sensor de freio para resposta imediata.

# Planejamento de Consumo e Dissipação
- **Consumo total dos LEDs:**
  - A ser calculado com base na potência final das lâmpadas (TBD).
- **Reguladores de Tensão:**
  - Garantir que a dissipação de calor esteja dentro de limites seguros.

# Testes e Validação

## Testes de Hardware
- **Iluminação:**
  - Testar acionamento de LEDs e fitas LED com diferentes cargas.
- **Motor do Para-brisa:**
  - Testar o controle de velocidade e inversão de rotação.

## Comunicação CAN
- **Simulação:**
  - Verificar troca de mensagens no barramento.

## Robustez e Resiliência
- **Vibrações e Temperaturas:**
  - Testar em condições típicas de uso no veículo.

# Prazos e Próximos Passos
1. **Definir potência dos LEDs e motor do para-brisa (TBD).**
2. **Projetar o layout físico da placa (TBD).**
3. **Especificar o chicote de conexões (TBD).**
4. **Implementar e validar o software após conclusão do hardware.**

# Conclusão
Este documento estabelece as diretrizes para o desenvolvimento do módulo de controle eletrônico do veículo Conceito Urbano. A implementação deve priorizar a eficiência, confiabilidade e integração com os sistemas do veículo. Ajustes poderão ser realizados conforme as definições pendentes sejam finalizadas.

