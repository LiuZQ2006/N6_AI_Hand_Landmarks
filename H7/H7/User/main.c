/**
 ******************************************************************
 * @file    main.c
 * @author  fire / 赛博老司机 (三段式缩放蓄力版)
 * @version V14.0 (三段变身 + 2.5秒硬直 + 原木戏台合成)
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

/* K210 动作宏 */
#define CMD_MOVE_LEFT  0x01
#define CMD_MOVE_RIGHT 0x02
#define CMD_JUMP_UP    0x04
#define CMD_DOWN       0x08
#define CMD_FIST_SKILL 0x10  
#define CMD_ULT_SKILL  0x20  

/* 蓝牙专属动作宏 */
#define CMD_CHANGE_SIZE 0x40 

/* 引入外部图片数组 */
extern const unsigned char gImage_bg[384776];
extern const unsigned char gImage_change1[64463]; // 变身过渡帧1
extern const unsigned char gImage_change2[64463]; // 变身过渡帧2
extern const unsigned char gImage_change3[64463]; // 变身极值帧3

/* 核心宏：RGB24 转 RGB565 */
#define RGB24_TO_565(r,g,b) ((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))

UART_HandleTypeDef huart1; // K210
UART_HandleTypeDef huart3; // 蓝牙

volatile uint8_t rx_byte;          
volatile uint8_t rx_state = 0;     
volatile uint8_t current_cmd = 0;  
volatile uint8_t action_flag = 0;  
volatile uint8_t action_cmd = 0;   

volatile uint8_t bt_rx_byte;         
volatile uint8_t bt_action_flag = 0; 
volatile uint8_t bt_action_cmd = 0;  

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
    huart3.Init.BaudRate = 9600; 
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
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1; 
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if(huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3; 
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
}

void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }
void USART3_IRQHandler(void) { HAL_UART_IRQHandler(&huart3); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        bt_action_cmd = bt_rx_byte;
        bt_action_flag = 1; 
        HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1);
    }
    else if (huart->Instance == USART1) {
        if (rx_byte == PACKET_HEADER) rx_state = 1; 
        else if (rx_state == 1) { current_cmd = rx_byte; rx_state = 2; } 
        else if (rx_state == 2) {
            if (rx_byte == PACKET_FOOTER) { action_cmd = current_cmd; action_flag = 1; }
            rx_state = 0; 
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    if (huart->Instance == USART3) HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1); 
}

/* ================================================================= */
/* 🏮 皮影戏台合成引擎 */
/* ================================================================= */
uint16_t Get_Composited_Pixel(uint16_t x, uint16_t y) {
    uint16_t bg_width = (gImage_bg[3] << 8) | gImage_bg[2];
    const unsigned char *bg_pal = &gImage_bg[8];
    const unsigned char *bg_pix = &gImage_bg[8 + 768];
    uint32_t bg_idx = y * bg_width + x;
    uint8_t ci = bg_pix[bg_idx];
    uint16_t img_color = RGB24_TO_565(bg_pal[ci * 3], bg_pal[ci * 3 + 1], bg_pal[ci * 3 + 2]);

    uint16_t CURTAIN_RED      = RGB24_TO_565(160, 20, 20); 
    uint16_t CURTAIN_DARK_RED = RGB24_TO_565(100, 10, 10); 
    uint16_t WOOD_LIGHT       = RGB24_TO_565(110, 75, 45); 
    uint16_t WOOD_BASE        = RGB24_TO_565(70, 40, 20);  
    uint16_t WOOD_SHADOW      = RGB24_TO_565(30, 15, 5);   

    if (y < 18) {
        if (y < 10 + (x % 30) / 4) return ((x % 40) < 20) ? CURTAIN_RED : CURTAIN_DARK_RED;
    }
    if (x < 160 && y < 220) {
        if (x < (130 - y / 2)) return ((x % 36) < 18) ? CURTAIN_RED : CURTAIN_DARK_RED;
    }
    if (x > 640 && y < 220) {
        if ((800 - x) < (130 - y / 2)) return (((800 - x) % 36) < 18) ? CURTAIN_RED : CURTAIN_DARK_RED;
    }
    if (y >= 390) {
        if (y >= 390 && y < 396) return (y == 390 || y == 395) ? WOOD_SHADOW : WOOD_LIGHT;
        if (y >= 396) {
            if ((x % 120) == 0 || (x % 120) == 1) return WOOD_SHADOW;
            if (y == 420 || y == 421 || y == 450 || y == 451) return WOOD_SHADOW;
            return WOOD_BASE;
        }
    }
    return img_color;
}

