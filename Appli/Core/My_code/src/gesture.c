#include "gesture.h"
#include <math.h>

/* 动作防抖参数定义 */
#define FILTER_FRAME_COUNT  2   /* 本地推理无丢包, 2帧防偶发抖动即可 (~200ms) */
#define MOVE_THRESHOLD_RATIO 0.3f


/* 屏幕中心与判定死区 (屏幕分辨率 800x480) */
#define SCREEN_CENTER_X     400.0f
#define SCREEN_CENTER_Y     240.0f
/* 死区范围：手在屏幕正中央偏离不超过这个像素值，就认定为 STOP
 * 缩小死区提高灵敏度: X 120→80, Y 90→60 */
#define DEADZONE_X          80.0f
#define DEADZONE_Y          60.0f
/* 方向偏置: |dx| > |dy| × BIAS 则判水平; BIAS<1.0 让水平方向更易触发 */
#define DIR_BIAS_H          0.85f  /* |dx| > |dy|×0.85 → 水平, |dy| 需更小才判垂直 */

static float get_distance(ld_point_t p1, ld_point_t p2)
{
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return sqrtf(dx * dx + dy * dy);
}


static uint8_t check_is_fist(ld_point_t lm[LD_LANDMARKS_NB])
{
    if (get_distance(lm[8], lm[0]) < get_distance(lm[5], lm[0]) &&
        get_distance(lm[12], lm[0]) < get_distance(lm[9], lm[0]) &&
        get_distance(lm[16], lm[0]) < get_distance(lm[13], lm[0]) &&
        get_distance(lm[20], lm[0]) < get_distance(lm[17], lm[0]))
    {
        return 1;
    }
    return 0;
}

static uint8_t check_is_Stick(ld_point_t lm[LD_LANDMARKS_NB])
{
    if (get_distance(lm[4],  lm[0]) > get_distance(lm[2],  lm[0]) &&
        get_distance(lm[8],  lm[0]) > get_distance(lm[5],  lm[0]) &&
        get_distance(lm[12], lm[0]) > get_distance(lm[9],  lm[0]) &&
        get_distance(lm[16], lm[0]) < get_distance(lm[13], lm[0]) &&
        get_distance(lm[20], lm[0]) < get_distance(lm[17], lm[0]))
        {return 1;}
    return 0;
}

/* 变身手势检测: 拇指+食指+小指 伸展, 中指+无名指 闭合 (🤟) */
static uint8_t check_is_change(ld_point_t lm[LD_LANDMARKS_NB])
{
    if (get_distance(lm[4],  lm[0]) > get_distance(lm[2],  lm[0]) &&
        get_distance(lm[8],  lm[0]) > get_distance(lm[5],  lm[0]) &&
        get_distance(lm[12], lm[0]) < get_distance(lm[9],  lm[0]) &&
        get_distance(lm[16], lm[0]) < get_distance(lm[13], lm[0]) &&
        get_distance(lm[20], lm[0]) > get_distance(lm[17], lm[0]))
        {return 1;}
    return 0;
}


static uint8_t gesture_need_reset = 0;

void app_gesture_init(void)
{
    gesture_need_reset = 1;
}

//解码
uint8_t app_gesture_decode(ld_point_t landmarks[LD_LANDMARKS_NB], float hand_cx, float hand_cy)
{
    static uint8_t last_action = STOP;
    static uint8_t action_count = 0;
    uint8_t current_action = STOP;

    if (gesture_need_reset) {
        last_action = STOP;
        action_count = 0;
        gesture_need_reset = 0;
    }

    
    if (check_is_fist(landmarks))
    {
        current_action = Fist_SKILL;
    }
    else if (check_is_Stick(landmarks))
    {
        current_action = Stick_Up;
    }
    else if (check_is_change(landmarks))
    {
        current_action = Change_Size;
    }
    else
    {
        
        float dx = hand_cx - SCREEN_CENTER_X;
        float dy = hand_cy - SCREEN_CENTER_Y;

        
        if (fabs(dx) > DEADZONE_X || fabs(dy) > DEADZONE_Y)
        {
            if (fabs(dx) > fabs(dy) * DIR_BIAS_H) {
                current_action = (dx > 0) ? Move_Right : Move_Left;
            } else {

                current_action = (dy > 0) ? DOWN : Jump_up;
            }
        }
        else
        {
            current_action = STOP; 
        }
    }
//防抖
    if (current_action != STOP)
    {
        if (current_action == last_action) {
            action_count++;
            if (action_count >= FILTER_FRAME_COUNT) {
                action_count = FILTER_FRAME_COUNT - 1;  /* 保持确认态, 避免50%占空比抖动 */
                return current_action;
            }
        } else {
            last_action = current_action;
            action_count = 1;
        }
    } else {
        action_count = 0;
    }

    return STOP;
}
