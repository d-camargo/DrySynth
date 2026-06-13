# CLAUDE.md

Guia para o Claude Code trabalhar neste projeto. Escreva código e comentários em **português** (o firmware atual já é assim).

> **Desenho completo: [`PLANEJAMENTO.md`](PLANEJAMENTO.md)** (fonte de verdade do
> que será construído e por quê) · **Plano de execução: [`ROADMAP.md`](ROADMAP.md)**
> (fases, checklists e portões de segurança). Este CLAUDE.md resume o contexto e as
> convenções de trabalho.

## O que é

**DrySynth** — caixa de secagem e armazenamento de filamento de impressão 3D.
Uma caixa térmica de mercado, com aquecimento controlado + circulação de ar para
manter o filamento seco (baixa umidade relativa/absoluta).

O código-fonte é um sketch Arduino: `Caixa_Filamento/Caixa_Filamento.ino`.
A lib do sensor está em `DHTlib-master.zip` (instalar na IDE Arduino).

## Estado atual (a base sobre a qual vamos trabalhar)

Hardware:
- **Arduino UNO**
- **Lâmpada incandescente** como elemento de aquecimento, chaveada por **relé** (pino 9; `HIGH` = relé fechado = lâmpada ON)
- **Cooler de PC 80x80mm**, controlado por PWM (pino 5). No layout antigo ele *puxava* ar da câmara onde ficava a lâmpada
- **Sensor DHT22** (temperatura + umidade) no pino `A2`, lido via `DHT.read22()`
- **LCD 16x2 I2C** (endereço `0x27`)
- **Botão** (pino 7, `INPUT_PULLUP`) para alternar de modo

Lógica de firmware (resumo do `.ino`):
- 3 modos cíclicos pelo botão: `SECAGEM`, `ECONOMIA` (eco/dorme), `VENTILA`
- Controle por **histerese** de temperatura: cada modo tem par liga/desliga
  (`T_*_LIGA` / `T_*_DESLIGA`). Liga aquecimento abaixo de `tMin`, desliga em `tMax`
- **Fan** acelera proporcional à temperatura (PWM linear 20°C→tMax) só quando o
  aquecedor está ON; quando OFF usa `FAN_IDLE_PWM` (circulação mínima)
- **Filtro de salto** (`MAX_SALTO` 3°C) descarta leituras espúrias do DHT
- Calcula **umidade absoluta** (g/m³) a partir de temp + umidade relativa e
  alterna o display entre umidade relativa e absoluta
- `delay(2000)` no fim do loop (bloqueante — ver notas do upgrade)

## O upgrade que estamos fazendo

Objetivo: trocar a lâmpada incandescente por **cartuchos aquecedores** e migrar a
plataforma para **ESP8266** com conectividade Wi-Fi/IoT.

Decisão: o firmware do ESP8266 será escrito **do zero**, não uma adaptação do
sketch da UNO. O `.ino` atual serve só como referência da lógica/equações.

Hardware novo (ver detalhes/BOM em `PLANEJAMENTO.md`):
- **1x cartucho aquecedor cerâmico da Creality K1: 24V / 60W** (confirmado).
  ≈ **2,5A @ 24V**. O elemento atinge até ~320°C no bloco
- **1x termistor 100K NTC** (~beta 3950, da K1) no **bloco de alumínio** — mede a
  temperatura do elemento (controle + segurança). Há um 2º termistor de reserva
- **Bloco/placa de alumínio** do hotend — o cartucho encaixa nele para
  **espalhar e propagar o calor** de forma mais uniforme (evita ponto quente)
- **ESP8266** (NodeMCU/Wemos D1 mini) — plataforma **decidida**, pois interface só
  por celular + alertas exigem Wi-Fi (UNO não tem). Termistor único cabe no **A0**,
  então **não há ADS1115**
- **Fonte 24V** disponível (≥3A)
- Sensores de ar: **2x DHT22 internos** + **1x DHT11 externo** (referência de
  ambiente, habilita ventilação inteligente)
- Reaproveitar: caixa térmica, cooler 80x80
- **Removidos: LCD e botão** — interface passa a ser pelo celular (Wi-Fi)

Operação: **controle por umidade** (umidade-alvo; temperatura é meio, não fim).
Filamentos: **PLA/PETG** (~45–55°C de ar). Setpoint histórico: ~50°C.

> Correção de hardware: inicialmente achávamos serem 2 cartuchos aquecedores. Na
> verdade é **1 aquecedor + 2 termistores**. Isso resolve o maior risco do
> projeto, pois agora dá para medir a temperatura do bloco (estilo hotend).

### Pontos de atenção técnicos (importantes — não ignorar)

**Plataforma ESP8266 (3.3V):**
- Lógica **3.3V**. DHTs funcionam em 3.3V.
- **1 entrada analógica** (`A0`): usada pelo **termistor** (com divisor). No chip nu
  é 0–1V; placas NodeMCU têm divisor para 0–3.3V — projetar o divisor conforme a
  placa. Como há só 1 termistor, **não é preciso ADS1115**.
- **PWM** por software (`analogWrite`; range default 0–1023). Dimensionar PWM de
  aquecedor e fan nesse range.
- Pinos com restrição de boot/flash (D0/D3/D4/D8) — evitar para saídas críticas.
  Mapear pinos explicitamente.
- Firmware **não-bloqueante** (`millis()`, sem `delay()` longo) — `delay()` trava o
  Wi-Fi. Considerar `yield()`/watchdog.

