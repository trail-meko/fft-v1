[根目录](../CLAUDE.md) > **SYSTEM**

## 模块职责

系统服务层，提供最基础的软件服务：基于 SysTick 的微秒/毫秒延时、位带操作的 GPIO 快速访问宏、以及 USART1 串口 printf 重定向和中断接收。

## 子模块结构

```
SYSTEM/
├── delay/
│   ├── delay.c    # SysTick 延时实现
│   └── delay.h    # 延时函数声明
├── sys/
│   ├── sys.c      # 内联汇编（WFI/关中断/开中断/设MSP）
│   └── sys.h      # 位带操作宏、IO快速访问宏
└── usart/
    ├── usart.c    # USART1初始化、printf重定向、中断接收
    └── usart.h    # 串口配置宏与函数声明
```

## 入口与启动

### sys — 系统核心
- **sys.h**: 使用 Cortex-M4 位带(Bit-band)机制实现类似 51 单片机的 `PAout(n)`/`PAin(n)` 位操作宏，支持 GPIOA~I 全部端口
- **sys.c**: ARMCC 内联汇编函数：
  - `WFI_SET()` — 执行 WFI 指令进入低功耗
  - `INTX_DISABLE()` — CPSID I 关闭全局中断
  - `INTX_ENABLE()` — CPSIE I 开启全局中断
  - `MSR_MSP(addr)` — 设置主栈指针
- **配置宏**: `SYSTEM_SUPPORT_OS = 0` — 当前禁用 RTOS 支持

### delay — 延时服务
- **delay_init(SYSCLK)**: 配置 SysTick 时钟源 = HCLK/8
  - `fac_us = SYSCLK/8` — 每微秒计数（168MHz → 21）
  - `fac_ms = fac_us * 1000` — 每毫秒计数
- **delay_us(nus)**: 非OS模式→单次SysTick精确定时（最大约798ms @168MHz）
- **delay_ms(nms)**: 分段调用 `delay_xms(540)` 避免24位计数器溢出
- **OS支持**: `#if SYSTEM_SUPPORT_OS` 预编译分支支持 UCOSII/UCOSIII，提供调度锁/解锁/系统延时封装

### usart — 串口服务
- **uart_init(bound)**: 初始化 USART1 (PA9 TX/PA10 RX, 115200bps, 8N1)
  - 使能中断接收 `USART_IT_RXNE`
  - NVIC: 抢占优先级3, 响应优先级3
- **printf 重定向**: 通过 `fputc()` 重定向到 USART1->DR，需启用 Keil "Use MicroLIB"
- **中断接收**: `USART1_IRQHandler()` 将接收字节存入 `USART_RX_BUF[]`，以 `\r\n` (0x0d 0x0a) 为帧尾
  - `USART_RX_STA`: bit15=接收完成, bit14=收到0x0d, bit13:0=有效字节数

## 对外接口

| 函数/宏 | 文件 | 说明 |
|---------|------|------|
| `delay_init(SYSCLK)` | delay.c | SysTick延时初始化 |
| `delay_us(nus)` | delay.c | 微秒延时 |
| `delay_ms(nms)` | delay.c | 毫秒延时 |
| `PAout(n)` / `PAin(n)` | sys.h | GPIO位带操作（A~I端口通用） |
| `WFI_SET()` | sys.c | 进入低功耗等待 |
| `INTX_DISABLE()` | sys.c | 关闭全局中断 |
| `INTX_ENABLE()` | sys.c | 开启全局中断 |
| `uart_init(bound)` | usart.c | 串口1初始化 |
| `printf(...)` | (重定向) | 通过 USART1 输出 |
| `USART_RX_BUF[200]` | usart.c | 接收缓冲区 |
| `USART_RX_STA` | usart.c | 接收状态寄存器 |

## 关键依赖与配置

| 依赖 | 说明 |
|------|------|
| stm32f4xx.h | STM32F4 寄存器定义 |
| stm32f4xx_conf.h | 外设配置（包含 USART/GPIO/RCC 等驱动头） |
| SYSTEM_SUPPORT_OS | 宏开关（0=裸机, 1=支持UCOS） |
| EN_USART1_RX | 串口接收使能（1=使能） |
| USART_REC_LEN | 接收缓冲区大小（200字节） |

## 数据模型

- **SysTick**: 24位递减计数器，时钟源 HCLK/8 = 21MHz，最大单次延时约 798ms
- **USART帧格式**: `\r\n` 结尾的文本帧，最大200字节
- **位带映射**: GPIO ODR寄存器 → 位带别名区，实现单bit原子操作

## 测试与质量

- 无自动化测试
- **稳定性**: delay和usart是最基础的服务层，所有上层模块依赖它们，需确保正确初始化
- **注意事项**:
  - `delay_init()` 必须在使用任何延时前调用
  - printf 需要 Keil 勾选 "Use MicroLIB"
  - USART中断优先级较低(3,3)，实时性要求高的中断应设更高优先级

## 常见问题 (FAQ)

- **Q: printf无输出？** A: 检查 (1) Keil 工程选项勾选 "Use MicroLIB"; (2) `uart_init()` 波特率匹配; (3) PA9/PA10 引脚复用配置
- **Q: delay不准？** A: 确认 `delay_init(168)` 参数与实际系统时钟匹配（STM32F407标准为168MHz）
- **Q: USART接收丢数据？** A: 增大 `USART_REC_LEN`，或在接收完成标志置位后及时处理

## 相关文件清单

| 文件 | 行数 | 说明 |
|------|------|------|
| delay/delay.c | ~243 | SysTick延时+UCOS支持 |
| delay/delay.h | ~15 | 延时声明 |
| sys/sys.c | ~52 | ARM内联汇编函数 |
| sys/sys.h | ~89 | 位带操作宏定义 |
| usart/usart.c | ~141 | USART1+printf+中断接收 |
| usart/usart.h | ~22 | 串口宏与声明 |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
