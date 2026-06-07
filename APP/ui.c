#include "ui.h"
#include "GUI.h"
#include "Lcd_Driver.h"
#include "adc_app.h"
#include "system_state.h"

extern void Lcd_SetRegion(u16 x, u16 y, u16 xe, u16 ye);

static uint8_t g_page = 0;
static uint8_t g_last_page = 0;

void ui_next_page(void) { g_page = !g_page; }

static void fill(u16 x, u16 y, u16 w, u16 h, u16 c)
{
    uint32_t i, n = (uint32_t)w * h;
    u8 hi = (u8)(c >> 8), lo = (u8)(c);
#if USE_HORIZONTAL == 1
    Lcd_SetRegion(x + 1, y + 2, x + w, y + h + 1);
#else
    Lcd_SetRegion(x + 2, y + 1, x + w + 1, y + h);
#endif
    for (i = 0; i < n; i++) { Lcd_WriteData(hi); Lcd_WriteData(lo); }
}

static void grid(void)
{
    u16 my = UI_WAVE_Y + UI_WAVE_HEIGHT / 2;
    Gui_DrawLine(0,  my, 159, my, UI_GRID_COLOR);
    Gui_DrawLine(40, UI_WAVE_Y, 40,  UI_WAVE_Y + UI_WAVE_HEIGHT - 1, UI_GRID_COLOR);
    Gui_DrawLine(80, UI_WAVE_Y, 80,  UI_WAVE_Y + UI_WAVE_HEIGHT - 1, UI_GRID_COLOR);
    Gui_DrawLine(120,UI_WAVE_Y, 120, UI_WAVE_Y + UI_WAVE_HEIGHT - 1, UI_GRID_COLOR);
}

static u16 my(uint16_t s) {
    return (u16)(UI_WAVE_Y + UI_WAVE_HEIGHT - 1 - ((uint32_t)s * UI_WAVE_HEIGHT) / 4096);
}
static u16 mx(uint16_t i) {
    return (u16)(((uint32_t)i * UI_WAVE_WIDTH) / ADC_SAMPLES);
}
static void wave(uint16_t *buf, u16 c)
{
    uint16_t i;
    for (i = 0; i < ADC_SAMPLES - 1; i++)
        Gui_DrawLine(mx(i), my(buf[i]), mx(i+1), my(buf[i+1]), c);
}

/* ================================================================
 * Page 0: 波形 + 信号源参数
 * ================================================================ */
static void page0(void)
{
    uint16_t *buf = (g_sys.uo_ch == 0) ? ttf_adc_ch1_buf :
                    (g_sys.uo_ch == 1) ? ttf_adc_ch2_buf : ttf_adc_ch3_buf;
    float fk = (float)g_sys.freq_hz / 1000.0f;
    char b[10];

    /* 波形 */
    fill(0, 0, 160, UI_WAVE_HEIGHT, UI_BG_COLOR);
    grid();
    wave(buf, UI_WAVE_COLOR);
    Gui_DrawLine(0, UI_DIVIDER_Y, 159, UI_DIVIDER_Y, UI_GRID_COLOR);

    /* 信号源设置 */
    lcd_printf(0, 64, RED, UI_BG_COLOR,
               (g_sys.waveform == WAVEFORM_SINE) ? " SIN" : " TRI");
    sprintf(b, "%.2fk", fk); lcd_printf(32, 64, RED, UI_BG_COLOR, b);
    lcd_printf(72, 64, RED, UI_BG_COLOR,
               (g_sys.amp_mode == MODE_MANUAL) ? "MAN" : "AGC");

    /* 简要测量 */
    {
        int um  = (int)(g_sys.ui_rms * 1000.0f + 0.5f);
        int uom = (int)(g_sys.uo_rms * 1000.0f + 0.5f);
        int ph  = (int)(g_sys.phase_diff_deg + 0.5f);
        lcd_printf(0,  82, BLUE, UI_BG_COLOR, "Ui");
        sprintf(b, "%dmV", um);  lcd_printf(16, 82, BLUE, UI_BG_COLOR, b);
        lcd_printf(64, 82, BLUE, UI_BG_COLOR, "Uo");
        sprintf(b, "%dmV", uom); lcd_printf(80, 82, BLUE, UI_BG_COLOR, b);
        lcd_printf(0, 100, RED, UI_BG_COLOR, "G");
        sprintf(b, "%.1f", g_sys.gain); lcd_printf(8, 100, RED, UI_BG_COLOR, b);
        lcd_printf(56,100, RED, UI_BG_COLOR, "P");
        sprintf(b, "%+d", ph);  lcd_printf(64,100, RED, UI_BG_COLOR, b);
        if (g_sys.amp_mode == MODE_AGC) {
            lcd_printf(112,100, RED, UI_BG_COLOR, "LK");
            lcd_printf(128,100, RED, UI_BG_COLOR, g_sys.agc_locked ? "Y" : "N");
        }
    }
}

