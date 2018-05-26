/*
 * qspi-flash.cpp
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
 * Created on: 9 Oct 2016 (LNP)
 */

/*
 * This file implements the common basic low level functions to control
 * a QSPI flash device.
 */

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>
#include "qspi-flash.h"
#include "qspi-descr.h"
#include "qspi-winbond.h"
#include "qspi-micron.h"

namespace os
{
  namespace driver
  {
    namespace stm32f7
    {
      /**
       * @brief Constructor.
       * @param hqspi: HAL qspi handle.
       */
      qspi_impl::qspi_impl (QSPI_HandleTypeDef* hqspi)
      {
        trace::printf ("%s(%p) @%p\n", __func__, hqspi, this);
        hqspi_ = hqspi;
      }

      qspi_impl::~qspi_impl ()
      {
        trace::printf ("%s(%p) @%p\n", __func__, this);
      }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

      //----------------- POSIX interface ------------------------------

      /**
       * @brief Check if the device is opened
       * @return Returns true if the device is already opened, false otherwise
       */
      bool
      qspi_impl::do_is_opened (void)
      {
        return is_opened_;
      }

      /**
       * @brief Open the block device
       * @param path: path to the device.
       * @param oflag: flags.
       * @param args: arguments list.
       * @return 0 if the device was successfully opened, -1 otherwise.
       */
      int
      qspi_impl::do_vopen (const char* path, int oflag, std::va_list args)
      {
        int result = -1;

        do
          {
            if (is_opened_)
              {
                errno = EEXIST; // already opened
                break;
              }

            if (hqspi_->Instance == nullptr)
              {
                errno = EIO;      // no QSPI IP defined
                break;
              }

            if (qspi_impl::initialize () != qspi_impl::ok)
              {
                errno = EIO;
                break;
              }

            num_blocks_ = qspi_impl::get_sector_count ();
            block_logical_size_bytes_ = qspi_impl::get_sector_size ();
            block_physical_size_bytes_ = qspi_impl::get_sector_size ();

            if (num_blocks_ == 0 || block_physical_size_bytes_ == 0)
              {
                errno = EIO;
                break;
              }

            is_opened_ = true;
            result = 0;
          }
        while (false);

        return result;
      }

      /**
       * @brief Read a block of data.
       * @param buf: buffer where the data will be returned.
       * @param blknum: the block number.
       * @param nblocks: number of blocks to read.
       * @return Number of blocks read.
       */
      ssize_t
      qspi_impl::do_read_block (void* buf, posix::block_device::blknum_t blknum,
                                std::size_t nblocks)
      {
        // compute the block's address and the total bytes to be read
        uint32_t address = block_logical_size_bytes_ * blknum;
        size_t count = block_logical_size_bytes_ * nblocks;

        if (qspi_impl::read (address, (uint8_t*) buf, count) != ok)
          {
            nblocks = 0;
          }
        return nblocks;
      }

      /**
       * @brief Write data to the block device.
       * @param buf: buffer with the data to be written.
       * @param blknum: the block number.
       * @param nblocks: number of blocks to be written.
       * @return Number of blocks written.
       */
      ssize_t
      qspi_impl::do_write_block (const void* buf,
                                 posix::block_device::blknum_t blknum,
                                 std::size_t nblocks)
      {
        // compute the block's address and the total bytes to be written
        uint32_t address = block_logical_size_bytes_ * blknum;
        size_t count = block_logical_size_bytes_ * nblocks;
        bool to_write = false;

        // check if we really need to write
        uint8_t* p = (uint8_t*) buf;
        for (posix::block_device::blknum_t i = 0; i < count; i++)
          {
            if (*p++ != 0xFF)
              {
                to_write = true;
                break;  // yes, we have data to write
              }
          }

        int i = nblocks;
        if (to_write == false)
          {
            // nothing to write, only erase then quit
            while (i--)
              {
                qspi_impl::erase_sector (blknum++);
              }
          }
        else
          {
            uint32_t sector_256 = 0;
            p = (uint8_t*) buf;
            bool to_erase = false;

            do
              {
                bool valid_data = false;

                if (qspi_impl::read (address + (sector_256 * sizeof(lbuff_)),
                                     lbuff_, sizeof(lbuff_)) != ok)
                  {
                    break;  // read error, exit
                  }

                // check if we need to erase before write
                for (int j = 0; j < (int) sizeof(lbuff_); j++, p++)
                  {
                    if (*p != lbuff_[j] && lbuff_[j] != 0xFF)
                      {
                        // yes, we must erase before write
                        to_erase = true;
                        break;
                      }
                    if (*p != 0xFF && *p != lbuff_[j])
                      {
                        valid_data = true;
                      }
                  }
                if (to_erase)
                  {
                    break;
                  }

                // sector already erased, just write but only if whole data != 0xFF
                if (valid_data)
                  {
                    if (qspi_impl::write (
                        address + (sector_256 * sizeof(lbuff_)),
                        (uint8_t*) buf + (sector_256 * sizeof(lbuff_)),
                        sizeof(lbuff_)) != ok)
                      {
                        nblocks = 0;
                        break;
                      }
                  }
                sector_256++;
              }
            while (p < ((uint8_t*) buf + count));

            if (to_erase == true && nblocks)
              {
                // write without erase did not work
                // so erase first the blocks to be written
                while (i--)
                  {
                    qspi_impl::erase_sector (blknum++);
                  }

                if (qspi_impl::write (address, (uint8_t*) buf, count) != ok)
                  {
                    nblocks = 0;
                  }
              }
          }
        return nblocks;
      }