void LCD_DrawFullBackground(void) {
    for (uint16_t j = 0; j < 480; j++) {
        for (uint16_t i = 0; i < 800; i++) {
            LCD_DrawPixel(i, j, Get_Composited_Pixel(i, j));
        }
    }
}

void LCD_DrawSprite_Palette(uint16_t x, uint16_t y, const unsigned char *pData) {
    uint16_t width  = (pData[3] << 8) | pData[2];
    uint16_t height = (pData[5] << 8) | pData[4];
    const unsigned char *pal = &pData[8];
    const unsigned char *pix = &pData[8 + 768];

    uint8_t  bg_idx    = pix[0];
    uint16_t bg_color  = RGB24_TO_565(pal[bg_idx * 3], pal[bg_idx * 3 + 1], pal[bg_idx * 3 + 2]);

    uint32_t idx = 0;
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            uint8_t ci = pix[idx++];
            uint16_t color = RGB24_TO_565(pal[ci * 3], pal[ci * 3 + 1], pal[ci * 3 + 2]);
            if (color != bg_color) {
                LCD_DrawPixel(x + i, y + j, color);
            }
        }
    }
}

void LCD_EraseSprite(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            uint16_t screen_x = x + i;
            uint16_t screen_y = y + j;
            if (screen_x >= 800 || screen_y >= 480) continue; 
            LCD_DrawPixel(screen_x, screen_y, Get_Composited_Pixel(screen_x, screen_y)); 
        }
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
   
    Direct_UART1_Init();        
    Direct_UART3_Init();

    GTP_Init_Panel(); 
    LCD_Init();
    
    LCD_LayerInit(0, LCD_FB_START_ADDRESS, RGB888);
    LCD_DisplayOn(); 

    LCD_SelectLayer(0);
    LCD_SetTransparency(0, 255); 

    LCD_DrawFullBackground(); 
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
    
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1);
    
    while(1)
    {       
        uint8_t pending_cmd = 0; 

        if (bt_action_flag == 1) {
            if (bt_action_cmd == '1') { 
                pending_cmd = CMD_CHANGE_SIZE; 
            }
            bt_action_flag = 0; 
        }
        else if (action_flag == 1) {
            pending_cmd = action_cmd; 
            action_flag = 0; 
        }

        if (pending_cmd != 0) {
            uint8_t is_moving = 0; 
            
            switch(pending_cmd) {
                /* ----- 蓝牙专属：三段式变身特效 ----- */
                case CMD_CHANGE_SIZE:
                {
                    // 1. 擦除原图，画出 change1
                    LCD_EraseSprite(last_x, last_y, last_width, 299); 
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_change1); 
                    last_width = 213; 
                    Delay(0x6FFFFF);

                    // 2. 擦除 change1，画出 change2
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299); 
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_change2); 
                    Delay(0x6FFFFF);

                    // 3. 擦除 change2，画出 change3 (变身极致)
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299); 
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_change3); 
                    
                    // 4. 时停：硬直等待 2.5 秒 (2500毫秒)
                    HAL_Delay(2500); 

                    // 5. 擦除 change3，退回 change2
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299); 
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_change2); 
                    Delay(0x6FFFFF);

                    // 6. 擦除 change2，退回 change1
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299); 
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_change1); 
                    Delay(0x6FFFFF);

                    // 7. 彻底恢复常态行走帧
                    LCD_EraseSprite(x_pos, y_pos, last_width, 299);
                    LCD_DrawSprite_Palette(x_pos, y_pos, gImage_walframe1code);
                    current_frame = gImage_walframe1code;
                    last_width = 213;
                    break;
                }
                
                /* ----- K210 原有动作库 ----- */
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
                        LCD_EraseSprite(last_x, last_y, last_width, 299); 
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
                            LCD_EraseSprite(last_x, last_y, last_width, 299);
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
            
            if (x_pos < 0) x_pos = 0;
            if (y_pos < 0) y_pos = 0;
            if (x_pos > (800 - 213)) x_pos = 800 - 213;
            if (y_pos > (480 - 299)) y_pos = 480 - 299;

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