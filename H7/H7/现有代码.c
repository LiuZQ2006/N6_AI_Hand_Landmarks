/**
 ******************************************************************
 * @file    main.c
 * @author  fire / 赛博老司机 (8位调色板完美减脂版 + 双核通信)
 * @version V9.0 (K210 + HC-06双串口并存，蓝牙绝对优先，自适应渲染)
 ******************************************************************
 */  
#include "stm32h7xx.h"
#include "main.h"
#include "./led/bsp_led.h" 
#include "./sdram/bsp_sdram.h" 
#include "./lcd/bsp_lcd.h"
#include "./touch/bsp_i2c_touch.h"
#include "./touch/bsp_touch_gtxx.h"
#include "./delay/core_delay.h" 
#include "./mpu/bsp_mpu.h" 
#include "animation_data.h"

#define PACKET_HEADER  0xAA  
#define PACKET_FOOTER  0xFF  

#define CMD_MOVE_LEFT  0x01
#define CMD_MOVE_RIGHT 0x02
#define CMD_JUMP_UP    0x04
#define CMD_DOWN       0x08
#define CMD_FIST_SKILL 0x10  
#define CMD_ULT_SKILL  0x20  

UART_HandleTypeDef huart1; // K210
UART_HandleTypeDef huart3; // HC-06 蓝牙

/* ================================================================= */
/* 🚀 串口1 (K210) 状态机变量 */
/* ================================================================= */
volatile uint8_t rx_byte;          
volatile uint8_t rx_state = 0;     
volatile uint8_t current_cmd = 0;  
volatile uint8_t action_flag = 0;  
volatile uint8_t action_cmd = 0;   

/* ================================================================= */
/* 🔵 串口3 (HC-06 蓝牙) 状态机变量 */
/* ================================================================= */
volatile uint8_t bt_rx_byte;         // 蓝牙单字节信箱
volatile uint8_t bt_action_flag = 0; // 蓝牙动作触发钥匙
volatile uint8_t bt_action_cmd = 0;  // 蓝牙收到的原字符

/* ================================================================= */
/* 🔧 串口初始化区域 */
/* ================================================================= */
void Direct_UART1_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) { while(1); }
}

void Direct_UART3_Init(void) {
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200; // HC-06
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) { while(1); }
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if(huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        // PA9: TX, PA10: RX
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1; 
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* K210 中断优先级较低 (抢占优先级 2) */
        HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if(huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        // PB10: TX, PB11: RX (接HC-06的RX和TX)
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3; 
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* 👑 蓝牙 中断优先级最高 (抢占优先级 1) */
        HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
}

/* ================================================================= */
/* ⚡ 中断服务函数与回调 */
/* ================================================================= */
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }
void USART3_IRQHandler(void) { HAL_UART_IRQHandler(&huart3); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    /* 1. 处理 HC-06 蓝牙数据 (通常是手机端发来的单字符) */
    if (huart->Instance == USART3) {
        bt_action_cmd = bt_rx_byte;
        bt_action_flag = 1;
        HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1);
    }
    /* 2. 处理 K210 数据 (3字节协议) */
    else if (huart->Instance == USART1) {
        if (rx_byte == PACKET_HEADER) {
            rx_state = 1; 
        } 
        else if (rx_state == 1) {
            current_cmd = rx_byte;
            rx_state = 2;
        } 
        else if (rx_state == 2) {
            if (rx_byte == PACKET_FOOTER) {
                action_cmd = current_cmd;
                action_flag = 1;
            }
            rx_state = 0; 
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1); // 重新开启接收
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) { HAL_UART_Receive_IT(&huart1, &rx_byte, 1); }
    if (huart->Instance == USART3) { HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1); }
}

/* ================================================================= */
/* 🎨 8位调色板 (256色) 专用智能画图函数 (自适应宽高 + 自动吸管扣绿) */
/* ================================================================= */
void LCD_DrawSprite_Palette(uint16_t x, uint16_t y, const unsigned char *pData) {
    uint16_t width  = (pData[3] << 8) | pData[2];
    uint16_t height = (pData[5] << 8) | pData[4];
    const unsigned char *pal = &pData[8];
    const unsigned char *pix = &pData[8 + 768];

    #define RGB24_TO_565(r,g,b) ((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))

    uint8_t  bg_idx    = pix[0];
    uint8_t  bg_r      = pal[bg_idx * 3];
    uint8_t  bg_g      = pal[bg_idx * 3 + 1];
    uint8_t  bg_b      = pal[bg_idx * 3 + 2];
    uint16_t bg_color  = RGB24_TO_565(bg_r, bg_g, bg_b);

    uint32_t idx = 0;
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            uint8_t ci = pix[idx++];
            uint8_t r  = pal[ci * 3];
            uint8_t g  = pal[ci * 3 + 1];
            uint8_t b  = pal[ci * 3 + 2];
            uint16_t color = RGB24_TO_565(r, g, b);

            if (color != bg_color) LCD_DrawPixel(x + i, y + j, color);
            else LCD_DrawPixel(x + i, y + j, LCD_COLOR_BLACK);
        }
    }
}

void LCD_EraseSprite(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) { LCD_DrawPixel(x + i, y + j, LCD_COLOR_BLACK); }
    }
}

void Delay(__IO uint32_t nCount); 
void SystemClock_Config(void);

