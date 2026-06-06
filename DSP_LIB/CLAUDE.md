[根目录](../CLAUDE.md) > **DSP_LIB**

## 模块职责

ARM CMSIS-DSP 软件库。提供针对 ARM Cortex-M 处理器优化的数字信号处理函数，包括本项目核心的 FFT 快速傅里叶变换。以预编译库 (.lib) + 头文件形式提供。

## 子模块结构

```
DSP_LIB/
├── arm_cortexM4lf_math.lib    # 预编译DSP库（Cortex-M4, 小端, FPU）
└── Include/                   # CMSIS-DSP 头文件 (12个)
    ├── arm_math.h             # 主头文件（所有DSP函数声明）
    ├── arm_common_tables.h    # 通用查找表（FFT旋转因子、三角函数表等）
    ├── arm_const_structs.h    # 常量结构体定义
    └── core_cm*.h             # 各Cortex-M变体的核心头文件（7个）
```

## 入口与启动

**预编译库选择**: `arm_cortexM4lf_math.lib`
- **l** = Little-endian (小端格式)
- **f** = Hardware FPU with SP extensions (硬件单精度浮点)

**编译配置要求** (Keil uVision):
- Target → Floating Point Hardware: "Use FPU"
- Target → Floating Point ABI: 使用硬件 FPU
- Linker → Misc controls: 添加 `arm_cortexM4lf_math.lib` 路径

## 对外接口（本项目使用）

| 函数 | 来源 | 说明 |
|------|------|------|
| `arm_cfft_radix4_init_f32()` | arm_math.h | 初始化基4复FFT实例结构体 |
| `arm_cfft_radix4_f32()` | arm_math.h | 执行基4 1024点复FFT（定点） |
| `arm_cmplx_mag_f32()` | arm_math.h | 计算复数幅度（√(real²+imag²)） |
| `arm_sin_f32()` | arm_math.h | 浮点正弦（查表法） |
| `arm_cos_f32()` | arm_math.h | 浮点余弦（查表法） |
| `PI` | arm_math.h | 圆周率宏 |

## 关键依赖

| 依赖 | 说明 |
|------|------|
| core_cm4.h | CMSIS-Core 寄存器定义 |
| FPU (硬件) | 必须启用硬件浮点单元 |

## 数据模型

### FFT 数据结构
```c
arm_cfft_radix4_instance_f32 scfft;  // FFT实例结构体
arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);
// 参数: 实例, 点数, 是否正变换(0=正), 是否按位反转(1=是)
```

### FFT 数据布局
- **输入**: `fft_inputbuf[FFT_LENGTH*2]` (float)，交替排列 [real0, imag0, real1, imag1, ...]
- **输出**: 原地覆盖输入缓冲
- **幅度**: `fft_outputbuf[FFT_LENGTH]` (float)，由 `arm_cmplx_mag_f32` 计算得到

## 测试与质量

- ARM 官方验证的 DSP 库
- FFT 功能通过软合成信号自测验证（main.c 中混合频率信号的频谱峰值应在基频、4倍频、8倍频处出现）

## 常见问题 (FAQ)

- **Q: 链接时 undefined symbol 的 arm_* 函数？** A: 确认 `arm_cortexM4lf_math.lib` 已加入链接器搜索路径
- **Q: FFT 结果异常？** A: 检查 (1) FFT 点数与 `arm_cfft_radix4_init_f32` 参数匹配; (2) fft_inputbuf 的实部/虚部交替格式正确; (3) FPU 已启用
- **Q: 编译警告关于 core_cm4.h 冲突？** A: DSP_LIB/Include/ 下的 core_cm*.h 与 CORE/ 下的版本可能冲突。确保 include 路径优先级：先 CORE/，后 DSP_LIB/Include/

## 相关文件清单

| 文件 | 说明 |
|------|------|
| arm_cortexM4lf_math.lib | 预编译DSP库 |
| Include/arm_math.h | DSP 主头文件 |
| Include/arm_common_tables.h | FFT 旋转因子表 |
| Include/arm_const_structs.h | 常量结构体定义 |
| Include/core_cm0.h | (冗余，DSP库自带) |
| Include/core_cm0plus.h | (冗余) |
| Include/core_cm3.h | (冗余) |
| Include/core_cm4.h | (冗余，可能与CORE/冲突) |
| Include/core_cm4_simd.h | (冗余) |
| Include/core_cmFunc.h | (冗余) |
| Include/core_cmInstr.h | (冗余) |
| Include/core_sc000.h | (冗余) |
| Include/core_sc300.h | (冗余) |

## 变更记录 (Changelog)

| 日期 | 变更 | 来源 |
|------|------|------|
| 2026-06-06 | 初始文档生成 | 架构师初始化 |
