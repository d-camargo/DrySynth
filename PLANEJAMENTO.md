# Planejamento — DrySynth (Caixa de Secagem/Armazenamento de Filamento)

Documento de projeto. Define **o que** será feito e **por quê**, antes de pinos e
código. Fonte de verdade do desenho; o `CLAUDE.md` é o guia de trabalho no repo.

Data: 2026-06-13 · Status: planejamento aprovado nas decisões-chave, pendências no fim.
Repositório: https://github.com/d-camargo/DrySynth (a pasta local ainda **não** está
conectada ao git — ver §11).

---

## 1. Objetivo

Caixa térmica (de mercado, isopor/plástico) que **mantém filamento de impressão 3D
seco** — controlando **umidade** com auxílio de aquecimento e circulação/ventilação
de ar, com **monitoramento e alertas pelo celular** via Wi-Fi.

Reaproveita o projeto existente (firmware Arduino em `Caixa_Filamento/`), mas o
firmware será **reescrito do zero** para a nova plataforma e nova lógica.

## 2. Decisões já tomadas

| Tema | Decisão |
|------|---------|
| Filamentos | **PLA / PETG** (temperaturas brandas, ~45–55 °C) |
| Aquecedor | **1x cartucho cerâmico Creality K1, 24V / 60W** (~2,5A) |
| Fonte | **24V já disponível** (≥3A) |
| Sensor do bloco | **1x termistor 100K NTC** (~beta 3950) no bloco de alumínio |
| Sensores de ar | **2x DHT22 internos** + **1x DHT11 externo** (referência de ambiente) |
| Interface | **Somente celular** (sem LCD, sem botão físico) |
| Lógica de operação | **Controle por umidade** (umidade-alvo; temperatura é meio, não fim) |
| Alvo de %UR | **Configurável na web, ajustável em uso** (não fixo no código) — ver §5 |
| Câmara de aquecimento | Bloco **fora da caixa**, em **câmara externa** (ABS + tecido fibra de vidro FM260FV); fan 80x80 no acoplamento — ver §6 e [`MECANICA.md`](MECANICA.md) |
| Recirculação | Ar retorna à câmara pelo **furo de baixo** (antiga entrada) via **duto Ø60mm** impresso |
| Ventilação | Caixa **não 100% vedada** (troca passiva). Purga (Etapa Final): **servo veda a entrada de ar novo**; saída = **aba passiva** unidirecional — ver §6, §13 e [`MECANICA.md`](MECANICA.md) |
| IoT | **Interface web** + **alertas via Telegram Bot** |
| Cooler | **12V** (precisa de buck 24V→12V) |
| Plataforma | **ESP8266 NodeMCU** (justificativa na seção 3) |

## 3. Decisão de plataforma: Arduino UNO × ESP8266

Você pediu para trocar **só se fizer sentido**. Com os requisitos escolhidos
(interface só por celular + alertas), faz sentido — na verdade é **necessário**.

### Por que o UNO não atende

- **Sem Wi-Fi.** A interface é 100% pelo celular e há alertas remotos. Sem rádio,
  o UNO não entrega nem a interface nem os alertas.
- **Sem TLS/HTTPS prático.** Telegram/push exigem HTTPS. O UNO (2KB RAM) não roda
  TLS. Precisaria de um módulo Wi-Fi externo (ESP-01) por serial — que é
  **adicionar um ESP8266 mesmo**, só que num arranjo de 2 chips, mais frágil e
  complicado, e ainda assim sem TLS no lado do UNO.

### Ganhos concretos do ESP8266

| Recurso | UNO | ESP8266 |
|---------|-----|---------|
| Wi-Fi nativo | ❌ | ✅ (habilita interface celular + alertas) |
| HTTPS/TLS (Telegram, push) | ❌ | ✅ |
| Servidor web local | ❌ (sem rede) | ✅ |
| Atualização OTA | ❌ | ✅ |
| CPU | 16 MHz | 80–160 MHz |
| RAM | 2 KB | ~80 KB |
| Flash | 32 KB | 1–4 MB |
| Entradas analógicas | 6 | **1** (A0) — limitação, ver abaixo |
| Tensão lógica | 5V | **3.3V** — exige atenção nos periféricos |
| PWM | hardware 8-bit | software 10-bit |

### Conclusão

