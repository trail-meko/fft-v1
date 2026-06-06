[根目录](../CLAUDE.md) > **APP**

## 模块职责

应用层模块，介于主程序(`USER/`)和硬件驱动(`HARDWARE/`)之间。包含 ADC 数据采集分拣、FFT 谐波分析、按键状态机、简易调度器、OLED 显示刷新。

## 文件清单

| 文件 | 职责 |
|------|------|
| `adc_app.c/h` | ADC1+DMA2双缓冲三通道采样 (PA3/PA4/PA5, 20kHz, TIM3触发) |
| `fft.c/h` | FFT谐波分析 — **arm_cfft_radix2_f32** (256点复数FFT)，输出 h1/h3/h5/THD |
| `scheduler.c/h` | TIM2 1ms时基调度器，50ms周期调用 adc_proc() |
| `key_app.c/h` | 按键应用层 (PG5-8, PC7) |
| `oled_app.c/h` | OLED/LCD 刷新 |
| `mydefine.h` | 聚合头文件 |

> ⚠️ FFT 必须用 `arm_cfft_radix2_f32`，详见 [ERRORS.md](../ERRORS.md)

## 入口与启动

- **adc_app.c**: ADC应用层核心
  - `adc1_init(sample_rate)` — 初始化ADC1完整链路（GPIO + DMA + ADC核心 + TIM3触发），接受每通道采样率(Hz)参数
  - `ttf_proc()` — ADC数据分拣函数，从DMA双缓冲中提取3通道数据到独立buf
  - `DMA2_Stream0_IRQHandler()` — DMA中断服务，半传输/全传输标志管理和指针切换

- **scheduler.c**: 简易任务调度器
  - `scheduler_init()` — 计算任务表长度
  - `scheduler_run()` — 遍历任务表，按周期(ms)调度执行
  - `TIM2_Init()` — 1ms SysTick（72MHz/72/1000），维护 `uwTick` 作为调度时基
  - `TIM2_IRQHandler()` — `uwTick++`
  - **注意**: 任务表 `scheduler_task[]` 当前为空（`{0}`），`adc_proc` 被注释

- **key_app.c**: 按键应用层（独立于 HARDWARE/KEY）
  - `key_init()` — 初始化PG5-8, PC7 上拉输入（5个按键）
  - `key_read()` — 读取当前按键值
  - `key_proc()` — 边沿检测状态机（下降沿触发），当前仅处理按键1切换 `oled_mode`
  - `check()` — 辅助函数，获取某年某月的天数

- **oled_app.c**: OLED显示应用
  - `oled_proc()` — 调用 `OLED_Refresh()`，注释中保留了 LCD/OLED 格式化输出示例

- **led.c/h**: **空文件**（0行），LED功能由 `HARDWARE/LED/` 提供

## 对外接口

| 函数 | 文件 | 说明 |
|------|------|------|
| `adc1_init(sample_rate)` | adc_app.c | ADC+DMA+TIM3 完整链路初始化 |
| `ttf_proc()` | adc_app.c | ADC数据分拣到3通道buf |
| `scheduler_init()` | scheduler.c | 调度器初始化 |
| `scheduler_run()` | scheduler.c | 每循环调用一次 |
| `TIM2_Init()` | scheduler.c | 1ms 时基初始化 |
| `TIM3_Init(frequency)` | scheduler.c | **注意:与HARDWARE/TIMER/timer.c中的TIM3_Int_Init冲突** |
| `key_init()` | key_app.c | 5按键GPIO初始化 |
| `key_proc()` | key_app.c | 按键状态机处理 |
| `oled_proc()` | oled_app.c | OLED刷新 |

