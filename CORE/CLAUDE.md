[根目录](../CLAUDE.md) > **CORE**

## 模块职责

Cortex-M4 核心支持文件。包含 ARM CMSIS-Core 标准头文件和 STM32F4 汇编启动文件。**此目录为标准文件，通常不应修改。**

## 子模块结构

```
CORE/
├── core_cm4.h           # CMSIS Cortex-M4 核心外设访问层头文件
├── core_cm4_simd.h      # Cortex-M4 SIMD 指令内联函数
└── startup_stm32f40_41xxx.s  # 汇编启动文件（MDK-ARM工具链版本）
```

## 入口与启动

### startup_stm32f40_41xxx.s
- **功能**:
  1. 设置初始堆栈指针 (SP)
  2. 设置初始程序计数器 (PC) = Reset_Handler
  3. 建立中断向量表（包含所有 Cortex-M4 异常和 STM32F4 外设中断）
  4. 配置系统时钟（调用 `SystemInit`）
  5. 跳转到 C 库的 `__main`（最终调用 `main()`）
- **版本**: V1.4.0 (2014-08-04), STMicroelectronics
- **芯片**: STM32F40xxx/41xxx 系列
- **注意**: 此文件不同于 GCC 的 `.s` 启动文件语法，专用于 ARM Compiler (MDK-ARM)

### core_cm4.h
- **功能**: Cortex-M4 处理器核心寄存器定义和内联函数
  - NVIC (嵌套向量中断控制器)
  - System Control Block (SCB)
  - SysTick (系统定时器)
  - MPU (内存保护单元)
  - FPU (浮点单元)
  - Debug 支持
- **版本**: ARM CMSIS v3.x 兼容

### core_cm4_simd.h
- **功能**: M4 SIMD 指令的 C 语言内联函数（如 `__SADD8`, `__QADD8` 等）
- **注意**: 本项目的 FFT 运算使用的是 CMSIS-DSP 库（预编译），不直接调用 SIMD 内联函数

## 对外接口

| 符号 | 来源 | 说明 |
|------|------|------|
| Reset_Handler | startup | 复位向量入口 |
| NMI_Handler | startup | 不可屏蔽中断 |
| HardFault_Handler | startup | 硬件错误 |
| SysTick_Handler | startup (弱引用) | 系统定时器中断 |
| 各外设 IRQHandler | startup (弱引用) | 外设中断向量 |
| `__initial_sp` | startup | 初始栈顶地址 |
| `SystemCoreClock` | (外部) | 系统时钟频率 |

## 关键依赖

| 依赖 | 说明 |
|------|------|
| system_stm32f4xx.h | SystemInit() 声明（由 USER/ 提供） |
| stm32f4xx.h | 芯片寄存器地址映射 |

## 相关文件清单

| 文件 | 说明 |
|------|------|
| core_cm4.h | Cortex-M4 核心寄存器定义 |
| core_cm4_simd.h | M4 SIMD 指令内联函数 |
| startup_stm32f40_41xxx.s | 向量表 + 复位启动序列 |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
