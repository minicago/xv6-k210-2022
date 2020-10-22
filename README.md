# XV6-RISCV On K210
Run xv6-riscv on k210 board
![run-k210](./img/run-k210.png)  

## Dependencies
+ k210 board
+ RISC-V Toolchain

## Installation
>\$ git clone https://github.com/SKTT1Ryze/xv6-k210

## Build
First you need to connect your k210 board to your PC.  
And check the USB port:  
>\$ ls /dev/ | grep USB  

In my situation it will be `ttyUSB0`  

>\$ cd xv6-k210  
>\$ mkdir target  
>\$ make build

## Run
>\$ make run-k210 k210-serialport=`Your-USB-port`(default by ttyUSB0)

## What I have done
+ Multicore boot
+ Implement bare-metal printf
+ Memory alloc
+ Page Table

## TODO
The rest part of xv6-kernel and xv6-fs

## LICENSE
MIT License