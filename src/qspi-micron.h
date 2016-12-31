/*
 * qspi-micron.h
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
 * Created on: 31 Dec 2016 (LNP)
 *
 * Version: 0.3, 31 Dec 2016
 */

#ifndef QSPI_MICRON_H_
#define QSPI_MICRON_H_

#include "qspi-flash.h"

#if defined (__cplusplus)

class qspi_micron : public qspi_impl
{

public:

  virtual bool
  mode_quad (qspi* pq) override;

  virtual bool
  memory_mapped (qspi* pq) override;

  virtual bool
  read (qspi* pq, uint32_t address, uint8_t* buff, size_t count) override;

private:
  // Micron/ST specific commands
  static constexpr uint8_t READ_VOLATILE_STATUS_REGISTER = 0x85;
  static constexpr uint8_t READ_ENH_VOLATILE_STATUS_REGISTER = 0x65;
  static constexpr uint8_t WRITE_VOLATILE_STATUS_REGISTER = 0x81;
  static constexpr uint8_t WRITE_ENH_VOLATILE_STATUS_REGISTER = 0x61;

};

#endif

#endif /* QSPI_MICRON_H_ */