**Usar ESP8266.** Os ganhos não são marginais: são o que torna o produto pedido
possível. Guardar a ESP só faria sentido se abríssemos mão de celular + alertas
(aí o UNO atual bastaria). Como **só temos 1 termistor**, a limitação de 1 entrada
analógica do ESP8266 deixa de ser problema (o termistor cabe no A0) — e o **ADS1115
não é mais necessário**.

> Alternativa superior, **se você tiver um ESP32 sobrando**: mais ADCs, dual-core,
> TLS mais folgado, Bluetooth. Não é necessário — o ESP8266 atende. Fica como nota.

## 4. Princípio de funcionamento (física da secagem) — importante

Entender isto define toda a lógica de controle:

1. **Aquecer reduz a umidade RELATIVA.** Para a mesma quantidade de água no ar
   (umidade absoluta, g/m³), subir a temperatura derruba a umidade relativa (%UR).
   Isso já protege o filamento de reabsorver água.
2. **Aquecer também "expulsa" água do filamento** para o ar da caixa (eleva a
   umidade absoluta interna).
3. **Caixa fechada não remove água de verdade.** A água que sai do filamento fica
   no ar interno; sem saída, o sistema chega ao equilíbrio e para de secar. Para
   **remover** umidade, há duas vias:
   - **Ventilação inteligente:** expelir o ar úmido interno **quando o ar externo
     estiver mais seco** (em umidade absoluta). É aqui que o **DHT11 externo** entra:
     comparando umidade absoluta interna × externa, ventilamos só quando isso
     realmente seca. Ventilar com ar externo mais úmido pioraria.
   - **Desumidificante (sílica gel):** absorve passivamente. Backup recomendado.

**Trabalhar sempre com umidade ABSOLUTA (g/m³)** nas decisões de ventilação — a
equação já existe no código atual (Magnus + conversão).

> **Nota (caixa real):** a caixa **não é 100% vedada** — há troca de ar lenta pela
> tampa/frestas. Nas etapas iniciais (sem servo) a remoção de umidade é **passiva +
> sílica gel**; a **purga ativa** (servo na saída) entra na **Etapa Final**. Ver §13.

## 5. Estratégia de controle (controle por umidade)

Objetivo: manter a **umidade relativa interna abaixo de um alvo** (ex.: < 20% UR
para PLA/PETG), usando temperatura e ventilação como meios, sempre dentro de
limites de segurança.

Malhas:

- **Aquecimento (atuador: cartucho via MOSFET/PWM)**
  - Sobe a temperatura do ar até derrubar a %UR ao alvo, **respeitando um teto de
    temperatura** (ar ~50 °C; bloco com limite de segurança pelo termistor).
  - Controle interno na **temperatura do bloco** (termistor) para evitar overshoot;
    controle externo na **umidade/temp do ar** (DHT22) como objetivo.
  - Começar simples (histerese de bloco + limite de duty); evoluir para PID se o
    overshoot incomodar.

- **Ventilação (atuadores: cooler 80x80 + servo SG90 da aba — ver §6)**
  - **Selada:** servo fecha a aba do furo de cima; cooler só circula ar interno
    sobre o bloco (transferir calor, evitar ponto quente).
  - **Purga:** servo abre a aba; cooler empurra o ar úmido pelo furo de cima e puxa
    ar externo pelo furo de baixo. **Somente quando** umidade absoluta externa
    (DHT11) < interna (DHT22) por margem — isto é, quando ventilar realmente seca.

- **Setpoint e modos (todos por software, no celular)**
  - **Secar:** alvo de UR baixo + aquecimento ativo + ventilação inteligente.
  - **Manter:** alvo de UR moderado, potência mínima, ventilação só quando útil.
  - **Desligado:** sem aquecimento; opcional só monitorar.

- **Alvo de %UR realista (importante)**
  - O alvo de UR é **configurável pela web e ajustável em uso** — não fixo no código.
    A UR mínima atingível depende da eficiência da caixa/sistema e do ambiente; só
    dá pra calibrar com o sistema rodando.
  - O controle busca a **menor UR alcançável dentro dos limites de temperatura**.
    **Não pode ficar preso** tentando atingir um alvo impossível (ex.: 10% se a
    caixa só chega a 18%) — era a limitação do layout anterior.
  - **Detecção de platô:** se a UR parou de cair por um tempo (com aquecimento/
    ventilação no limite), considera "secagem concluída para esta caixa" → alerta +
    passa para **manter**. Evita aquecer pra sempre.

## 6. Arquitetura de hardware

Diagrama de blocos (texto):

