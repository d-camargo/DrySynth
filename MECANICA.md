# Mecânica — DrySynth

Desenho mecânico do dispositivo: **como o ar circula**, **onde** ficam os
elementos, **como** montar e **quais peças** são necessárias. Complementa o
[`PLANEJAMENTO.md`](PLANEJAMENTO.md) (§6) e segue o `ROADMAP.md`.

Princípio central: **o bloco de aquecimento fica FORA da caixa térmica**, numa
**câmara externa** dedicada, e o ar da caixa **recircula** por essa câmara passando
pelo bloco. A caixa é pequena — tirar o bloco de dentro libera volume útil e afasta
o ponto quente (até ~320 °C no pior caso) do isopor.

---

## 1. Mapa de ar (visão geral)

```
                          CAIXA TÉRMICA (não 100% vedada)
                       ┌───────────────────────────────────┐
   saída/purga  ◄══════┤▒ ABA PASSIVA (unidirecional)       │
   (furo de cima)      │   abre na purga, cai/veda sozinha  │
                       │        ░░ filamento ░░             │
   ┌──────────────┐    │                                    │
   │  CÂMARA      │    │                                    │
   │  EXTERNA     │    │                                    │
   │  ┌────────┐  │    │                                    │
   │  │ BLOCO  │  │    │                                    │
   │  │ +cartu.│  │    │                                    │
   │  │ +termi.│  │    │                                    │
   │  └────────┘  │    │                                    │
   │   [FAN 80]──────► ├─► insuflação (ar quente p/ a caixa)│
   │      ▲       │    │                                    │
   │      │       │    │                                    │
   │  ENTRADA ar  │    │                                    │
   │  novo +SERVO │    │                                    │
   │ (veda/abre)  │    │                                    │
   └──────┬───────┘    └──────────────┬────────────────────┘
          │                           │
          │   duto Ø60 (~10cm)        │  furo de baixo
          └◄──────────────────────────┘  (antiga entrada = RETORNO)
                   ar de RETORNO da caixa
```

**Circuito de recirculação (sempre ativo):**
caixa → **furo de baixo** → **duto Ø60** → câmara → **bloco** (aquece) → **fan** →
**insuflação** de volta na caixa.

- **Insuflação em cima / retorno embaixo** = trabalha a favor da convecção natural
  (quente sobe; puxa-se o mais frio de baixo).
- **Retorno em diagonal** (entra pela face do fan em cima, sai pela base) reduz o
  **curto-circuito de ar** (o jato quente não é sugado de volta na hora).

**Troca com o exterior:**
- **Etapas Essencial/Média:** a caixa **não é 100 % vedada** — escapa umidade
  lentamente pela **tampa/frestas**; a **sílica gel** faz o grosso da desumidificação.
- **Etapa Final — purga forçada:** o **servo veda a ENTRADA de ar novo** (furos da
  câmara). Fechado = recirculação selada; aberto = o fan suga ar externo, aquece e
  **empurra** na caixa, expulsando o ar úmido por uma **aba passiva** na saída (furo
  de cima). Ver §4.

> **Por que o servo na entrada (e não na saída)?** O fan puxa do lado da **câmara**;
> se os furos externos ficassem abertos, ele sugaria ar de fora **o tempo todo**,
> contaminando a recirculação. Vedando a **entrada**, o loop fica limpo quando
> fechado e a purga é **forçada** quando aberto. A saída fica passiva.

---

## 2. Câmara externa de aquecimento

A peça-chave nova. Abriga o bloco e acopla na face da caixa onde fica o fan.

| Atributo | Valor / decisão |
|---|---|
| Dimensão aproximada | ~**10 × 10 cm** (ajustar ao bloco + fan) |
| Estrutura | **ABS** (impresso) |
| Revestimento térmico | **Tecido de fibra de vidro FM260FV** (~500 °C) |
| Acoplamento à caixa | abertura ~**80 mm** que aloja o **fan 80×80** |
| Conteúdo | bloco de alumínio + cartucho 24V/60W + termistor 100K |
| Entradas extras | furos de ar externo (**só Etapa Final**, vide §4) |

### Onde colocar
- Acoplada **por fora**, na face da caixa, com o **fan no acoplamento** (insufla ar
  quente para dentro). O bloco fica **atrás do fan**, dentro da câmara.
