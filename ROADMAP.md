# Roadmap — DrySynth

Plano de execução, fase a fase. Cada fase tem **objetivo**, **tarefas** (checklist)
e um **portão de saída** (o que precisa estar verdadeiro para avançar). Detalhes de
desenho e justificativas: ver [`PLANEJAMENTO.md`](PLANEJAMENTO.md).

**Princípio de ordem:** hardware e segurança **antes** de aquecer. O aquecedor só é
energizado de verdade na Fase 5 — e só depois da camada de segurança (Fase 4)
existir e ter sido testada.

Legenda: `[ ]` a fazer · `[~]` em andamento · `[x]` feito · ⚠️ portão de segurança.

---

## Fase 0 — Preparação e aquisição

**Objetivo:** ter todas as peças e ferramentas na bancada.

- [ ] Revisar a BOM do `PLANEJAMENTO.md`
- [ ] Adquirir o que falta:
  - [ ] Buck **24V→12V** (para o cooler)
  - [ ] Buck **24V→5V** (para NodeMCU + servo)
  - [ ] **MOSFET** para o aquecedor (IRLZ44N / módulo low-side com PWM)
  - [ ] Driver/MOSFET para o cooler (se o fan não tiver controle próprio)
  - [ ] **Fusível térmico ~100 °C** (para o bloco)
  - [ ] Fusível de corrente para a linha 24V
  - [ ] Resistor série do divisor do termistor (calcular na Fase 3)
  - [ ] Pasta térmica, fios, conectores, borne, capacitor de desacoplamento p/ servo
  - [ ] Sílica gel
- [ ] Confirmar que já tem: NodeMCU, cartucho K1, bloco de alumínio, termistor,
      2x DHT22, DHT11, cooler 80x80, servo SG90, fonte 24V

**Portão:** todas as peças disponíveis e identificadas.

---

## Fase 1 — Alimentação na bancada (sem aquecer) ⚠️

**Objetivo:** árvore de energia montada e medida, antes de qualquer carga perigosa.

- [ ] Montar fonte 24V → bucks (12V e 5V); ajustar a saída de cada buck **com
      multímetro** antes de conectar cargas
- [ ] **GND comum** entre fonte, bucks e NodeMCU
- [ ] Fusível de corrente na linha 24V instalado
- [ ] Alimentar a NodeMCU pelo buck 5V; confirmar que liga e fica estável
- [ ] Conferir folga de corrente: 24V suporta aquecedor (~2,5A) + margem

**Portão:** ⚠️ tensões corretas e estáveis (24/12/5/3.3V), GND comum, fusível no
lugar. **Aquecedor ainda NÃO conectado.**

---

## Fase 2 — Montagem mecânica

**Objetivo:** elementos posicionados na caixa de forma segura.

- [ ] Inserir o **cartucho** e o **termistor** no bloco de alumínio (pasta térmica;
      termistor bem fixo — termistor solto = leitura falsa = risco)
- [ ] **Isolar o bloco do isopor** (standoff/isolante) — bloco quente nunca toca a
      caixa
- [ ] Prender o **fusível térmico** em contato com o bloco
- [ ] Posicionar o **cooler** para empurrar ar sobre o bloco e em direção ao furo
      de cima (saída)
- [ ] Montar a **aba + servo SG90** no furo de cima (servo por fora, fora do jato
      de ar quente); furo de baixo livre (entrada)
- [ ] Posicionar sensores: **DHT22 #1** perto do fluxo, **DHT22 #2** no lado do
      filamento, **DHT11** do lado de fora
- [ ] Passar a fiação para fora até a NodeMCU/bornes

**Portão:** tudo fixo, bloco isolado da caixa, aba abre/fecha livremente à mão.

---

## Fase 3 — Bring-up do firmware: sensores (sem aquecer)

**Objetivo:** ler e calibrar **todos** os sensores, com o aquecedor ainda OFF.

- [ ] Configurar a IDE/`arduino-cli` para **ESP8266 NodeMCU**
- [ ] Definir o **mapa de pinos** (heater PWM, fan PWM, servo, 3x DHT, A0 termistor),
      evitando D0/D3/D4/D8 para saídas críticas — documentar no código e no `CLAUDE.md`
- [ ] Esqueleto de firmware **não-bloqueante** (`millis()`, sem `delay()` longo)
- [ ] Ler o **termistor** no A0: projetar o divisor, fazer **média de amostras**,
      linearizar (Steinhart-Hart ou beta 3950); **calibrar** contra um termômetro
- [ ] Ler **3x DHT** (respeitar intervalo mínimo ~2s do DHT22); reaproveitar o
      filtro de salto e o cálculo de **umidade absoluta** do sketch legado
- [ ] Mover o servo (abrir/fechar a aba) e pulsar o cooler em PWM, validando o
      movimento/fluxo

**Portão:** leituras coerentes e estáveis de termistor + 3 DHTs; servo e cooler
respondem. Aquecedor ainda OFF.

---

## Fase 4 — Camada de segurança ⚠️ (antes de aquecer de verdade)

**Objetivo:** todos os cortes de proteção existindo e **testados forçando falhas**.

