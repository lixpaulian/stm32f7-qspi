/*
 * qspi-micron.h
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

#ifndef QSPI_MICRON_H_
#define QSPI_MICRON_H_

#include "qspi-flash.h"

#if defined (__cplusplus)

namespace os
{
  namespace driver
  {
    namespace stm32f7
    {

      class qspi_micron : public qspi_intern
      {

      public:
        virtual qspi_impl::qspi_result_t
        enter_quad_mode (qspi_impl* pq) override;

      private:
        // Micron/ST specific commands
        static constexpr uint8_t READ_VOLATILE_STATUS_REGISTER = 0x85;
        static constexpr uint8_t READ_ENH_VOLATILE_STATUS_REGISTER = 0x65;
        static constexpr uint8_t WRITE_VOLATILE_STATUS_REGISTER = 0x81;
        static constexpr uint8_t WRITE_ENH_VOLATILE_STATUS_REGISTER = 0x61;
        static constexpr uint8_t ENTER_QUAD_MODE = 0x38;

      };

    } /* namespace stm32f7 */
  } /* namespace driver */
} /* namespace os */

#endif

#endif /* QSPI_MICRON_H_ */
