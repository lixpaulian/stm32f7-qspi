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
#include <cmsis-plus/diag/trace.h>

#include "qspi-flash.h"
#include "test-qspi.h"
#if defined M717
#include "io.h"
#endif

#define TEST_VERBOSE false

extern "C"
{
  QSPI_HandleTypeDef hqspi;
}

using namespace os;

qspi flash
  { &hqspi };

/**
 * @brief  Status match callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_StatusMatchCallback (
    QSPI_HandleTypeDef *hqspi __attribute__ ((unused)))
{
  flash.cb_event ();
}

/**
 * @brief  Rx Transfer completed callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_RxCpltCallback (QSPI_HandleTypeDef *hqspi __attribute__ ((unused)))
{
  flash.cb_event ();
}

/**
 * @brief  This is a test function that exercises the qspi driver.
 */
void
test_qspi (void)
{
  int i;
  uint8_t *pf = (uint8_t *) 0x90000000;	// memory-mapped flash address
  int sector_size;
  int sector_count;
  int total_write = 0;
  int total_read = 0;

  stopwatch sw
    { };

#if defined M717
  led red
    { LED_RED };

  red.power_up ();
#endif

  do
    {
      // configure hardware and power the QSPI peripheral
      flash.power (true);

      // read memory parameters
      if (flash.initialize () == false)
	{
	  trace::printf (
	      "Failed to read the memory parameters, try reseting the chip\n");

	  // just in case we are in deep sleep
	  if (flash.sleep (false) == false)
	    trace::printf ("Failed to exit deep sleep");

	  if (flash.reset_chip () == false)
	    {
	      trace::printf ("Failed to reset the chip");
	      break;
	    }
	  if (flash.initialize () == false)
	    {
	      trace::printf ("Failed twice to read the memory parameters");
	      break;
	    }
	}

      sector_size = flash.get_sector_size ();
      sector_count = flash.get_sector_count ();
      uint8_t version_major, version_minor;

      flash.get_version (version_major, version_minor);
      trace::printf ("Driver version: %d.%d\n", version_major, version_minor);
      trace::printf ("Manufacturer: %s, type: %s, sector size: %d bytes, "
		     "sector count: %d\n",
		     flash.get_manufacturer (), flash.get_memory_type (),
		     sector_size, sector_count);
//      break;

      // switch qspi flash to quad mode
      if (flash.enter_quad_mode () == false)
	{
	  trace::printf ("Failed to switch the flash to quad mode\n");
	  break;
	}
      trace::printf ("Entered quad mode\n");

      // switch mode to memory mapped
      if (flash.enter_mem_mapped () == false)
	{
	  trace::printf ("Failed enter memory mapped mode\n");
	  break;
	}
      trace::printf ("Entered memory mapped mode\n");
//      break;

      // check if flash is erased
      sw.start ();
      for (i = 0; i < (sector_count * sector_size); i++, pf++)
	if (*pf != 0xFF)
	  break;
      trace::printf ("Checked if flash is erased in %.3f ms (%d)\n",
		     sw.stop () / (float) 1000, i);

      if (flash.exit_mem_mapped () == false)
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
#if defined M717
	  red.turn_on ();
#endif
	  sw.start ();
	  if (flash.erase_chip () == false)
	    {
	      trace::printf ("Failed to erase flash chip\r\n");
	      break;
	    }
	  trace::printf ("Erased in %.2f s\n", sw.stop () / (float) 1000000);
#if defined M717
	  red.turn_off ();
#endif
	}
//      break;

      // get two RAM buffers
      uint8_t *pw = reinterpret_cast<uint8_t*> (malloc (sector_size));
      uint8_t *pr = reinterpret_cast<uint8_t*> (malloc (sector_size));
      int j;

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
		pw[i] = (uint8_t) random ();

	      // write block
#if defined M717
	      red.turn_on ();
#endif
	      sw.start ();
	      if (flash.write_sector (j, pw, sector_size) == false)
		{
		  trace::printf ("Block write error\n");
		  break;
		}
	      total_write += sw.stop ();
#if defined M717
	      red.turn_off ();
#endif

	      // read block
	      sw.start ();
	      if (flash.read_sector (j, pr, sector_size) == false)
		{
		  trace::printf ("Block read error\n");
		  break;
		}
	      total_read += sw.stop ();

	      // compare data
	      if (memcmp (pw, pr, sector_size) != 0)
		{
		  trace::printf ("Compare error at block %d\n", j);
		  break;
		}
	    }

	  // done, clean-up and exit
	  free (pr);
	  free (pw);
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
    }
  while (false);

  if (flash.sleep (true) == false)
    {
      trace::printf ("Failed to switch flash chip into deep sleep\n");
    }
  else
    {
      trace::printf ("Flash chip successfully switched to deep sleep\n");
    }

  flash.power (false);

  trace::printf ("Exiting flash tests.\n");
}

