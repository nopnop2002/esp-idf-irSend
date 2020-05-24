# esp-idf-irSend
M5Stick and M5StickC have internal IR Transmitter.   
You can use these as Remote control transmitter.   

---

# How to get IR code.
You can get IR code using [this](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/ir_protocols) example.   

You need to connect a IR receiver to GPIO19.   
IR transmitter don't use.   
When changing GPIO19, you can change it to any GPIO using menuconfig.   

Build the project and flash it to the board, then run monitor tool to view serial output.   
When you press a button of the remote control, you will find there output:   
```
I (3513574) NEC: RMT RCV --- addr: 0xff00 cmd: 0xe718
```

0x00 is addr, and 0x18 is cmd.

Note:   
You can obtain only NEC format IR code using this example.   
You can't obtain other format IR code.   

If you can't obtain any IR code, you have to increase this value in infrared_nec_main.c.   
My recommendation is 100.   
```
#define NEC_BIT_MARGIN         20
```

---

# Setup this project.
You have to edit Display.def in font directory.   
Display.def have IR code which you want to fire.   
And Display.def have information text which you want to view.   
After which, Build this project and flash it to board.   

example of Display.def.   
```
#This is define file for isp-idf-irSend
#Text,cmd,addr;
Play,0x18,0x00; cmd:0xe718 addr:0xff00
Stop,0x1C,0x00; cmd:0xe31c addr:0xff00
Next,0x5A,0x00; cmd:0xa55a addr:0xff00
Prev,0x08,0x00; cmd:0xf708 addr:0xff00
```

**Note:**   
**Each line terminated by semicolon.

---

# NEC IR Code Specification

![NEC-IR-SPEC](https://user-images.githubusercontent.com/6020549/59671633-f7fd4b80-91f8-11e9-9bc6-45ab6e18ebc8.jpg)

