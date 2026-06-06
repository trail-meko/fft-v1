#include "ui.h"
#include "GUI.h"
#include "Lcd_Driver.h"
#include "adc_app.h"

/* Lcd_SetRegion 在 Lcd_Driver.c 中定义但未在 .h 中声明，此处前向声明 */
extern void Lcd_SetRegion(u16 x_start, u16 y_start, u16 x_end, u16 y_end);

/* ================================================================
 * 全局状态变量
 * ================================================================ */

/* 浏览选中的通道 (0=ch1, 1=ch2, 2=ch3)，默认 ch2 */
uint8_t g_ui_select_ch = 1;

/* 确认激活的通道（波形数据来源），默认 ch2 */
uint8_t g_ui_active_ch = 1;

/* ================================================================
 * 内部辅助 — 通道数据指针选择
 * ================================================================ */

/**
 * @brief  根据通道号获取对应的 ADC 数据缓冲区
 */
static uint16_t *ui_get_buf(uint8_t ch)
{
	if (ch == 0) return ttf_adc_ch1_buf;
	if (ch == 1) return ttf_adc_ch2_buf;
	return ttf_adc_ch3_buf;
}

/**
 * @brief  根据通道号获取对应的 FFT 分析结果
 */
static adc_signal_result_t *ui_get_result(uint8_t ch)
{
	if (ch == 0) return &ch1_result;
	if (ch == 1) return &ch2_result;
	return &ch3_result;
}

/* ================================================================
 * 内部辅助函数
 * ================================================================ */

/**
 * @brief  快速填充矩形区域
 * @note   利用 LCD 控制器硬件自增寻址，比逐点 Gui_DrawPoint 快约 10 倍
 * @param  x     左上角 X 坐标
 * @param  y     左上角 Y 坐标
 * @param  w     宽度（像素）
 * @param  h     高度（像素）
 * @param  color RGB565 颜色值
 */
static void ui_fill_rect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
	uint32_t i;
	uint32_t total;
	u8 hi;
	u8 lo;

	total = (uint32_t)w * h;
	hi = (u8)(color >> 8);
	lo = (u8)(color);

#if USE_HORIZONTAL == 1
	Lcd_SetRegion(x + 1, y + 2, x + w, y + h + 1);
#else
	Lcd_SetRegion(x + 2, y + 1, x + w + 1, y + h);
#endif

	for (i = 0; i < total; i++)
	{
		Lcd_WriteData(hi);
		Lcd_WriteData(lo);
	}
}

/**
 * @brief  绘制参考网格
 * @note   1 条水平中线 + 3 条垂直分割线（1/4、1/2、3/4 处）
 */
static void ui_draw_grid(void)
{
	u16 mid_y;

	mid_y = UI_WAVE_Y + UI_WAVE_HEIGHT / 2;

	/* 水平中线 */
	Gui_DrawLine(0, mid_y, 159, mid_y, UI_GRID_COLOR);

	/* 垂直分割线：x = 40, 80, 120 */
	Gui_DrawLine(40,  UI_WAVE_Y, 40,  UI_WAVE_Y + UI_WAVE_HEIGHT - 1, UI_GRID_COLOR);
	Gui_DrawLine(80,  UI_WAVE_Y, 80,  UI_WAVE_Y + UI_WAVE_HEIGHT - 1, UI_GRID_COLOR);
	Gui_DrawLine(120, UI_WAVE_Y, 120, UI_WAVE_Y + UI_WAVE_HEIGHT - 1, UI_GRID_COLOR);
}

/**
 * @brief  ADC 采样值 → Y 像素坐标映射
 * @note   屏幕原点在左上角，所以反相映射：大值在上、小值在下
 * @param  sample ADC 原始值（0-4095）
 * @return Y 坐标（UI_WAVE_Y ~ UI_WAVE_Y+UI_WAVE_HEIGHT-1）
 */
static u16 ui_map_y(uint16_t sample)
{
	uint32_t scaled;

	scaled = ((uint32_t)sample * UI_WAVE_HEIGHT) / 4096;
	return (u16)(UI_WAVE_Y + UI_WAVE_HEIGHT - 1 - scaled);
}

/**
 * @brief  缓冲区索引 → X 像素坐标映射
 * @note   256 个采样点等比映射到 160 像素宽
 * @param  index 缓冲区索引（0-255）
 * @return X 坐标（0-159）
 */
static u16 ui_map_x(uint16_t index)
{
	return (u16)(((uint32_t)index * UI_WAVE_WIDTH) / ADC_SAMPLES);
}

/**
 * @brief  绘制波形曲线
 * @note   遍历 256 个采样点，逐对用 Gui_DrawLine 连接
 * @param  buf 波形数据缓冲区（长度 = ADC_SAMPLES）
 */
static void ui_draw_waveform(uint16_t *buf)
{
	uint16_t i;
	uint16_t x0, y0, x1, y1;

	for (i = 0; i < ADC_SAMPLES - 1; i++)
	{
		x0 = ui_map_x(i);
		y0 = ui_map_y(buf[i]);
		x1 = ui_map_x(i + 1);
		y1 = ui_map_y(buf[i + 1]);
		Gui_DrawLine(x0, y0, x1, y1, UI_WAVE_COLOR);
	}
}