      /**
       * @brief Control the device parameters.
       * @param request: command to the device.
       * @param args: command's parameter(s).
       * @return 0 if successfull, -1 otherwise.
       */
      int
      qspi_impl::do_vioctl (int request, std::va_list args)
      {
        return -1;
      }

      /**
       * @brief Synch (flush) the data to the device.
       */
      void
      qspi_impl::do_sync (void)
      {
        ;
      }

      /**
       * @brief Close the block device.
       * @return 0 if successfull, -1 otherwise.
       */
      int
      qspi_impl::do_close (void)
      {
        if (qspi_impl::uninitialize () != ok)
          {
            errno = EIO;
            return -1;
          }

        is_opened_ = false;

        return 0;
      }

      //------------- End of POSIX interface ---------------------------

      /**
       * @brief  Read the flash chip ID and initialize the internal structures accordingly.
       * @return qspi_impl::ok if successful, or a qspi_impl error if the flash could not be identified
       * 	 or it is not supported by the driver.
       */
      qspi_impl::qspi_result_t
      qspi_impl::initialize (void)
      {
        qspi_impl::qspi_result_t result;

        // Read flash device ID
        if ((result = qspi_impl::read_JEDEC_ID ()) != ok)
          {
            // Flash device might be in deep sleep
            qspi_impl::sleep (false);

            // Reset and try reading ID again
            if ((result = qspi_impl::reset_chip ()) == ok)
              {
                result = qspi_impl::read_JEDEC_ID ();
              }
          }

        // If all OK, switch flash device in quad mode
        if (result == ok)
          result = enter_quad_mode ();

        return result;
      }