- A boca de **retorno** da câmara recebe o **duto Ø60** que vem do furo de baixo.

### Como montar (e cuidados)
- **Bloco em standoffs metálicos**, **sem encostar** na parede de ABS. Razão: ABS
  amolece na transição vítrea **~105 °C** e libera estireno (fumaça tóxica) se
  esquentar. O tecido FM260FV protege contra calor radiante/contato, mas o **ABS
  atrás dele é o elo fraco** — quem segura o pior caso é o **fusível térmico**.
- **Pasta térmica** entre cartucho/termistor e o bloco; **termistor bem fixo**
  (termistor solto = leitura falsa = risco).
- **Fusível térmico (~100 °C)** em contato com o bloco — rede de segurança física
  independente do firmware.
- **Fan na entrada → bloco → caixa**: o ar passa pelo bloco e o fan empurra/puxa
  para a caixa. A ~50 °C um fan de PC opera bem; ainda assim, os cortes de
  segurança (Fase 4) nunca podem deixar o bloco disparar — o fan está no caminho do
  ar quente.
- **Medir a temperatura da parede da câmara** com o cooler ligado **antes** de
  confiar no ABS. Se esquentar demais perto do bloco, usar **chapa de alumínio
  fina** nessa região (ou afastar mais o bloco / reforçar o fluxo).
- **Isolamento:** a câmara quente não deve transmitir calor excessivo para a face de
  isopor da caixa no ponto de acoplamento.

---

## 3. Duto de recirculação (Ø60 mm)

Leva o ar de **dentro da caixa** de volta à câmara.

| Atributo | Valor / decisão |
|---|---|
| Diâmetro interno | **Ø60 mm** (~65 % da área do fan 80 mm; não estrangula) |
| Comprimento | **curto, ~10 cm** (menos perda; fan 80 mm tem pouca pressão estática) |
| Material | **PETG** (mín.) ou **ASA/ABS** na ponta junto à câmara — **PLA não** (amolece ~55–60 °C) |
| Furo de retorno | a **antiga entrada de ar**, alargada para **≥ 60 mm** |
| Vedação | **gaxeta/espuma** nas duas pontas (vazamento puxa ar da sala) |

### Onde colocar
- Boca no **furo de baixo** da caixa (~10 cm abaixo do acoplamento do fan), do lado
  de **dentro**; outra ponta na **boca de retorno da câmara**.

### Como montar (impressão)
- **Curto e direto**; curvas **suaves** (nada de 90° vivo).
- **Adaptador quadrado→redondo** cônico do fan 80×80 para o Ø60 (transição gradual,
  sem degrau brusco que gera turbulência).
- **Parede interna lisa** (mais perímetros; evitar suportes internos que deixam
  degraus).
- **Flange + gaxeta** em cada ponta; se o furo antigo for menor que 60 mm, **alargar**
  ou imprimir flange adaptador.

---

## 4. Mecanismo de ventilação ativa (purga) — Etapa Final

Adiado para a **Etapa Final** por decisão de prazo. Antes disso a caixa respira
passivamente e a sílica desumidifica.

**Arquitetura (decidida):** o **servo veda a ENTRADA de ar novo** (furos da câmara)
e a **SAÍDA** (furo de cima) é uma **aba passiva unidirecional**. Função: purgar o
ar úmido quando o ar externo está **mais seco** (umidade absoluta DHT11 < DHT22).

> **Por que o servo na entrada (e não na saída)?** O fan puxa do lado da **câmara**.
> Com os furos externos abertos, ele sugaria ar de fora **o tempo todo**,
> contaminando a recirculação e jogando fora ar quente/seco. Vedando a **entrada**,
> o loop fica **selado** quando fechado e a purga é **forçada** quando aberto — sem
> precisar de 2º motor na saída.

### 4.1 Servo na ENTRADA de ar novo (furos da câmara)
- **Onde:** nos furos de ar externo da **câmara** (lado de fora, antes do bloco),
  com o servo por **fora** e **fora do jato** quente.
- **Função:**
  - **Fechado** → recirculação **selada** (o fan só puxa pelo duto de retorno).
  - **Aberto** → o fan suga ar externo, aquece no bloco e **empurra** na caixa =
    **purga forçada**.
