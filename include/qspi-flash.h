/*
 * qspi-flash.h
 *
 * Copyright (c) 2016, 2017 Lix N. Paulian (lix@paulian.net)
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
 */

#ifndef QSPI_FLASH_H_
#define QSPI_FLASH_H_

#include <cmsis-plus/rtos/os.h>
#include "cmsis_device.h"
#include "quadspi.h"

#if defined (__cplusplus)

typedef struct qspi_device_s qspi_device_t;
class qspi_impl;

class qspi
{
public:
  qspi (QSPI_HandleTypeDef* hqspi);

  ~qspi () = default;

  enum qspi_result_t {
    ok = 0,
    error = HAL_ERROR,
    busy = HAL_BUSY,
    timeout = HAL_TIMEOUT,
    type_not_found,
  };

  void
  get_version (uint8_t& version_major, uint8_t& version_minor);

  void
  power (bool state);

  qspi_result_t
  sleep (bool state);

  qspi_result_t
  initialize (void);

  qspi_result_t
  uninitialize (void);

  qspi_result_t
  enter_mem_mapped (void);

  qspi_result_t
  exit_mem_mapped (void);

  qspi_result_t
  read (uint32_t address, uint8_t* buff, size_t count);

  qspi_result_t
  write (uint32_t address, uint8_t* buff, size_t count);

  qspi_result_t
  read_sector (uint32_t sector, uint8_t* buff, size_t count);

  qspi_result_t
  write_sector (uint32_t sector, uint8_t* buff, size_t count);

  qspi_result_t
  erase_sector (uint32_t sector);

  qspi_result_t
  erase_block32K (uint32_t address);

  qspi_result_t
  erase_block64K (uint32_t address);

  qspi_result_t
  erase_chip (void);

  qspi_result_t
  reset_chip (void);

  const char*
  get_manufacturer (void);

  const char*
  get_memory_type (void);

  size_t
  get_sector_size (void);

  size_t
  get_sector_count (void);

  void
  cb_event (void);

  friend class qspi_winbond;
  friend class qspi_micron;

protected:
  qspi_result_t
  enter_quad_mode (void);

  // Standard command sub-set (common for all flash chips)
  static constexpr uint8_t JEDEC_ID = 0x9F;

  static constexpr uint8_t WRITE_ENABLE = 0x06;
  static constexpr uint8_t WRITE_DISABLE = 0x04;

  static constexpr uint8_t READ_STATUS_REGISTER = 0x05;
  static constexpr uint8_t WRITE_STATUS_REGISTER = 0x01;

  static constexpr uint8_t SECTOR_ERASE = 0x20;
  static constexpr uint8_t BLOCK_32K_ERASE = 0x52;
  static constexpr uint8_t BLOCK_64K_ERASE = 0xD8;
  static constexpr uint8_t CHIP_ERASE = 0xC7;

  static constexpr uint8_t RESET_ENABLE = 0x66;
  static constexpr uint8_t RESET_DEVICE = 0x99;

  static constexpr uint8_t POWER_DOWN = 0xB9;
  static constexpr uint8_t RELEASE_POWER_DOWN = 0xAB;

  static constexpr uint8_t PAGE_PROGRAM = 0x02;
  static constexpr uint8_t QUAD_PAGE_PROGRAM = 0x32;

  static constexpr uint8_t READ_DATA = 0x03;
  static constexpr uint8_t FAST_READ_DATA = 0x0B;
  static constexpr uint8_t FAST_READ_QUAD_OUT = 0x6B;
  static constexpr uint8_t FAST_READ_QUAD_IN_OUT = 0xEB;

  // Some timeouts; all timeouts are in # of uOS++ ticks (normally 1 ms)
  static constexpr uint32_t QSPI_TIMEOUT = 100;
  static constexpr uint32_t QSPI_ERASE_TIMEOUT = 2000;
  static constexpr uint32_t QSPI_CHIP_ERASE_TIMEOUT = 200000;

  QSPI_HandleTypeDef* hqspi_;
  os::rtos::semaphore_binary semaphore_
    { "qspi", 0 };
  os::rtos::mutex mutex_
    { "qspi" };

private:
  qspi_result_t
  page_write (uint32_t address, uint8_t* buff, size_t count);

  qspi_result_t
  read_JEDEC_ID (void);

  qspi_result_t
  erase (uint32_t address, uint8_t which);

  static constexpr uint8_t QSPI_VERSION_MAJOR = 0;
  static constexpr uint8_t QSPI_VERSION_MINOR = 9;

  class qspi_impl* pimpl = nullptr;
  uint8_t manufacturer_ID_ = 0;
  uint16_t memory_type_ = 0;
  const char* pmanufacturer_ = nullptr;
  const qspi_device_t* pdevice_ = nullptr;

};

class qspi_impl
{
public:
  qspi_impl (void)
  {
  }

  virtual
  ~qspi_impl () = default;

  virtual qspi::qspi_result_t
  enter_quad_mode (qspi* pq) = 0;

};

inline void
qspi::get_version (uint8_t& version_major, uint8_t& version_minor)
{
  version_major = QSPI_VERSION_MAJOR;
  version_minor = QSPI_VERSION_MINOR;
}

inline qspi::qspi_result_t
qspi::enter_quad_mode (void)
{
  return (pimpl == nullptr) ? error : pimpl->enter_quad_mode (this);
}

inline qspi::qspi_result_t
qspi::exit_mem_mapped (void)
{
  return ((qspi::qspi_result_t) (HAL_QSPI_Abort (hqspi_)));
}

inline qspi::qspi_result_t
qspi::erase_block32K (uint32_t address)
{
  return erase (address, BLOCK_32K_ERASE);
}

inline qspi::qspi_result_t
qspi::erase_block64K (uint32_t address)
{
  return erase (address, BLOCK_64K_ERASE);
}

inline qspi::qspi_result_t
qspi::erase_chip (void)
{
  return erase (0, CHIP_ERASE);
}

inline const char*
qspi::get_manufacturer (void)
{
  return pmanufacturer_;
}

#endif

#endif /* QSPI_FLASH_H_ */
