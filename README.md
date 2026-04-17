# Transceptor de Código Morse via Processamento de Sinais Analógicos

## 1. Visão Geral
Este projeto consiste em um sistema embarcado desenvolvido para a transcepção e decodificação de sinais de Código Morse. O diferencial técnico desta implementação é o uso de uma **cadeia de condicionamento de sinal analógico** dedicada, que permite uma digitalização limpa e reduz significativamente a necessidade de processamento digital pesado para filtragem de ruído.

---

## 2. Arquitetura do Sistema

O fluxo de processamento foi projetado para garantir máxima fidelidade na conversão do sinal sonoro para caracteres alfanuméricos.

### A. Front-end Analógico (Hardware)
Para evitar o uso de algoritmos complexos de FFT ou filtros digitais que consumiriam ciclos de clock desnecessários, o sinal passa por:

1.  **Captação:** Microfone de eletreto sensível para entrada de áudio.
2.  **Detector de Envoltório (Envelope Detector):** Um circuito retificador com filtro $RC$ que extrai a amplitude da onda sonora, eliminando a frequência da portadora de áudio e mantendo apenas a "forma" do pulso.
3.  **Schmitt Trigger:** Um comparador com **histerese** que transforma o envoltório analógico em um sinal digital quadrado perfeito. Isso elimina o *jitter* e garante que o microcontrolador receba apenas transições de estado sólido ($0$ ou $1$).

### B. Firmware e Lógica de Tempo (Software)
O software atua como um cronômetro de precisão, medindo a duração dos pulsos recebidos para classificar os símbolos conforme o padrão internacional:

* **Ponto (.):** Unidade base de tempo $t$.
* **Traço (-):** Equivalente a $3t$.
* **Espaço entre símbolos:** $1t$.
* **Espaço entre letras:** $3t$.
* **Espaço entre palavras:** $7t$.

---

## 3. Especificações Técnicas
* **Microcontrolador:** STM32F411CEU6
* **Linguagem:** C
* **Input:** Sinal analógico via Microfone + Circuito Comparador.
* **Output:** Decodificação em tempo real via Serial/Display.

---

## 4. Decisões de Engenharia e Trade-offs
* **Hardware vs Software:** A escolha de usar um **Schmitt Trigger** físico em vez de um `analogRead()` com limiar via software foi tomada para garantir **determinismo**. Isso evita atrasos causados pela conversão AD (Analógico-Digital) e permite que o sistema reaja instantaneamente a mudanças de sinal.
* **Robustez:** O uso de filtros analógicos torna o sistema mais resiliente a ruídos de alta frequência presentes no ambiente, algo essencial em comunicações de rádio e defesa.

---