**Acionamento do aquecedor:**
- 60W @ 24V (~2,5A) ainda é **bastante potência** para uma caixa que só precisa de
  ~50°C de ar. Em regime, a potência necessária é baixa. Tratar como "potência de
  pico" e **limitar por PWM** conforme a temperatura do bloco
- Usar **MOSFET** (lado baixo, ex.: IRLZ44N ou módulo) com PWM, **nunca relé** para
  esse elemento. Relé não controla potência e chavear repetidamente o destrói
- Separar a alimentação: aquecedor em **24V**, o ESP8266 em 3.3V/5V (regulador
  buck ou fonte própria). **GND comum**
- Controle recomendado: **PID na temperatura do BLOCO** (termistor), com o **ar
  (DHT22) como setpoint do sistema**. Ou seja: malha externa lenta no ar (alvo
  ~50°C) define o alvo de bloco; malha interna no bloco evita overshoot. Começar
  simples (limite de duty + histerese de bloco) e evoluir para PID se necessário

**Sensoriamento (bloco + ar):**
- **Termistor no bloco** (no `A0`): leitura direta da temperatura do elemento.
  Sensor primário de **controle e segurança** do aquecedor. Calibrar com
  Steinhart-Hart (ou beta) para 100K NTC ~3950
- **DHTs no ar**: medem ambiente/umidade. Limites: DHT22 ≤ ~80°C, DHT11 ≤ ~50°C
- Plano de pontos:
  - **Termistor** — no bloco (controle + segurança)
  - **DHT22 #1** — interno, próximo ao fluxo de ar do aquecedor
  - **DHT22 #2** — interno, lado do filamento (temperatura/umidade "real")
  - **DHT11** — **externo**, referência de ambiente (habilita ventilação inteligente)
- A decisão de ventilar usa **umidade absoluta** (g/m³): ventila para fora só quando
  o ar externo (DHT11) está mais seco que o interno (DHT22). Equação já no `.ino`
- Regra de segurança: se sensores **divergirem muito**, **algum falhar/`NaN`/curto/
  aberto**, ou a **resposta térmica não bater com o comando**, **cortar o
  aquecimento**

**Segurança (crítico — 60W concentrados em caixa de isopor/plástico):**
- **Limite de temperatura máxima do BLOCO** no firmware (definir valor seguro p/ a
  caixa, ex.: bem abaixo do que derrete isopor) → desliga e exige reset
- Proteção contra **thermal runaway** estilo Marlin, agora possível com o
  termistor: se o aquecedor está comandado mas a temperatura do bloco **não sobe**
  no tempo esperado → falha (termistor solto, MOSFET em curto, etc.) → cortar
- **Detecção de termistor solto/curto/aberto** (leitura fora de faixa) → cortar
- **Fusível térmico físico no bloco de alumínio** (ex.: corte ~100°C) como rede de
  segurança independente do firmware — funciona mesmo se o ESP travar ou o MOSFET
  ficar em curto. Barato, recomendado
- **Fusível de corrente** na linha 24V da fonte
- **Watchdog** + estado seguro no boot (aquecedor OFF até medir e validar sensores)
- O bloco de alumínio quente **não pode encostar** no isopor/plástico da caixa —
  precisa de isolamento/standoff e dissipação para o fluxo do cooler
- **Intertravamento**: não aquecer com o fan parado (fan move o calor do bloco p/
  o ar e evita ponto quente)

**IoT — interface no celular + ALERTAS (decidido):**
- Sem LCD: a interface é **servidor web local no ESP8266** (celular na mesma rede)
  para status/controle, **mais alertas** por evento.
- Canal de alerta: **Telegram Bot** (decidido) via HTTPS (`WiFiClientSecure`).
  Token/chat_id são segredos → `secrets.h` no `.gitignore`, nunca commitar
- Eventos que devem gerar alerta:
  - **Secagem concluída** (atingiu setpoint / tempo de ciclo)
  - **Falha de sensor** (`NaN`, divergência grande entre os DHTs)
  - **Superaquecimento / corte de segurança** acionado
  - **Umidade alta** detectada / filamento precisa secar
  - (opcional) Wi-Fi reconectado, boot do dispositivo
- Implementar de forma **não-bloqueante** e tolerante a falha de rede: alerta que
  não envia **nunca pode travar nem atrasar o controle de segurança**. Rede é
  best-effort; a segurança (cortes locais) é independente do Wi-Fi
- Evitar spam: aplicar **debounce/cooldown** por tipo de alerta

## Convenções

- Idioma: português em código, comentários e mensagens (web/alertas)
- Nome de marca do projeto: **DrySynth**
- Mantenha constantes de configuração (limites de temperatura, alvos de umidade,
  PWM) agrupadas e nomeadas no topo do arquivo
- A **camada de segurança** (cortes locais) é independente do Wi-Fi e tem
  prioridade — rede é best-effort e nunca pode travar/atrasar a segurança
- Firmware **não-bloqueante**; o `.ino` atual é só referência de equações

## Build / flash

Compilação e upload pela **Arduino IDE** (ou `arduino-cli`).
- Placa alvo: **ESP8266** (instalar o core esp8266 no Board Manager)
- Dependências: lib DHT (`DHTlib-master.zip`); para Wi-Fi/web/alertas, libs do core
  ESP8266 (`ESP8266WiFi`, `ESP8266WebServer`, cliente HTTPS p/ Telegram)
- O sketch antigo (UNO, com LCD I2C) fica como referência em `Caixa_Filamento/`
