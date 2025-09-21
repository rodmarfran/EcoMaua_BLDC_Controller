---
title: Tutorial Completo de Montagem e Validação do Sistema Elétrico – EcoMauá Urban Concept 2025
Version: 1.1
Date: 2025-04-20
Team: EcoMauá
subtitle: Guia para a construção em bancada do sistema elétrico do veículo Shell Eco-Marathon – Conceito Urbano
---

# 0 · Introdução
Este documento é um **tutorial passo-a-passo**, pensado para que qualquer membro da equipe EcoMauá — mesmo sem grande experiência prévia em elétrica automotiva — consiga **identificar, montar, testar e documentar** o sistema elétrico de baixa tensão (12 V) do veículo Conceito Urbano 2025.  Cada seção inclui **check-lists, dicas práticas, armadilhas comuns e critérios de aceitação** para que o processo seja reprodutível e auditável.

> ℹ️ **Escopo**: Parte 1 do desenvolvimento – protótipo em bancada.  A Parte 2 (confecção do chicote definitivo e instalação no chassi) será produzida em documento separado.

> ⚠️ **Segurança**: Trabalhamos com tensões relativamente baixas, mas correntes que podem ultrapassar 40 A em surto.  Use sempre EPI (óculos de proteção e luvas isolantes), mantenha um extintor de CO₂ próximo e NUNCA alimente a bancada sem fusível inline.

---

# 1 · Visão Geral do Sistema
O diagrama oficial (arquivo *EletricalDiagram_Eco2025_UrbanConcept.pdf*) define sete subsistemas principais:

1. **Iluminação Dianteira** – Dois faróis em barra LED 5730 (D1 & D9, farol baixo + alto por PWM).
2. **Setas** – Conjunto de quatro indicadores (D3, D7, D4, D8) controlados por relé de pisca LED 2 pinos.
3. **Iluminação Traseira** – Luz de posição (D5 & D6) e luz de freio (D2 & D10) com acionamento separado.
4. **Buzina** – Atuador eletromecânico (CGEN2) acionado por botão momentâneo.
5. **Motor de Partida** – Simulado em bancada por carga resistiva de 10 A (M2).
6. **Limpador de Para-brisa** – Motor DC (M1) comandado por relé SPDT.
7. **Segurança** – Chaves de emergência interna (SW7) e externa (SW8) mais fusível de 10 A próximo à bateria.

O sistema é alimentado por **bateria selada de 12 V 7 Ah** (ou fonte de bancada equivalente ≥ 10 A).  Todos os retornos convergem num barramento **GND** comum.

---

# 2 · Lista de Materiais (BOM) Detalhada
| # | Componente | Qtde | Especificações / Código de compra | Função |
|–––|––––––––––––|–––––|––––––––––––––––––––––––––––––––––|––––––––|
| 1 | Punho de luz Honda Titan 160 (esquerdo) | 1 | ML #5198681980 | Seta, buzina, farol, lampejador |
| 2 | Punho de partida Honda CB 500 (direito) | 1 | ML #5198681980 | Partida, corta-corrente |
| 3 | Relé de pisca LED **ajustável** 2 pinos | 1 | ML #4611674552 | Intermitência das setas |
| 4 | Relé automotivo SPDT 12 V/40 A c/ chicote | 3 | ML #1927649123 | K1 (Setas), K2 (Farol), K3 (Freio) |
| 5 | Barra LED 5730 50 cm 12 V (branco-quente) | 2 | ML #1363105895 | Faróis dianteiros |
| 6 | Fita LED vermelha 12 V | 2 m | genérico | Luz traseira de posição |
| 7 | LED Alta potência verm. 3 W + dissipador | 2 | genérico | Luz de freio |
| 8 | Buzina 12 V compacta | 1 | tipo moto | Sinal sonoro |
| 9 | Motor limpador parabrisas 12 V | 1 | sucata VW | M1 |
| 10 | Carregador de bateria inteligente 12 V | 1 | >3 A | Manutenção da bateria |
| 11 | Fusível de lâmina 10 A + porta-fusível | 1 | – | Proteção geral |
| 12 | Cabos 2,5 mm² (vermelho/ preto) | 5 m | – | Barramentos +12 V / GND |
| 13 | Cabos 1,0 mm² multicores | 20 m | – | Sinais |
| 14 | Terminais Faston fêmea 6,3 mm | 50 | – | Conexão relés & buzina |
| 15 | Termorretrátil 3 → 6 mm | 2 m | – | Identificação de fios |
| 16 | Multímetro, fonte CC, ferro de solda 60 W, estanho Sn60/Pb40, alicate de crimpagem, extrator de terminais, organizador de cabos, etiquetadora Brother P-Touch | – | Ferramentas |

> ✅ **Dica EcoMauá**: Sempre compre 10 % de material extra (terminais, cabos, termorretrátil) — evita atrasos caso algo precise ser retrabalhado.

---

