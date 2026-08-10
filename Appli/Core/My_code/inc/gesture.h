# ifndef __AI_PREDICT__CV__H
# define __AI_PREDICT__CV__H


//include
#include "Trans.h"
#include "ld.h"
#include <stdint.h>//宏

//变量

//函数
void app_gesture_init(void);

/* 手势核心解码函数，传入21个特征点，返回当前动作 */
uint8_t app_gesture_decode(ld_point_t landmarks[LD_LANDMARKS_NB], float hand_cx, float hand_cy);

# endif
