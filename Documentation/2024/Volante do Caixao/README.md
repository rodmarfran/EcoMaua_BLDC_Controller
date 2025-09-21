# Virtualização do instrumento VenSpec-M através da ferramenta QEMU para utilização nas missões VERITAS e EnVision

Plano de Desenvolvimento Geral -- (17/03/2024)

<!-- ver:1.0.0 -->

## Introdução 
<!-- sec:introduction -->

Este documento se dedica a ser um plano de desenvolvimento para a Iniciação Cientifica (IC) nomeada "Virtualização do instrumento VenSpec-M através da ferramenta QEMU para utilização nas missões VERITAS e EnVision", pelo NSEE-IMT.

O responsável pelo desenvolvimento da IC é o aluno Felipe Fazio da Costa, RA 23.00055-4, da 2ª série do curso de Engenharia de Computação.

O orientador do trabalho é o professor Rodrigo de Marca França, pesquisador responsável pela Divisão Espacial do NSEE-IMT.

## Autoria do Documento
<!-- sec:document_authorship -->

1. Rodrigo França ([rodrigo.franca@maua.br](rodrigo.franca@maua.br)).

## Etapa 1 -- Ponte SpaceWire - TCP/IP

### Período de Desenvolvimento

-   **Início:** 01/03/2024

-   **Término:** 01/05/2024

### Descritivo

Sistema capaz de conectar canais físicos do protocolo SpaceWire, usando o equipamento Brick MK3 da STAR-Dundee, com um servidor TCP/IP que permita o recebimento e envido de dados.

O sistema deve agir como uma ponte SpaceWire -- TCP/IP, permitindo que dados que sejam recebidos pelo protocolo SpaceWire seja retransmitido por TCP/IP, assim como dados recebidos por TCP/IP sejam enviados por SpaceWire.

Essa ponte tem diversos usos de interesse, como permitir a conexão remota de dispositivos SpaceWire e a criação de bancos de testes automatizados. Entre os diversos usos, ela irá permitir a conexão de um instrumento virtualizado no QEMU com canais SpaceWire reais.

### Requerimentos Funcionais

1.  Iniciar um servidor TCP/IP do tipo Socket Server na máquina local.

2.  Utilizar o Brick MK3 da STAR-Dundee através das APIs em Python.

3.  Enviar dados por SpaceWire usando o Brick MK3.

4.  Receber dados por SpaceWire usando o Brick MK3.

5.  Configurar o Brick MK3 da STAR-Dundee conforme necessidade.

6.  Receber e enviar dados por TCP/IP;

7.  Transmitir pacotes recebidos por SpaceWire por TCP/IP, em um formato padronizado.

8.  Transmitir pacotes recebidos por TCP/IP por SpaceWire, em um formato padronizado.

9.  Utilizar o numpy para realizar qualquer processamento de dados.

### Requerimentos Não-Funcionais

1.  Funcionar também com o Brick MK2 da STAR-Dundee.

### Diagrama de Blocos

O diagrama de blocos proposto para o sistema pode ser visto na Figura \ref{fig:diag_blocos_sistema_etapa_1}.

![Diagrama de Blocos do sistema](figs/diagrama_blocos_etapa_1.svg)
<!-- fig:diag_blocos_sistema_etapa_1 -->

### Características Técnicas do Sistema

Um dos pontos mais importantes é qual formato de dados usar para a comunicação TCP/IP, pois precisa ser um formato padronizado que permita o uso de todas as funções do SpaceWire, mas que seja compacto e simples. A sugestão de pacote por TCP/IP são pacotes de 16b (2 bytes), onde o primeiro byte é composto do canal de comunicação e um identificador de pacotes e o segundo byte é um dado bruto. A estrutura desse pacote pode ser vista na Figura \ref{fig:protocolo_dados_tcpip}:

![Protocolo de dados para a Interface TCP/IP](figs/interface_dados_tcpip.svg)
<!-- fig:protocolo_dados_tcpip -->

Esse formato tem a vantagem de ser simples e fácil de implementar, além de flexível. Ele tem a desvantagem de potencialmente reduzir a velocidade de comunicação de dados longos, mas precisa ser verificado se esse realmente é o caso ou não.

O campo de Channel ID especifica qual o canal SpaceWire a ser usado, permitindo o uso de até 16 canais diferentes para recebimento ou transmissão de dados.

O campo de Data ID especifica qual o tipo de dado que vem a seguir, e pode assumir os seguintes valores:

-   **0x0:** Indica que o campo de Data Value é um byte de dados brutos para ser enviado (ou que foi recebido) por SpaceWire.

-   **0x1:** Indica que o campo de Data Value é um identificados de fim de pacote, que pode assumir os valores:

    -   **0x00:** End-of-Packet (EOP).

    -   **0x01:** Error-end-of-Packet (EEP).

Novos identificadores podem e devem ser criados conforme a necessidade, para indicar estados de erro e similares.

### Sugestão de Desenvolvimento

O sistema deve ser executado como um sistema escrito em python. O sistema deve permitir que a porta do Socket Server TCP/IP seja escolhida na inicialização dele. Ex:

- `run_spacewire_tcpip_bridge(server_port)`

É sugerido o desenvolvimento das seguintes funções (representando as funcionalidades críticas) para criar o sistema final:

1.  Função que permite enviar dados pelo Brick MK3:

    -   Função em python que recebe um canal do Brick, um vetor de dados e o fim de pacote e envia um pacote de dados por SpaceWire.

    -   `brickmk3_transmit_spacewire_packet(channel, data, end_of_packet)`

2.  Função que permite receber dados pelo Brick MK3:

    -   Função em python que recebe um canal do Brick e retorna o canal, um vetor de dados e fim de pacote de um pacote recebido por SpaceWire.

    -   `channel, data, end_of_packet = brickmk3_receive_spacewire_packet(channel)`

3.  Função que permite abrir um Socket Server TCP/IP:

    -   Função em python que recebe uma porta e abre um Socket Server TCP/IP para troca de dados.

    -   `tcpip_open_socket_server (server_port)`

4.  Função que permite enviar dados pelo Socket Server TCP/IP:

    -   Função em python que recebe um canal, um vetor de dados e o fim de pacote e envia um pacote de dados pelo Socket Server TCP/IP com o formato definido na Figura \ref{fig:protocolo_dados_tcpip}.

    -   `tcpip_transmit_spacewire_packet(channel, data, end_of_packet)`

5.  Função que permite receber dados pelo Brick MK3:

    -   Função em python que retorna um canal, vetor de dados e fim de pacote de um pacote recebido pelo Socket Server TCP/IP com o formato definido na Figura \ref{fig:protocolo_dados_tcpip}.

    -   `channel, data, end_of_packet = tcpip_receive_spacewire_packet()`

6.  Função que permite resetar o Brick MK3:

    -   Função em python que reseta Brick MK3.

    -   `brickmk3_reset_device()`

7.  Função que permite configurar um canal do Brick MK3:

    -   Função em python que recebe um canal e um conjunto de configurações e configura o canal do Brick MK3.

    -   `brickmk3_configure_channel(channel_configuration)`

O fluxograma geral proposto para o sistema pode ser visto na Figura \ref{fig:fluxograma_sistema_etapa_1}.

![Fluxograma Geral proposto para o sistema](figs/fluxograma_dados_etapa_1.svg)
<!-- fig:fluxograma_sistema_etapa_1 -->