int main(void)
{   
    HAL_Init(); 
    SystemClock_Config();
    LED_GPIO_Config();
  
    Board_MPU_Config(0, MPU_Normal_WT, 0xD0000000, MPU_32MB);
    Board_MPU_Config(1, MPU_Normal_WT, 0x24000000, MPU_512KB);
    SCB_EnableICache();
    SCB_EnableDCache();
   
    /* 🚀 初始化双串口 */
    Direct_UART1_Init();        
    Direct_UART3_Init();

    GTP_Init_Panel(); 
    LCD_Init();
    LCD_LayerInit(0, LCD_FB_START_ADDRESS, RGB888);
    LCD_LayerInit(1, LCD_FB_START_ADDRESS+(LCD_GetXSize()*LCD_GetYSize()*4), RGB888);
    LCD_DisplayOn(); 

    LCD_SelectLayer(0);
    LCD_Clear(LCD_COLOR_BLACK);  
    LCD_SelectLayer(1);
    LCD_Clear(LCD_COLOR_TRANSPARENT);

    LCD_SetTransparency(0, 0);
    LCD_SetTransparency(1, 255);
    Delay(0xfff);
    
    int x_pos = 295;
    int y_pos = 90;
    int last_x = 295;
    int last_y = 90;
    int move_speed = 5; 
    
    int last_width = 213; 
    uint8_t frame_step = 0;
    const unsigned char* current_frame = gImage_walframe1code;

    LCD_DrawSprite_Palette(x_pos, y_pos, current_frame);
    
    /* 🚀 同时启动 K210 和 蓝牙 的中断监听 */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1);
    
    while(1)
    {       
        uint8_t pending_cmd = 0; // 统一调度变量

        /* 👑 逻辑优先级第一层：判断蓝牙是否有命令 */
        if (bt_action_flag == 1) {
            // 将手机蓝牙发送的字母，映射为内部动画命令宏
            switch(bt_action_cmd) {
                case 'L': pending_cmd = CMD_MOVE_LEFT;  break;
                case 'R': pending_cmd = CMD_MOVE_RIGHT; break;
                case 'U': pending_cmd = CMD_JUMP_UP;    break;
                case 'D': pending_cmd = CMD_DOWN;       break;
                case 'F': pending_cmd = CMD_FIST_SKILL; break; // 发送 'F' 触发打拳
                case 'S': pending_cmd = CMD_ULT_SKILL;  break; // 发送 'S' 触发大招
            }
            bt_action_flag = 0; // 消费完毕
        }
        /* 🔧 逻辑优先级第二层：如果蓝牙没动作，才看 K210 有没有命令 */
        else if (action_flag == 1) {
            pending_cmd = action_cmd;
            action_flag = 0; // 消费完毕
        }

        /* 🎬 统一动画渲染引擎：不管是蓝牙还是K210发出的命令，都在这里执行 */
        if (pending_cmd != 0) {
            uint8_t is_moving = 0; 
            
            switch(pending_cmd) {
                case CMD_MOVE_LEFT:  x_pos -= move_speed; is_moving = 1; break; 
                case CMD_MOVE_RIGHT: x_pos += move_speed; is_moving = 1; break; 
                case CMD_JUMP_UP:    y_pos -= move_speed; is_moving = 1; break; 
                case CMD_DOWN:       y_pos += move_speed; is_moving = 1; break; 
                
                case CMD_FIST_SKILL:
                {
                    if (x_pos > (800 - 311)) x_pos = 800 - 311;
                    const unsigned char* atk_frames[] = {
                        gImage_attackframe1, gImage_attackframe2, gImage_attackframe3,
                        gImage_attackframe4, gImage_attackframe5, gImage_attackframe4,
                        gImage_attackframe3, gImage_attackframe2, gImage_attackframe1
                    };
                    for(int i = 0; i < 9; i++) {
                        LCD_EraseSprite(x_pos, y_pos, last_width, 299); 
                        LCD_DrawSprite_Palette(x_pos, y_pos, atk_frames[i]); 
                        last_width = 311; 
                        Delay(0x6FFFFF); 
                    }
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299);
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_walframe1code);
                    last_width = 213;
                    break;
                }
                
                case CMD_ULT_SKILL:
                {
                    const unsigned char* ult_frames[] = {
                        gImage_Ultframe1, gImage_Ultframe2, gImage_Ultframe3, gImage_Ultframe4
                    };
                    for(int loop = 0; loop < 4; loop++) { 
                        for(int i = 0; i < 4; i++) {
                            LCD_EraseSprite(x_pos, y_pos, last_width, 299);
                            LCD_DrawSprite_Palette(x_pos, y_pos, ult_frames[i]);
                            last_width = 213;
                            Delay(0x6FFFFF); 
                        }
                    }
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299);
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_walframe1code);
                    last_width = 213;
                    break;
                }
                default: break; 
            }
            
            // 边界约束
            if (x_pos < 0) x_pos = 0;
            if (y_pos < 0) y_pos = 0;
            if (x_pos > (800 - 213)) x_pos = 800 - 213;
            if (y_pos > (480 - 299)) y_pos = 480 - 299;

            // 移动动画帧循环
            if (is_moving && (x_pos != last_x || y_pos != last_y)) {
                frame_step++;
                if (frame_step > 5) frame_step = 0;
                
                switch(frame_step) {
                    case 0: current_frame = gImage_walframe1code; break;
                    case 1: current_frame = gImage_walframe2code; break;
                    case 2: current_frame = gImage_walframe3code; break;
                    case 3: current_frame = gImage_walframe4code; break;
                    case 4: current_frame = gImage_walframe3code; break;
                    case 5: current_frame = gImage_walframe2code; break;
                }
                LCD_EraseSprite(last_x, last_y, last_width, 299);
                LCD_DrawSprite_Palette(x_pos, y_pos, current_frame);
                
                last_x = x_pos;
                last_y = y_pos;
                last_width = 213; 
            }
        }
    }
}

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { while(1); }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) { while(1); }
}

void Delay(__IO uint32_t nCount) { for(; nCount != 0; nCount--); }