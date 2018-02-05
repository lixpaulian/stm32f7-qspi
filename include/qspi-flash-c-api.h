/*
 * qspi-flash-c-api.h
 *
 * Copyright (c) 2017, 2018 Lix N. Paulian (lix@paulian.net)
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
 * Created on: 5 Feb 2017 (LNP)
 */

#ifndef INCLUDE_QSPI_FLASH_C_API_H_
#define INCLUDE_QSPI_FLASH_C_API_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "qspi-flash.h"

#ifdef  __cplusplus
extern "C"
{
#endif

  typedef enum
  {
    qspi_ok = 0,
    qspi_error = HAL_ERROR,
    qspi_busy = HAL_BUSY,
    qspi_timeout = HAL_TIMEOUT,
    qspi_type_not_found,
  } qspi_result_t;

  typedef struct
  {
  } qspi_t;

  qspi_t *
  qspi_new (QSPI_HandleTypeDef* hqspi);

  void
  qspi_delete (qspi_t* qspi_instance);

  void
  qspi_get_version (qspi_t* qspi_instance, uint8_t* version_major,
               uint8_t* version_minor);

  void
  qspi_power (qspi_t* qspi_instance, bool state);

  qspi_result_t
  qspi_sleep (qspi_t* qspi_instance, bool state);

  qspi_result_t
  qspi_initialize (qspi_t* qspi_instance);

  qspi_result_t
  qspi_uninitialize (qspi_t* qspi_instance);

  qspi_result_t
  qspi_enter_mem_mapped (qspi_t* qspi_instance);

  qspi_result_t
  qspi_exit_mem_mapped (qspi_t* qspi_instance);

  qspi_result_t
  qspi_read (qspi_t* qspi_instance, uint32_t address, uint8_t* buff,
             size_t count);

  qspi_result_t
  qspi_write (qspi_t* qspi_instance, uint32_t address, uint8_t* buff,
              size_t count);

  qspi_result_t
  qspi_read_sector (qspi_t* qspi_instance, uint32_t sector, uint8_t* buff,
                    size_t count);

  qspi_result_t
  qspi_write_sector (qspi_t* qspi_instance, uint32_t sector, uint8_t* buff,
                     size_t count);

  qspi_result_t
  qspi_erase_sector (qspi_t* qspi_instance, uint32_t sector);

  qspi_result_t
  qspi_erase_block32K (qspi_t* qspi_instance, uint32_t address);

  qspi_result_t
  qspi_erase_block64K (qspi_t* qspi_instance, uint32_t address);

  qspi_result_t
  qspi_erase_chip (qspi_t* qspi_instance);

  qspi_result_t
  qspi_reset_chip (qspi_t* qspi_instance);

  const char*
  qspi_get_manufacturer (qspi_t* qspi_instance);

  const char*
  qspi_get_memory_type (qspi_t* qspi_instance);

  size_t
  qspi_get_sector_size (qspi_t* qspi_instance);

  size_t
  qspi_get_sector_count (qspi_t* qspi_instance);

  void
  qspi_event_cb (qspi_t* qspi_instance);

#ifdef  __cplusplus
}
#endif

#endif /* INCLUDE_QSPI_FLASH_C_API_H_ */
