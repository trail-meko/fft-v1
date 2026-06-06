[根目录](../CLAUDE.md) > **FWLIB**

## 模块职责

ST官方STM32F4xx标准外设库(Standard Peripherals Library)。提供所有MCU外设的寄存器级驱动函数，是项目最底层的硬件抽象层。**此目录为官方库文件，不应修改。**

## 子模块结构

```
FWLIB/
├── inc/    # 外设驱动头文件 (28个)
└── src/    # 外设驱动源文件 (32个)
```

## 已启用的外设模块

根据 `USER/stm32f4xx_conf.h` 的配置，本项目启用了以下外设驱动：

| 外设 | 头文件 | 源文件 | 用途 |
|------|--------|--------|------|
| ADC | stm32f4xx_adc.h | stm32f4xx_adc.c | ADC1 3通道采集 |
| CRC | stm32f4xx_crc.h | stm32f4xx_crc.c | (保留) |
| DBGMCU | stm32f4xx_dbgmcu.h | stm32f4xx_dbgmcu.c | 调试支持 |
| DMA | stm32f4xx_dma.h | stm32f4xx_dma.c | DMA2_Stream0 ADC传输 |
| EXTI | stm32f4xx_exti.h | stm32f4xx_exti.c | (保留) |
| FLASH | stm32f4xx_flash.h | stm32f4xx_flash.c | Flash编程 |
| GPIO | stm32f4xx_gpio.h | stm32f4xx_gpio.c | 所有GPIO初始化 |
| I2C | stm32f4xx_i2c.h | stm32f4xx_i2c.c | (保留) |
| IWDG | stm32f4xx_iwdg.h | stm32f4xx_iwdg.c | (保留) |
| PWR | stm32f4xx_pwr.h | stm32f4xx_pwr.c | 电源管理 |
| RCC | stm32f4xx_rcc.h | stm32f4xx_rcc.c | 时钟配置 |
| RTC | stm32f4xx_rtc.h | stm32f4xx_rtc.c | (保留) |
| SDIO | stm32f4xx_sdio.h | stm32f4xx_sdio.c | (保留) |
| SPI | stm32f4xx_spi.h | stm32f4xx_spi.c | (保留，当前用GPIO模拟SPI) |
| SYSCFG | stm32f4xx_syscfg.h | stm32f4xx_syscfg.c | 系统配置 |
| TIM | stm32f4xx_tim.h | stm32f4xx_tim.c | TIM2/TIM3定时器 |
| USART | stm32f4xx_usart.h | stm32f4xx_usart.c | USART1串口 |
| WWDG | stm32f4xx_wwdg.h | stm32f4xx_wwdg.c | (保留) |

**芯片特定外设** (STM32F40_41xxx条件编译):
- CRYP, HASH, RNG, CAN, DAC, DCMI, FSMC

## 对外接口

头文件 `inc/*.h` 提供各外设的完整API声明，包括初始化结构体、配置函数、命令函数等。具体函数参考ST官方文档。

**通用辅助函数** (misc.h/c):
- `NVIC_PriorityGroupConfig()` — 中断优先级分组
- `NVIC_Init()` — NVIC初始化
- `SysTick_CLKSourceConfig()` — SysTick时钟源选择

## 关键依赖

| 依赖 | 说明 |
|------|------|
| stm32f4xx.h | 芯片寄存器地址映射 |
| core_cm4.h | CMSIS Cortex-M4 核心寄存器定义 |
| system_stm32f4xx.h | 系统时钟初始化 |

## 数据模型

库使用结构体参数传递模式（STM32标准库风格），例如：
```c
GPIO_InitTypeDef GPIO_InitStructure;
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
GPIO_Init(GPIOA, &GPIO_InitStructure);
```

## 测试与质量

- ST官方测试和验证
- 版本: V1.4.0 (2014-08-04)
- **禁止修改此目录下任何文件**

## 常见问题 (FAQ)

- **Q: 编译时报 "Undefined symbol" 的外设函数？** A: 确认 `stm32f4xx_conf.h` 中已 include 对应外设头文件，且 `.c` 源文件已加入工程
- **Q: 想使用硬件SPI替代模拟SPI？** A: `FWLIB/src/stm32f4xx_spi.c` 已可用，只需在应用层调用 SPI 初始化函数

## 相关文件清单

| 目录 | 文件数 | 说明 |
|------|--------|------|
| inc/ | 28 | 外设头文件 |
| src/ | 32 | 外设源文件（含 cryp_aes/des/tdes, hash_md5/sha1 等子模块） |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
