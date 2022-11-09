# How to build

```
git clone https://github.com/nopnop2002/esp-idf-irSend
cd esp-idf-irSend/esp-idf-irSend-Atom/
idf.py set-target esp32
idf.py menuconfig
idf.py flash -b 115200 monitor
```

\*There is no MENU ITEM where this application is peculiar.   

__You need to specify Baud rate for flashing.__   

--- 

# How to use

M5Atom only has one button.    
There is no display.    
Pressing the button fires lines 1 and 2 of Display.def in the font directory alternately.   
```
Play,0x18,0x00;	cmd:0xe718 addr:0xff00
Stop,0x1C,0x00;	cmd:0xe31c addr:0xff00
```

When the 1st and 2nd lines are the same, they always fire the same signal.   
```
Play,0x18,0x00;	cmd:0xe718 addr:0xff00
Play,0x18,0x00;	cmd:0xe718 addr:0xff00
```

