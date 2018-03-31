/*
 * test-qspi.cpp
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
 * Created on: 8 Dec 2016 (LNP)
 */

/*
 * Test the qspi driver functionality.
 */

#include <stdio.h>
#include <stdint.h>
#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/posix-io/block-device.h>
#include <cmsis-plus/posix-io/file-descriptors-manager.h>
#include <cmsis-plus/diag/trace.h>

#include "sysconfig.h"
#include "qspi-flash.h"
#include "test-qspi.h"
#include "test-qspi-config.h"

#if defined M717
#include "io.h"
#endif

#if QSPI_TEST == true
#if TEST_CPLUSPLUS_API == true

#define TEST_VERBOSE false

extern "C"
{
  QSPI_HandleTypeDef hqspi;
}

using namespace os;
using namespace os::driver::stm32f7;

// Static manager
os::posix::file_descriptors_manager descriptors_manager
  { 8 };

using qspi = os::posix::block_device_implementable<qspi_impl>;

qspi flash
  { "flash", &hqspi };

/**
 * @brief  Status match callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_StatusMatchCallback (QSPI_HandleTypeDef *phqspi)
{
  if (phqspi == &hqspi)
    {
      flash.impl ().cb_event ();
    }
}

/**
 * @brief  Receive completed callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_RxCpltCallback (QSPI_HandleTypeDef *phqspi)
{
  if (phqspi == &hqspi)
    {
      flash.impl ().cb_event ();
    }
}

/**
 * @brief  Transmit completed callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_TxCpltCallback (QSPI_HandleTypeDef *phqspi)
{
  if (phqspi == &hqspi)
    {
      flash.impl ().cb_event ();
    }
}

/**
 * @brief  This is a test function that exercises the qspi driver.
 */
