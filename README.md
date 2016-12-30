# stm32f7-qspi
This is a QSPI serial flash driver for the STM32F7xx family of controllers.

## Version
* 0.2

## License
* MIT

## Package
The driver is provided as an XPACK and can be installed in an Eclipse based project using the attached script (however, the include and source paths must be manually added to the project in Eclipse). For more details on XPACKs see https://github.com/xpacks. The installation script requires the helper scripts that can be found at https://github.com/xpacks/scripts.

## Dependencies
The driver depends on the following software packages:
* STM32F7 CMSIS (https://github.com/xpacks/stm32f7-cmsis)
* STM32F7xx HAL Library (https://github.com/xpacks/stm32f7-hal)
* uOS++ (https://github.com/micro-os-plus/micro-os-plus-iii)

Note that the hardware initialisations (uController clock, peripherals clocks, etc.) must be separately performed, normaly in the initialize_hardware.c file of a gnuarmeclipse project. You can do this using the MX-Cube generator from ST. You may find helpful to check the following project:
* https://github.com/micro-os-plus/eclipse-demo-projects/tree/master/f746gdiscovery-blinky-micro-os-plus
* https://github.com/micro-os-plus/eclipse-demo-projects/tree/master/f746gdiscovery-blinky-micro-os-plus/cube-mx which details how to integrate the Cube-MX generated code into a uOS++ based project.

The driver can be easily ported to other RTOSes, as it uses only a semaphore and a mutex. It has been tested on the Winbond W25Q128FV flash, but support for other chips will be  added in the future.

## Tests
There is a test that must be run on a real target. Note that the test is distructive, the whole content of the flash will be lost!

The test performs the following flash operations:
* Switches the flash to memory-mapped mode; the flash is mapped at the address 0x90000000
* Checks if the flash is erased (all FFs); if it is not, the flash will be erased
* Generates a stream of random bytes and writes them to the flash, 4K at a time (the erase block length)
* Compares the values written to the original values in RAM.
