#ifndef __UI_H
#define __UI_H

#include <stdint.h>

/* 布局: 波形 160x64 + 分割线 + 信息4行(16px字体) = 64+4+64=132 > 128, 波形减到60px */
#define UI_WAVE_X          0
#define UI_WAVE_Y          0
#define UI_WAVE_WIDTH      160
#define UI_WAVE_HEIGHT     60

#define UI_DIVIDER_Y       60
#define UI_INFO_Y          62

#define UI_BG_COLOR        GRAY0
#define UI_WAVE_COLOR      BLACK
#define UI_GRID_COLOR      GRAY2
#define UI_TEXT_COLOR1     RED
#define UI_TEXT_COLOR2     BLUE

void ui_init(void);
void ui_proc(void);
void ui_next_page(void);

#endif /* __UI_H */
