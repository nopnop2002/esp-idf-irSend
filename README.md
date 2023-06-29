# esp-idf-irSend
M5Atom, M5Stick and M5StickC(+) have internal IR Transmitter.   
You can use these as Remote control transmitter.   

# Software requirements
esp-idf v4.4/v5.0.   

__Note for esp-idf v5.0__   
esp-idf v5.0 gives this warning, but work.   

```
#warning "The legacy RMT driver is deprecated, please use driver/rmt_tx.h and/or driver/rmt_rx.h"
```
This is because the specifications of the RMT driver have changed significantly with esp-idf v5.   
Presumably, esp-idf v5.1 will completely obsolete the legacy RMT driver.   


# How to get IR code.
You can get IR code using esp-idf-irAnalysis.   

```
git clone https://github.com/nopnop2002/esp-idf-irSend
cd esp-idf-irSend/esp-idf-irAnalysis/
idf.py set-target esp32
idf.py menuconfig
idf.py flash monitor
```

Connect the IR receiver to GPIO19.   
I used the TL1838, but you can use anything as long as it is powered by 3.3V.   
IR transmitter don't use.   
When changing GPIO19, you can change it to any GPIO using menuconfig.   

```
 IR Receiver (TL1838)               ESP Board
+--------------------+       +-------------------+
|                  RX+-------+GPIO19             |
|                    |       |                   |
|                 3V3+-------+3V3                |
|                    |       |                   |
|                 GND+-------+GND                |
+--------------------+       +-------------------+
```

Build the project and flash it to the board, then run monitor tool to view serial output.   
When you press a button of the remote control, you will find there output:   
```
I (86070) main: Scan Code  --- addr: 0xff00 cmd: 0xe718
I (86120) main: Scan Code (repeat) --- addr: 0xff00 cmd: 0xe718
```
addr and cmd is displayed as below:   
addr: 0xff00 --> {0xff-addr} << 8 + addr   
cmd: 0xe718 --> {0xff-cmd} << 8 + cmd   

This shows addr as 0x00 and cmd as 0x18.

**Note:**   
You can get only NEC format IR code using this.   
You can't get other format IR code.   


# Setup this project.
You have to edit Display.def in font directory.   
Display.def have IR code which you want to fire.   
And Display.def have information text which you want to view.   
After which, Build this project and flash it to board.   

example of Display.def.   
```
#This is define file for isp-idf-irSend
#Text,cmd,addr;comment
Play,0x18,0x00; cmd:0xe718 addr:0xff00
Stop,0x1C,0x00; cmd:0xe31c addr:0xff00
Next,0x5A,0x00; cmd:0xa55a addr:0xff00
Prev,0x08,0x00; cmd:0xf708 addr:0xff00
```

**Note:**   
Each line terminated by semicolon.


# NEC IR Code Specification

[NEC-IR-SPEC](https://user-images.githubusercontent.com/6020549/59671633-f7fd4b80-91f8-11e9-9bc6-45ab6e18ebc8.jpg)