```
        24V PSU ──┬──────────────► Cartucho cerâmico 60W
                  │                      ▲
                  │                 MOSFET (PWM, low-side)
                  │                      │ gate
   buck 24V→5V/3V3│                 ┌────┴─────┐
                  └──► ESP8266 ─────┤ GPIO PWM │ (aquecedor)
                         │          ├──────────┤
                         ├─ GPIO PWM┤ MOSFET   ├──► Cooler 80x80 (12V, via buck 24→12V)
                         │          └──────────┘
                         ├─ GPIO PWM ──► Servo SG90 (aba do furo de saída)
                         ├─ A0  ◄── divisor + termistor 100K (bloco)
                         ├─ GPIO ◄── DHT22 #1 (ar, perto do fluxo)
                         ├─ GPIO ◄── DHT22 #2 (ar, lado do filamento)
                         ├─ GPIO ◄── DHT11   (externo, ambiente)
                         └─ Wi-Fi ◄─► celular (web + alertas)
```

\* O cooler é **12V** → usar um **buck 24V→12V** para alimentá-lo. O servo SG90 é
alimentado em **5V** (do buck 24→5V), sinal por 1 GPIO. Resumo de tensões:
**24V** (aquecedor), **12V** (cooler), **5V** (servo + NodeMCU), **3.3V** (lógica,
regulada na própria NodeMCU). GND comum em tudo.

### Câmara externa de aquecimento e recirculação (decidido)

Decisão mecânica: o **bloco de aquecimento fica FORA da caixa**, numa **câmara
dedicada** acoplada a uma das faces. Motivo: a caixa é pequena e o bloco quente
(até ~320 °C no pior caso) não deve ocupar nem encostar no volume útil. Detalhes
construtivos completos em [`MECANICA.md`](MECANICA.md).

- **Câmara externa (~10×10 cm):** estrutura em **ABS** revestida com **tecido de
  fibra de vidro FM260FV** (suporta ~500 °C). Abriga o bloco (cartucho + termistor)
  em **standoffs metálicos** (não toca a parede). Acopla à caixa por uma abertura
  de **~80 mm** que aloja o **fan 80×80** (insuflação).
  - ⚠️ ABS amolece ~105 °C: o bloco **não pode encostar** na parede; o fluxo do
    cooler resfria a parede; o **fusível térmico** no bloco é a rede de segurança.
    Medir a temperatura da parede com o cooler ligado antes de confiar no ABS.
- **Recirculação (circuito quase fechado):** o ar de dentro da caixa volta à câmara
  pelo **furo de baixo** (a antiga entrada passiva), por um **duto impresso Ø60 mm**
  (~10 cm, PETG/ASA), passa pelo **bloco** e o fan o devolve aquecido à caixa.
  Insuflação em cima / retorno embaixo trabalha **a favor da convecção**; retorno
  em diagonal reduz curto-circuito de ar.
  - Ø60 (~65 % da área do fan) evita estrangular o fan 80 mm (baixa pressão
    estática). Furo de retorno ≥ 60 mm, **vedado**. Dimensionamento em `MECANICA.md`.
- **Caixa NÃO 100 % vedada:** há troca de ar passiva pela tampa/frestas — suficiente
  para um escape lento de umidade (auxiliado pela **sílica gel**). Não é caixa de
  alta tecnologia.

### Ventilação ativa (purga) — adiada para a Etapa Final

Arquitetura: **servo veda a ENTRADA de ar novo** (furos da câmara); a **SAÍDA**
(furo de cima) é uma **aba passiva unidirecional** (check flap). Sem 2º motor.

- **Servo fechado** → recirculação **selada** (o fan só puxa pelo duto de retorno).
- **Servo aberto** → o fan suga ar externo, aquece no bloco e **empurra** na caixa
  (purga forçada); o ar úmido sai pela aba passiva da saída, que **fecha sozinha**
  (gravidade) quando a caixa não está pressurizada — sem perda de calor pelo topo.
- **Por que na entrada, não na saída:** o fan puxa do lado da câmara; furos externos
  abertos fariam ele sugar ar de fora **o tempo todo**, contaminando a recirculação.
  Vedando a **entrada**, o loop fica limpo fechado e a purga é forçada aberto.
- Aciona **só quando** umidade absoluta externa (DHT11) < interna (DHT22) por margem
  (ver §5).
- **Decisão de prazo:** servo + purga ficam para a **Etapa Final** (§13). Nas etapas
  Essencial/Média a remoção de umidade é **passiva + sílica gel**.
