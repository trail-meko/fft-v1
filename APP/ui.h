#ifndef __UI_H
#define __UI_H

#include <stdint.h>

/* ================================================================
 * 屏幕布局常量（横屏 160×128，USE_HORIZONTAL=1）
 * ================================================================ */

/* 波形区域：顶部 160×94 像素 */
#define UI_WAVE_X          0
#define UI_WAVE_Y          0
#define UI_WAVE_WIDTH      160
#define UI_WAVE_HEIGHT     94

/* 分割线 Y 坐标 */
#define UI_DIVIDER_Y       94

/* 文字信息区域起始 Y 坐标 */
#define UI_INFO_Y          96

/* 配色方案 */
#define UI_BG_COLOR        GRAY0
#define UI_WAVE_COLOR      GREEN
#define UI_GRID_COLOR      GRAY2
#define UI_TEXT_COLOR1     RED
#define UI_TEXT_COLOR2     BLUE

/* ================================================================
 * 通道选择状态
 * ================================================================ */

/** 浏览选中的通道（按键1循环切换，0=ch1, 1=ch2, 2=ch3） */
extern uint8_t g_ui_select_ch;

/** 确认激活的通道（按键2确认后生效，波形数据来源） */
extern uint8_t g_ui_active_ch;

/* ================================================================
 * 公开接口
 * ================================================================ */

/**
 * @brief  UI 初始化：清屏、画网格、显示占位文字
 * @note   在 main() 中 Lcd_Clear 之后调用一次
 */
void ui_init(void);

/**
 * @brief  UI 刷新函数：擦波形区 → 画网格 → 画波形曲线 → 更新文字信息
 * @note   由调度器每 200ms 调用一次
 */
void ui_proc(void);

/**
 * @brief  按键1：浏览下一个通道（ch1→ch2→ch3 循环）
 * @note   仅改变选中指示，不切换波形数据
 */
void ui_select_next_channel(void);

/**
 * @brief  按键2：确认当前选择的通道
 * @note   将 g_ui_active_ch 设为 g_ui_select_ch，波形数据立即切换
 */
void ui_confirm_channel(void);

#endif /* __UI_H */