void
test_qspi (void)
{
  size_t sector_size;
  posix::block_device::blknum_t sector_count;
  int total_write = 0;
  int total_read = 0;

  stopwatch sw
    { };

  do
    {
#if 0
      os::posix::block_device* blk_dev;

      blk_dev =
      static_cast<posix::block_device*> (posix::open ("/dev/flash", 0));

      sector_size = blk_dev->block_physical_size_bytes ();
      sector_count = blk_dev->blocks ();

      uint8_t* pw = new uint8_t[sector_size];
      uint8_t* pr = new uint8_t[sector_size];

      if (pw && pr)
        {
          trace::printf ("Write %d blocks...\n", sector_count);

          // generate a random block of data
          srand (0xBABA);
          posix::block_device::blknum_t sector;

          for (sector = 0; sector < sector_count; sector++)
            {
              for (size_t i = 0; i < sector_size; i++)
                {
                  pw[i] = (uint8_t) random ();
                }

              // write block
              sw.start ();
              size_t count = blk_dev->write_block (pw, sector, 1);
              if (count == 0)
                {
                  trace::printf ("Block write error (%d)\n", sector);
                  break;
                }
              total_write += sw.stop ();
             }

          if (sector == sector_count)
            {
              trace::printf ("Read and verify...\n");

              srand (0xBABA);
              for (sector = 0; sector < sector_count; sector++)
                {
                  for (size_t i = 0; i < sector_size; i++)
                    {
                      pw[i] = (uint8_t) random ();
                    }

                  // read block
                  sw.start ();
                  size_t count = blk_dev->read_block (pr, sector, 1);
                  if (count == 0)
                    {
                      trace::printf ("Block read error (%d)\n", sector);
                      break;
                    }
                  total_read += sw.stop ();

                  // compare data
                  if (memcmp (pw, pr, sector_size) != 0)
                    {
                      trace::printf ("Compare error at block %d\n", sector);
                      break;
                    }
                }
              if (sector == sector_count)
                {
                  trace::printf ("Test passed\n");
                }
            }
        }

      blk_dev->close ();

#else
      uint32_t i;
      uint8_t *pf = (uint8_t *) 0x90000000; // memory-mapped flash address

      // read memory parameters and initialize system
      if (flash.impl ().initialize () != qspi_impl::ok)
        {
          trace::printf ("Failed to initialize\n");
          break;
        }

      sector_size = flash.impl ().get_sector_size ();
      sector_count = flash.impl ().get_sector_count ();
      uint8_t version_major, version_minor;

      flash.impl ().get_version (version_major, version_minor);
      trace::printf ("QSPI driver version: %d.%d\n", version_major,
                     version_minor);
      trace::printf (
          "Flash chip manufacturer: %s, type: %s, sector size: %d bytes, "
          "sector count: %d\n",
          flash.impl ().get_manufacturer (), flash.impl ().get_memory_type (),
          sector_size, sector_count);
//      break;

      // switch mode to memory mapped
      if (flash.impl ().enter_mem_mapped () != qspi_impl::ok)
        {
          trace::printf ("Failed enter memory mapped mode\n");
          break;
        }
      trace::printf ("Entered memory mapped mode\n");
//      break;

      // check if flash is erased
      sw.start ();
      for (i = 0; i < (sector_count * sector_size); i++, pf++)
        {
          if (*pf != 0xFF)
            {
              break;
            }
        }
      trace::printf ("Checked if flash is erased in %.3f ms (%d)\n",
                     sw.stop () / (float) 1000, i);

      if (flash.impl ().exit_mem_mapped () != qspi_impl::ok)
        {
          trace::printf ("Failed to exit from memory mapped mode\n");
          break;
        }
      else
        {
          trace::printf ("Memory mapped mode switched off\n");
        }
//      break;

      // if not clear, erase whole flash chip
      if (i < (sector_count * sector_size))
        {
          trace::printf (
              "Flash not empty, trying to erase (it will take some time...)\n");
          sw.start ();
          if (flash.impl ().erase_chip () != qspi_impl::ok)
            {
              trace::printf ("Failed to erase flash chip\n");
              break;
            }
          trace::printf ("Erased in %.2f s\n", sw.stop () / (float) 1000000);
        }
//      break;

      // get two RAM buffers
      uint8_t *pw = new uint8_t[sector_size];
      uint8_t *pr = new uint8_t[sector_size];
      uint32_t j;

      trace::printf ("Write/read test started...\n");
      if (pw && pr)
        {
          // generate a random block of data
          srand (0xBABA);
          for (j = 0; j < sector_size; j++)
            {
#if TEST_VERBOSE == true
              trace::printf ("Test block #%5d\n", j);
#endif
              for (i = 0; i < sector_size; i++)
                {
                  pw[i] = (uint8_t) random ();
                }

              // write block
              sw.start ();
              if (flash.impl ().write_sector (j, pw, sector_size)
                  != qspi_impl::ok)
                {
                  trace::printf ("Block write error (%d)\n", j);
                  break;
                }
              total_write += sw.stop ();
              memset (pr, 0xAA, sector_size);

              // read block
              sw.start ();
              if (flash.impl ().read_sector (j, pr, sector_size)
                  != qspi_impl::ok)
                {
                  trace::printf ("Block read error\n");
                  break;
                }
              total_read += sw.stop ();

              // compare data
#if true
              if (memcmp (pw, pr, sector_size) != 0)
                {
                  trace::printf ("Compare error at block %d\n", j);
                  break;
                }
#else
              int k;
              for (k = 0; k < sector_size; k++)
                {
                  if (*(pr + k) != *(pw + k))
                    {
                      trace::printf ("Compare error at block %d, count %d\n", j, k);
                      for (i = 0; i < 16; i++)
                      trace::printf ("%02X<->%02X  ", *(pw + k + i), *(pr + k + i));
                      trace::putchar ('\n');
                      break;
                    }
                }
              if (k != sector_size)
              break;
#endif
            }

          // done, clean-up and exit
          delete (pr);
          delete (pw);
          if (j == sector_count)
            {
              trace::printf (
                  "Flash test passed\nTotal write time %.2f s, "
                  "total read time %.2f s\n"
                  "Avg. sector write time %.2f ms, avg. sector read time %.2f ms\n",
                  total_write / (float) 1000000, total_read / (float) 1000000,
                  (total_write / sector_count) / (float) 1000,
                  (total_read / sector_count) / (float) 1000);
            }
        }
      else
        trace::printf ("Out of memory\n");
#endif
    }
  while (false);

  if (flash.impl ().sleep (true) != qspi_impl::ok)
    {
      trace::printf ("Failed to switch flash chip into deep sleep\n");
    }
  else
    {
      trace::printf ("Flash chip successfully switched to deep sleep\n");
    }

  trace::printf ("Exiting flash tests.\n");
}

#endif  // TEST_CPLUSPLUS_API
#endif  // TEST_QSPI

