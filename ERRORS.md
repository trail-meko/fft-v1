# FFT-v1 开发踩坑记录

> 电赛仪表题 STM32F4 FFT 频谱分析项目  
> 持续更新，每次遇到新问题追加以此作为知识库

---

## 1. CMSIS-DSP `arm_rfft_fast_f32` 与老版预编译库不兼容

**日期**: 2026-06-06

**现象**: FFT 计算结果全部异常，h1 输出 `3.4e38`（FLT_MAX），h3/h5 输出几亿到几十亿的随机巨大值。

**排查过程**:
1. 先用 VCC 接 ADC 输入——排除 ADC 硬件问题（Vpp=40~60，正常纹波）
2. 写 `fft_self_test()` 用纯软件合成 1kHz 正弦波做 FFT——仍然 FLT_MAX
3. 确认问题在 DSP 库 API 层，与 ADC/DMA/硬件无关

**根因**: `arm_rfft_fast_f32` 是 CMSIS-DSP v1.5.0+ 引入的新 API，项目使用的 `arm_cortexM4lf_math.lib` 是 Keil MDK5 自带的旧版预编译库。头文件声明了新 API，但 `.lib` 内部实现与声明不匹配（输出格式或内部结构不同），导致 FFT 输出不可预测的浮点极值。

**解决方案**: 换用 `arm_cfft_radix2_f32`——CMSIS-DSP 最老最稳定的复数 FFT API，与旧 `.lib` 完美兼容。256 点实数 FFT 通过构建 256 点复数输入（实部=信号，虚部=0）来实现。

**关键教训**:
- 旧版 `.lib` 优先用老 API：`arm_cfft_radix2_f32` / `arm_cfft_radix4_f32` / `arm_rfft_f32`
- 避免用 `_fast` 后缀的新 API，除非确认 `.lib` 版本 ≥ CMSIS-DSP v1.5.0
- 先用合成数据自测 FFT，再接入真实 ADC 信号，分层排查

**状态**: ✅ 已解决

---

## 2. `arm_cfft_radix2_f32` 老 API 参数个数不同

**日期**: 2026-06-06

**现象**: 编译报错 `#140: too many arguments in function call`

```c
arm_cfft_radix2_f32(&cfft, cfft_buf, 0, 1);  // ❌ 编译失败
```

**根因**: 老版 CMSIS-DSP 中 `arm_cfft_radix2_f32` 只接受 2 个参数（instance + buffer），`ifftFlag` 和 `bitReverseFlag` 在 `arm_cfft_radix2_init_f32()` 时设定。新版头文件可能有 4 参数的重载声明，但 `.lib` 里只有 2 参数版本。

**解决方案**:
```c
arm_cfft_radix2_init_f32(&cfft, CFFT_LEN, 0, 1);  // init 时设方向和位反转
arm_cfft_radix2_f32(&cfft, cfft_buf);               // 运行时只传 2 个参数
```

**关键教训**: 老 CMSIS-DSP API 的设计哲学是 **init 时配置一切，运行时只传数据**。遇到 `too many arguments` 先查 `.lib` 实际函数签名，不要盲目信头文件。

**状态**: ✅ 已解决

---

## 3. Python 脚本写 C 文件时 `\r\n` 转义问题

**日期**: 2026-06-06

**现象**: 用 Python bytes 写入的 C 源代码中，printf 格式串的 `\r\n` 变成真实的 CR+LF 字节（0x0D 0x0A），导致编译报错或格式串被截断。

**根因**: Python 的 `b'...\r\n...'` 在字节串中 `\r` 是回车符 (0x0D)，`\n` 是换行符 (0x0A)。要写入 C 源码的 `\r\n` 转义序列，需要 `b'...\\r\\n...'`。但在 `-c` 命令行中 shell 还会再吃一层反斜杠。

**解决方案**:
- 用 Write 工具直接写入，避免 Python/shell 多层转义
- 如必须用 Python：写成单独 `.py` 文件执行，而非 `-c` 命令行
- 写入后用 hex dump 验证关键字节

**状态**: ⚠️ 注意事项

---

## 4. Keil 编译环境的 GBK 编码

**日期**: 2026-06-06

**现象**: Edit 工具匹配中文注释总是失败，因为工具收到的字节和实际文件字节不同。

**根因**: Keil MDK5 中文版源代码使用 **GBK/GB2312** 编码保存中文字符，而非 UTF-8。Edit 工具在处理时可能存在编码转换差异。

**解决方案**: 
- Edit 匹配字符串时避免包含中文注释
- 用 Python `rb` 模式读二进制字节做精确匹配
- 重写文件时用 Write 工具或 `wb` 模式

**状态**: ⚠️ 注意事项

---

## 5. `git push` 应等用户确认后再执行

**日期**: 2026-06-06

**教训**: 不要在修复 bug 的中间过程自动推送。等用户确认代码没问题、明确要求推送时再推。中间版本用 `git commit` 本地保存即可。

**状态**: ⚠️ 流程规范

---

## 项目版本历史

| Tag | 说明 |
|-----|------|
| `v1.0` | 原始 Keil 工程，官方 DSP FFT 示例 |
| `v2.0` | 新增 FFT 分析模块、ADC 三通道 (PA3/4/5)、调度器 |
| `v3.0` | 增加 DC 信号保护 (Vpp<10 时不跑 FFT) |
| `v4.0` | ❌ 尝试静态 FFT 实例 + 启动保护（无效，问题在 DSP API） |
| `v5.0` | ✅ 最终版：`arm_cfft_radix2_f32` 替代 `arm_rfft_fast_f32` |
