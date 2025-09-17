## H730 Firmware

This directory contains the firmware source code for the H730 Audio board, including the STM32 HAL and ARM CMSIS libraries that were used to build it. There is source code for basic functionality but the cloud and delay applications are withheld at this time.

The two applications are:

- blinky - basic initial test of the blinking LED on the board.

- main - a boilerplate / template for creating other applications that intializes all the on-board resources and creates a basic audio I/O stream.


