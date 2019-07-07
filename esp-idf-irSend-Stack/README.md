# External IR Transmiter
M5Stack don't have internal IR Transmitter.   
You have to attach IR Unit to GROVE-B port.   
It works very stable.   

![Stack-1](https://user-images.githubusercontent.com/6020549/59958246-574faa00-94de-11e9-95f1-24871f8c8f20.JPG)

---

# How to build

```
git clone https://github.com/nopnop2002/esp-idf-irSend
cd esp-idf-irSend/esp-idf-irSend-Stack/
make menuconfig
make flash
```

\*There is no MENU ITEM where this application is peculiar.   

---

# How to use

Select IR code by ButtonB/C (Center/Right Button) press.   
Fire IR code by ButtonA (Left Button) press.   

![Stack-2](https://user-images.githubusercontent.com/6020549/60749723-dfeb4180-9fd8-11e9-828f-8b58d1c9fc59.JPG)
