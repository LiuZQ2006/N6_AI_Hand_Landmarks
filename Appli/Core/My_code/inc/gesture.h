# ifndef __AI_PREDICT__CV__H
# define __AI_PREDICT__CV__H


//include
#include "ld.h"
#include <stdint.h>

// ──────────────── 手势动作宏 ────────────────
#define  STOP                 0x00
#define  Move_Left            0x01
#define  Move_Right           0x02
#define  Jump_up              0x04
#define  DOWN                 0x08
#define  Fist_SKILL           0x10
#define  Stick_Up             0x20
#define  Change_Size          0x40

//变量

//函数
void app_gesture_init(void);

/* 手势核心解码函数，传入21个特征点，返回当前动作 */
uint8_t app_gesture_decode(ld_point_t landmarks[LD_LANDMARKS_NB], float hand_cx, float hand_cy);

# endif
