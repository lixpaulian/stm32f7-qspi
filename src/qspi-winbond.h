/*
 * qspi-winbond.h
 *
 * Copyright (c) 2016-2020 Lix N. Paulian (lix@paulian.net)
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

#ifndef QSPI_WINBOND_H_
#define QSPI_WINBOND_H_

#include "qspi-flash.h"

#if defined (__cplusplus)

namespace os
{
  namespace driver
  {
    namespace stm32f7
    {

      class qspi_winbond : public qspi_intern
      {

      public:
        virtual qspi_impl::qspi_result_t
        enter_quad_mode (qspi_impl* pq) override;

      private:
        // Winbond specific commands
        static constexpr uint8_t VOLATILE_SR_WRITE_ENABLE = 0x50;
        static constexpr uint8_t READ_STATUS_REGISTER_2 = 0x35;
        static constexpr uint8_t WRITE_STATUS_REGISTER_2 = 0x31;
        static constexpr uint8_t READ_STATUS_REGISTER_3 = 0x15;
        static constexpr uint8_t WRITE_STATUS_REGISTER_3 = 0x11;
        static constexpr uint8_t ENTER_QUAD_MODE = 0x38;
        static constexpr uint8_t SET_READ_PARAMETERS = 0xC0;

      };

    } /* namespace stm32f7 */
  } /* namespace driver */
} /* namespace os */

#endif

#endif /* QSPI_WINBOND_H_ */
