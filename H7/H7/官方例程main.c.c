/**
  ******************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   SDMMC-SD¿¨¶Áд²⊔
  ******************************************************************
  * @attention
  *
  * ʵѩƽ̨:Ұ»𠓔M32H750 ¿ª·¢°堊  * ̳    :http://www.firebbs.cn
  * ̔±¦    :http://firestm32.taobao.com
  *
  ******************************************************************
  */  
#include "stm32h7xx.h"
#include "main.h"
#include "./led/bsp_led.h" 
#include "./usart/bsp_debug_usart.h"
#include "./sd_card/bsp_sdio_sd.h"
#include "./key/bsp_key.h" 
#include "./delay/core_delay.h" 
#include "./mpu/bsp_mpu.h" 

static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();

  //½«Cacheɨփwrite-through·½ʽ
  SCB->CACR|=1<<2;
}

/**
  * @brief  ַº¯ʽ
  * @param  Ξ
  * @retval Ξ
  */
int main(void)
{
    /* ϵͳʱ֓³õʼ»¯³ɴ80MHz */
    SystemClock_Config(); 
  
		/* Ĭȏ²»Ťփ MPU£¬ȴШҪ¸ü¸ߐԄܣ¬µ±Ťփ MPU º󣬊¹Ӄ 
		 DMA ʱШעҢ Cache ӫ Ě´愚ȝһւДµĎʌ⣬
		 ¾ߌ嗢ҢʂϮǫ²ο¼Ť̗½̵̳ĠMPU ŤփϠ¹ؕ½ڠ*/
//		Board_MPU_Config(0, MPU_Normal_WT, 0xD0000000, MPU_32MB);
//		Board_MPU_Config(1, MPU_Normal_WT, 0x24000000, MPU_512KB);
//  
    CPU_CACHE_Enable();
  
    LED_GPIO_Config();
    LED_BLUE;	
    /* ³õʼ»¯USART1 ŤփģʽΪ 115200 8-N-1 */
    DEBUG_USART_Config();	
    /* ³õʼ»¯¶ÀÁ¢°´¼ü */
    Key_GPIO_Config();
    printf("\r\n»¶ӭʹӃҰ»𠠓TM32 H750 ¿ª·¢°塣\r\n");	
    printf("Ԛ¿ªʼ½øАSD¿¨»ù±¾²⊔ǰ£¬ǫ¸ø¿ª·¢°岥ȫ32GҔĚµēD¿¨\r\n");			
    printf("±¾³̐򻡶ԓD¿¨½øА·ǎļþϵͳ·½ʽ¶Áд£¬»ቾ³ýSD¿¨µĎļþϵͳ\r\n");		
    printf("ʵѩº󿉍¨¹ýµ焔¸񊽻¯»򊹓ÓD¿¨΄¼þϵͳµĀý³ָ̻´SD¿¨΄¼þϵͳ\r\n");		
    printf("\r\n µ«sd¿¨ĚµĔ­΄¼þ²»¿ɻָ´£¬ʵѩǰα±ر¸·ݓD¿¨ĚµĔ­΄¼þ£¡£¡£¡\r\n");		
    printf("\r\n ȴґȷȏ£¬ǫ°´¿ª·¢°嵄KEY1°´¼ü£¬¿ªʼSD¿¨²⊔ʵѩ....\r\n");	  
  
    while(Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_OFF);
    printf("\r\n¿ªʼ½øАSD¿¨¶Áдʵѩ\r\n");	
    SD_Test();	
    while(1)
    {
            
    }			
}
/**
  * @brief  System Clock Ťփ
  *         system Clock Ťփȧς: 
	*            System Clock source  = PLL (HSE)
	*            SYSCLK(Hz)           = 480000000 (CPU Clock)
	*            HCLK(Hz)             = 240000000 (AXI and AHBs Clock)
	*            AHB Prescaler        = 2
	*            D1 APB3 Prescaler    = 2 (APB3 Clock  120MHz)
	*            D2 APB1 Prescaler    = 2 (APB1 Clock  120MHz)
	*            D2 APB2 Prescaler    = 2 (APB2 Clock  120MHz)
	*            D3 APB4 Prescaler    = 2 (APB4 Clock  120MHz)
	*            HSE Frequency(Hz)    = 25000000
	*            PLL_M                = 5
	*            PLL_N                = 192
	*            PLL_P                = 2
	*            PLL_Q                = 4
	*            PLL_R                = 2
	*            VDD(V)               = 3.3
	*            Flash Latency(WS)    = 4
  * @param  None
  * @retval None
  */
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** ƴӃµ甴Ťփ¸üЂ
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** ŤփַĚΈѹƷʤ³öµ瑹
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** ³õʼ»¯CPU¡¢AHBº́PBלϟʱ֓
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 24;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
		while(1);
  }
  /** ³õʼ»¯CPU¡¢AHBº́PBלϟʱ֓
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
		while(1);
  }
}
/****************************END OF FILE***************************/
