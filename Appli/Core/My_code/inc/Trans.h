# ifndef __Trans_H__

# define __Trans_H__

#include  "uart.h"
#include "main.h"
#include "string.h"
#include <stdbool.h>
//宏
//手指
#define  STOP 				 0x00
#define	 Wave_The_Right_Hand 0x01
#define  Wave_The_Left_Hand	 0X02
#define  Kick_The_Left_Leg   0X04
#define  Kick_The_Right_Leg  0X08

//CV
#define Move_Left	 0X01
#define Move_Right 	 0X02
#define Jump_up		 0X04
#define DOWN   		 0X08
#define Fist_SKILL	 0X10
#define Stick_Up     0X20
#define STOP		 0x00

//包头
#define Finger_begin 0XAA
#define Finger_end   0XFF

//变量

//函数
void Command_Transmit(uint8_t data);
# endif
