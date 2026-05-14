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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

// STM32F103C8T6 için son flash sayfası adresi (blink_count burada saklanır)
#define FLASH_ADDRESS 0x0801FC00

// LED'in kaç kere yanıp söneceği (4-7 arası)
volatile uint8_t blink_count = 4;

// Durum makinesi: 0 = yanıp sönme modu, 1 = 5 saniye bekleme modu
volatile uint8_t state = 0;

// Kaçıncı toggle'da olduğumuzu takip eder (blink_count*2 toggle = blink_count yanıp sönme)
volatile uint8_t blink_internal_idx = 0;

// Bekleme modunda kaç saniye geçti
volatile uint32_t wait_seconds = 0;

// Butona basılma anının zamanı (3sn kontrolü için)
volatile uint32_t manal_press_start = 0;

// Buton şu an basılı mı? (0=hayır, 1=evet)
volatile uint8_t manal_pressed = 0;

/* USER CODE END PV */



/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Flash belleğe 32-bit değer yazar
// Önce sayfayı siler, sonra yazar (STM32 flash önce silinmeden yazılamaz)
void Flash_Write(uint32_t value) {
  HAL_FLASH_Unlock();                          // Flash kilidini aç
  FLASH_EraseInitTypeDef erase;
  uint32_t pageError;
  erase.TypeErase = FLASH_TYPEERASE_PAGES;     // Sayfa silme modu
  erase.PageAddress = FLASH_ADDRESS;           // Silinecek sayfa adresi
  erase.NbPages = 1;                           // 1 sayfa sil
  HAL_FLASHEx_Erase(&erase, &pageError);       // Sayfayı sil
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,    // 32-bit yaz
                    FLASH_ADDRESS, value);
  HAL_FLASH_Lock();                            // Flash kilidini kapat
}

