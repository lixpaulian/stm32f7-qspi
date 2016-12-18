/*
 * qspi-flash.h
 *
 * Copyright (c) 2016 Lix N. Paulian (lix@paulian.net)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Created on: 9 Oct 2016 (LNP)
 *
 * Version: 0.1, 12 Dec 2016
 */

#ifndef QSPI_FLASH_H_
#define QSPI_FLASH_H_

#include <cmsis-plus/rtos/os.h>
#include "cmsis_device.h"

#if defined (__cplusplus)

class qspi
{
public:
  qspi (QSPI_HandleTypeDef* hqspi);

  ~qspi (void) {};

  bool
  read_JEDEC_ID (void);

  bool
  get_ID_data (uint8_t& manufacturer_ID, uint8_t& memory_type,
	       uint8_t& memory_capacity)
  {
    if (manufacturer_ID_ && memory_type_ && memory_capacity_)
      {
	manufacturer_ID = manufacturer_ID_;
	memory_type = memory_type_;
	memory_capacity = memory_capacity_;
	return true;
      }
    return false;
  }

  bool
  mode_quad (void);

  bool
  memory_mapped (void);

  bool
  read (uint32_t address, uint8_t* buff, size_t count);

  bool
  write (uint32_t address, uint8_t* buff, size_t count);

  bool
  sector_erase (uint32_t address);

  bool
  block32K_erase (uint32_t address);

  bool
  block64K_erase (uint32_t address);

  bool
  chip_erase (void);

  void
  cb_event (void);

private:

  // Winbond W25Q128FV command set (only a subset)
  static constexpr uint8_t WRITE_ENABLE = 0x06;
  static constexpr uint8_t VOLATILE_SR_WRITE_ENABLE = 0x50;
  static constexpr uint8_t WRITE_DISABLE = 0x04;

  static constexpr uint8_t READ_STATUS_REGISTER_1 = 0x05;
  static constexpr uint8_t WRITE_STATUS_REGISTER_1 = 0x01;
  static constexpr uint8_t READ_STATUS_REGISTER_2 = 0x35;
  static constexpr uint8_t WRITE_STATUS_REGISTER_2 = 0x31;
  static constexpr uint8_t READ_STATUS_REGISTER_3 = 0x15;
  static constexpr uint8_t WRITE_STATUS_REGISTER_3 = 0x11;

  static constexpr uint8_t CHIP_ERASE = 0xC7;
  static constexpr uint8_t POWER_DOWN = 0xB9;
  static constexpr uint8_t JEDEC_ID = 0x9F;
  static constexpr uint8_t RESET_DEVICE = 0x99;

  static constexpr uint8_t PAGE_PROGRAM = 0x02;
  static constexpr uint8_t QUAD_PAGE_PROGRAM = 0x32;
  static constexpr uint8_t SECTOR_ERASE = 0x20;
  static constexpr uint8_t BLOCK_32K_ERASE = 0x52;
  static constexpr uint8_t BLOCK_64K_ERASE = 0xD8;

  static constexpr uint8_t READ_DATA = 0x03;
  static constexpr uint8_t FAST_READ_DATA = 0x0B;
  static constexpr uint8_t FAST_READ_QUAD_OUT = 0x6B;
  static constexpr uint8_t FAST_READ_QUAD_IN_OUT = 0xEB;

  bool
  erase (uint32_t address, uint8_t which);

  bool
  page_write (uint32_t address, uint8_t* buff, size_t count);

  uint8_t manufacturer_ID_= 0;
  uint8_t memory_type_ = 0;
  uint8_t memory_capacity_ = 0;

  QSPI_HandleTypeDef* hqspi_;
  os::rtos::semaphore_binary semaphore_
    { "qspi", 0 };
  os::rtos::mutex mutex_
    { "qspi" };

};

inline bool
qspi::sector_erase (uint32_t address)
{
  return erase (address, SECTOR_ERASE);
}

inline bool
qspi::block32K_erase (uint32_t address)
{
  return erase (address, BLOCK_32K_ERASE);
}

inline bool
qspi::block64K_erase (uint32_t address)
{
  return erase (address, BLOCK_64K_ERASE);
}

inline bool
qspi::chip_erase (void)
{
  return erase (0, CHIP_ERASE);
}

#endif

#endif /* QSPI_FLASH_H_ */
