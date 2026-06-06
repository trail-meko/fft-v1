[根目录](../CLAUDE.md) > **HARDWARE**

## 模块职责

硬件驱动层，直接操作MCU外设寄存器。包含5个子模块：1.8寸TFT LCD显示驱动、按键驱动(KEY)、LED驱动、定时器驱动(TIMER)、以及环形缓冲区(ringbuffer)工具库。

## 子模块结构

```
HARDWARE/
├── 1.8TFT/           # LCD显示驱动
│   ├── Lcd_Driver.c  # 底层SPI通信与LCD寄存器操作
│   ├── Lcd_Driver.h  # LCD宏定义、引脚宏、函数声明
│   ├── GUI.c         # 上层绘图API（点、线、圆、矩形、汉字）
│   ├── GUI.h         # 绘图函数声明
│   ├── LCD_Config.h  # LCD初始化参数
│   ├── Font.h        # GBK字库数据
│   └── Picture.h     # 图片数据
├── KEY/              # 按键驱动
│   ├── key.c         # 按键初始化与扫描
│   └── key.h         # 按键宏(WK_UP)和函数声明
├── LED/              # LED驱动
│   ├── led.c         # LED初始化（PA1）
│   └── led.h         # LED宏定义(LED2 = PAout(1))
├── TIMER/            # 定时器驱动
│   ├── timer.c       # TIM3中断初始化与ISR
│   └── timer.h       # 函数声明
├── ringbuffer.c      # 环形缓冲区(RT-Thread移植)
└── ringbuffer.h      # 环形缓冲区接口
```

## 入口与启动

### 1.8TFT LCD
- **LCD_GPIO_Init()** — 初始化5线模拟SPI GPIO (PB1/PB12-15, PC5)
- **Lcd_Init()** — LCD初始化序列
- **通信方式**: GPIO模拟SPI（非硬件SPI），通过 `SPI_WriteData()` 位操作
- **分辨率**: 128x160 像素，16位色 (RGB565)
- **GUI功能**: 点、线、圆、矩形框、GBK16/GBK24汉字、32号数字
- **颜色定义**: RED(0xf800), GREEN(0x07e0), BLUE(0x001f), WHITE/BLACK/YELLOW/GRAY0-2

### KEY 按键
- **KEY_Init()** — PA0 下拉输入，对应 K1 (WK_UP)
- **KEY_Scan(mode)** — 消抖扫描，mode=0 不连续触发，mode=1 连续触发
- **返回值**: WKUP_PRES(1) 或 0

### LED
- **LED_Init()** — PA1 推挽输出，上拉，默认高电平（灭）
- **LED2宏**: `PAout(1)` — 位带操作直接翻转

### TIMER
- **TIM3_Int_Init(arr, psc)** — 通用定时器初始化
  - 168MHz/2 → 84MHz TIM时钟
  - `Tout = (arr+1)*(psc+1)/84MHz` (us)
  - 使用场景: FFT耗时计时（arr=65535, psc=83 → 65536us溢出周期）
- **TIM3_IRQHandler()** — `timeout++`，用于测量超过65536us的长时间运算
- **注意**: TIM3同时被 `adc_app.c` 用于ADC触发和 `scheduler.c` 用于独立触发定时

### ringbuffer 环形缓冲区
- **来源**: 移植自 RT-Thread 内核，无RT-Thread依赖，仅使用标准C库
- **机制**: Mirror位算法，16位索引（最大32KB缓冲）
- **主要API**:
  - `rt_ringbuffer_init()` — 初始化
  - `rt_ringbuffer_put()` — 写入（空间不足则截断）
  - `rt_ringbuffer_put_force()` — 写入（覆盖旧数据）
  - `rt_ringbuffer_get()` — 读取
  - `rt_ringbuffer_getchar()` / `putchar()` — 单字节操作
  - `rt_ringbuffer_data_len()` — 当前数据长度
  - `rt_ringbuffer_reset()` — 重置清空
- **适配**: `__aeabi_assert()` 空实现以解决Keil链接错误

## 对外接口