      /**
       * @brief  Set flash device to default state.
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::uninitialize (void)
      {
        pimpl = nullptr;
        qspi_impl::sleep (false);
        return qspi_impl::reset_chip ();
      }

      /**
       * @brief  Read the memory parameters (manufacturer and type).
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::read_JEDEC_ID (void)
      {
        qspi_impl::qspi_result_t result = error;
        uint8_t buff[3];
        QSPI_CommandTypeDef sCommand;

        // Read command settings
        sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
        sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
        sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
        sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
        sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        sCommand.AddressMode = QSPI_ADDRESS_NONE;
        sCommand.DataMode = QSPI_DATA_1_LINE;
        sCommand.DummyCycles = 0;
        sCommand.NbData = 3;
        sCommand.Instruction = JEDEC_ID;

        // Initiate read and wait for the event
        result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_, &sCommand,
                                                              TIMEOUT);
        if (result == ok)
          {
            result = (qspi_impl::qspi_result_t) HAL_QSPI_Receive_IT (hqspi_,
                                                                     buff);
            if (result == ok)
              {
                if (semaphore_.timed_wait (TIMEOUT) == rtos::result::ok)
                  {
                    manufacturer_ID_ = buff[0];
                    memory_type_ = buff[1] << 8;
                    memory_type_ += buff[2];

                    // Do we know this device?
                    result = type_not_found;
                    for (const qspi_manuf_t* pqm = qspi_manufacturers;
                        pqm->manufacturer_ID != 0; pqm++)
                      {
                        if (pqm->manufacturer_ID == manufacturer_ID_)
                          {
                            // Manufacturer found
                            for (const qspi_device_t* pqd = pqm->devices;
                                pqd->device_ID != 0; pqd++)
                              {
                                if (pqd->device_ID == memory_type_)
                                  {
                                    // Device found, initialize class
                                    pmanufacturer_ = pqm->manufacturer_name;
                                    pdevice_ = pqd;
                                    pimpl = pqm->qspi_factory ();
                                    result = ok;
                                    break;
                                  }
                              }
                          }
                      }
                  }
                else
                  {
                    result = timeout;
                  }
              }
          }
        return result;
      }

      /**
       * @brief  Switch the flash chip into or out of deep sleep.
       * @param  state: if true, enter deep sleep; if false, exit deep sleep.
       * @return qspi::ok if successful, an error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::sleep (bool state)
      {
        qspi_impl::qspi_result_t result = error;
        QSPI_CommandTypeDef sCommand;

        // Initial command settings
        sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
        sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
        sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
        sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
        sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        sCommand.AddressMode = QSPI_ADDRESS_NONE;
        sCommand.DataMode = QSPI_DATA_NONE;
        sCommand.DummyCycles = 0;

        // Enable/disable deep sleep
        sCommand.Instruction = state ? POWER_DOWN : RELEASE_POWER_DOWN;
        result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_, &sCommand,
                                                              TIMEOUT);
        return result;
      }

      /**
       * @brief  Map the flash to the addressing space of the controller, starting at
       * 	address 0x90000000.
       * @return qspi::ok if successful, false otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::enter_mem_mapped (void)
      {
        qspi_impl::qspi_result_t result = error;
        QSPI_CommandTypeDef sCommand;
        QSPI_MemoryMappedTypeDef sMemMappedCfg;

        if (pdevice_ != nullptr)
          {
            sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
            sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
            sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
            sCommand.AlternateBytes = 0;	// Continuous read mode off
            sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
            sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
            sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
            sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
            sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
            sCommand.DataMode = QSPI_DATA_4_LINES;
            sCommand.DummyCycles = pdevice_->dummy_cycles - 2; // Subtract alternate byte
            sCommand.Instruction = FAST_READ_QUAD_IN_OUT;

            sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

            result = (qspi_impl::qspi_result_t) HAL_QSPI_MemoryMapped (
                hqspi_, &sCommand, &sMemMappedCfg);
          }
        return result;
      }

      /**
       * @brief  Read a block of data from the flash.
       * @param  address: start address in flash where to read from.
       * @param  buff: buffer where to copy data to.
       * @param  count: amount of data to be retrieved from flash.
       * @return qspi::ok if successful, a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::read (uint32_t address, uint8_t* buff, size_t count)
      {
        qspi_impl::qspi_result_t result = error;
        QSPI_CommandTypeDef sCommand;

        if (pdevice_ != nullptr)
          {
            // Read command settings
            sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
            sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
            sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
            sCommand.AlternateBytes = 0;	// Continuous read mode off
            sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
            sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
            sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
            sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
            sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
            sCommand.DataMode = QSPI_DATA_4_LINES;
            sCommand.DummyCycles = pdevice_->dummy_cycles - 2; // Subtract alternate byte
            sCommand.Address = address;
            sCommand.NbData = count;
            sCommand.Instruction = FAST_READ_QUAD_IN_OUT;

            // Flush and clean the data cache to mitigate incoherence after
            // DMA transfers (DTCM RAM is not cached)
            if ((buff + count) >= (uint8_t *) SRAM1_BASE)
              {
                uint32_t *aligned_buff = (uint32_t *) (((uint32_t) (buff))
                    & 0xFFFFFFE0);
                uint32_t aligned_count = (uint32_t) (count & 0xFFFFFFE0) + 32;
                SCB_CleanInvalidateDCache_by_Addr (aligned_buff, aligned_count);
              }

            // Initiate read and wait for the event
            result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_,
                                                                  &sCommand,
                                                                  TIMEOUT);
            if (result == ok)
              {
                result = (qspi_impl::qspi_result_t) HAL_QSPI_Receive_DMA (
                    hqspi_, buff);
                if (result == ok)
                  {
                    result =
                        (semaphore_.timed_wait (TIMEOUT) == rtos::result::ok) ?
                            ok : timeout;
                  }
              }
          }
        return result;
      }

      /**
       * @brief  Write data to flash.
       * @param  address: start address in flash where to write data to.
       * @param  buff: source data to be written.
       * @param  count: amount of data to be written.
       * @return qspi::ok if successful, a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::write (uint32_t address, uint8_t* buff, size_t count)
      {
        qspi_impl::qspi_result_t result = error;
        size_t in_block_count;

        if (pdevice_ != nullptr)
          {
            // Clean the data cache to mitigate incoherence before DMA transfers
            // (DTCM RAM is not cached)
            if ((buff + count) >= (uint8_t *) SRAM1_BASE)
              {
                uint32_t *aligned_buff = (uint32_t *) (((uint32_t) (buff))
                    & 0xFFFFFFE0);
                uint32_t aligned_count = (uint32_t) (count & 0xFFFFFFE0) + 32;
                SCB_CleanDCache_by_Addr (aligned_buff, aligned_count);
              }

            do
              {
                in_block_count = 0x100 - (address & 0xFF);
                if (in_block_count > count)
                  {
                    in_block_count = count;
                  }
                else if (in_block_count == 0)
                  {
                    in_block_count = (count > 0x100) ? 0x100 : count;
                  }
                if ((result = page_write (address, buff, in_block_count)) != ok)
                  {
                    break;
                  }
                address += in_block_count;
                buff += in_block_count;
                count -= in_block_count;
              }
            while (count > 0);
          }
        return result;
      }

      /**
       * @brief  Write a page of data to the flash (max. 256 bytes).
       * @param  address: address of the page in flash.
       * @param  buff: buffer of the source data.
       * @param  count: number of bytes to be written (max 256).
       * @return qspi::ok if successful, a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::page_write (uint32_t address, uint8_t* buff, size_t count)
      {
        qspi_impl::qspi_result_t result = error;
        QSPI_CommandTypeDef sCommand;
        QSPI_AutoPollingTypeDef sConfig;

        // Initial command settings
        sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
        sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
        sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
        sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
        sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        sCommand.AddressMode = QSPI_ADDRESS_NONE;
        sCommand.DataMode = QSPI_DATA_NONE;
        sCommand.DummyCycles = 0;

        // Enable write
        sCommand.Instruction = WRITE_ENABLE;
        result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_, &sCommand,
                                                              TIMEOUT);
        if (result == ok)
          {
            // Initiate write
            sCommand.Instruction = PAGE_PROGRAM;
            sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
            sCommand.DataMode = QSPI_DATA_4_LINES;
            sCommand.Address = address;
            sCommand.NbData = count;
            result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_,
                                                                  &sCommand,
                                                                  TIMEOUT);
            if (result == ok)
              {
                result = (qspi_impl::qspi_result_t) HAL_QSPI_Transmit_DMA (
                    hqspi_, buff);
                if (result == ok)
                  {
                    if (semaphore_.timed_wait (TIMEOUT) == rtos::result::ok)
                      {
                        // Set auto-polling and wait for the event
                        sCommand.AddressMode = QSPI_ADDRESS_NONE;
                        sCommand.DataMode = QSPI_DATA_4_LINES;
                        sCommand.Instruction = READ_STATUS_REGISTER;
                        sConfig.Match = 0;
                        sConfig.Mask = 1;
                        sConfig.MatchMode = QSPI_MATCH_MODE_AND;
                        sConfig.StatusBytesSize = 1;
                        sConfig.Interval = 0x10;
                        sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
                        result =
                            (qspi_impl::qspi_result_t) HAL_QSPI_AutoPolling_IT (
                                hqspi_, &sCommand, &sConfig);
                        if (result == ok)
                          {
                            result =
                                (semaphore_.timed_wait (TIMEOUT)
                                    == rtos::result::ok) ? ok : timeout;
                          }
                      }
                    else
                      {
                        result = timeout;
                      }
                  }
              }
          }
        return result;
      }

      /**
       * @brief  Erase a sector (4K), block (32K), large block (64K) or whole flash.
       * @param  address: address of the block to be erased.
       * @param  which: command to erase, can be either SECTOR_ERASE, BLOCK_32K_ERASE,
       * 	BLOCK_64K_ERASE or CHIP_ERASE.
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::erase (uint32_t address, uint8_t which)
      {
        qspi_impl::qspi_result_t result = error;
        QSPI_CommandTypeDef sCommand;
        QSPI_AutoPollingTypeDef sConfig;

        if (pdevice_ != nullptr)
          {
            // Initial command settings
            sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
            sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
            sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
            sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
            sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
            sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
            sCommand.AddressMode = QSPI_ADDRESS_NONE;
            sCommand.DataMode = QSPI_DATA_NONE;
            sCommand.DummyCycles = 0;

            // Enable write
            sCommand.Instruction = WRITE_ENABLE;
            result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_,
                                                                  &sCommand,
                                                                  TIMEOUT);
            if (result == ok)
              {
                // Initiate erase
                sCommand.Instruction = which;
                sCommand.AddressMode =
                    (which == CHIP_ERASE) ? QSPI_ADDRESS_NONE : //
                        QSPI_ADDRESS_4_LINES;
                sCommand.DataMode = QSPI_DATA_NONE;
                sCommand.Address = address;
                result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_,
                                                                      &sCommand,
                                                                      TIMEOUT);
                if (result == ok)
                  {
                    // Set auto-polling and wait for the event
                    sCommand.Instruction = READ_STATUS_REGISTER;
                    sCommand.AddressMode = QSPI_ADDRESS_NONE;
                    sCommand.DataMode = QSPI_DATA_4_LINES;
                    sConfig.Match = 0;
                    sConfig.Mask = 1;
                    sConfig.MatchMode = QSPI_MATCH_MODE_AND;
                    sConfig.StatusBytesSize = 1;
                    sConfig.Interval = 0x10;
                    sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
                    result =
                        (qspi_impl::qspi_result_t) HAL_QSPI_AutoPolling_IT (
                            hqspi_, &sCommand, &sConfig);
                    if (result == ok)
                      {
                        result =
                            (semaphore_.timed_wait (
                                (which == CHIP_ERASE) ? CHIP_ERASE_TIMEOUT : //
                                    ERASE_TIMEOUT) == rtos::result::ok) ?
                                ok : timeout;
                      }
                  }
              }
          }
        return result;
      }

      /**
       * @brief  Read sector.
       * @param  sector: sector number to read from.
       * @param  buff: buffer to receive the data from flash.
       * @param  count: number of bytes to read (buffer should be large enough).
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::read_sector (uint32_t sector, uint8_t* buff, size_t count)
      {
        return read (sector * pdevice_->sector_size, buff, count);
      }

      /**
       * @brief  Write sector.
       * @param  sector: sector number to write to.
       * @param  buff: buffer containing the data to be written to flash.
       * @param  count: number of bytes to be written.
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::write_sector (uint32_t sector, uint8_t* buff, size_t count)
      {
        return write (sector * pdevice_->sector_size, buff, count);
      }

      /**
       * @brief  Erase sector.
       * @param  sector: sector to be erased.
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::erase_sector (uint32_t sector)
      {
        return erase (sector * pdevice_->sector_size, SECTOR_ERASE);
      }

      /**
       * @brief  Software reset the flash chip.
       * @return qspi::ok if successful, or a qspi error otherwise.
       */
      qspi_impl::qspi_result_t
      qspi_impl::reset_chip (void)
      {
        qspi_impl::qspi_result_t result = busy;
        QSPI_CommandTypeDef sCommand;

        // Initial command settings
        sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
        sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
        sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
        sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
        sCommand.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        sCommand.AddressMode = QSPI_ADDRESS_NONE;
        sCommand.DataMode = QSPI_DATA_NONE;
        sCommand.DummyCycles = 0;

        // Enable reset
        sCommand.Instruction = RESET_ENABLE;
        result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_, &sCommand,
                                                              TIMEOUT);
        if (result == ok)
          {
            // Send reset command
            sCommand.Instruction = RESET_DEVICE;
            result = (qspi_impl::qspi_result_t) HAL_QSPI_Command (hqspi_,
                                                                  &sCommand,
                                                                  TIMEOUT);
          }
        return result;
      }

      /**
       * @brief  Return the memory type.
       * @return Pointer to a string representing the human readable memory type, or
       * 	null pointer if the system is not initialized.
       */
      const char*
      qspi_impl::get_memory_type (void)
      {
        return (pdevice_ == nullptr) ? nullptr : pdevice_->device_name;
      }

      /**
       * @brief  Return the sector size.
       * @return The sector size in bytes, zero if system is not initialized.
       */
      size_t
      qspi_impl::get_sector_size (void)
      {
        return (pdevice_ == nullptr) ? 0 : pdevice_->sector_size;
      }

      /**
       * @brief  Return the sectors count.
       * @return The sectors count, zero if system is not initialized.
       */
      size_t
      qspi_impl::get_sector_count (void)
      {
        size_t size = 0;

        if (pdevice_ != nullptr)
          {
            size_t size = pdevice_->device_ID & 0xFF;
            size = (1 << size);
            return size / pdevice_->sector_size;
          }
        return size;
      }

      /**
       * @brief  QSPI peripheral interrupt call-back.
       */
      void
      qspi_impl::cb_event (void)
      {
        semaphore_.post ();
      }

    } /* namespace stm32f7 */
  } /* namespace driver */
} /* namespace os */

#pragma GCC diagnostic pop