# 3 · Ferramentas de Trabalho
- **Mesa MDF 90×60 cm** com régua AC e disjuntor individual.
- **Suporte tipo terceira-mão** com lupa iluminada.
- **Alimentação por fonte ajustável 0–24 V / 10 A** para testes controlados.
- **Software KiCad 9** (abre o .kicad_sch em anexo) para consulta rápida do diagrama.
- **Planilha de Log (Google Sheets)** – modelo "Log_Testes_Eco2025_v1" no Drive compartilhado.

---

# 4 · Procedimento Passo-a-Passo
## 4.1 Preparação da Bancada
1. **Imprima o diagrama A4** e plastifique — mantém limpo durante soldagem.
2. Fixe três **trilhos DIN 15 cm** sobre a mesa com parafusos M4.
3. Encaixe **bornes de engate rápido** (cinza) para +12 V, GND e saída controlada.
4. Faça a **ponte de aterramento**: *GND → barra de metal → terminal banana* da fonte.
5. Monte o **porta-fusível** a 10 cm da banana +12 V; instale fusível de 10 A.

> ⚡ **Importante**: Nunca ligue a fonte antes de inserir o fusível.  Queimaduras de arco elétrico acontecem em < 10 ms.

## 4.2 Identificação das Pinagens dos Punhos
### 4.2.1 Punho Esquerdo (Luz Titan 160)
| Fio (cor típica) | Função esperada | Teste de continuidade |
|–––––––––––––––––|––––––––––––––––|––––––––––––––––––––––|
| Verde | GND comum | Sonda preta no pino, sonda vermelha em carcaça metálica → *Bip* |
| Azul-claro | Seta direita | Pressione seta → continuidade c/ comum |
| Laranja | Seta esquerda | Idem acima |
| Cinza | Farol alto | Chave alta → continuidade |
| Branco | Farol baixo | Chave baixa → continuidade |
| Amarelo | Lampejador | Aperte gatilho ➜ pulso momentâneo com farol alto |
| Preto | Buzina | Pressione buzina → continuidade |

> 🧰 Registre cada combinação na **tabela "Pinagem_Punhos.xlsx"** (modelo no Drive).  Cole termorretrátil com abreviações: `GRN_GND`, `BL_SET_RE`, etc.

### 4.2.2 Punho Direito (Partida CB 500)
1. Deslocar o botão vermelho *engine-stop* — medir continuidade entre fio branco-preto e GND.
2. Pressionar **botão de partida** — continuidade entre fio amarelo-verde e GND.
3. Se existir farol ON/OFF, repetir teste com fio azul-branco.

> 🔍 **Armadilha comum**: Alguns conjuntos reutilizam cores — valide SEMPRE olhando *qual* fio chega ao botão mecânico.

## 4.3 Instalação dos Relés
1. Fixe **K1 (Setas)** no trilho DIN: pino 30 → +12 V (após fusível), pino 87 → Relé de pisca (L) e (R), pino 85 → Saída punho seta, pino 86 → GND.
2. Fixe **K2 (Farol)**: 30 → +12 V, 87 → Barra LED, 85 → Saída farol baixo/alto (via seletor), 86 → GND.
3. Fixe **K3 (Freio)**: 30 → +12 V, 87 → LEDs de freio, 85 → Sensor pressão freio, 86 → GND.

> 📐 **Dica de layout**: Mantenha cabos de carga ≥ 5 A no trilho inferior e sinais no superior; diminui ruído e facilita a inspeção.

## 4.4 Testes Unitários
### 4.4.1 Procedimento de Ensaio para as Setas
1. Conecte uma **lâmpada de teste 12 V 21 W** no lugar de cada LED — evita dano caso a frequência de pisca esteja errada.
2. Ajuste o trimpot do relé de pisca para ≈ 90 ± 10 fpm (flash-per-minute) – use cronômetro.
3. Acione seta esquerda → LED (L) deve pulsar; direita idem.  Verifique que a corrente RMS ≤ 1,2 A.
4. Ative a **chave hazard (SW6)** – ambos lados devem piscar simultaneamente.

### 4.4.2 Buzina
1. Desligue relés K1-K3 para isolar ruído.
2. Pressione botão da buzina; medir queda de tensão: **V_batt ≥ 11,0 V** durante 2 s.
3. Verifique fixação mecânica – ressonância excessiva indica parafuso frouxo.

### 4.4.3 Faróis
| Teste | Ação | Esperado |
|–––|–––|–––|
| Farol baixo | Chave na posição I | Corrente ≈ 700 mA por barra LED |
| Farol alto | Posição II | Corrente dobra (~1,4 A) e luminosidade aumenta |
| Lampejador | Pressionar gatilho | Pulso de < 0,5 s independente da chave |

### 4.4.4 Luz de freio & posição
1. Ligue fonte 12 V → LED traseiro de posição acende (corrente < 300 mA).
2. Pressione sensor de freio hidráulico → LED freio acende (corrente adicional < 500 mA).
3. Meça luminosidade com luxímetro: **≥ 90 lux a 1 m**.