/* ================================================================
 * Page 1: 全屏测量值 (仅ADC采集+计算, 无DAC设置)
 * ================================================================ */
static void page1(void)
{
    int um   = (int)(g_sys.ui_rms * 1000.0f + 0.5f);
    int uom  = (int)(g_sys.uo_rms * 1000.0f + 0.5f);
    int ph   = (int)(g_sys.phase_diff_deg + 0.5f);
    int uif  = (int)(g_sys.ui_freq + 0.5f);
    int uof  = (int)(g_sys.uo_freq + 0.5f);
    char b[16];

    fill(0, 0, 160, 128, UI_BG_COLOR);

    /* 标题 */
    lcd_printf(0, 0, RED, UI_BG_COLOR, "== Measure ==");

    /* Ui */
    lcd_printf(0,  20, BLUE, UI_BG_COLOR, "Ui RMS:");
    sprintf(b, "%dmV", um);         lcd_printf(64, 20, BLUE, UI_BG_COLOR, b);
    sprintf(b, "%.3fV", g_sys.ui_rms); lcd_printf(112,20, BLUE, UI_BG_COLOR, b);

    /* Uo */
    lcd_printf(0,  38, RED, UI_BG_COLOR, "Uo RMS:");
    sprintf(b, "%dmV", uom);        lcd_printf(64, 38, RED, UI_BG_COLOR, b);
    sprintf(b, "%.3fV", g_sys.uo_rms); lcd_printf(112,38, RED, UI_BG_COLOR, b);

    /* 增益 */
    lcd_printf(0,  56, BLUE, UI_BG_COLOR, "Gain:");
    sprintf(b, "%.2f", g_sys.gain); lcd_printf(48, 56, BLUE, UI_BG_COLOR, b);

    /* 相位差 */
    lcd_printf(0,  74, RED, UI_BG_COLOR, "Phase:");
    sprintf(b, "%+d deg", ph);      lcd_printf(56, 74, RED, UI_BG_COLOR, b);

    /* 频率 */
    lcd_printf(0,  92, BLUE, UI_BG_COLOR, "Ui Freq:");
    sprintf(b, "%d Hz", uif);       lcd_printf(64, 92, BLUE, UI_BG_COLOR, b);
    lcd_printf(0, 110, BLUE, UI_BG_COLOR, "Uo Freq:");
    sprintf(b, "%d Hz", uof);       lcd_printf(64,110, BLUE, UI_BG_COLOR, b);
}

/* ================================================================ */
void ui_init(void)
{
    g_page = 0;
    fill(0, 0, 160, 128, UI_BG_COLOR);
    grid();
    Gui_DrawLine(0, UI_DIVIDER_Y, 159, UI_DIVIDER_Y, UI_GRID_COLOR);
    lcd_printf(0, 64, RED,  UI_BG_COLOR, "OpAmp Gain Test");
    lcd_printf(0, 82, BLUE, UI_BG_COLOR, "K1:F K2:M K3:W K4:Pg");
    lcd_printf(0,100, BLUE, UI_BG_COLOR, "Ready...");
}

void ui_proc(void)
{
    /* 页面切换时全清防止叠加 */
    if (g_page != g_last_page) {
        fill(0, 0, 160, 128, UI_BG_COLOR);
        g_last_page = g_page;
    }

    if (g_page == 0)
        page0();
    else
        page1();
}
