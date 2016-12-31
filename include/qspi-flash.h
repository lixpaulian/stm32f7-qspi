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
 * Version: 0.3, 31 Dec 2016
 */

#ifndef QSPI_FLASH_H_
#define QSPI_FLASH_H_

#include <cmsis-plus/rtos/os.h>
#include "cmsis_device.h"

#if defined (__cplusplus)

// All timeouts are in # of uOS++ ticks (normally 1 ms)
#define QSPI_TIMEOUT 100
#define QSPI_ERASE_TIMEOUT 2000
#define QSPI_CHIP_ERASE_TIMEOUT 200000

class qspi_impl;

class qspi
{
public:
  qspi (QSPI_HandleTypeDef* hqspi);

  ~qspi () = default;

  bool
  read_JEDEC_ID (void);

  bool
  get_ID_data (uint8_t& manufacturer_ID, uint8_t& memory_type,
	       uint8_t& memory_capacity);

  bool
  mode_quad ();

  bool
  memory_mapped ();

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

  friend class qspi_winbond;
  friend class qspi_micron;

protected:
  bool
  page_write (uint32_t address, uint8_t* buff, size_t count);

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

  static constexpr uint8_t PAGE_PROGRAM = 0x02;
  static constexpr uint8_t QUAD_PAGE_PROGRAM = 0x32;

  static constexpr uint8_t READ_DATA = 0x03;
  static constexpr uint8_t FAST_READ_DATA = 0x0B;
  static constexpr uint8_t FAST_READ_QUAD_OUT = 0x6B;
  static constexpr uint8_t FAST_READ_QUAD_IN_OUT = 0xEB;

  QSPI_HandleTypeDef* hqspi_;
  os::rtos::semaphore_binary semaphore_
    { "qspi", 0 };
  os::rtos::mutex mutex_
    { "qspi" };

private:
  bool
  erase (uint32_t address, uint8_t which);

  class qspi_impl* pimpl = nullptr;

  uint8_t manufacturer_ID_ = 0;
  uint8_t memory_type_ = 0;
  uint8_t memory_capacity_ = 0;
  bool valid_mem_ID = false;

};

class qspi_impl
{
public:
  qspi_impl ()
  {
  }

  virtual
  ~qspi_impl () = default;

  virtual bool
  mode_quad (qspi* pq) = 0;

  virtual bool
  memory_mapped (qspi* pq) = 0;

  virtual bool
  read (qspi* pq, uint32_t address, uint8_t* buff, size_t count) = 0;

};

inline bool
qspi::mode_quad ()
{
  return (pimpl == nullptr) ? false : pimpl->mode_quad (this);
}

inline bool
qspi::memory_mapped ()
{
  return (pimpl == nullptr) ? false : pimpl->memory_mapped (this);
}

inline bool
qspi::read (uint32_t address, uint8_t* buff, size_t count)
{
  return (pimpl == nullptr) ? false : pimpl->read (this, address, buff, count);
}

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
