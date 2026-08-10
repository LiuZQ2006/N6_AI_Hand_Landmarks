#include "Trans.h"
/* 引入正点原子 uart.c 中定义好的串口句柄 */
extern UART_HandleTypeDef huart1;

/**
 * @brief  CV动作数据下发函数 (由 main.c 的 while(1) 轮询调用)
 * @note   将 CV 识别结果打包，通过 huart1 发送给下位机
 */
void Command_Transmit(uint8_t data)
{


        /* 2. 定义局部数组组装数据包 (3字节) */
        uint8_t tx_buffer[3];

        // 装填包头
        tx_buffer[0] = Finger_begin;     // 0xAA

        // 装填有效数据
        tx_buffer[1] = data;

        // 装填包尾
        tx_buffer[2] = Finger_end;       // 0xFF

        /* 3. 调用底层的 HAL 库，通过 uart.c 中的 huart1 阻塞发送 */
        HAL_UART_Transmit(&huart1, tx_buffer, sizeof(tx_buffer), 100);
}