- O usuário tem **1x SG90** (suficiente) e um **28BYJ-48** de reserva (não usado).
  Geometria das abas e peças: ver [`MECANICA.md`](MECANICA.md).

### BOM (lista de materiais)

| Item | Qtd | Obs |
|------|-----|-----|
| ESP8266 (NodeMCU / Wemos D1 mini) | 1 | placa principal |
| Cartucho cerâmico K1 24V/60W | 1 | aquecedor (já tem) |
| Bloco de alumínio do hotend | 1 | espalha o calor (já tem) |
| Termistor 100K NTC (~3950) | 1 | no bloco (já tem; 2º como reserva) |
| DHT22 | 2 | internos (já tem) |
| DHT11 | 1 | externo (já tem) |
| Cooler 80x80 | 1 | já tem |
| Servo SG90 | 1 | **entrada de ar novo** da câmara (já tem; 28BYJ-48 de reserva) |
| Aba passiva unidirecional na saída (impressa) | 1 | check flap; abre na purga, fecha por gravidade |
| MOSFET p/ aquecedor (ex.: IRLZ44N ou módulo) | 1 | low-side, PWM |
| MOSFET/driver p/ fan | 1 | se fan não tiver controle próprio |
| Fonte 24V (≥3A) | 1 | já tem |
| Buck 24V→12V | 1 | alimenta o cooler (12V) |
| Buck 24V→5V | 1 | alimenta NodeMCU + servo SG90 |
| Resistor série p/ divisor do termistor | 1 | calcular (ex.: 100K) |
| **Fusível térmico no bloco (~100 °C)** | 1 | segurança física |
| Fusível de corrente na linha 24V | 1 | segurança |
| Isolamento/standoff do bloco | — | bloco não toca o isopor |
| Sílica gel (desumidificante) | — | **principal** nas etapas iniciais (sem servo) |
| Câmara externa: ABS (impressão) + tecido fibra de vidro **FM260FV** | — | abriga o bloco fora da caixa (~500 °C) |
| Duto de recirculação Ø60 mm (impresso, PETG/ASA) | 1 | retorno do ar à câmara (~10 cm) |
| Standoffs/parafusos metálicos p/ o bloco | — | bloco não toca a parede da câmara |
| Pasta térmica | — | interface bloco ↔ cartucho/termistor |
| Gaxeta/espuma de vedação (EPDM) | — | vedar duto e (na fase final) a aba |

### Cuidados elétricos do ESP8266 NodeMCU (3.3V)

- Lógica 3.3V. DHTs funcionam em 3.3V.
- **A0 da NodeMCU já tem divisor on-board para 0–3,3V** — projetar o divisor do
  termistor (resistor série) para essa faixa. ADC de 10 bits e ruidoso: **fazer
  média de várias amostras**.
- Pinos com restrição de boot/flash (D0/D3/D4/D8) — evitar para saídas críticas.
  Mapear na fase de pinos (heater PWM, fan PWM, servo, 3x DHT).
- **GND comum** entre 24V, os bucks (12V/5V) e a NodeMCU.
- Servo SG90 puxa picos de corrente — alimentar do buck 5V (não do pino 3V3 da
  placa) e capacitor de desacoplamento perto do servo.

## 7. Sensores e posicionamento

- **Termistor → bloco de alumínio.** Sensor primário de controle e segurança do
  aquecedor. Calibrar por Steinhart-Hart ou equação beta (100K NTC ~3950).
- **DHT22 #1 → interno, perto do fluxo de ar do aquecedor.** Reação rápida do ar.
- **DHT22 #2 → interno, junto ao filamento (lado oposto).** Condição "real" do que
  está guardado.
- **DHT11 → externo.** Referência do ambiente: habilita ventilação inteligente e
  enriquece alertas/logs. (Lembrar: DHT11 é grosseiro e vai até ~50 °C — ok para
  ambiente.)
- DHT22 lê no máx. ~80 °C; como o alvo é ~50 °C (PLA/PETG), há folga.

## 8. Segurança (crítico — 60W em caixa de isopor)

Em camadas, da física ao firmware:

1. **Fusível térmico físico no bloco (~100 °C):** corta a energia mesmo se o ESP
   travar ou o MOSFET ficar em curto. Rede de segurança independente. Imprescindível.
2. **Fusível de corrente** na linha 24V.
3. **Limite de temperatura do bloco** (termistor) no firmware → desliga e trava
   em FALHA (exige reset).
