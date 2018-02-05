/*
 * qspi-descr.cpp
 *
 * Copyright (c) 2017, 2018 Lix N. Paulian (lix@paulian.net)
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



// Micron devices
const qspi_device_t micron_devices[] =
  {
    { 0xBA18, 4096, "MT25QL128ABA", true, 8 },	// use even # of dummy cycles!
    { 0xBB18, 4096, "MT25QL128ABA", true, 8 },
    { } //
  };

// Winbond devices
const qspi_device_t winbond_devices[] =
  {
    { 0x4016, 4096, "W25Q32FV", false, 6 },	// use even # of dummy cycles!
    { 0x6016, 4096, "W25Q32FV", false, 6 },
    { 0x4017, 4096, "W25Q64FV", false, 6 },
    { 0x6017, 4096, "W25Q64FV", false, 6 },
    { 0x4018, 4096, "W25Q128FV", false, 6 },
    { 0x6018, 4096, "W25Q128FV", false, 6 },
    { 0x7018, 4096, "W25Q128JV", true, 6 },
    { } //
  };

// Factories for the manufacturer's table
static qspi_impl* new_micron (void)
{
  return new qspi_micron { };
}

static qspi_impl* new_winbond (void)
{
  return new qspi_winbond { };
}

// Supported manufactures
const qspi_manuf_t qspi_manufacturers[] =
  {
    { MANUF_ID_MICRON, "Micron/ST", micron_devices, new_micron },
    { MANUF_ID_WINBOND, "Winbond", winbond_devices, new_winbond },
    { } //
  };


