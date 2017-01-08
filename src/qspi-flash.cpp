/*
 * qspi-flash.cpp
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

/*
 * This file implements the common basic low level functions to control
 * a QSPI flash device.
 */

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>
#include "qspi-flash.h"
#include "qspi-descr.h"
#include "qspi-winbond.h"
#include "qspi-micron.h"

using namespace os;

/**
 * @brief Constructor.
 * @param hqspi: HAL qspi handle.
 */
qspi::qspi (QSPI_HandleTypeDef* hqspi)
{
  trace::printf ("%s(%p) @%p\n", __func__, hqspi, this);
  hqspi_ = hqspi;
}

/**
 * @brief  Read the flash chip ID and initialize the internal structures accordingly.
 * @return true if successful, false if the flash could not be identified or it is
 * 	not supported by the driver.
 */
bool
qspi::initialize (void)
{
  bool result = false;

  if (qspi::read_JEDEC_ID ())
    {
      for (const qspi_manuf_t* pqm = qspi_manufacturers;
	  pqm->manufacturer_ID != 0; pqm++)
	{
	  if (pqm->manufacturer_ID == manufacturer_ID_)
	    {
	      for (const qspi_device_t* pqd = pqm->devices; pqd->device_ID != 0;
		  pqd++)
		{
		  if (pqd->device_ID == memory_type_)
		    {
		      pmanufacturer_ = pqm->manufacturer_name;
		      pdevice_ = pqd;
		      switch (manufacturer_ID_)
			{
			case MANUF_ID_MICRON:
			  pimpl = new qspi_micron
			    { };
			  break;

			case MANUF_ID_WINBOND:
			  pimpl = new qspi_winbond
			    { };
			  break;
			}
		      result = true;
		    }
		}
	    }
	}
    }
  return result;
}

/**
 * @brief  Read the memory parameters (manufacturer and type).
 * @return true if successful, false otherwise.
 */
