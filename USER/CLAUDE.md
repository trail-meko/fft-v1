[根目录](../CLAUDE.md) > **USER**

## 模块职责

主程序入口与工程配置模块。包含 `main()` 函数、中断服务例程、Keil MDK5 工程文件（`.uvprojx`/`.uvoptx`），以及 CMSIS 系统初始化文件。是整个项目的启动和调度核心。

## 入口与启动

- **main.c**: 程序唯一入口 `main()`，初始化顺序：
  1. NVIC优先级分组
  2. delay_init(168) — 基于 SysTick 的延时
  3. uart_init(115200) — USART1，使能中断接收
  4. adc1_init(20000) — ADC1+DMA+TIM3，20kHz采样率
  5. LED_Init(), KEY_Init() — GPIO 外设
  6. Lcd_Init(), Lcd_Clear() — TFT 初始化
  7. 显示欢迎信息
  8. TIM3_Int_Init(65535,84-1) — 用于FFT计时
  9. arm_cfft_radix4_init_f32 — 初始化基4 FFT实例
  10. 进入 `while(1)` 主循环

- **主循环逻辑**: 轮询按键 K1(WK_UP)，按下时执行软合成信号 → FFT → 取模 → 串口打印。每10次循环翻转 LED2。

- **系统初始化**:
  - `system_stm32f4xx.c/h` — 设置系统时钟(168MHz)，由 ST 官方提供
  - `stm32f4xx.h` — 项目级芯片头文件
  - `stm32f4xx_conf.h` — 外设库配置头文件，选择性包含所需外设驱动头文件

## 对外接口

- **FFT参数**: `#define FFT_LENGTH 1024` （在main.c中定义）
- **数据缓冲区**:
  - `fft_inputbuf[2048]` — FFT输入（实部+虚部交替）
  - `fft_outputbuf[1024]` — FFT输出幅度
- **中断服务**: `stm32f4xx_it.c` — 包含 NMI、HardFault、MemManage、BusFault、UsageFault、SVC、DebugMon、PendSV、SysTick 的默认Handler

## 关键依赖与配置

| 依赖 | 路径 | 说明 |
|------|------|------|
| sys.h | ../SYSTEM/sys/ | 位带操作宏定义 |
| delay.h | ../SYSTEM/delay/ | SysTick 延时 |
| usart.h | ../SYSTEM/usart/ | 串口 printf 重定向 |
| arm_math.h | ../DSP_LIB/Include/ | CMSIS-DSP FFT 函数 |
| adc_app.h | ../APP/ | ADC应用层 |
| GUI.h | ../HARDWARE/1.8TFT/ | TFT 显示 |
| timer.h | ../HARDWARE/TIMER/ | FFT 计时 |

**Keil 工程配置**:
- 文件: `DSP_FFT.uvprojx`
- 芯片: STM32F407VGTx
- 编译器: ARM Compiler 5 (V5.06 update 7)
- FPU: 启用 (Cortex-M4)
- 输出: OBJ/DSP_FFT.axf / DSP_FFT.hex
- 调试器: J-Link (JLinkSettings.ini)

## 数据模型

- `fft_inputbuf[FFT_LENGTH*2]` (float) — 交替存储实部(偶数索引)和虚部(奇数索引)
- `fft_outputbuf[FFT_LENGTH]` (float) — 存放幅度谱
- `timeout` (u8) — TIM3中断溢出计数，用于计算FFT耗时
- `time` (float) — FFT计算耗时(ms)

## 测试与质量

- 无自动化测试
- 手动验证：烧录后观察LCD启动画面 → 按K1触发FFT → 串口监控输出 → 观察LED闪烁
- TIM3中断含 `timeout++` 但**未配置NVIC**（在main中INTX_DISABLE未调用，TIM3_Int_Init中已配置NVIC）
- 注意: main.c中使用的TIM3与`HARDWARE/TIMER/timer.c`中的TIM3中断有潜在冲突（两处都初始化了TIM3 NVIC）

## 常见问题 (FAQ)

- **Q: FFT输出全为0？** A: 检查 CMSIS-DSP 库是否正确链接（`arm_cortexM4lf_math.lib`），确认 FPU 已启用
- **Q: LCD无显示？** A: 检查IO初始化 `LCD_GPIO_Init()` 中的引脚配置是否匹配实际硬件
- **Q: 串口无法接收？** A: 确认 `EN_USART1_RX` 宏为 1，检查PA9/PA10引脚复用配置

## 相关文件清单

| 文件 | 行数 | 说明 |
|------|------|------|
| main.c | ~94 | 主程序入口 |
| stm32f4xx_it.c | ~169 | 中断Handler模板 |
| stm32f4xx_it.h | ~40+ | 中断Handler声明 |
| stm32f4xx_conf.h | ~126 | 外设库头文件选择 |
| stm32f4xx.h | - | 芯片级宏定义 |
| system_stm32f4xx.c | - | 系统时钟初始化 |
| system_stm32f4xx.h | ~106 | 系统时钟初始化声明 |
| DSP_FFT.uvprojx | - | Keil工程文件 |
| DSP_FFT.uvoptx | - | Keil用户选项 |
| JLinkSettings.ini | - | J-Link调试配置 |
| DebugConfig/* | - | 调试配置 |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