**全局变量** (adc_app.h):
- `g_adc_dma_buf[1536]` — DMA原始缓冲（3通道 x 256采样 x 2帧双缓冲）
- `ttf_adc_ch1/2/3_buf[256]` — 各通道分拣后数据
- `adc_flag` — DMA状态标志（0=空闲, 1=前缓冲完成, 2=后缓冲完成）
- `lcd_wave_buf[]` / `ttf_wave_buf[]` — 在头文件声明但 `adc_app.c` 中未定义同名变量

## 关键依赖与配置

| 依赖 | 路径 | 说明 |
|------|------|------|
| sys.h | ../SYSTEM/sys/ | 位带操作 |
| delay.h | ../SYSTEM/delay/ | 延时函数 |
| usart.h | ../SYSTEM/usart/ | 串口 |
| key.h | ../HARDWARE/KEY/ | 底层按键驱动 |
| mydefine.h | ./ | 聚合头文件 |
| stdarg.h/stdio.h/string.h | (系统) | 标准C库 |

**mydefine.h**: 聚合包含所有常用头文件，作为 APP 层的统一入口。

**ADC参数配置** (adc_app.h):
- `ADC_CH_NUM = 3` — 采集通道数(PA3/PA4/PA5)
- `ADC_SAMPLES = 256` — 每通道每帧采样数
- `ADC_DMA_BUF_SIZE = 1536` — DMA总缓冲大小

## 数据模型

- **DMA双缓冲机制**: 总缓冲 = 2帧 x 3通道 x 256采样 = 1536个uint16_t。半传输中断(HT)触发前缓冲有效，全传输中断(TC)触发后缓冲有效
- **ADC分辨率**: 12位（0-4095）
- **采样触发**: TIM3 TRGO 事件，可配置频率

## 测试与质量

- 无自动化测试
- 代码问题:
  1. **TIM3重复初始化**: `scheduler.c: TIM3_Init()` 与 `HARDWARE/TIMER/timer.c: TIM3_Int_Init()` 重复初始化TIM3，可能导致预期行为差异
  2. **adc_app.c 未使用 `lcd_adc_ch*` 和 `lcd_proc()`/`lcd_proc()`**: 当前只用 `ttf_proc()`，LCD 显示通路未完整实现
  3. **usart_app.c 完全注释**: USART1 DMA收发功能已注释，当前使用 SYSTEM/usart 的中断方式
  4. **led.c/h 空文件**: 疑似冗余，LED功能已由 HARDWARE/LED/ 提供
  5. **key_app.c 与 HARDWARE/KEY/key.c 功能重叠**: 两边都有按键初始化逻辑，使用不同GPIO (APP用PG/PC，HARDWARE用PA0)

## 常见问题 (FAQ)

- **Q: DMA中断不触发？** A: 确认 `DMA_ITConfig(DMA2_Stream0, DMA_IT_HT|DMA_IT_TC, ENABLE)` 已配置，NVIC优先级设置正确
- **Q: ADC采样值异常？** A: 检查 TIM3 触发频率和 ADC 采样时间（当前84周期，约1us）
- **Q: 调度器不执行任务？** A: `scheduler_task[]` 当前为空表，需手动添加任务条目

## 相关文件清单

| 文件 | 行数 | 说明 |
|------|------|------|
| adc_app.c | ~241 | ADC+DMA+TIM3 链路与数据分拣 |
| adc_app.h | ~52 | ADC参数宏定义与全局变量声明 |
| scheduler.c | ~121 | 简易调度器+TIM2时基 |
| scheduler.h | ~10 | 调度器函数声明 |
| key_app.c | ~67 | 按键应用层（5按键，PG/PC） |
| key_app.h | ~10 | 按键声明 |
| oled_app.c | ~20 | OLED刷新逻辑 |
| oled_app.h | ~8 | OLED声明 |
| usart_app.c | ~137 | **全部注释**，USART DMA方案预留 |
| usart_app.h | ~11 | USART DMA声明 |
| led.c | 0 (空) | 冗余文件 |
| led.h | 0 (空) | 冗余文件 |
| mydefine.h | ~26 | 聚合头文件 |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
