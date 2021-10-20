/*
 * qspi-descr.cpp
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
 * Created on: 1 Jan 2017 (LNP)
 */

#include "qspi-descr.h"
#include "qspi-micron.h"
#include "qspi-winbond.h"

namespace os
{
  namespace driver
  {
    namespace stm32f7
    {

      // Micron devices; accepted dummy cycles can be between 1 and 14
      const qspi_device_t micron_devices[] =
        {
          { 0xBA18, 4096, "MT25QL128ABA", 0, QSPI_ALTERNATE_BYTES_NONE,
          QSPI_ALTERNATE_BYTES_8_BITS, 8, 0, true },

          { 0xBB18, 4096, "MT25QL128ABA", 0, QSPI_ALTERNATE_BYTES_NONE,
          QSPI_ALTERNATE_BYTES_8_BITS, 8, 0, true },

          { } //
        };

      // Winbond devices; accepted dummy cycles can be either 2, 4, 6 or 8
      const qspi_device_t winbond_devices[] =
        {
          { 0x6016, 4096, "W25Q32FV", 0xF, QSPI_ALTERNATE_BYTES_4_LINES,
          QSPI_ALTERNATE_BYTES_8_BITS, 6, 2, false },

          { 0x6017, 4096, "W25Q64FV", 0xF, QSPI_ALTERNATE_BYTES_4_LINES,
          QSPI_ALTERNATE_BYTES_8_BITS, 6, 2, false },
            
          { 0x6018, 4096, "W25Q128FV", 0xF, QSPI_ALTERNATE_BYTES_4_LINES,
          QSPI_ALTERNATE_BYTES_8_BITS, 6, 2, false },

          { 0x7018, 4096, "W25Q128JV", 0xF, QSPI_ALTERNATE_BYTES_4_LINES,
          QSPI_ALTERNATE_BYTES_8_BITS, 6, 2, true },

          { } //
        };

      // Factories for the manufacturer's table
      static qspi_intern*
      new_micron (void)
      {
        return new qspi_micron
          { };
      }

      static qspi_intern*
      new_winbond (void)
      {
        return new qspi_winbond
          { };
      }

      // Supported manufactures
      const qspi_manuf_t qspi_manufacturers[] =
        {
          { MANUF_ID_MICRON, "Micron/ST", micron_devices, new_micron },
          { MANUF_ID_WINBOND, "Winbond", winbond_devices, new_winbond },
          { } //
        };

    } /* namespace stm32f7 */
  } /* namespace driver */
} /* namespace os */