bool
qspi::read_JEDEC_ID (void)
{
  bool result = false;
  uint8_t buff[3];
  QSPI_CommandTypeDef sCommand;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Read command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_1_LINE;
      sCommand.DummyCycles = 0;
      sCommand.NbData = 3;
      sCommand.Instruction = JEDEC_ID;

      // Initiate read and wait for the event
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  if (HAL_QSPI_Receive_IT (hqspi_, buff) == HAL_OK)
	    {
	      if (semaphore_.timed_wait (QSPI_TIMEOUT) == rtos::result::ok)
		{
		  manufacturer_ID_ = buff[0];
		  memory_type_ = buff[1] << 8;
		  memory_type_ += buff[2];
		  result = true;
		}
	    }
	}
      if (result == false)
	{
	  HAL_QSPI_Abort (hqspi_);
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Map the flash to the addressing space of the controller, starting at
 * 	address 0x90000000.
 * @return true if successful, false otherwise.
 */
bool
qspi::enter_mem_mapped (void)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;
  QSPI_MemoryMappedTypeDef sMemMappedCfg;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
      sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
      sCommand.AlternateBytes = 0;	// Continuous read mode off
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
      sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
      sCommand.DataMode = QSPI_DATA_4_LINES;
      sCommand.DummyCycles = pdevice_->dummy_cycles;
      sCommand.Instruction = FAST_READ_QUAD_IN_OUT;

      sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

      if (HAL_QSPI_MemoryMapped (hqspi_, &sCommand, &sMemMappedCfg) == HAL_OK)
	{
	  result = true;
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Read a block of data from the flash.
 * @param  address: start address in flash where to read from.
 * @param  buff: buffer where to copy data to.
 * @param  count: amount of data to be retrieved from flash.
 * @return true if successful, false otherwise.
 */
bool
qspi::read (uint32_t address, uint8_t* buff, size_t count)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Read command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
      sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
      sCommand.AlternateBytes = 0;	// Continuous read mode off
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
      sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
      sCommand.DataMode = QSPI_DATA_4_LINES;
      sCommand.DummyCycles = pdevice_->dummy_cycles;
      sCommand.Address = address;
      sCommand.NbData = count;
      sCommand.Instruction = FAST_READ_QUAD_IN_OUT;

      // Initiate read and wait for the event
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  if (HAL_QSPI_Receive_IT (hqspi_, buff) == HAL_OK)
	    {
	      if (semaphore_.timed_wait (QSPI_TIMEOUT) == rtos::result::ok)
		{
		  result = true;
		}
	    }
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Write data to flash.
 * @param  address: start address in flash where to write data to.
 * @param  buff: source data to be written.
 * @param  count: amount of data to be written.
 * @return true if successful, false otherwise.
 */
bool
qspi::write (uint32_t address, uint8_t* buff, size_t count)
{
  bool result = true;
  size_t in_block_count;

  do
    {
      in_block_count = 0x100 - (address & 0xFF);
      if (in_block_count > count)
	in_block_count = count;
      else if (in_block_count == 0)
	{
	  in_block_count = (count > 0x100) ? 0x100 : count;
	}
      if (page_write (address, buff, in_block_count) == false)
	{
	  result = false;
	  break;
	}
      address += in_block_count;
      buff += in_block_count;
      count -= in_block_count;
    }
  while (count);

  return result;
}

/**
 * @brief  Write a page of data to the flash (max. 255 bytes).
 * @param  address: address of the page in flash.
 * @param  buff: buffer of the source data.
 * @param  count: number of bytes to be written (max 255).
 * @return true if successful, false otherwise.
 */
bool
qspi::page_write (uint32_t address, uint8_t* buff, size_t count)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Initial command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_NONE;
      sCommand.DummyCycles = 0;

      // Enable write
      sCommand.Instruction = WRITE_ENABLE;
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  // Initiate write
	  sCommand.Instruction = PAGE_PROGRAM;
	  sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
	  sCommand.DataMode = QSPI_DATA_4_LINES;
	  sCommand.Address = address;
	  sCommand.NbData = count;
	  if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	    {
	      if (HAL_QSPI_Transmit (hqspi_, buff, QSPI_TIMEOUT) == HAL_OK)
		{
		  // Set auto-polling and wait for the event
		  sCommand.AddressMode = QSPI_ADDRESS_NONE;
		  sCommand.DataMode = QSPI_DATA_4_LINES;
		  sCommand.Instruction = READ_STATUS_REGISTER;
		  sConfig.Match = 0;
		  sConfig.Mask = 1;
		  sConfig.MatchMode = QSPI_MATCH_MODE_AND;
		  sConfig.StatusBytesSize = 1;
		  sConfig.Interval = 0x10;
		  sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
		  if (HAL_QSPI_AutoPolling_IT (hqspi_, &sCommand, &sConfig)
		      == HAL_OK)
		    {
		      if (semaphore_.timed_wait (QSPI_TIMEOUT)
			  == rtos::result::ok)
			{
			  result = true;
			}
		    }
		}
	    }
	}
      if (result == false)
	{
	  HAL_QSPI_Abort (hqspi_);
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Erase a sector (4K), block (32K), large block (64K) or whole flash.
 * @param  address: address of the block to be erased.
 * @param  which: command to erase, can be either SECTOR_ERASE, BLOCK_32K_ERASE,
 * 	BLOCK_64K_ERASE or CHIP_ERASE.
 * @return true if successful, false otherwise.
 */
bool
qspi::erase (uint32_t address, uint8_t which)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Initial command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_NONE;
      sCommand.DummyCycles = 0;

      // Enable write
      sCommand.Instruction = WRITE_ENABLE;
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  // Initiate erase
	  sCommand.Instruction = which;
	  sCommand.AddressMode = (which == CHIP_ERASE) ? QSPI_ADDRESS_NONE : //
	      QSPI_ADDRESS_4_LINES;
	  sCommand.DataMode = QSPI_DATA_NONE;
	  sCommand.Address = address;
	  if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	    {
	      // Set auto-polling and wait for the event
	      sCommand.Instruction = READ_STATUS_REGISTER;
	      sCommand.AddressMode = QSPI_ADDRESS_NONE;
	      sCommand.DataMode = QSPI_DATA_4_LINES;
	      sConfig.Match = 0;
	      sConfig.Mask = 1;
	      sConfig.MatchMode = QSPI_MATCH_MODE_AND;
	      sConfig.StatusBytesSize = 1;
	      sConfig.Interval = 0x10;
	      sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
	      if (HAL_QSPI_AutoPolling_IT (hqspi_, &sCommand, &sConfig)
		  == HAL_OK)
		{
		  if (semaphore_.timed_wait (
		      (which == CHIP_ERASE) ? QSPI_CHIP_ERASE_TIMEOUT : //
			  QSPI_ERASE_TIMEOUT) == rtos::result::ok)
		    {
		      result = true;
		    }
		}
	    }
	}
      if (result == false)
	{
	  HAL_QSPI_Abort (hqspi_);
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Read sector.
 * @param  sector: sector number to read from.
 * @param  buff: buffer to receive the data from flash.
 * @param  count: number of bytes to read (buffer should be large enough).
 * @return true if successful, false otherwise.
 */
bool
qspi::read_sector (uint32_t sector, uint8_t* buff, size_t count)
{
  return read (sector * pdevice_->sector_size, buff, count);
}

/**
 * @brief  Write sector.
 * @param  sector: sector number to write to.
 * @param  buff: buffer containing the data to be written to flash.
 * @param  count: number of bytes to be written.
 * @return true if successful, false otherwise.
 */
bool
qspi::write_sector (uint32_t sector, uint8_t* buff, size_t count)
{
  return write (sector * pdevice_->sector_size, buff, count);
}

/**
 * @brief  Erase sector.
 * @param  sector: sector to be erased.
 * @return true if successful, false otherwise.
 */
bool
qspi::erase_sector (uint32_t sector)
{
  return erase (sector * pdevice_->sector_size, SECTOR_ERASE);
}

/**
 * @brief  Software reset the flash chip.
 * @return true if successful, false otherwise.
 */
bool
qspi::reset_chip (void)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;

  if (mutex_.timed_lock (QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Initial command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_NONE;
      sCommand.DummyCycles = 0;

      // Enable reset
      sCommand.Instruction = RESET_ENABLE;
      if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	{
	  // Send reset command
	  sCommand.Instruction = RESET_DEVICE;
	  if (HAL_QSPI_Command (hqspi_, &sCommand, QSPI_TIMEOUT) == HAL_OK)
	    {
	      result = true;
	    }
	}
      mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Return the memory type.
 * @return Pointer to a string representing the human readable memory type.
 */
const char*
qspi::get_memory_type (void)
{
  return pdevice_->device_name;
}

/**
 * @brief  Return the sector size.
 * @return The sector size in bytes.
 */
size_t
qspi::get_sector_size (void)
{
  return pdevice_->sector_size;
}

/**
 * @brief  Return the sectors count.
 * @return The sectors count.
 */
size_t
qspi::get_sector_count (void)
{
  size_t size = pdevice_->device_ID & 0xFF;
  size = (1 << size);
  return size / pdevice_->sector_size;
}

/**
 * @brief  QSPI peripheral interrupt call-back.
 */
void
qspi::cb_event (void)
{
  semaphore_.post ();
}