/**
 * @brief  绘制通道选择指示器
 * @note   显示 "CH1  CH2  CH3"，选中通道加括号高亮
 *         激活通道用绿色，非激活选中用黄色，普通用灰色
 * @param  select_ch  浏览选中的通道 (0/1/2)
 * @param  active_ch  确认激活的通道 (0/1/2)
 */
static void ui_draw_channel_selector(uint8_t select_ch, uint8_t active_ch)
{
	uint8_t ch;
	u16 x;
	u16 color;
	char buf[8];

	for (ch = 0; ch < 3; ch++)
	{
		x = 4 + ch * 52;  /* 三个通道均匀分布：4, 56, 108 */

		if (ch == active_ch && ch == select_ch)
		{
			/* 激活且选中：绿色 + 括号 */
			color = GREEN;
			sprintf(buf, "[CH%d]", ch + 1);
		}
		else if (ch == select_ch)
		{
			/* 选中但未确认：黄色 + 括号 */
			color = YELLOW;
			sprintf(buf, "[CH%d]", ch + 1);
		}
		else if (ch == active_ch)
		{
			/* 已激活但光标不在：绿色无括号 */
			color = GREEN;
			sprintf(buf, " CH%d ", ch + 1);
		}
		else
		{
			/* 其他通道：灰色无括号 */
			color = GRAY2;
			sprintf(buf, " CH%d ", ch + 1);
		}

		lcd_printf(x, 98, color, UI_BG_COLOR, buf);
	}
}

/**
 * @brief  绘制底部文字信息区域
 * @note   第1行（y=98）：通道选择器
 *         第2行（y=114）：频率 + 占空比
 * @param  r 指向当前激活通道的 FFT 分析结果
 */
static void ui_draw_info(adc_signal_result_t *r)
{
	/* 擦除文字区域背景 */
	ui_fill_rect(0, UI_INFO_Y, UI_WAVE_WIDTH, 32, UI_BG_COLOR);

	/* 分割线 */
	Gui_DrawLine(0, UI_DIVIDER_Y, 159, UI_DIVIDER_Y, UI_GRID_COLOR);

	/* 第1行：通道选择器 */
	ui_draw_channel_selector(g_ui_select_ch, g_ui_active_ch);

	/* 第2行：频率 + 占空比 */
	lcd_printf(0, 114, UI_TEXT_COLOR1, UI_BG_COLOR,
	           "F=%.1fHz  D=%.1f%%", r->freq, r->duty);
}

/* ================================================================
 * 公开接口
 * ================================================================ */

/**
 * @brief  UI 初始化
 * @note   在 main() 中 Lcd_Clear 之后调用一次
 */
void ui_init(void)
{
	/* 绘制参考网格 */
	ui_draw_grid();

	/* 底部显示占位信息 */
	ui_fill_rect(0, UI_INFO_Y, UI_WAVE_WIDTH, 32, UI_BG_COLOR);
	Gui_DrawLine(0, UI_DIVIDER_Y, 159, UI_DIVIDER_Y, UI_GRID_COLOR);
	lcd_printf(0, 98, WHITE, UI_BG_COLOR,
	           "FFT Spectrum Analyzer");
	lcd_printf(0, 114, YELLOW, UI_BG_COLOR,
	           "Waiting for signal...");
}

/**
 * @brief  UI 刷新函数（200ms 周期）
 * @note   完整刷新流程：擦波形区 → 画网格 → 画波形 → 更新文字
 *         数据来源由 g_ui_active_ch 决定
 */
void ui_proc(void)
{
	uint16_t *buf;
	adc_signal_result_t *result;

	/* 根据激活通道选择数据源 */
	buf = ui_get_buf(g_ui_active_ch);
	result = ui_get_result(g_ui_active_ch);

	/* 1. 擦除波形区域 */
	ui_fill_rect(UI_WAVE_X, UI_WAVE_Y, UI_WAVE_WIDTH, UI_WAVE_HEIGHT, UI_BG_COLOR);

	/* 2. 重画静态网格 */
	ui_draw_grid();

	/* 3. 绘制波形曲线 */
	ui_draw_waveform(buf);

	/* 4. 更新底部文字信息（含通道选择器） */
	ui_draw_info(result);
}

/**
 * @brief  按键1：浏览下一个通道
 * @note   循环切换 g_ui_select_ch：0→1→2→0
 *         仅改变选中指示，不切换波形数据
 */
void ui_select_next_channel(void)
{
	g_ui_select_ch = (g_ui_select_ch + 1) % 3;
}

/**
 * @brief  按键2：确认当前选择的通道
 * @note   将 g_ui_active_ch 同步为 g_ui_select_ch，波形立即切换
 */
void ui_confirm_channel(void)
{
	g_ui_active_ch = g_ui_select_ch;
}