4. **Limite de temperatura do ar** (DHT22) → idem.
5. **Thermal runaway (estilo Marlin):** comandou aquecer e o bloco **não sobe** no
   tempo esperado → falha (termistor solto, MOSFET aberto/curto) → corta.
6. **Sanidade dos sensores:** leitura fora de faixa / `NaN` / curto / aberto /
   divergência grande entre DHTs → corta.
7. **Intertravamento fan↔aquecedor:** não aquecer com o fan parado.
8. **Watchdog** + **estado seguro no boot** (aquecedor OFF até medir e validar).
9. **Wi-Fi é best-effort:** falha de rede/alerta **nunca** pode travar ou atrasar a
   segurança local. A segurança roda independente da conectividade.
10. **Bloco isolado do isopor** (standoff/isolamento) — nunca em contato direto.

## 9. IoT — interface e alertas

Interface (já que não há LCD): **servidor web local no ESP8266** acessado pelo
celular na mesma rede — mostra temp/umidade dos sensores, estado, modo, e permite
ligar/desligar e ajustar alvo de umidade/modo.

Alertas (push no celular): **Telegram Bot** (decidido). Grátis, via HTTPS, sem
servidor próprio. No firmware: guardar **token do bot** e **chat_id**; enviar com
cliente HTTPS do core ESP8266 (`WiFiClientSecure`). Tratar como segredo (não
commitar token — usar arquivo de config / `secrets.h` no `.gitignore`).

Eventos de alerta:
- Secagem concluída / alvo de umidade atingido
- Falha de sensor (NaN, curto, aberto, divergência)
- Superaquecimento / corte de segurança acionado
- Umidade alta detectada
- (opcional) Wi-Fi reconectado, boot do dispositivo

Regras: envio **não-bloqueante**, tolerante a queda de rede, com **cooldown** por
tipo de alerta (evitar spam).

## 10. Arquitetura de firmware (visão de alto nível)

Princípios: **não-bloqueante** (`millis()`, sem `delay()` longo — protege o Wi-Fi);
segurança independente da rede; estados explícitos.

Módulos:
- **sensores:** lê termistor (ADC + linearização) e DHTs (respeitando o intervalo
  mínimo de ~2s do DHT22); filtra leituras espúrias (filtro de salto já existe).
- **segurança:** roda todo ciclo, autoridade para **forçar aquecedor OFF** e travar
  em FALHA. Independente de tudo.
- **controle:** umidade-alvo → setpoint de temperatura → PWM do aquecedor; lógica
  de ventilação (comparação de umidade absoluta interna×externa).
- **rede:** Wi-Fi, servidor web, alertas. Best-effort.
- **persistência:** salvar configurações (alvo, modo, credenciais) em flash.

Máquina de estados:
```
BOOT/SELFTEST → IDLE → (SECAR | MANTER) → ... 
        └──────────────► FALHA (travado, exige reset) ◄── qualquer violação de segurança
```

## 11. Pendências / decisões em aberto

1. ~~Ventilação para o exterior~~ — **revisado:** bloco movido para **câmara
   externa** com **recirculação** (duto Ø60 no furo de baixo). Caixa **não vedada**
   (troca passiva). **Purga por servo SG90** no furo de cima **adiada para a Etapa
   Final** (§13). Ver §6 e [`MECANICA.md`](MECANICA.md).
2. ~~Canal de alerta~~ — **resolvido: Telegram Bot** (ver §9).
3. ~~Tensão do cooler~~ — **resolvido: 12V**, via buck 24V→12V (ver §6).
4. ~~Alvo de umidade~~ — **resolvido:** **configurável na web e ajustável em uso**,
   com busca da menor UR alcançável + detecção de platô (ver §5). Calibrar rodando.
5. ~~Desumidificante~~ — **resolvido: sim, usar sílica gel** como backup passivo
   (ajuda a atingir UR mais baixa, sobretudo em dias úmidos quando ventilar não
   ajuda). Já listada na BOM.
6. ~~Placa do ESP8266~~ — **resolvido: NodeMCU** (A0 com divisor on-board 0–3,3V).

### Pendência operacional
7. **Conectar a pasta local ao GitHub** (`d-camargo/DrySynth`). Hoje a pasta não é
   um repositório git; o remoto tem só `drysynth.ino`. Decidir estrutura (mover o
   sketch antigo, adicionar os `.md`) e fazer o primeiro commit. Ver conversa.

## 12. Roadmap