- **Geometria (3 opções):**
  1. **Aba basculante (recomendada):** pá impressa articulada que o braço do SG90
     empurra/recolhe (~0–90°). Simples e tolerante a folga.
  2. **Damper borboleta:** pá no eixo girando 90° no furo (direto no horn). Compacto.
  3. **Guilhotina:** placa deslizante por alavanca. Veda bem, mais peças/atrito.
  - Como é **entrada** (lado frio), a temperatura é menos crítica que na saída.
- **Vedação:** gaxeta/espuma EPDM na borda; a aba precisa **fechar de fato** (senão o
  fan ingere ar externo na recirculação).

### 4.2 Aba passiva na SAÍDA (furo de cima) — sem motor
- **Função:** válvula **unidirecional** (check flap), só deixa o ar **sair**:
  - **Recirculação:** entrada fechada → caixa não pressuriza → a aba **cai por
    gravidade e veda**. Não perde calor por convecção pelo topo.
  - **Purga:** entrada aberta → o fan pressuriza a caixa → o ar **empurra a aba** e
    sai; fecha sozinha depois.
- **Como fazer:** pá leve impressa (**PETG/ASA** — vê ar a ~50 °C), articulada na
  **borda superior** do furo, abrindo **para fora**. Leve o bastante para pouca
  pressão abrir; **batente + gaxeta** para vedar fechada.

### Montagem do servo (entrada)
- **Suporte impresso** fixa o corpo do SG90 fora do jato quente.
- **5 V** do buck (não do pino 3V3 da placa); **capacitor** (~470–1000 µF) perto do
  servo; **sinal por 1 GPIO**.
- Aciona **só** quando umidade absoluta externa < interna por margem (ver
  `PLANEJAMENTO.md` §5).

### Peças do mecanismo
| Item | Qtd | Obs |
|---|---|---|
| Servo **SG90** | 1 | na **ENTRADA** de ar novo (28BYJ-48 de reserva, não usado) |
| Suporte do servo (impresso, PETG/ASA) | 1 | fixa o corpo fora do jato quente |
| Aba da entrada + eixo (impresso) | 1 | basculante/borboleta, acionada pelo servo |
| **Aba passiva da saída** + articulação (impresso PETG/ASA) | 1 | unidirecional, abre por pressão |
| Gaxeta/espuma EPDM | — | vedar entrada + batente da saída |
| Capacitor de desacoplamento (~470–1000 µF) | 1 | perto do servo |
| Parafusos / horn / fios | — | montagem e sinal (1 GPIO) |

---

## 5. Posicionamento dos sensores (resumo mecânico)

Detalhe funcional em `PLANEJAMENTO.md` §7; aqui só o **onde físico**:

- **Termistor 100K** → **no bloco** de alumínio, dentro da câmara (controle +
  segurança). Bem fixo, com pasta térmica.
- **DHT22 #1** → **interno, perto do fluxo** de insuflação (reação rápida do ar).
- **DHT22 #2** → **interno, lado do filamento** (condição "real" do que é guardado).
- **DHT11** → **externo** à caixa (referência de ambiente; habilita a purga
  inteligente da Etapa Final).
- Passar toda a fiação **para fora** até a NodeMCU/bornes; cabos longe do bloco.

---

## 6. Ordem de montagem mecânica (resumo)

1. Imprimir **câmara** (ABS) e **duto Ø60** (PETG/ASA); revestir a câmara com o
   **tecido FM260FV**.
2. Montar **bloco + cartucho + termistor** (pasta térmica, termistor firme) em
   **standoffs** dentro da câmara; prender o **fusível térmico** no bloco.
3. Instalar o **fan 80×80** no acoplamento; acoplar a câmara à face da caixa.
4. Alargar o **furo de baixo** para ≥ 60 mm; montar o **duto** câmara↔caixa com
   **gaxeta** nas pontas.
5. Posicionar **sensores** (termistor no bloco; 2× DHT22 internos; DHT11 externo);
   passar a fiação para fora.
6. *(Etapa Final)* Montar o **servo SG90 na ENTRADA de ar novo** (furos da câmara) e
   a **aba passiva unidirecional na SAÍDA** (furo de cima); gaxetas nas bordas.

**Portões de segurança** (ver `ROADMAP.md`): bloco **isolado** da caixa, bloco
**nunca** encosta no ABS/isopor, **fusível térmico** instalado, **fan obrigatório**
antes de aquecer (intertravamento), parede de ABS **medida** sob fluxo antes de
operar de verdade.
