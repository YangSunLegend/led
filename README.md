# led
 对platform_device和platform_driver中核心结构体的编写和功能函数的实现 开发环境：  Linux操作系统 开发工具：  vim文本编辑器、gcc编译器 开发流程：      1：在platform_device中定义资源数组等结构体，然后在platform_driver中进行相关功能函数的实现      2：在platform_driver的操作函数中操作WDT的DAT寄存器来设置初始计时时间      3：在使能WDT的同时，通过timer_list进行计时，在WDT的CNT寄存器为1时重置该寄存器
