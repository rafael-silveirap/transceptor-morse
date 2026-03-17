/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void Tocar_Bip(uint32_t duracao);
void Processar_Transmissao_USB(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define TEMPO_PONTO_MIN   50
#define TEMPO_PONTO_MAX   190
#define TEMPO_TRACO_MIN   200
#define TEMPO_TRACO_MAX   450
#define TEMPO_FIM_LETRA   250
#define TEMPO_FIM_PALAVRA 600

#define TEMPO_BASE_TX     100

// Variáveis do Microfone (RX)
volatile uint32_t tempo_inicio_som = 0;
volatile uint32_t tempo_ultimo_som = 0;
volatile uint8_t estado_som = 0;
char buffer_letra[10] = "";
volatile uint8_t indice_buffer = 0;
uint8_t espaco_enviado = 1;

// Variáveis do USB
uint8_t usb_rx_buffer[256];
uint16_t usb_rx_len = 0;
volatile uint8_t usb_rx_ready = 0;

// Proteção Half-Duplex
volatile uint8_t is_transmitting = 0;


extern TIM_HandleTypeDef htim2;

// Dicionário Morse
const char* dicionario_morse[26] = {
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", // A - H
    "..",   ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", // I - P
    "--.-", ".-.",  "...",  "-",    "..-",  "...-", ".--",  "-..-", // Q - X
    "-.--", "--.."                                                  // Y - Z
};

char Decodificar_Letra(char* morse) {
    for (int i = 0; i < 26; i++) {
        if (strcmp(morse, dicionario_morse[i]) == 0) return 'A' + i;
    }
    return '?';
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
        // Recebe morse
        if (usb_rx_ready == 1)
        {
            Processar_Transmissao_USB();
        }

        // Decodifica microfone
        else if (estado_som == 0 && is_transmitting == 0)
        {
            uint32_t tempo_silencio = HAL_GetTick() - tempo_ultimo_som;

            // Fecha a letra
            if (tempo_silencio > TEMPO_FIM_LETRA && indice_buffer > 0)
            {
                char letra_traduzida = Decodificar_Letra(buffer_letra);
                if (letra_traduzida != '?') {
                    uint8_t tx_buffer[1] = {letra_traduzida};
                    CDC_Transmit_FS(tx_buffer, 1);
                }
                indice_buffer = 0;
                buffer_letra[0] = '\0';
            }

            // Fecha a palavra
            if (tempo_silencio > TEMPO_FIM_PALAVRA && espaco_enviado == 0)
            {
                uint8_t tx_espaco[1] = {' '};
                CDC_Transmit_FS(tx_espaco, 1);
                espaco_enviado = 1;
            }
        }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 24999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 12499;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Tocar_Bip(uint32_t duracao) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_Delay(duracao);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
}

void Processar_Transmissao_USB(void) {
    is_transmitting = 1; // FICA SURDO PARA O MICROFONE
    HAL_Delay(200);      // Dá tempo do circuito estabilizar


    for(int i = 0; i < usb_rx_len; i++) {
        char c = usb_rx_buffer[i];

        if (c == '.') {
            Tocar_Bip(TEMPO_BASE_TX);
            HAL_Delay(TEMPO_BASE_TX);
        }
        else if (c == '-') {
            Tocar_Bip(TEMPO_BASE_TX * 3);
            HAL_Delay(TEMPO_BASE_TX);
        }
        else if (c == ' ') {
            HAL_Delay(TEMPO_BASE_TX * 3);
        }
        else if (c == '/') {
            HAL_Delay(TEMPO_BASE_TX);
        }
    }

    // Limpeza de segurança após apitar
    estado_som = 0;
    indice_buffer = 0;
    usb_rx_ready = 0;

    HAL_Delay(500);
    is_transmitting = 0; // VOLTA A ESCUTAR O MICROFONE
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(is_transmitting == 1) return;

    if(GPIO_Pin == GPIO_PIN_12)
    {
    	// Lê se o pino está em 0V (RESET) ou 3.3V/5V (SET)
    	        GPIO_PinState estado = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

    	        if(estado == GPIO_PIN_RESET)
    	        {
    	            // ------------------------------------------------
    	            // O SOM COMEÇOU (A saída do NE555 foi para 0V)
    	            // ------------------------------------------------
    	            tempo_inicio_som = HAL_GetTick(); // Liga o cronômetro
    	            estado_som = 1;                   // Avisa que está fazendo barulho
    	            espaco_enviado = 0;               // Prepara para a próxima letra
    	        }
    	        else
    	        {
    	            // ------------------------------------------------
    	            // O SOM TERMINOU (A saída do NE555 voltou para VCC)
    	            // ------------------------------------------------
    	            tempo_ultimo_som = HAL_GetTick(); // Marca a hora que acabou
    	            estado_som = 0;                   // Avisa que ficou em silêncio

    	            // Calcula a duração exata do bipe em milissegundos
    	            uint32_t duracao = tempo_ultimo_som - tempo_inicio_som;

    	            // Classifica o som e guarda no buffer
    	            if(duracao >= TEMPO_PONTO_MIN && duracao < TEMPO_PONTO_MAX)
    	            {
    	                // É um PONTO (.)
    	                if(indice_buffer < 9) buffer_letra[indice_buffer++] = '.';
    	            }
    	            else if(duracao >= TEMPO_TRACO_MIN && duracao < TEMPO_TRACO_MAX)
    	            {
    	                // É um TRAÇO (-)
    	                if(indice_buffer < 9) buffer_letra[indice_buffer++] = '-';
    	            }

    	            // Finaliza a string de texto para segurança
    	            buffer_letra[indice_buffer] = '\0';
    	        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