// Flash bellekten 32-bit değer okur
uint32_t Flash_Read(void) {
  return *(__IO uint32_t*)FLASH_ADDRESS;
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

  /* USER CODE BEGIN 2 */

  // ----- ADIM e: Flash'tan blink_count değerini yükle -----
  blink_count = (uint8_t)Flash_Read();

  // ----- ADIM f: Flash boşsa veya geçersiz değerse 4 yap -----
  // Flash yazılmamışsa tüm byte'lar 0xFF olur, uint32 olarak 0xFFFFFFFF
  // uint8_t olarak cast edilince 255 olur, bu da 7'den büyük, yani geçersiz
  if (blink_count < 4 || blink_count > 7) {
      blink_count = 4;
      Flash_Write(blink_count);
  }

  // ----- ADIM g (boot): Güç verilirken buton 3sn basılı tutulursa fabrika ayarı -----
  // Buton pull-down olduğu için normalde 0, basılınca 3.3V = GPIO_PIN_SET
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
      uint32_t manal_boot_start = HAL_GetTick();
      while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
          if ((HAL_GetTick() - manal_boot_start) >= 3000) {
              blink_count = 4;              // Fabrika ayarına dön
              Flash_Write(blink_count);    // Flash'a kaydet
              break;
          }
      }
  }

  // Timer interrupt'ı başlat (saniyede 1 kere tetiklenecek)
  HAL_TIM_Base_Start_IT(&htim2);

  /* USER CODE END 2 */


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
while (1)
{
    // PA1 lojik 0 kalmalı [cite: 52, 53]
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
        uint32_t press_start = HAL_GetTick();
        
        // Buton bırakılana kadar bekle (Bloklamasız ölçüm için alternatifler var ama bu ödev seviyesi için uygundur)
        while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
        uint32_t duration = HAL_GetTick() - press_start;

        if (duration >= 3000) { // 3 saniye basılı tutulursa [cite: 50, 51]
            blink_count = 4;
        } else if (duration > 50) { // Kısa basım (Debounce sonrası) [cite: 43]
            blink_count++;
            if (blink_count > 7) blink_count = 4; // 7'den büyükse 4 yap [cite: 44]
        }
        Flash_Write(blink_count); // Her durumda Flash'a kaydet [cite: 46]
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
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

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  // ----- ADIM a: TIM2'yi saniyede 1 interrupt verecek şekilde ayarla -----
  // Formül: Interrupt süresi = (Prescaler+1) * (Period+1) / Saat frekansı
  // = (7999+1) * (999+1) / 8.000.000 = 8000 * 1000 / 8.000.000 = 1 saniye
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;                              // 8MHz / 8000 = 1kHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;                                  // 1kHz / 1000 = 1Hz = 1sn
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  // PC13 - Dahili LED (low-active: 0=yanar, 1=söner)
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  // PA0 - Buton girişi
  // GPIO_MODE_IT_RISING_FALLING: hem basınca hem bırakınca interrupt üretir
  // GPIO_PULLDOWN: buton basılı değilken pin 0'da kalır, basılınca 3.3V=1 olur
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;  // Her iki kenar
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;                // Pull-down
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  // PA1 - Adım h: lojik 0 çıkış
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // EXTI0 interrupt önceliğini ayarla ve aktif et
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// ----- ADIM b, c: TIM2 saniyede 1 kere bu fonksiyonu çağırır -----
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        if (state == 0)
        {
            // Yanıp sönme modu
            // blink_count kadar yanıp sönmek için blink_count*2 toggle gerekir
            // (1 toggle = ya yak ya söndür, 2 toggle = 1 tam yanıp sönme)
            if (blink_internal_idx < (blink_count * 2))
            {
                // ----- ADIM b: LED'i her saniye toggle et -----
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                blink_internal_idx++;
            }
            else
            {
                // Tüm yanıp sönmeler bitti, bekleme moduna geç
                state = 1;
                blink_internal_idx = 0;
                wait_seconds = 0;
                // LED'i söndür (PC13 low-active: SET = söndür)
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            }
        }
        else if (state == 1)
        {
            // ----- ADIM c: 5 saniye bekleme modu -----
            wait_seconds++;
            if (wait_seconds >= 5)
            {
                // 5 saniye doldu, tekrar yanıp sönme moduna dön
                state = 0;
                wait_seconds = 0;
            }
        }
    }
}

// ----- ADIM d, g: Buton interrupt callback (External Interrupt) -----
// PA0 her değiştiğinde (basılınca ve bırakılınca) bu fonksiyon çağrılır
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
        {
            // ---- Rising edge: Buton şimdi basıldı ----
            // Sadece ilk basışta zamanı kaydet (bouncing'den gelen
            // sahte rising edge'lerin başlangıç zamanını sıfırlamasını önler)
            if (manal_pressed == 0)
            {
                manal_press_start = HAL_GetTick();  // Basılma zamanını kaydet
                manal_pressed = 1;                  // Buton basılı işaretle
            }
        }
        else
        {
            // ---- Falling edge: Buton şimdi bırakıldı ----

            // Eğer zaten basılı değilse (sahte interrupt) çık
            if (manal_pressed == 0) return;
            manal_pressed = 0;  // Buton bırakıldı işaretle

            // Kaç milisaniye basılı tutuldu?
            uint32_t manal_duration = HAL_GetTick() - manal_press_start;

            // 50ms'den kısa ise bouncing (titreşim) etkisi, yoksay
            if (manal_duration < 50) return;

            if (manal_duration >= 3000)
            {
                // ----- ADIM g: 3 saniye basılı = fabrika ayarı -----
                blink_count = 4;
            }
            else
            {
                // ----- ADIM d: Kısa basış = blink_count 1 artır -----
                blink_count++;
                if (blink_count > 7) blink_count = 4;  // 7'den büyükse 4'e dön
            }

            // ----- ADIM e: Her değişiklikte Flash'a kaydet -----
            Flash_Write(blink_count);
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
