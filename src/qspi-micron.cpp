/*
 * qspi-micron.cpp
 *
 * Copyright (c) 2016-2018 Lix N. Paulian (lix@paulian.net)
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

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>

#include "qspi-micron.h"
#include "qspi-descr.h"

using namespace os;

/**
 * @brief  Switch the flash chip to quad mode.
 * @return true if successful, false otherwise.
 */
qspi::qspi_result_t
qspi_micron::enter_quad_mode (qspi* pq)
{
  QSPI_CommandTypeDef sCommand;
  qspi::qspi_result_t result = qspi::busy;
  uint8_t datareg;

  if (pq->mutex_.timed_lock (qspi::TIMEOUT) == rtos::result::ok)
    {
      // Initial command settings
      sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.DataMode = QSPI_DATA_NONE;
      sCommand.DummyCycles = 0;
      sCommand.NbData = 1;

      // Enable volatile write
      sCommand.Instruction = qspi::WRITE_ENABLE;
      result = (qspi::qspi_result_t) HAL_QSPI_Command (pq->hqspi_, &sCommand,
                                                       qspi::TIMEOUT);
      if (result == qspi::ok)
        {
          // Write enhanced volatile register
          sCommand.DataMode = QSPI_DATA_1_LINE;
          sCommand.Instruction = WRITE_VOLATILE_STATUS_REGISTER;
          result = (qspi::qspi_result_t) HAL_QSPI_Command (pq->hqspi_,
                                                           &sCommand,
                                                           qspi::TIMEOUT);
          if (result == qspi::ok)
            {
              // Compute dummy cycles
              datareg = (pq->pdevice_->dummy_cycles << 4);
              datareg |= 0xB;
              result = (qspi::qspi_result_t) HAL_QSPI_Transmit (
                  pq->hqspi_, &datareg, qspi::TIMEOUT);
              if (result == qspi::ok)
                {
                  // Enable write
                  sCommand.DataMode = QSPI_DATA_NONE;
                  sCommand.Instruction = qspi::WRITE_ENABLE;
                  result = (qspi::qspi_result_t) HAL_QSPI_Command (
                      pq->hqspi_, &sCommand, qspi::TIMEOUT);
                  if (result == qspi::ok)
                    {
                      // Set number of dummy cycles
                      sCommand.DataMode = QSPI_DATA_1_LINE;
                      sCommand.Instruction = WRITE_ENH_VOLATILE_STATUS_REGISTER;
                      result = (qspi::qspi_result_t) HAL_QSPI_Command (
                          pq->hqspi_, &sCommand, qspi::TIMEOUT);
                      if (result == qspi::ok)
                        {
                          datareg = 0x6F;	// Enable quad protocol
                          result = (qspi::qspi_result_t) HAL_QSPI_Transmit (
                              pq->hqspi_, &datareg, qspi::TIMEOUT);
                          if (result == qspi::ok)
                            {
                              sCommand.DataMode = QSPI_DATA_NONE;
                              sCommand.Instruction = ENTER_QUAD_MODE;
                              result = (qspi::qspi_result_t) HAL_QSPI_Command (
                                  pq->hqspi_, &sCommand, qspi::TIMEOUT);
                            }
                        }
                    }
                }
            }
        }
      pq->mutex_.unlock ();
    }
  return result;
}
