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
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "sonar.h"
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
CAN_HandleTypeDef hcan;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart3_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_CAN_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


#define uart2_buffer_size 64
uint8_t uart2_rx_buffer[uart2_buffer_size]={0};
#define uart3_buffer_size 64
uint8_t uart3_rx_buffer[uart3_buffer_size]={0};
void UART2_IT(void)
{
    // ????
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);
        // ? DMA,???
        HAL_UART_DMAStop(&huart2);
        uint16_t len = uart2_buffer_size- __HAL_DMA_GET_COUNTER(huart2.hdmarx);
				{
				/*for(int i=0;i<len;i++)
					{
						printf("%#x\n",uart2_rx_buffer[i]);
					}
					*/
					
				}
				//uint8_t data[7]={0x01,0x6a,0x07,0x01,0xff,0x3f,0xb1};
				//HAL_UART_Transmit(&huart2,data,7,0xff);
      __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
      HAL_UART_Receive_DMA(&huart2, uart2_rx_buffer, uart2_buffer_size);
    }
}
void UART3_IT(void)
{
    // ????
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
        // ? DMA,???
        HAL_UART_DMAStop(&huart3);
        uint16_t len = uart3_buffer_size- __HAL_DMA_GET_COUNTER(huart3.hdmarx);
				{
				for(int i=0;i<len;i++)
					{
						printf("%#x\n",uart3_rx_buffer[i]);
					}
					
				}
				uint8_t data[7]={0x01,0x6a,0x07,0x01,0xff,0x3f,0xb1};
				//HAL_UART_Transmit(&huart3,data,7,0xff);
      __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
      HAL_UART_Receive_DMA(&huart3, uart3_rx_buffer, uart3_buffer_size);
    }
}
int fputc(int ch,FILE*f)
{
  HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,0xff);
  return ch;
}
int fgetc(FILE *f) 
{
  char ch;
  HAL_UART_Receive(&huart1,(uint8_t*)&ch,1,0xff);
  return((int)ch);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
if(htim->Instance==TIM3)
{


}

}

void can_init()
{
	CAN_FilterTypeDef filter;
	filter.FilterActivation=ENABLE;
	filter.FilterBank=1;
	filter.FilterFIFOAssignment=CAN_FILTER_FIFO0;
	filter.FilterIdHigh=0;
	filter.FilterIdLow=0;
	filter.FilterMaskIdHigh=0;
	filter.FilterMaskIdLow=0;
	filter.FilterMode=CAN_FILTERMODE_IDMASK;
	filter.FilterScale=CAN_FILTERSCALE_32BIT;
	
	HAL_CAN_ConfigFilter(&hcan,&filter);
	HAL_CAN_ActivateNotification(&hcan,CAN_IT_RX_FIFO0_MSG_PENDING);
	HAL_CAN_Start(&hcan);
}
uint32_t can_mailbox;
void can_send(int id,uint8_t*data,int len)
{
	CAN_TxHeaderTypeDef tx;
	tx.StdId=id;
	tx.ExtId=id;
	tx.DLC=len;
	tx.RTR=CAN_RTR_DATA;
	tx.IDE=CAN_ID_STD;
	
	if(HAL_CAN_AddTxMessage(&hcan,&tx,data,&can_mailbox)!=HAL_OK)
	{
	Error_Handler();
	};
	
	

}
//uint8_t can_buffer[16];
//void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
//{
//	CAN_RxHeaderTypeDef rx;
//HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&rx,can_buffer);
//	if(rx.StdId==0x01)
//	{
//	uint8_t pack4[8]={0x01,0x6a,0x07,0x01,0xff,0x3e,0xb0};
//	HAL_UART_Transmit(&huart2,pack4,7,0xff);
//	} 
//	if(rx.StdId==0x02)
//	{
//	
//	uint8_t pack4[8]={0x01,0x6a,0x07,0x01,0xff,0x3e,0xb0};
//	HAL_UART_Transmit(&huart3,pack4,7,0xff);
//	}
//	
//}
uint8_t can_buffer[16];
typedef struct
{
  int head;
  int bits;
} control_pack;
control_pack pack;
int sona_byte=0;
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef rx;
HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&rx,can_buffer);
	if(rx.StdId==0x410)
	{
		uint8_t*p=(uint8_t*)&pack;
	for(int i=0;i<8;i++)
	{
		p[i]=can_buffer[i];
	}
	sona_byte=pack.bits;
	//uint8_t pack4[8]={0x01,0x6a,0x07,0x01,0xff,0x3e,0xb0};
	//HAL_UART_Transmit(&huart2,pack4,7,0xff);
	} 

	
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
  MX_DMA_Init();
  MX_CAN_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */


			{
		HAL_UART_Receive_DMA(&huart2, uart2_rx_buffer,uart2_buffer_size);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
				HAL_UART_Receive_DMA(&huart3, uart3_rx_buffer,uart3_buffer_size);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
		}
		//uint8_t pack[8]={0xff,0x06,0x02,0x3c,0x00,0x04,0x5c,0x63}; 4bits
		uint8_t pack[8]={0xff,0x06,0x02,0x3c,0x00,0x08,0x5c,0x66}; //8bits
		//uint8_t pack1[8]={0x01,0x06,0x02,0x3f,0x00,0x01,0x79,0xbe};
				uint8_t pack1[8]={0x01,0x06,0x02,0x3f,0x00,0x00,0xb8,0x7e};
		//uint8_t pack2[8]={0x01,0x6c,0x07,0x01,0xff,0x01,0x75};
		uint8_t pack3[8]={0x01,0x03,0x02,0x01,0x00,0x01,0xd4,0x72};
		//uint8_t pack4[8]={0x01,0x6a,0x07,0x01,0xff,0x3f,0xb1};
		//uint8_t msg[8]={0xff,0x06,0x02,0x3d,0x00,0x00,0x0c,0x60};
		HAL_UART_Transmit(&huart2,pack,8,0xff);
		HAL_Delay(10);
 		HAL_UART_Transmit(&huart3,pack,8,0xff);
		HAL_Delay(10);
		can_init();////uart3为旧换能器
while(1)
{
	//uint8_t data[8]={0xfa};
	//can_send(0x01,data,8);
//HAL_UART_Transmit(&huart2,pack4,7,0xff);
//uint8_t pack4[8]={0x01,0x6a,0x07,0x01,0xff,0x3f,0xb1};
//HAL_UART_Transmit(&huart3,pack4,7,0xff);
//HAL_UART_Transmit(&huart2,pack4,7,0xff);
	uint8_t pack4[7]={0x01,0x6a,0x07,0x01,0xff,0x3f,0xb1};
	pack4[5]=(uint8_t)((sona_byte>>8)&0xff);
	uint8_t sum=0;
	for(int i=0;i<6;i++)
	{
	sum+=pack4[i];
	}
	pack4[6]=(uint8_t)sum&0xff;
	HAL_UART_Transmit(&huart2,pack4,7,0xff);
	HAL_Delay(1000);
	HAL_UART_Transmit(&huart3,pack4,7,0xff);
			//HAL_UART_Receive_IT(&huart2,uart2_rx_buffer,11);
	HAL_Delay(1000);
};






uint8_t data[8]={0x01,0x02,0x03};




//printf("hello\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		can_send(0x100,data,8);
		HAL_Delay(100);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 20;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_4TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 72;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
