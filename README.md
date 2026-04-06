# MyHealthDetector

**技术栈：** STM32F407VET6 (Cortex-M4) | FreeRTOS | LVGL 8.2 | I2C | SPI | QT |SQLite

**核心功能：** 非接触式红外体温测量(MLX90614) + 光电心率检测，配合实时GUI交互与智能电源管理

### 系统架构亮点

**1. 分层软件架构**

- **GUI层：** 基于LVGL的轻量级图形框架，支持多页面状态机（心率/体温/设置），实现60fps流畅动画与触摸/按键双模交互
- **RTOS层：** FreeRTOS任务调度，独立任务处理传感器采样(10Hz)、GUI刷新(60Hz)、按键扫描(50Hz)、电源管理(1Hz)，保证实时性
- **驱动层：** 硬件抽象封装MLX90614 SMBus协议、PWM背光驱动、ADC心率采样

**2. 高可靠性电源管理系统**

- **双电源无缝切换：** 电池与Type-C供电自动切换电路，支持热插拔不掉电
- **防误触开关机逻辑：** 状态机实现短按开机、长按2秒进入关机确认页，二次确认防止误操作
- **三级低功耗策略：** 自动休眠(30s无操作)→背光调节(5-100% PWM)→硬关机，延长便携续航

**3. 人机交互优化**

- **工业级交互设计：** 物理按键+GUI状态机双控，测量过程中可中断退出
- **可视化反馈：** 实时温度曲线、心率波形绘制，异常值红色警示
- **数据滤波算法：** 滑动平均滤波+中值滤波，抑制传感器噪声，确保测量稳定性

------

### 技术难点攻克

- **SMBus时序精准控制：** 自主实现MLX90614红外温度传感器SMBus通信协议，解决I2C兼容时序问题，实现±0.2°C精度体温测量
- **FreeRTOS与LVGL协同：** 设计双缓冲队列解耦传感器数据与GUI显示，避免阻塞式测量导致界面卡顿
- **复杂电源状态机：** 处理上电、运行、休眠、关机、充电等7种状态迁移，确保异常断电数据安全

------

**适用场景：** 便携式健康监测、嵌入式GUI开发参考、RTOS实时系统设计案例

------

**英文版（供外企/国际项目使用）：**

**Health Monitor Embedded System | STM32F407 + FreeRTOS + LVGL**

A medical-grade portable health monitoring device featuring **dual-mode power management** and **RTOS-based real-time GUI**.

**Key Technical Features:**

- **Hardware:** STM32F407VET6 (Cortex-M4@168MHz), MLX90614 IR temperature sensor, dual-power switching circuit (Battery/USB-C), hardware anti-miss-touch power logic
- **Firmware:** FreeRTOS multi-tasking (Sensor/GUI/Power/Key tasks), LVGL8.x lightweight graphics library with 60fps animation
- **Power Management:** State-machine controlled power sequence (Short-press ON / Long-press 2s shutdown confirm / Auto-sleep), seamless power switching without reboot
- **Algorithms:** Real-time temperature sampling with sliding average filtering, heart rate detection via PPG signal processing

**Engineering Highlights:**

- Implemented SMBus protocol from scratch for MLX90614 communication, achieving ±0.2°C accuracy
- Decoupled sensor acquisition and GUI rendering via message queues, ensuring real-time responsiveness during measurement
- Robust power state machine handling 7 states (Boot/Run/Sleep/Shutdown/Charging/Error), with data protection on abnormal power loss

**Stack:** C/C++, STM32,FreeRTOS, LVGL, Git

------



