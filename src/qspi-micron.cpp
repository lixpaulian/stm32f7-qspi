/*
 * qspi-micron.cpp
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
 * Created on: 31 Dec 2016 (LNP)
 */

/*
 * This file implements the specific basic low level functions to control
 * Micron QSPI flash devices (acquired from ST).
 */

#include "qspi-micron.h"

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>

using namespace os;

/**
 * @brief  Switch the flash chip to quad mode.
 * @return true if successful, false otherwise.
 */
bool
qspi_micron::enter_quad_mode (qspi* pq)
{
  QSPI_CommandTypeDef sCommand;
  bool result = false;
  uint8_t datareg;

  if (pq->mutex_.timed_lock (qspi::QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Initial command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_1_LINE;
      sCommand.DummyCycles = 0;
      sCommand.NbData = 1;

      // Read enhanced volatile register
      sCommand.Instruction = READ_ENH_VOLATILE_STATUS_REGISTER;
      if (HAL_QSPI_Command (pq->hqspi_, &sCommand, qspi::QSPI_TIMEOUT)
	  == HAL_OK)
	{
	  if (HAL_QSPI_Receive (pq->hqspi_, &datareg, qspi::QSPI_TIMEOUT)
	      == HAL_OK)
	    {
	      if ((datareg & 0x80) == 0)
		{
		  // Quad protocol bit not set
		  // Enable write
		  sCommand.DataMode = QSPI_DATA_NONE;
		  sCommand.Instruction = qspi::WRITE_ENABLE;
		  if (HAL_QSPI_Command (pq->hqspi_, &sCommand,
					qspi::QSPI_TIMEOUT) == HAL_OK)
		    {
		      // Write back enhanced volatile register (enable Quad)
		      sCommand.DataMode = QSPI_DATA_1_LINE;
		      sCommand.Instruction = WRITE_ENH_VOLATILE_STATUS_REGISTER;
		      if (HAL_QSPI_Command (pq->hqspi_, &sCommand,
					    qspi::QSPI_TIMEOUT) == HAL_OK)
			{
			  datareg |= 0x80;
			  if (HAL_QSPI_Transmit (pq->hqspi_, &datareg,
						 qspi::QSPI_TIMEOUT) == HAL_OK)
			    {
			      result = true;
			    }
			}
		    }
		}
	      else
		{
		  result = true;
		}
	    }
	}
      pq->mutex_.unlock ();
    }
  return result;
}

/**
 * @brief  Map the flash to the addressing space of the controller, starting at
 * 	address 0x90000000.
 * @return true if successful, false otherwise.
 */
bool
qspi_micron::enter_mem_mapped (qspi* pq)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;
  QSPI_MemoryMappedTypeDef sMemMappedCfg;

  if (pq->mutex_.timed_lock (qspi::QSPI_TIMEOUT) == rtos::result::ok)
    {
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
      sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
      sCommand.DataMode = QSPI_DATA_4_LINES;
      sCommand.DummyCycles = 6; //10;
      sCommand.Instruction = qspi::FAST_READ_QUAD_OUT;

      sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

      if (HAL_QSPI_MemoryMapped (pq->hqspi_, &sCommand, &sMemMappedCfg)
	  == HAL_OK)
	{
	  result = true;
	}
      pq->mutex_.unlock ();
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
qspi_micron::read (qspi* pq, uint32_t address, uint8_t* buff, size_t count)
{
  bool result = false;
  QSPI_CommandTypeDef sCommand;

  if (pq->mutex_.timed_lock (qspi::QSPI_TIMEOUT) == rtos::result::ok)
    {
      // Read command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
      sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
      sCommand.DataMode = QSPI_DATA_4_LINES;
      sCommand.DummyCycles = 6; //10;
      sCommand.Address = address;
      sCommand.NbData = count;
      sCommand.Instruction = qspi::FAST_READ_QUAD_OUT;

      // Initiate read and wait for the event
      if (HAL_QSPI_Command (pq->hqspi_, &sCommand, qspi::QSPI_TIMEOUT)
	  == HAL_OK)
	{
	  if (HAL_QSPI_Receive_IT (pq->hqspi_, buff) == HAL_OK)
	    {
	      if (pq->semaphore_.timed_wait (qspi::QSPI_TIMEOUT)
		  == rtos::result::ok)
		{
		  result = true;
		}
	    }
	}
      if (result == false)
	{
	  HAL_QSPI_Abort (pq->hqspi_);
	}
      pq->mutex_.unlock ();
    }
  return result;
}

