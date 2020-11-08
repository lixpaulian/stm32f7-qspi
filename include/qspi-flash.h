/*
 * qspi-flash.h
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
 * Created on: 9 Oct 2016 (LNP)
 */

#ifndef QSPI_FLASH_H_
#define QSPI_FLASH_H_

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/posix-io/block-device.h>
#include "cmsis_device.h"
#include "quadspi.h"

#if defined (__cplusplus)

namespace os
{
  namespace driver
  {
    namespace stm32f7
    {
      typedef struct qspi_device_s qspi_device_t;
      class qspi_intern;

      class qspi_impl : public os::posix::block_device_impl
      {
      public:
        qspi_impl (QSPI_HandleTypeDef* hqspi);

        ~qspi_impl ();

        typedef enum
        {
          ok = HAL_OK,                // HAL errors
          error = HAL_ERROR,
          busy = HAL_BUSY,
          timeout = HAL_TIMEOUT,
          type_not_found = 10,        // qspi specific errors
        } qspi_result_t;

        virtual bool
        do_is_opened (void) override;

        virtual int
        do_vopen (const char* path, int oflag, std::va_list args) override;

        virtual ssize_t
        do_read_block (void* buf, blknum_t blknum, std::size_t nblocks)
            override;

        virtual ssize_t
        do_write_block (const void* buf, blknum_t blknum, std::size_t nblocks)
            override;

        virtual int
        do_vioctl (int request, std::va_list args) override;

        virtual void
        do_sync (void) override;

        virtual int
        do_close (void) override;

        void
        get_version (uint8_t& version_major, uint8_t& version_minor,
                     uint8_t& version_patch);

        qspi_result_t
        sleep (bool state);

        qspi_result_t
        initialize (void);

        qspi_result_t
        uninitialize (void);

        qspi_result_t
        enter_mem_mapped (void);

        qspi_result_t
        exit_mem_mapped (void);

        qspi_result_t
        read (uint32_t address, uint8_t* buff, size_t count);

        qspi_result_t
        write (uint32_t address, uint8_t* buff, size_t count);

        qspi_result_t
        read_sector (uint32_t sector, uint8_t* buff, size_t count);

        qspi_result_t
        write_sector (uint32_t sector, uint8_t* buff, size_t count);

        qspi_result_t
        erase_sector (uint32_t sector);

        qspi_result_t
        erase_block32K (uint32_t address);

        qspi_result_t
        erase_block64K (uint32_t address);

        qspi_result_t
        erase_chip (void);

        qspi_result_t
        reset_chip (void);

        const char*
        get_manufacturer (void);

        const char*
        get_memory_type (void);

        size_t
        get_sector_size (void);

        size_t
        get_sector_count (void);

        void
        cb_event (void);

        friend class qspi_winbond;
        friend class qspi_micron;

      protected:
        qspi_result_t
        enter_quad_mode (void);

        // Standard command sub-set (common for all flash chips)
        static constexpr uint8_t JEDEC_ID = 0x9F;

        static constexpr uint8_t WRITE_ENABLE = 0x06;
        static constexpr uint8_t WRITE_DISABLE = 0x04;

        static constexpr uint8_t READ_STATUS_REGISTER = 0x05;
        static constexpr uint8_t WRITE_STATUS_REGISTER = 0x01;

        static constexpr uint8_t SECTOR_ERASE = 0x20;
        static constexpr uint8_t BLOCK_32K_ERASE = 0x52;
        static constexpr uint8_t BLOCK_64K_ERASE = 0xD8;
        static constexpr uint8_t CHIP_ERASE = 0xC7;

        static constexpr uint8_t RESET_ENABLE = 0x66;
        static constexpr uint8_t RESET_DEVICE = 0x99;

        static constexpr uint8_t POWER_DOWN = 0xB9;
        static constexpr uint8_t RELEASE_POWER_DOWN = 0xAB;

        static constexpr uint8_t PAGE_PROGRAM = 0x02;
        static constexpr uint8_t QUAD_PAGE_PROGRAM = 0x32;

        static constexpr uint8_t READ_DATA = 0x03;
        static constexpr uint8_t FAST_READ_DATA = 0x0B;
        static constexpr uint8_t FAST_READ_QUAD_OUT = 0x6B;
        static constexpr uint8_t FAST_READ_QUAD_IN_OUT = 0xEB;

        // Some timeouts
        static constexpr uint32_t one_ms = 1000
            / os::rtos::sysclock.frequency_hz;
        static constexpr uint32_t one_sec = 1000 * one_ms;
        static constexpr uint32_t TIMEOUT = 10 * one_ms;
        static constexpr uint32_t WRITE_TIMEOUT = 50 * one_ms;
        static constexpr uint32_t ERASE_TIMEOUT = 2 * one_sec;
        static constexpr uint32_t CHIP_ERASE_TIMEOUT = 200 * one_sec;

        QSPI_HandleTypeDef* hqspi_;
        os::rtos::semaphore_binary semaphore_
          { "qspi", 0 };

      private:
        qspi_result_t
        page_write (uint32_t address, uint8_t* buff, size_t count);

        qspi_result_t
        read_JEDEC_ID (void);

        qspi_result_t
        erase (uint32_t address, uint8_t which);

        static constexpr uint8_t VERSION_MAJOR = 2;
        static constexpr uint8_t VERSION_MINOR = 2;
        static constexpr uint8_t VERSION_PATCH = 0;

        class qspi_intern* pimpl = nullptr;
        uint8_t manufacturer_ID_ = 0;
        uint16_t memory_type_ = 0;
        const char* pmanufacturer_ = nullptr;
        const qspi_device_t* pdevice_ = nullptr;
        bool volatile is_opened_ = false;
        uint8_t lbuff_[256];

      };

      class qspi_intern
      {
      public:
        qspi_intern (void)
        {
        }

        virtual
        ~qspi_intern () = default;

        virtual qspi_impl::qspi_result_t
        enter_quad_mode (qspi_impl* pq) = 0;

      };

      inline void
      qspi_impl::get_version (uint8_t& version_major, uint8_t& version_minor,
                              uint8_t& version_patch)
      {
        version_major = VERSION_MAJOR;
        version_minor = VERSION_MINOR;
        version_patch = VERSION_PATCH;
      }

      inline qspi_impl::qspi_result_t
      qspi_impl::enter_quad_mode (void)
      {
        return (pimpl == nullptr) ? error : pimpl->enter_quad_mode (this);
      }

      inline qspi_impl::qspi_result_t
      qspi_impl::exit_mem_mapped (void)
      {
        return ((qspi_impl::qspi_result_t) (HAL_QSPI_Abort (hqspi_)));
      }

      inline qspi_impl::qspi_result_t
      qspi_impl::erase_block32K (uint32_t address)
      {
        return erase (address, BLOCK_32K_ERASE);
      }

      inline qspi_impl::qspi_result_t
      qspi_impl::erase_block64K (uint32_t address)
      {
        return erase (address, BLOCK_64K_ERASE);
      }

      inline qspi_impl::qspi_result_t
      qspi_impl::erase_chip (void)
      {
        return erase (0, CHIP_ERASE);
      }

      inline const char*
      qspi_impl::get_manufacturer (void)
      {
        return pmanufacturer_;
      }

    } /* namespace stm32f7 */
  } /* namespace driver */
} /* namespace os */

#endif // (__cplusplus)

#endif /* QSPI_FLASH_H_ */
