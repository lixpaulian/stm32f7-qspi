# stm32f7-qspi
This is a QSPI serial flash driver for the STM32F7xx family of controllers.

## Version
* 2.11 (26 May 2018)

## License
* MIT

## Package
The driver is provided as an XPACK and can be installed in an Eclipse based project using the attached script (however, the include and source paths must be manually added to the project in Eclipse). For more details on XPACKs see https://github.com/xpacks. The installation script requires the helper scripts that can be found at https://github.com/xpacks/scripts.

## Dependencies
The driver depends on the following software packages:
* STM32F7 CMSIS (https://github.com/xpacks/stm32f7-cmsis)
* STM32F7xx HAL Library (https://github.com/xpacks/stm32f7-hal)
* uOS++ (https://github.com/micro-os-plus/micro-os-plus-iii), version 6.3.14 and up.

Alternatively, the CMSIS and HAL Library xpacks can be provided by ST's CubeMX code generator (recommened, as the STM32xxx xpacks will probably be discontinued).

The hardware initialisations (uController clock, peripherals clocks, etc.) must be separately performed, normaly in, or called from the initialize_hardware.c file of a GNU MCU Eclipse project. You can do this using the CubeMX code generator from ST. You may find helpful to check the following projects as references:
* https://github.com/micro-os-plus/eclipse-demo-projects/tree/master/f746gdiscovery-blinky-micro-os-plus
* https://github.com/micro-os-plus/eclipse-demo-projects/tree/master/f746gdiscovery-blinky-micro-os-plus/cube-mx which details how to integrate the CubeMX generated code into a uOS++ based project.

Since version 2.0 of the driver, the API has been changed for a better integration with the POSIX layer of uOS++. It is an implementation of an uOS++ block device. Although the driver is now more tightly coupled to the µOS++ ecosystem, it can be however ported to other RTOSes. It has been tested on the Winbond W25Q128FV and Micrel/ST MT25QL128ABA flash chips, but support for other devices will be  added in the future.

An optional plain C API is also provided. Note that in the case of the C interface the qspi object is generated dynamically. However, this API may be discontinued in the future, as a better approach is to use the native C Posix interface offered through uOS++.

## Short theory of operation
Most QSPI flash devices operate in two basic modes:
* Extended SPI mode: instruction, address and data can be sent/received to/from the chip both in single and quad (or dual) mode (e.g. instruction and address in single line mode and data in quad mode).
* Quad (or QPI) mode: the communication to/from the chip is done exclusively in quad mode.

A device cannot operate in both modes at the same time. There are provisions to switch the chip from one mode to the other, however there are differences on how the switch is done from chip to chip. There is also the danger that the flash chip and the controller get out of sync, e.g. if the controller is reset but the flash chip is not.

The philosophy behind the driver is that there is only one command executed in standard mode: read ID. This is done right after the system comes up and is initialized. If the chip is identified and known for the driver, it is immediately switched to quad mode. From now on, all commands are implemented in quad mode. If for any unforeseen reasons there is a need to switch back to standard mode, you can use the reset function call. For an example on how to use the driver, check out the "test" directory.

## Tests
There is a test that must be run on a real target. Note that the test is distructive, the whole content of the flash will be lost! Test files are provided for both C++ and C APIs. To select what API to use, you have to set the proper value for the TEST_CPLUSPLUS_API symbol in the test-qspi-config.h file.

The test performs the following flash operations:
* Switches the power-on
* Reads-out the chip ID and initializes the internal driver structures (manufacturer, flash type, sector count and size)
* Switches the flash to memory-mapped mode; the flash is mapped at the address 0x90000000
* Checks if the flash is erased (all FFs); if it is not, the flash will be erased
* Generates a stream of random bytes and writes them to the flash, one sector (4 KBytes at a time).
* Compares the values written to the original values in RAM.
* Switches the flash in deep sleep mode, then switches the power off.

The C++ version includes timings for most of the operations, whereas the C version does not.

In addition, a test is provided to assess compatibility with the ChaN FAT file system, offered through uOS++; for running this test, you need to install the Chan FAT file system xpack at https://github.com/xpacks/chan-fatfs.git. This xpack contains among other things, a C++ diskio wrapper.



