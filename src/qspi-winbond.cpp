/*
 * qspi-winbond.cpp
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
 * Created on: 29 Dec 2016 (LNP)
 */

/*
 * This file implements the specific basic low level functions to control
 * Winbond QSPI flash devices.
 */

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>

#include "qspi-winbond.h"
#include "qspi-descr.h"

namespace os
{
  namespace driver
  {
    namespace stm32f7
    {

      /**
       * @brief  Switch the flash chip to quad mode.
       * @return true if successful, false otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_winbond::enter_quad_mode (qspi_impl* pq)
      {
        QSPI_CommandTypeDef sCommand;
        qspi_impl::qspi_result_t result = qspi_impl::busy;
        uint8_t datareg;

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
        sCommand.Instruction = VOLATILE_SR_WRITE_ENABLE;
        result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (
            pq->hqspi_, &sCommand, qspi_impl::TIMEOUT);
        if (result == qspi_impl::ok)
          {
            // Write status register 2 (enable Quad Mode)
            sCommand.DataMode = QSPI_DATA_1_LINE;
            sCommand.Instruction = WRITE_STATUS_REGISTER_2;
            result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (
                pq->hqspi_, &sCommand, qspi_impl::TIMEOUT);
            if (result == qspi_impl::ok)
              {
                datareg = 2;
                result = (qspi_impl::qspi_result_t) HAL_QSPI_Transmit (
                    pq->hqspi_, &datareg, qspi_impl::TIMEOUT);
                if (result == qspi_impl::ok)
                  {
                    sCommand.DataMode = QSPI_DATA_NONE;
                    sCommand.Instruction = ENTER_QUAD_MODE;
                    result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (
                        pq->hqspi_, &sCommand, qspi_impl::TIMEOUT);
                    if (result == qspi_impl::ok)
                      {
                        sCommand.DataMode = QSPI_DATA_4_LINES;
                        sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
                        sCommand.Instruction = SET_READ_PARAMETERS;
                        result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (
                            pq->hqspi_, &sCommand, qspi_impl::TIMEOUT);
                        if (result == qspi_impl::ok)
                          {
                            // Compute and set number of dummy cycles
                            datareg = (pq->pdevice_->dummy_cycles / 2) - 1;
                            datareg <<= 4;
                            result =
                                (qspi_impl::qspi_result_t) HAL_QSPI_Transmit (
                                    pq->hqspi_, &datareg, qspi_impl::TIMEOUT);
                          }
                      }
                  }
              }
          }
        return result;
      }

    } /* namespace stm32f7 */
  } /* namespace driver */
} /* namespace os */
