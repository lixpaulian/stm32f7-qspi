/*
 * test-qspi.h
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
 *  Created on: 8 Dec 2016 (LNP)
 */

#ifndef TEST_TEST_QSPI_H_
#define TEST_TEST_QSPI_H_

#include "test-qspi-config.h"

#if TEST_CPLUSPLUS_API == true

void
test_qspi (void);

#endif

class stopwatch
{
public:

  stopwatch () = default;

  void
  start (void);

  uint32_t
  stop (void);

private:
  os::rtos::clock::timestamp_t lap_ = 0;
};

inline void
stopwatch::start (void)
{
  lap_ = os::rtos::hrclock.now ();
}

inline uint32_t
stopwatch::stop (void)
{
  return (uint32_t) ((os::rtos::hrclock.now () - lap_)
      / (SystemCoreClock / 1000000));
}

#endif /* TEST_TEST_QSPI_H_ */