### 4.4.5 Motor de Partida (Simulado)
1. Substitua M2 por resistor de carga 1,2 Ω/200 W (produz ~10 A).
2. Pressione botão de partida: registrar corrente de partida, verificar que relé não cola.

### 4.4.6 Limpador de Para-brisa
1. Injetar 12 V no M1 por 1 s para determinar direção "home".
2. Acionar chave SW1: ciclo completo (ida + volta) em < 1,8 s.
3. Verificar corrente de pico < 5 A e média ≤ 3 A.

> 📝 **Registrar tudo** na planilha: Data, nº de ensaio, trem de testes, "OK" ou "Falha", observações.

## 4.5 Teste Integrado em Bancada
1. Reative **todos** os relés.
2. Proceda à **"dança das funções"**:
   - Seta L  → Buzina → Freio → Farol alto → Hazard → Partida.
3. Durante 2 min monitore tensão: **mínimo absoluto 10,8 V**.
4. Nenhum relé pode superaquecer (> 60 °C) — toque rápido com termopar.
5. Fotografe a bancada de múltiplos ângulos e anexe imagens ao relatório.

---

# 5 · Cronograma Detalhado
| Semana ISO | Datas (2025) | Marco | Entregáveis |
|–––––––|––––––––|––––––––––|––––––––––|
| 17 | 22 → 26 abr | Compra + bancada | NFs arquivadas, fotos da bancada montada |
| 18 | 28 abr → 02 mai | Pinagem punhos | Planilha Pinagem_Punhos.xlsx preenchida |
| 19 | 05 → 09 mai | Instalação relés | Bancada energizada sem carga |
| 20 | 12 → 16 mai | Testes unitários I | Check-list setas, buzina, farol |
| 21 | 19 → 23 mai | Testes unitários II | Check-list partida, limpador, freio |
| 22 | 26 → 30 mai | Teste integrado | Log integrado + Relatório v1.0 |

> 📆 **Revisão semanal**: toda 6ª feira às 14h no laboratório – presença obrigatória.

---

# 6 · Gestão de Riscos
| Risco | Prob. | Impacto | Ação Preventiva | Ação Corretiva |
|–––––|––––––|–––––––|–––––––––––––––|––––––––––––––|
| Curto-circuito | M | A | Inspeção visual + fusível | Desligar, substituir fusível, RCA ✔︎ |
| Superaquec. LED | M | M | Resistência série, dissipador | Interromper, aplicar pasta térmica |
| Conectores soltos | L | M | Crimpagem dupla + tração | Recrimpar, colocar trava |
| Atraso material | M | M | Pedido +10 % reserva | Replanejar, solicitar fornecedor B |
| Falha de teste | L | A | Procedimento escrito | Rodar RCA, atualizar doc | 

---

# 7 · Critérios de Aceitação
1. Todos os **26 casos de teste unitários + integrados** classificados como *OK*.
2. Barramento de 12 V **não** cai abaixo de 10,8 V em nenhuma condição.
3. Documento **Relatório_Testes_Parte1.pdf** concluído e arquivado no Drive.
4. Feedback de QA aprovado sem *Action Items* pendentes.

---

# 8 · Encerramento e Próximos Passos
Concluída a Parte 1, a equipe EcoMauá irá:
1. Reunir-se com a célula de *Body & Chassis* para definir **roteamento físico** do chicote.
2. Atualizar o diagrama KiCad para rev V1.1 com eventuais correções.
3. Gerar **BOM Parte 2** (cabos, espaguete corrugado, terminais AVL, conectores IP67).
4. Planejar montagem **on-vehicle** na semana 24 (10 → 14 jun 2025).

---

# 9 · Anexos
## Anexo A – Tabela de Cores dos Conectores (Work In Progress)
```
Fio  Função            Origem          Destino       Bitola  Obs.
VDE  GND Comum         Punho E/D       Barramento    1 mm²   –
...
```
## Anexo B – EPI e Boas Práticas
- Utilize **luvas de borracha classe 0** para 1 kV – evita cortes e choques.
- Estação de solda sempre com **ponta aterrada**.
- Ventile a bancada – fumaça de solda contém chumbo e colofônia.

## Anexo C – Modelos de Documentos
- *Pinagem_Punhos.xlsx*
- *Log_Testes_Eco2025_v1*
- *Relatório_Testes_Parte1.pdf* (template).

---

# 10 · Glossário
| Termo | Definição |
|––––––|–––––––––––|
| **K1/K2/K3** | Designações de relés SPDT conforme diagrama |
| **FPM** | *Flashes per Minute*, unidade para frequência de piscas |
| **RCA** | *Root Cause Analysis* – análise de causa raiz |
| **Hazard** | Modo emergência: liga todas as setas simultâneas |
| **DIN** | Padrão de trilho metálico para fixação de componentes elétricos |

---

> © 2025 Equipe EcoMauá.  Reutilização permitida sob licença CC BY-SA 4.0.