Roadmap completo, fase a fase, com checklists e portões de segurança em
[`ROADMAP.md`](ROADMAP.md). Resumo das fases:

0. Preparação e aquisição (BOM)
1. Alimentação na bancada (sem aquecer) ⚠️
2. Montagem mecânica
3. Bring-up do firmware: sensores (sem aquecer)
4. **Camada de segurança** ⚠️ (pré-requisito de aquecer)
5. Controle de aquecimento
6. Controle por umidade + ventilação inteligente
7. IoT: Wi-Fi e interface web
8. Alertas via Telegram
9. Validação de campo e ajuste fino
10. Documentação e encerramento

**Regra de ouro:** hardware/segurança antes de aquecer; a Fase 4 é pré-requisito
absoluto da Fase 5.

## 13. Etapas de entrega (priorização por prazo)

Há **pressa** no projeto. Por isso ele é dividido em **3 entregas incrementais** —
cada uma já é **utilizável** por si. A divisão é por *valor entregue*, não por área
técnica; as fases do `ROADMAP.md` se encaixam nelas (mapa no fim).

> **Inegociável:** a **camada de segurança** (cortes locais, fusível térmico,
> thermal runaway, watchdog, estado seguro) é obrigatória **já na Etapa Essencial**.
> Não existe versão "rápida" que ligue o aquecedor sem ela.

### 🟢 Etapa Essencial — "seca e é seguro" (MVP)

Objetivo: a caixa **aquece, recircula e seca** filamento **com segurança**, mesmo
que de forma menos eficiente e sem controle fino.

- **Mecânica:** câmara externa (bloco + cartucho + termistor) + acoplamento do fan
  + duto de retorno Ø60. Bloco em standoff/isolado. **Fusível térmico** no bloco.
- **Energia:** 24V + bucks (5V e 12V), **GND comum**, fusível de corrente.
- **Firmware mínimo** (não-bloqueante, `millis()`):
  - Termistor (controle/segurança) + **1x DHT22** (ar).
  - Aquecimento por **temperatura do bloco** (histerese + duty limitado); alvo de ar
    **fixo** razoável (~50 °C).
  - Cooler recirculando + **intertravamento** (não aquece sem fan).
  - **Camada de segurança completa** (Fase 4): estado seguro no boot, limites
    bloco/ar, thermal runaway, sanidade de sensor, watchdog, **FALHA travada**.
- **Remoção de umidade:** **passiva** (caixa não vedada) + **sílica gel** (principal
  aqui, pois ainda não há ventilação ativa).
- **Interface:** mínima — serial/USB ou web só-leitura; sem app/alertas.

➡️ **Resultado:** seca PLA/PETG com segurança. Menos eficiente (sem purga
inteligente, sem controle por umidade, 1 sensor de ar). *Cobre ROADMAP fases 0–5.*

### 🟡 Etapa Média — "controla por umidade e mostra no celular"

Objetivo: vira **produto controlado por umidade** e **monitorável**.

- **Sensores completos:** 2º DHT22 (lado do filamento) + DHT11 externo.
- **Controle por umidade-alvo** (malha externa) → setpoint de temperatura; **modos**
  Secar/Manter/Desligado; **detecção de platô** (não aquece pra sempre).
- **IoT:** Wi-Fi + **servidor web** (status + controle pelo celular); **persistência**
  de config na flash.
- **Alertas Telegram** (secagem concluída, falha de sensor, superaquecimento,
  umidade alta), não-bloqueantes com **cooldown**.

➡️ **Resultado:** controlado por umidade, ajustável e monitorável pelo celular.
*Cobre ROADMAP fases 7–8 + parte da 6.*

### 🔵 Etapa Final — "eficiência e autonomia"

Objetivo: **máxima eficiência de secagem** e refino.

- **Ventilação inteligente:** **servo SG90 na entrada de ar novo** (furos da câmara)
  + **aba passiva unidirecional na saída** → **purga forçada** por umidade absoluta
  (DHT11 < DHT22). Geometria/peças em [`MECANICA.md`](MECANICA.md). *(servo adiado
  para cá por decisão de prazo.)*
- **PID** no bloco (se o overshoot incomodar); **OTA**.
- **Calibração de campo**, teste de longa duração, ajuste de margens/cooldowns; UR
  mínima realista.

➡️ **Resultado:** secagem mais eficiente e autônoma, com ventilação inteligente.
*Cobre ROADMAP fase 6 (ventilação) + 9–10.*