| 函数/宏 | 文件 | 说明 |
|---------|------|------|
| `Lcd_Init()` | Lcd_Driver.c | LCD完整初始化 |
| `Lcd_Clear(Color)` | Lcd_Driver.c | 全屏填充 |
| `Lcd_SetXY(x, y)` | Lcd_Driver.c | 设置像素坐标 |
| `Gui_DrawPoint(x, y, c)` | Lcd_Driver.c | 画点 |
| `Gui_DrawFont_GBK16(x,y,fc,bc,s)` | GUI.c | 显示16号汉字 |
| `Gui_DrawFont_GBK24(x,y,fc,bc,s)` | GUI.c | 显示24号汉字 |
| `Gui_DrawFont_Num32(x,y,fc,bc,num)` | GUI.c | 显示32号数字 |
| `KEY_Init()` | KEY/key.c | 按键GPIO初始化 |
| `KEY_Scan(mode)` | KEY/key.c | 按键扫描 |
| `WK_UP` | KEY/key.h | PA0引脚读取宏 |
| `LED2` | LED/led.h | PA1位带输出宏 |
| `LED_Init()` | LED/led.c | LED初始化 |
| `TIM3_Int_Init(arr,psc)` | TIMER/timer.c | TIM3中断初始化 |

## 关键依赖与配置

| 依赖 | 说明 |
|------|------|
| sys.h | 位带操作宏 (PAout/BIT_ADDR) |
| delay.h | 按键消抖延时 (delay_ms) |
| stm32f4xx.h | ST官方寄存器定义 |
| font.h | LCD字库数据 |
| Picture.h | LCD图片数据 |

**引脚配置汇总**:

| 外设 | 引脚 | 模式 | 功能 |
|------|------|------|------|
| TFT SDA | PB15 | 推挽输出 | 模拟SPI数据 |
| TFT SCL | PB13 | 推挽输出 | 模拟SPI时钟 |
| TFT CS | PB12 | 推挽输出 | 片选 |
| TFT RST | PB14 | 推挽输出 | LCD复位 |
| TFT RS | PC5 | 推挽输出 | 命令/数据 |
| TFT BLK | PB1 | 推挽输出 | 背光 |
| KEY K1 | PA0 | 下拉输入 | WK_UP按键 |
| LED D2 | PA1 | 推挽输出 | 状态灯 |

## 数据模型

- **LCD像素格式**: RGB565 (16位)，大端字节序通过 `LCD_WriteData_16Bit()` 写入
- **LCD坐标**: x: 0-127, y: 0-159（竖屏模式）
- **ringbuffer**: 16位索引(mirror:1bit + index:15bit)，最大缓冲 2^15 = 32768 字节

## 测试与质量

- 无自动化测试
- **已知问题**:
  1. **TIM3功能冲突**: `timer.c` 中 TIM3 用于FFT计时（含中断），而 `adc_app.c` 中TIM3用作ADC触发定时器，`scheduler.c` 中 `TIM3_Init()` 又是一套独立配置。三处TIM3使用存在潜在竞争
  2. **ringbuffer依赖**: 需要 `assert.h` 但 `__aeabi_assert` 是Keil Compiler 5的特有符号，已通过空实现绕过
  3. **LCD模拟SPI无总线锁保护**：如在中断中使用LCD可能导致显示异常

## 常见问题 (FAQ)

- **Q: LCD花屏或白屏？** A: 确认初始化序列时序正确，RST引脚有足够的复位脉冲。检查PB14(RST)是否被其他模块占用
- **Q: ringbuffer编译报错 `Undefined symbol __aeabi_assert`？** A: 已在 ringbuffer.c 末尾添加 `__aeabi_assert()` 空实现，确认该文件已加入工程编译
- **Q: 按键按下无反应？** A: 检查PA0外部下拉电阻，PA0默认高电平需外接上拉至VCC(3.3V)

## 相关文件清单

| 文件 | 行数 | 说明 |
|------|------|------|
| 1.8TFT/Lcd_Driver.c | ~500+ | LCD底层驱动与SPI |
| 1.8TFT/Lcd_Driver.h | ~82 | LCD宏定义 |
| 1.8TFT/GUI.c | ~500+ | 绘图API |
| 1.8TFT/GUI.h | ~24 | 绘图声明 |
| 1.8TFT/LCD_Config.h | - | LCD配置参数 |
| 1.8TFT/Font.h | - | GBK字库 |
| 1.8TFT/Picture.h | - | 图片数据 |
| KEY/key.c | ~64 | 按键驱动 |
| KEY/key.h | ~27 | 按键宏 |
| LED/led.c | ~34 | LED驱动 |
| LED/led.h | ~18 | LED宏 |
| TIMER/timer.c | ~58 | TIM3中断 |
| TIMER/timer.h | ~19 | TIM3声明 |
| ringbuffer.c | ~476 | 环形缓冲完整实现 |
| ringbuffer.h | ~119 | 环形缓冲接口 |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
