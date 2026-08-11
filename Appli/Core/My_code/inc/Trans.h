# ifndef __TRANS_H__
# define __TRANS_H__

// ──────────────── 包含 ────────────────
#include "main.h"
#include "gesture.h"
#include <stdint.h>

// ──────────────── 配置宏 ────────────────
#define SPRITE_NORFLASH_BASE    0x70800000UL   /* NOR Flash 精灵数据起始 (与 SPRITEROM 一致, AI模型后) */
#define SPRITE_HYPERRAM_BASE    0x91612000UL   /* HyperRAM 可用起始 */
#define SPRITE_TOTAL_SIZE       0x170000UL     /* 精灵数据实际大小 ~1.5MB (17个数组) */

#define LCD_WIDTH               800
#define LCD_HEIGHT              480

#define ANIM_FRAME_INTERVAL     2              /* 帧间隔(dp_thread tick数, ~66ms@30fps → 15FPS动画) */
#define ANIM_WALK_SPEED         5              /* 走路像素偏移 (15FPS × 5px = 75px/s) */
#define CHANGE_PAUSE_TICKS      25             /* 变身第3帧停顿 tick 数 (~2.5s) */

// ──────────────── 图片格式类型 ────────────────
/* H7 调色板图片格式:
 *   [0:1]  0x08 0x08  签名
 *   [2:3]  width       (小端)
 *   [4:5]  height      (小端)
 *   [6:7]  reserved
 *   [8:775] 调色板     (256色 × RGB24, 共768字节)
 *   [776:end] 像素索引  (1字节/像素, 宽×高个)
 *   第一个像素索引 = 透明色
 */
typedef struct {
    uint16_t          width;
    uint16_t          height;
    const uint8_t    *palette;      /* 指向 256×3 调色板 */
    const uint8_t    *pixels;       /* 指向像素索引数据 */
    uint8_t           bg_index;     /* 透明色索引(第一个像素) */
} sprite_image_t;

// ──────────────── 精灵数据外部声明 ────────────────
/* 注: extern 不标大小, 避免与 Trans.c 内实际定义冲突 */
extern const unsigned char gImage_bg[];

extern const unsigned char gImage_walframe1code[];
extern const unsigned char gImage_walframe2code[];
extern const unsigned char gImage_walframe3code[];
extern const unsigned char gImage_walframe4code[];

extern const unsigned char gImage_Ultframe1[];
extern const unsigned char gImage_Ultframe2[];
extern const unsigned char gImage_Ultframe3[];
extern const unsigned char gImage_Ultframe4[];

extern const unsigned char gImage_attackframe1[];
extern const unsigned char gImage_attackframe2[];
extern const unsigned char gImage_attackframe3[];
extern const unsigned char gImage_attackframe4[];
extern const unsigned char gImage_attackframe5[];

extern const unsigned char gImage_change1[];
extern const unsigned char gImage_change2[];
extern const unsigned char gImage_change3[];

// ──────────────── 精灵引擎 API ────────────────
/* 解析图片头, 填充 sprite_image_t 结构 */
void sprite_parse_header(sprite_image_t *img, const unsigned char *data);

/* 启动时预渲染舞台背景到 HyperRAM (仅调用一次) */
void stage_cache_init(void);

/* 启动时预展开全部角色精灵到 HyperRAM RGB888 缓存 (仅调用一次) */
void sprite_cache_init_all(void);

/* 画完整戏台背景到 buffer (从缓存 memcpy, 极快) */
void sprite_draw_stage(uint8_t *buffer);

/* 画单个调色板精灵到 buffer, x/y 为左上角坐标, 透明色跳过 */
void sprite_draw_image(uint8_t *buffer, const unsigned char *data, int x, int y);

/* 用背景重绘精灵矩形区域(擦除) */
void sprite_erase_rect(uint8_t *buffer, int x, int y, uint16_t w, uint16_t h);

// ──────────────── 动画控制 API ────────────────
/* 初始化动画状态机 */
void animation_init(void);

/* 手势触发动画 (由 nn_thread 调用) */
void animation_trigger(uint8_t gesture);

/* 每个 display tick 调用, 推进帧和位置 */
void animation_tick(void);

/* 原子获取渲染状态 (frame + x/y 在锁内一次性读取, 线程安全) */
void animation_get_render_state(const unsigned char **frame, int *x, int *y);

/* 获取当前帧精灵数据指针 */
const unsigned char *animation_get_frame(void);

/* 获取当前精灵的左上角坐标 */
int animation_get_x(void);
int animation_get_y(void);

/* 获取当前帧精灵宽高(从图片头解析) */
int animation_get_w(void);
int animation_get_h(void);

# endif
