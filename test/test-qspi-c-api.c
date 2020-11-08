/*
 * test-qspi-c-api.c
 *
 * Copyright (c) 2017-2020 Lix N. Paulian (lix@paulian.net)
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
 * Created on: 6 Feb 2017 (LNP)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmsis-plus/diag/trace.h>

#include "qspi-flash-c-api.h"
#include "test-qspi-c-api.h"
#include "test-qspi-config.h"

#if TEST_CPLUSPLUS_API == false

extern QSPI_HandleTypeDef hqspi;
qspi_t* qspi_instance = NULL;

/**
 * @brief  Status match callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_StatusMatchCallback (
    QSPI_HandleTypeDef* hqspi __attribute__ ((unused)))
{
  if (qspi_instance != NULL)
    {
      qspi_event_cb (qspi_instance);
    }
}

/**
 * @brief  Receive completed callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_RxCpltCallback (QSPI_HandleTypeDef* hqspi __attribute__ ((unused)))
{
  if (qspi_instance != NULL)
    {
      qspi_event_cb (qspi_instance);
    }
}

/**
 * @brief  Transmit completed callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_TxCpltCallback (QSPI_HandleTypeDef* hqspi __attribute__ ((unused)))
{
  if (qspi_instance != NULL)
    {
      qspi_event_cb (qspi_instance);
    }
}

/**
 * @brief  This is a test function that exercises the qspi driver.
 */
void
test_qspi (void)
{
  int i;
  uint8_t* pf = (uint8_t*) 0x90000000; // memory-mapped flash address
  int sector_size;
  int sector_count;

  if ((qspi_instance = qspi_new (&hqspi)) != NULL)
    {
      do
        {
          // read memory parameters and initialize system
          if (qspi_initialize (qspi_instance) != qspi_ok)
            {
              trace_printf ("Failed to initialize\n");
              break;
            }

          sector_size = qspi_get_sector_size (qspi_instance);
          sector_count = qspi_get_sector_count (qspi_instance);
          uint8_t version_major, version_minor, version_patch;

          qspi_get_version (qspi_instance, &version_major, &version_minor,
                            &version_patch);
          trace_printf ("Driver version: %d.%d.%d\n", version_major,
                        version_minor, version_patch);
          trace_printf ("Manufacturer: %s, type: %s, sector size: %d bytes, "
                        "sector count: %d\n",
                        qspi_get_manufacturer (qspi_instance),
                        qspi_get_memory_type (qspi_instance), sector_size,
                        sector_count);

          // switch mode to memory mapped
          if (qspi_enter_mem_mapped (qspi_instance) != qspi_ok)
            {
              trace_printf ("Failed enter memory mapped mode\n");
              break;
            }
          trace_printf ("Entered memory mapped mode\n");

          // check if flash is erased
          for (i = 0; i < (sector_count * sector_size); i++, pf++)
            if (*pf != 0xFF)
              break;
          trace_printf ("Checked if flash is erased\n");

          if (qspi_exit_mem_mapped (qspi_instance) != qspi_ok)
            {
              trace_printf ("Failed to exit from memory mapped mode\n");
              break;
            }
          else
            {
              trace_printf ("Memory mapped mode switched off\n");
            }

          // if not clear, erase whole flash chip
          if (i < (sector_count * sector_size))
            {
              trace_printf (
                  "Flash not empty, trying to erase (it will take some time...)\n");
              if (qspi_erase_chip (qspi_instance) != qspi_ok)
                {
                  trace_printf ("Failed to erase flash chip\n");
                  break;
                }
              trace_printf ("Erased\n");
            }

          // get two RAM buffers
          uint8_t* pw = malloc (sector_size);
          uint8_t* pr = malloc (sector_size);
          int j;

          trace_printf ("Write/read test started...\n");
          if (pw && pr)
            {
              // generate a random block of data
              srand (0xBABA);
              for (j = 0; j < sector_size; j++)
                {
#if TEST_VERBOSE == true
                  trace_printf ("Test block #%5d\n", j);
#endif
                  for (i = 0; i < sector_size; i++)
                    pw[i] = (uint8_t) random ();

                  // write block
                  if (qspi_write_sector (qspi_instance, j, pw, sector_size)
                      != qspi_ok)
                    {
                      trace_printf ("Block write error (%d)\n", j);
                      break;
                    }
                  memset (pr, 0xAA, sector_size);
                  // read block
                  if (qspi_read_sector (qspi_instance, j, pr, sector_size)
                      != qspi_ok)
                    {
                      trace_printf ("Block read error\n");
                      break;
                    }
                  // compare data
                  if (memcmp (pw, pr, sector_size) != 0)
                    {
                      trace_printf ("Compare error at block %d\n", j);
                      break;
                    }
                }

              // done, clean-up and exit
              free (pr);
              free (pw);
              if (j == sector_count)
                {
                  trace_printf ("Flash test passed\n");
                }
            }
          else
            trace_printf ("Out of memory\n");
        }
      while (false);

      if (qspi_sleep (qspi_instance, true) != qspi_ok)
        {
          trace_printf ("Failed to switch flash chip into deep sleep\n");
        }
      else
        {
          trace_printf ("Flash chip successfully switched to deep sleep\n");
        }

      qspi_delete (qspi_instance);
    }
  else
    trace_printf ("Could not create qspi instance\n");
}

#endif
