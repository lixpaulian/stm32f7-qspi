/*
 * qspi-flash-c-wrapper.cpp
 *
 * Copyright (c) 2017, 2018, 2020 Lix N. Paulian (lix@paulian.net)
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

#include "qspi-flash.h"
#include "qspi-flash-c-api.h"

using namespace os::driver::stm32f7;

// Explicit template instantiation
template class os::posix::block_device_implementable<
    os::driver::stm32f7::qspi_impl>;
using qspi_c = os::posix::block_device_implementable<os::driver::stm32f7::qspi_impl>;

/**
 * @brief  Allocate a qspi_flash object instance and construct it.
 * @param  hqspi: qspi handle.
 * @return qspi_ok if the class could be created, qspi_error otherwise.
 */
qspi_t *
qspi_new (QSPI_HandleTypeDef* hqspi)
{
  qspi_t* pqspi = reinterpret_cast<qspi_t*> (new qspi_c
    { "flash", hqspi });
  return pqspi;
}

/**
 * @brief  Destruct the qspi_flash object instance and deallocate it.
 * @param  qspi_instance: pointer to the qspi object.
 */
void
qspi_delete (qspi_t* qspi_instance)
{
  delete reinterpret_cast<class qspi_impl*> (qspi_instance);
}

/**
 * @brief  Return the driver's version.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  version_major: pointer where the major version number will be returned to.
 * @param  version_minor: pointer where the minor version number will be returned to.
 * @param  version_patch: pointer where the patch number will be returned to.
 */
void
qspi_get_version (qspi_t* qspi_instance, uint8_t* version_major,
                  uint8_t* version_minor, uint8_t* version_patch)
{
  ((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).get_version (
      *version_major, *version_minor, *version_patch);
}

/**
 * @brief  Switch the flash chip into or out of deep sleep.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  state: if true, enter deep sleep; if false, exit deep sleep.
 * @return qspi_ok if successful, an error otherwise.
 */
qspi_result_t
qspi_sleep (qspi_t* qspi_instance, bool state)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).sleep (
      state));
}

/**
 * @brief  Read the flash chip ID and initialize the internal structures accordingly.
 * @param  qspi_instance: pointer to the qspi object.
 * @return qspi_ok if successful, or a qspi error if the flash could not be identified
 *       or it is not supported by the driver.
 */
qspi_result_t
qspi_initialize (qspi_t* qspi_instance)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).initialize ());
}

/**
 * @brief  Set flash device to default state.
 * @param  qspi_instance: pointer to the qspi object.
 * @return if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_uninitialize (qspi_t* qspi_instance)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).uninitialize ());
}

/**
 * @brief  Map the flash to the addressing space of the controller, starting at
 *      address 0x90000000.
 * @param  qspi_instance: pointer to the qspi object.
 * @return qspi_ok if successful, false otherwise.
 */
qspi_result_t
qspi_enter_mem_mapped (qspi_t* qspi_instance)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).enter_mem_mapped ());
}

/**
 * @brief  Exit memory mapped mode.
 * @param  qspi_instance: pointer to the qspi object.
 * @return qspi_ok if successful, false otherwise.
 */
qspi_result_t
qspi_exit_mem_mapped (qspi_t* qspi_instance)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).exit_mem_mapped ());
}

/**
 * @brief  Read a block of data from the flash.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  address: start address in flash where to read from.
 * @param  buff: buffer where to copy data to.
 * @param  count: amount of data to be retrieved from flash.
 * @return qspi_ok if successful, a qspi error otherwise.
 */
qspi_result_t
qspi_read (qspi_t* qspi_instance, uint32_t address, uint8_t* buff, size_t count)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).read (
      address, buff, count));
}

/**
 * @brief  Write data to flash.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  address: start address in flash where to write data to.
 * @param  buff: source data to be written.
 * @param  count: amount of data to be written.
 * @return qspi_ok if successful, a qspi error otherwise.
 */
qspi_result_t
qspi_write (qspi_t* qspi_instance, uint32_t address, uint8_t* buff,
            size_t count)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).write (
      address, buff, count));
}

/**
 * @brief  Read sector.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  sector: sector number to read from.
 * @param  buff: buffer to receive the data from flash.
 * @param  count: number of bytes to read (buffer should be large enough).
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_read_sector (qspi_t* qspi_instance, uint32_t sector, uint8_t* buff,
                  size_t count)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).read_sector (
      sector, buff, count));
}

/**
 * @brief  Write sector.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  sector: sector number to write to.
 * @param  buff: buffer containing the data to be written to flash.
 * @param  count: number of bytes to be written.
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_write_sector (qspi_t* qspi_instance, uint32_t sector, uint8_t* buff,
                   size_t count)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).write_sector (
      sector, buff, count));
}

/**
 * @brief  Erase sector.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  sector: sector to be erased.
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_erase_sector (qspi_t* qspi_instance, uint32_t sector)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).erase_sector (
      sector));
}

/**
 * @brief  Erase 32K block.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  address: address in the block to be erased.
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_erase_block32K (qspi_t* qspi_instance, uint32_t address)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).erase_block32K (
      address));
}

/**
 * @brief  Erase 64K block.
 * @param  qspi_instance: pointer to the qspi object.
 * @param  address: address in the block to be erased.
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_erase_block64K (qspi_t* qspi_instance, uint32_t address)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).erase_block64K (
      address));
}

/**
 * @brief  Erase chip.
 * @param  qspi_instance: pointer to the qspi object.
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_erase_chip (qspi_t* qspi_instance)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).erase_chip ());
}

/**
 * @brief  Software reset the flash chip.
 * @param  qspi_instance: pointer to the qspi object.
 * @return qspi_ok if successful, or a qspi error otherwise.
 */
qspi_result_t
qspi_reset_chip (qspi_t* qspi_instance)
{
  return (qspi_result_t) (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).reset_chip ());
}

/**
 * @brief  Return the manufacturer.
 * @param  qspi_instance: pointer to the qspi object.
 * @return Pointer to a string representing the human readable manufacturer, or
 *      null pointer if the system is not initialized.
 */
const char*
qspi_get_manufacturer (qspi_t* qspi_instance)
{
  return (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).get_manufacturer ());
}

/**
 * @brief  Return the memory type.
 * @param  qspi_instance: pointer to the qspi object.
 * @return Pointer to a string representing the human readable memory type, or
 *      null pointer if the system is not initialized.
 */
const char*
qspi_get_memory_type (qspi_t* qspi_instance)
{
  return (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).get_memory_type ());
}

/**
 * @brief  Return the sector size.
 * @param  qspi_instance: pointer to the qspi object.
 * @return The sector size in bytes, zero if system is not initialized.
 */
size_t
qspi_get_sector_size (qspi_t* qspi_instance)
{
  return (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).get_sector_size ());
}

/**
 * @brief  Return the sectors count.
 * @param  qspi_instance: pointer to the qspi object.
 * @return The sectors count, zero if system is not initialized.
 */
size_t
qspi_get_sector_count (qspi_t* qspi_instance)
{
  return (((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).get_sector_count ());
}

/**
 * @brief  Events call-back handler
 * @param  qspi_instance: pointer to the qspi object.
 */
void
qspi_event_cb (qspi_t* qspi_instance)
{
  ((reinterpret_cast<qspi_c*> (qspi_instance))->impl ()).cb_event ();
}

