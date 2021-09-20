# How to build

```
git clone https://github.com/nopnop2002/esp-idf-irSend
cd esp-idf-irSend/esp-idf-irSend-StickC/
idf.py set-target esp32
idf.py menuconfig
idf.py flash -b 115200 monitor
```

\*There is no MENU ITEM where this application is peculiar.   

__You need to specify Baud rate for flashing.__   

---

# How to use

Select IR code by ButtonB (Side Button) press.   
When a ButtonB is pressed for more than 2 seconds, it show initial screen.   
Fire IR code by ButtonA (Front Button) press.   

![StickC](https://user-images.githubusercontent.com/6020549/59671353-79a0a980-91f8-11e9-9e87-bf8760172742.JPG)
