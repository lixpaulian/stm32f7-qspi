/*
 * test-qspi.cpp
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

#define FLASH_SIZE (4096 * 4096) // TODO: replace with the info read-out from the flash chip

extern "C"
{
  QSPI_HandleTypeDef hqspi;
}

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

  do
    {
      // read memory parameters
      if (flash.read_JEDEC_ID () == false)
	{
	  trace_printf ("Failed read the memory parameters\n");
	}
      else
	{
	  uint8_t manufacturer_ID, memory_type, memory_capacity;
	  if (flash.get_ID_data (manufacturer_ID, memory_type,
				   memory_capacity) == true)
	    {
	      trace_printf ("Manufacturer: 0x%02x, type: 0x%02x, capacity: 0x%02x\n",
			    manufacturer_ID, memory_type, memory_capacity);
	    }
	  else
	    {
	      trace_printf ("Invalid memory info\n");
	    }
	}
//      break;

      // switch qspi flash to quad mode
      if (flash.mode_quad () == false)
	{
	  trace_printf ("Failed to switch the flash to quad mode\n");
	  break;
	}

      // switch mode to memory mapped
      if (flash.memory_mapped () == false)
	{
	  trace_printf ("Failed to switch the flash to memory mapped mode\n");
	  break;
	}
//	break;

      // check if flash is erased
      for (i = 0; i < FLASH_SIZE; i++, pf++)
	if (*pf != 0xFF)
	  break;

      // if not clear, erase whole flash chip
      if (i < FLASH_SIZE)
	{
	  trace_printf (
	      "Flash not empty, trying to erase (it will take some time...)\n");
	  if (flash.chip_erase () == false)
	    {
	      trace_printf ("Failed to erase flash chip\r\n");
	      break;
	    }
	}

      // get two RAM buffers
      uint8_t *pw = reinterpret_cast<uint8_t*> (malloc (4096));
      uint8_t *pr = reinterpret_cast<uint8_t*> (malloc (4096));
      int j;

      if (pw && pr)
	{
	  // generate a random block of data
	  srand (0xBABA);
	  for (j = 0; j < 4096; j++)
	    {
	      trace_printf ("Test block #%5d\n", j);
	      for (i = 0; i < 4096; i++)
		pw[i] = (uint8_t) random ();

	      // write block
	      if (flash.write (j * 4096, pw, 4096) == false)
		{
		  trace_printf ("Block write error\n");
		  break;
		}

	      // read block
	      else if (flash.read (j * 4096, pr, 4096) == false)
		{
		  trace_printf ("Block read error\n");
		  break;
		}

	      // compare data
	      else if (memcmp (pw, pr, 4096) != 0)
		{
		  trace_printf ("Compare error\n");
		  break;
		}
	    }

	  // done, clean-up and exit
	  free (pr);
	  free (pw);
	  if (j == 4096)
	    trace_printf ("Flash test passed\n");
	}
      else
	trace_printf ("Out of memory\n");
    }
  while (false);

  trace_printf ("Exiting flash tests.\n");
}