- [ ] **Estado seguro no boot**: aquecedor OFF até medir e validar sensores
- [ ] **Limite de temp do bloco** (termistor) → desliga e trava em FALHA
- [ ] **Limite de temp do ar** (DHT22) → idem
- [ ] **Thermal runaway** (estilo Marlin): comandou aquecer e o bloco não sobe no
      tempo esperado → FALHA
- [ ] **Sanidade dos sensores**: fora de faixa / `NaN` / curto / aberto /
      divergência grande entre DHTs → corta
- [ ] **Intertravamento**: não aquecer com o cooler parado
- [ ] **Watchdog** habilitado
- [ ] Estado **FALHA travado** (exige reset), independente do Wi-Fi
- [ ] **Testar cada proteção forçando a falha** (desconectar termistor, segurar o
      DHT, parar o fan, etc.) com o aquecedor ainda desconectado ou em duty mínimo

**Portão:** ⚠️ cada proteção comprovadamente corta/trava quando provocada. **Só
depois deste portão o aquecedor é energizado em operação.**

---

## Fase 5 — Controle de aquecimento

**Objetivo:** aquecer de forma controlada e segura, pela temperatura do bloco.

- [ ] Ligar o **MOSFET do aquecedor** com **PWM**; começar com **duty limitado**
- [ ] Controle pela **temperatura do bloco**: histerese + limite de duty
- [ ] **Teto de temperatura** do bloco e do ar respeitados (PLA/PETG: ar ~50 °C)
- [ ] Primeiros aquecimentos **monitorados de perto** (medir bloco e ar)
- [ ] Caracterizar overshoot/inércia; se necessário, evoluir para **PID** no bloco
- [ ] Confirmar que as proteções da Fase 4 atuam **com o aquecedor real ligado**

**Portão:** caixa aquece até o alvo de ar sem overshoot perigoso; proteções ativas.

---

## Fase 6 — Controle por umidade + ventilação inteligente

**Objetivo:** a lógica-fim do produto — manter UR baixa.

- [ ] Malha de **umidade**: alvo de %UR (configurável) → setpoint de temperatura,
      dentro dos limites
- [ ] **Ventilação inteligente**: abrir servo + cooler para purga **só quando**
      umidade absoluta externa (DHT11) < interna (DHT22) por margem
- [ ] **Detecção de platô**: UR parou de cair com tudo no limite → conclui secagem
      → muda para "manter" (não aquece pra sempre)
- [ ] **Modos**: Secar / Manter / Desligado
- [ ] Calibrar a **UR mínima alcançável** da caixa (definir alvos realistas)

**Portão:** a caixa seca até a menor UR viável e mantém, sem ficar presa em alvo
impossível.

---

## Fase 7 — IoT: Wi-Fi e interface web

**Objetivo:** monitorar e controlar pelo celular.

- [ ] Conectar Wi-Fi (credenciais em `secrets.h`, fora do git)
- [ ] **Servidor web local**: status (sensores, estado, modo) + controles
      (liga/desliga, modo, alvo de UR)
- [ ] **Persistência** de configuração (alvo, modo) na flash
- [ ] Garantir que Wi-Fi/web são **best-effort** e não atrasam a segurança local

**Portão:** dá pra ver e controlar tudo pelo navegador do celular na rede local.

---

## Fase 8 — Alertas via Telegram

**Objetivo:** avisos por evento no celular.

- [ ] Criar o **bot** (token) e obter o **chat_id**; guardar em `secrets.h`
- [ ] Envio via `WiFiClientSecure` (HTTPS), **não-bloqueante**
- [ ] Eventos: secagem concluída/alvo atingido · falha de sensor · superaquecimento/
      corte de segurança · umidade alta · (opcional) boot/Wi-Fi reconectado
- [ ] **Cooldown** por tipo de alerta (anti-spam)

**Portão:** alertas chegam no celular nos eventos certos, sem travar o controle.

---

## Fase 9 — Validação de campo e ajuste fino

**Objetivo:** provar com filamento real e calibrar.

- [ ] Secar um filamento real (PLA/PETG) e medir a curva de UR ao longo do tempo
- [ ] Ajustar **alvos de UR**, parâmetros de **PID**, margens de ventilação e
      **cooldown** de alertas
- [ ] Teste de **longa duração** observando estabilidade e segurança
- [ ] Conferir comportamento da sílica gel como backup

**Portão:** opera sozinha por horas com segurança e resultado de secagem aceitável.

---

## Fase 10 — Documentação e encerramento

**Objetivo:** deixar o projeto reprodutível.

- [ ] Atualizar `PLANEJAMENTO.md` e `CLAUDE.md` com o que mudou na prática
- [ ] **Esquema elétrico** / diagrama de ligações final
- [ ] Mapa de pinos final documentado
- [ ] `README.md` no repositório (visão geral, fotos, como compilar/flashar)
- [ ] Tag/release no GitHub

**Portão:** outra pessoa (ou você no futuro) consegue reconstruir a partir do repo.

---

## Dependências e marcos de segurança

- **Hardware (0→2)** antes de **firmware (3→8)**.
- **Fase 4 (segurança) é pré-requisito absoluto da Fase 5 (aquecer).** Não pular.
- IoT (7) e alertas (8) podem ser feitos em paralelo ao ajuste fino, mas **nunca**
  podem comprometer a autonomia da camada de segurança.
- Em qualquer fase, ao tocar no aquecedor: rever os portões ⚠️.
