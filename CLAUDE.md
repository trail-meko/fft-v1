# STM32F4 FFT频谱分析仪表项目

## 变更记录 (Changelog)

| 日期 | 变更内容 | 来源 |
|------|---------|------|
| 2026-06-06 | v5.0: FFT API 修复 + 踩坑知识库 | 开发迭代 |
| 2026-06-06 | 初始架构文档生成（全仓扫描，覆盖率约85%） | 架构师初始化 |

> 📖 踩坑记录见 **[ERRORS.md](ERRORS.md)** — DSP 库兼容性、API 差异、编码问题等

## 项目愿景

基于STM32F407 ARM Cortex-M4平台的FFT频谱分析仪表项目（电赛仪表题）。核心功能：ADC1三通道(PA3/PA4/PA5) 20kHz同步采样 → DMA2双缓冲 → 256点FFT谐波分析 → 1.8寸TFT LCD实时显示基波/3次/5次谐波幅值与THD，同时串口(115200)输出。

**技术栈**: C语言裸机编程，Keil MDK5 IDE，ARM Compiler 5 (V5.06)，STM32F4标准外设库(STD Library V1.4.0)，CMSIS-DSP预编译库(arm_cortexM4lf_math.lib，旧版)，无RTOS。

## 架构总览

本项目采用典型嵌入式裸机分层架构，自底向上分为5层：

```
层5: USER/        — 主程序入口、中断服务、Keil工程配置
层4: APP/         — 应用层模块（ADC数据处理、按键状态机、调度器、OLED显示）
层3: HARDWARE/    — 硬件驱动层（TFT LCD、按键、LED、定时器、环形缓冲区）
层2: SYSTEM/      — 系统服务层（延时、系统时钟、串口printf）
层1: FWLIB/CORE/DSP_LIB — 平台抽象层（STM32F4标准库、CMSIS-Core、CMSIS-DSP）
```

**控制流**: `main()` 初始化各模块 → `scheduler_init()` (TIM2 1ms时基) → `while(1)` 调用 `scheduler_run()` → 每50ms执行 `adc_proc()` 进行数据分拣+FFT+LCD刷新。

**数据流**: 模拟信号(PA3/PA4/PA5) → ADC1三通道扫描(TIM3 TRGO触发, 20kHz) → DMA2_Stream0双缓冲循环 → `adc_proc()` 按通道分拣 → `fft_analyze_signal()` → `fft_calc_basic_params()` (Vpp/avg/RMS) → `fft_calc_time_params()` (freq/duty) → `fft_calc_harmonics()` (**arm_cfft_radix2_f32**, 256点复数FFT) → LCD显示 + 串口输出。

## 模块结构图

```mermaid
graph TD
    A["(根) STM32F4 FFT频谱分析"] --> B["USER/ 主程序与工程"]
    A --> C["APP/ 应用层"]
    A --> D["HARDWARE/ 硬件驱动层"]
    A --> E["SYSTEM/ 系统服务层"]
    A --> F["FWLIB/ STM32F4标准外设库"]
    A --> G["CORE/ Cortex-M4核心"]
    A --> H["DSP_LIB/ CMSIS-DSP库"]

    D --> D1["KEY 按键驱动"]
    D --> D2["LED LED驱动"]
    D --> D3["TIMER 定时器驱动"]
    D --> D4["1.8TFT LCD显示驱动"]
    D --> D5["ringbuffer 环形缓冲区"]

    E --> E1["delay 延时服务"]
    E --> E2["sys 系统时钟与位带操作"]
    E --> E3["usart 串口服务"]

    click B "./USER/CLAUDE.md" "查看 USER 模块文档"
    click C "./APP/CLAUDE.md" "查看 APP 模块文档"
    click D "./HARDWARE/CLAUDE.md" "查看 HARDWARE 模块文档"
    click E "./SYSTEM/CLAUDE.md" "查看 SYSTEM 模块文档"
    click F "./FWLIB/CLAUDE.md" "查看 FWLIB 模块文档"
    click G "./CORE/CLAUDE.md" "查看 CORE 模块文档"
    click H "./DSP_LIB/CLAUDE.md" "查看 DSP_LIB 模块文档"
```

## 模块索引

| 模块路径 | 语言 | 职责 | 入口文件 | 测试目录 |
|---------|------|------|---------|---------|
| `USER/` | C, ASM | 主程序入口、中断服务、Keil工程 | `main.c` | 无 |
| `APP/` | C | 应用层：ADC数据分拣、按键状态机、调度器、OLED | `adc_app.c`, `scheduler.c` | 无 |
| `HARDWARE/` | C | 硬件驱动：TFT LCD、按键、LED、定时器、环形缓冲 | `1.8TFT/Lcd_Driver.c` | 无 |
| `SYSTEM/` | C | 系统服务：延时、时钟、串口printf | `delay/delay.c` | 无 |
| `FWLIB/` | C | STM32F4标准外设库（官方，不建议修改） | (vendor library) | 无 |
| `CORE/` | C, ASM | Cortex-M4启动文件与CMSIS核心头文件 | `startup_stm32f40_41xxx.s` | 无 |
| `DSP_LIB/` | Lib, H | ARM CMSIS-DSP预编译库（arm_cortexM4lf_math.lib） | `arm_cortexM4lf_math.lib` | 无 |

