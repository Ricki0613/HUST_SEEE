实验四：串口通讯及中断｜STM32F407ZGT6 探索者
1. 解压整个工程到本机，Keil 5打开 USER/USMART.uvprojx。工程沿用实验14的HAL、LCD和时钟，使用原工程的ARM Compiler 5；不需要CubeMX。选择自己的下载器（例如CMSIS-DAP），Rebuild后下载。不要直接烧旧例程的hex。
2. 上电显示MODE: PC ECHO、115200 8N1。PC连接板载USB_232（CH340），选择对应COM，8数据位/无校验/1停止位/无流控；P6保留板载USB串口与PA9/PA10的连接。串口助手使用ASCII，打开“发送新行”，确保是CR和LF两个字节0D 0A，不要发送四个可见字符。每次等屏幕更新后再发送，不启用自动循环。
3. 任务1：打开samples/120.txt，全选复制正文到发送框，发送一次。文件本身无换行。应收到ok+和完全相同的120个字符，尾部为CRLF；屏幕RX:120 bytes。短帧可测通断，但验收请发送至少100字节。检查首尾和长度，不只看有没有回显。
4. 任务2：用相同方法发送samples/4000.txt（4000个ASCII字符，不含换行），确认回包为4005字节。KEY1切换波特率，再把PC调为9600，重复测量。LED0每100ms翻转，完整周期0.2s，只在收发期间闪烁；120字节高速传输太短，验灯用4000字节。屏幕Time：首字节进入中断至应答最后停止位发完，单位ms。last_us保留微秒；PC端往返含USB和系统延迟，不能直接当单程。正文太长时LCD只显示前若干行，串口回传完整。
5. 可选PC校验：先关闭串口助手。安装pyserial：py -m pip install pyserial。两次测量均先手工切换板端波特率。示例：py tools/pc_test.py COM5 115200 120；py tools/pc_test.py COM5 115200 4000；py tools/pc_test.py COM5 9600 4000。以实际COM替换COM5。脚本核对整个返回值并给出PASS/FAIL和PC往返耗时。
6. 任务3：两板烧相同工程。断电后取下两板P6上连接CH340与USART1的两只帽，接MCU侧A.PA9→B.PA10、B.PA9→A.PA10、GND↔GND。两板各自供电，不互接5V/3V3。不要接RS232电平的DB9。两板上电，各按KEY2切至BOARD，两边均为115200。先按A的KEY0：B显示收到的120字节信息并自动回复；A显示收到的ok+应答。待显示稳定，再按B的KEY0反向验证。不要同时按；ok+为双板应答保留前缀。回到PC实验先断开板间线、恢复P6，再切PC模式。仅在空闲时切换模式或波特率。
7. 源码：USER/main.c（按键与任务、回传、LCD），SYSTEM/usart/usart.c及.h（USART1接收中断、计时和发送），SYSTEM/usart/frame.h（分帧），USER/stm32f4xx_it.c（1ms节拍驱动LED），USER/USMART.uvprojx（移除命令解析组、外部晶振8MHz）。原LCD、串口初始化顺序和printf重定向保留。Watch：last_len、last_us、uart_errors。超过4096字节或串口硬件错误，丢弃当前帧，遇CRLF回复ERR。缺少CRLF则等待，切换模式可复位。
8. 已核对工程源文件路径、运行C分帧测试（含4000、4096边界、超长和正文内单个CR），用GCC及CMSIS头文件做修改模块语法检查；未在Keil链接或实物板烧录。报告的实测栏需根据实际验收填写。