## 运行与开发

### 前置条件
- **IDE**: Keil MDK5 (uVision V5)
- **编译器**: ARM Compiler 5 (V5.06 update 7, 内部版本960)
- **目标芯片**: STM32F407VGTx (Cortex-M4, FPU, 1024KB Flash, 192KB RAM)
- **烧录调试**: J-Link (通过 JLinkSettings.ini 配置)
- **库依赖**: CMSIS-DSP 预编译库 `arm_cortexM4lf_math.lib`

### 工程文件
- 工程文件: `USER/DSP_FFT.uvprojx`
- 输出目录: `OBJ/` (已在 `.gitignore` 中忽略)
- 编译产物: DSP_FFT.axf, DSP_FFT.hex

### 编译与烧录
```bash
# 方式1: Keil IDE 打开工程
# 双击 USER/DSP_FFT.uvprojx，Keil MDK自动打开
# F7 编译，F8 下载

# 方式2: 命令行清理
.\keilkilll.bat
```

### 关键编译选项
- Use MicroLIB: 启用（串口printf重定向需要）
- FPU: 启用（Cortex-M4 硬件浮点单元）
- CMSIS-DSP: 链接 `arm_cortexM4lf_math.lib`（小端格式，M4带FPU版本）

### 硬件引脚分配
| 功能 | GPIO | 说明 |
|-----|------|------|
| 按键 K1 | PA0 | 下拉输入，WK_UP触发FFT |
| LED D2 | PA1 | 推挽输出，心跳闪烁 |
| 串口 TX | PA9 | USART1，复用推挽 |
| 串口 RX | PA10 | USART1，复用推挽 |
| ADC通道3 | PA3 | ADC1，模拟输入 |
| ADC通道4 | PA4 | ADC1，模拟输入 |
| ADC通道5 | PA5 | ADC1，模拟输入 |
| TFT SDA | PB15 | 模拟SPI数据 |
| TFT SCL | PB13 | 模拟SPI时钟 |
| TFT CS | PB12 | 片选 |
| TFT RST | PB14 | 复位 |
| TFT RS | PC5 | 命令/数据切换 |
| TFT BLK | PB1 | 背光控制 |

### 时钟配置
- 系统时钟: 168MHz (HSE 8MHz → PLL)
- APB1: 42MHz, APB2: 84MHz
- SysTick: HCLK/8 = 21MHz

## 测试策略

**当前状态**: 本项目无自动化测试框架和测试用例。作为电赛原型项目，采用"烧录-观察-验证"的手动测试方式。

**验证方法**:
1. **FFT功能验证**: `fft_self_test()`（已移除，仅开发调试用）用软合成1kHz正弦波验证FFT输出
2. **LCD显示验证**: TFT屏幕实时显示 ch2 (PA4) 的频率、占空比、Vpp、RMS、H3、H5
3. **ADC验证**: 串口打印原始ADC min/max/Vpp，确认信号通路正常
4. **LED验证**: D2(PA1)心跳闪烁

**已知限制**:
- ⚠️ **DSP库兼容性**: 预编译 `.lib` 为旧版，**禁止使用 `arm_rfft_fast_*` 系列 API**。使用 `arm_cfft_radix2_f32` 替代，详见 [ERRORS.md](ERRORS.md)
- 无自动化测试框架
- ADC输入未做前端信号调理（阻抗匹配、抗混叠滤波）

## 编码规范

- 语言: C (C89/C90 兼容，ARM Compiler 5)
- 缩进: Tab缩进
- 编码: GB2312/GBK（中文注释为乱码在UTF-8环境下显示）
- 命名: 蛇形命名 `snake_case`，外设函数名与ST官方库保持一致
- 注释: 关键算法有中文注释
- 模块文件: 每个模块 xxx.h + xxx.c 成对出现
- 全局变量: `g_` 前缀表示全局

## AI 使用指引

1. **修改代码前**: 确认目标文件所属模块层级，底层库(FWLIB/CORE/DSP_LIB)不建议修改
2. **新增功能**: 在APP/层添加应用逻辑，在HARDWARE/层添加驱动封装
3. **FFT相关**: ⚠️ 必须使用 `arm_cfft_radix2_f32` 或 `arm_cfft_radix4_f32`，**禁止** `arm_rfft_fast_f32`
4. **避免**: 在FWLIB/中修改ST官方库文件，避免引入RTOS级依赖
5. **编译工具链**: ARM Compiler 5 (V5.06)，GBK编码源码，不同于GCC ARM
6. **提交规范**: 用户明确要求推送前再 git push，不要自动推送
7. 📖 遇问题先查 [ERRORS.md](ERRORS.md)
