/*
 * test-chan-fatfs.cpp
 *
 * This code is based on code written originally by ChaN and adapted by
 * Liviu Ionescu.
 *
 * Created on: 31 Mar 2018
 */

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>

#include <cmsis-plus/posix-io/block-device.h>
#include <cmsis-plus/posix-io/block-device-partition.h>
#include <cmsis-plus/posix-io/chan-fatfs-file-system.h>
#include <cmsis-plus/posix-io/file-descriptors-manager.h>

#include "chan-fatfs/ff.h"         /* Declarations of sector size */
#include "chan-fatfs/diskio.h"     /* Declarations of disk functions */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "qspi-flash.h"
#include "sysconfig.h"
#include "test-chan-fatfs.h"

using namespace os;
using namespace os::driver::stm32f7;

#if FS_ENABLED == true

extern "C"
{
  QSPI_HandleTypeDef hqspi;
}

#if FILE_SYSTEM_TEST == true
static DWORD
pn (DWORD pns);

int
test_diskio (PDRV pdrv, UINT ncyc, DWORD* buf, UINT sz_buff);
#endif

#if CONSOLE_ON_VCP == false
// Static manager
os::posix::file_descriptors_manager descriptors_manager
  { 8 };
#endif

// Explicit template instantiation.
template class posix::block_device_lockable<qspi_impl, rtos::mutex>;
using qspi = posix::block_device_lockable<qspi_impl, rtos::mutex>;

os::rtos::mutex flash_mx
  { "flash_mx" };

// /dev/flash
qspi flash
  { "flash", flash_mx, &hqspi };

/**
 * @brief  Status match callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void
HAL_QSPI_StatusMatchCallback (QSPI_HandleTypeDef* phqspi)
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
HAL_QSPI_RxCpltCallback (QSPI_HandleTypeDef* phqspi)
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
HAL_QSPI_TxCpltCallback (QSPI_HandleTypeDef* phqspi)
{
  if (phqspi == &hqspi)
    {
      flash.impl ().cb_event ();
    }
}

rtos::mutex mx_fat
  { "mx_fat" };

#ifdef DISCO

posix::chan_fatfs_file_system_lockable<rtos::mutex> fat_fs
  { "fat", flash, mx_fat };

#endif

#ifdef M717

// explicit template instantiation
template class posix::block_device_partition_implementable<>;
using partition = os::posix::block_device_partition_implementable<>;

// instantiate the partitions
// /dev/fat
partition fat
  { "fat", flash };

// /dev/fifo
partition fifo
  { "fifo", flash };

// /dev/config
partition p_config
  { "config", flash };

// /dev/ro
partition ro
  { "read-only", flash };

// /dev/log
partition logp
  { "log", flash };

posix::chan_fatfs_file_system_lockable<rtos::mutex> fat_fs
  { "fat-fs", fat, mx_fat };

/**
 * @brief Initialise all block devices, partitions, the log system and the
 *      historians database.
 */
void
init_block_devices (void)
{
  posix::block_device::blknum_t bks = 0;

  // The number of blocks is known only after open().
  if (flash.open () < 0)
    {
      os::trace::printf ("Failed to open the flash block device\n");
    }
  else
    {
      bks = flash.blocks ();

      /*
       * Define partitions:
       *     - FIFO partition -> 3 MBytes (768 blocks)
       *     - Log partition -> ~1 MByte (247 blocks)
       *     - Configuration partition -> 32 Kbytes (8 blocks)
       *     - Read-only partition -> 4096 Bytes (1 block)
       *     - Main partition for FAT -> the rest, i.e. 12 MBytes
       *        (3072 blocks for 16 MB flash chips)
       */

      static constexpr std::size_t fifo_size = 768;
      static constexpr std::size_t log_size = 247;
      static constexpr std::size_t config_size = 8;
      static constexpr std::size_t ro_size = 1;
      std::size_t fat_size = bks
          - (fifo_size + log_size + config_size + ro_size);

      // configure the partitions
      fat.configure (0, fat_size);

      fifo.configure (fat_size, fifo_size);
      p_config.configure (fat_size + fifo_size + log_size, config_size);
      ro.configure (fat_size + fifo_size + log_size + config_size, ro_size);
      logp.configure (fat_size + fifo_size, log_size);
    }
}
#endif

#if FILE_SYSTEM_TEST == true

uint8_t buff[4096 + 10];

int
test_ff ()
{
  int rc;

  /* Check function/compatibility of the physical drive #0 */
  rc = test_diskio (&flash, 3, (DWORD*) buff, sizeof buff);
  if (rc)
    {
      printf (
          "Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not work with this disk driver.\n",
          rc);
    }
  else
    {
      printf ("Congratulations! The disk driver works well.\n");
    }

  return rc;
}

static DWORD
pn ( /* Pseudo random number generator */
DWORD pns /* 0:Initialize, !0:Read */
)
{
  static DWORD lfsr;
  UINT n;

  if (pns)
    {
      lfsr = pns;
      for (n = 0; n < 32; n++)
        pn (0);
    }
  if (lfsr & 1)
    {
      lfsr >>= 1;
      lfsr ^= 0x80200003;
    }
  else
    {
      lfsr >>= 1;
    }
  return lfsr;
}

int
test_diskio (PDRV pdrv, /* Physical drive number to be checked (all data on the drive will be lost) */
             UINT ncyc, /* Number of test cycles */
             DWORD* buf, /* Pointer to the working buffer */
             UINT sz_buff /* Size of the working buffer in unit of byte */
             )
{
  UINT n, cc, ns;
  DWORD sz_drv, lba, lba2, sz_eblk, pns = 1;
  WORD sz_sect;
  BYTE *pbuff = (BYTE*) buf;
  DSTATUS ds;
  DRESULT dr;

  printf ("test_diskio(%p, %u, 0x%08X, 0x%08X)\n", pdrv, ncyc, (UINT) buff,
          sz_buff);

  if (sz_buff < FF_MAX_SS + 4)
    {
      printf ("Insufficient work area to run program.\n");
      return 1;
    }

  for (cc = 1; cc <= ncyc; cc++)
    {
      printf ("**** Test cycle %u of %u start ****\n", cc, ncyc);

      printf (" disk_initalize(%p)", pdrv);
      ds = disk_initialize (pdrv);
      if (ds & STA_NOINIT)
        {
          printf (" - failed.\n");
          return 2;
        }
      else
        {
          printf (" - ok.\n");
        }

      printf ("**** Get drive size ****\n");
      printf (" disk_ioctl(%p, GET_SECTOR_COUNT, 0x%08X)", pdrv,
              (UINT) &sz_drv);
      sz_drv = 0;
      dr = disk_ioctl (pdrv, GET_SECTOR_COUNT, &sz_drv);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 3;
        }
      if (sz_drv < 12)
        {
          printf ("Failed: Insufficient drive size to test.\n");
          return 4;
        }
      printf (" Number of sectors on the drive %p is %lu.\n", pdrv, sz_drv);

#if FF_MAX_SS != FF_MIN_SS
      printf("**** Get sector size ****\n");
      printf(" disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (UINT)&sz_sect);
      sz_sect = 0;
      dr = disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
      if (dr == RES_OK)
        {
          printf(" - ok.\n");
        }
      else
        {
          printf(" - failed.\n");
          return 5;
        }
      printf(" Size of sector is %u bytes.\n", sz_sect);
#else
      sz_sect = FF_MAX_SS;
#endif

      printf ("**** Get block size ****\n");
      printf (" disk_ioctl(%p, GET_BLOCK_SIZE, 0x%X)", pdrv, (UINT) &sz_eblk);
      sz_eblk = 0;
      dr = disk_ioctl (pdrv, GET_BLOCK_SIZE, &sz_eblk);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
        }
      if (dr == RES_OK || sz_eblk >= 2)
        {
          printf (" Size of the erase block is %lu sectors.\n", sz_eblk);
        }
      else
        {
          printf (" Size of the erase block is unknown.\n");
        }

      /* Single sector write test */
      printf ("**** Single sector write test 1 ****\n");
      lba = 0;
      for (n = 0, pn (pns); n < sz_sect; n++)
        pbuff[n] = (BYTE) pn (0);
      printf (" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT) pbuff, lba);
      dr = disk_write (pdrv, pbuff, lba, 1);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 6;
        }
      printf (" disk_ioctl(%p, CTRL_SYNC, NULL)", pdrv);
      dr = disk_ioctl (pdrv, CTRL_SYNC, 0);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 7;
        }
      memset (pbuff, 0, sz_sect);
      printf (" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT) pbuff, lba);
      dr = disk_read (pdrv, pbuff, lba, 1);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 8;
        }
      for (n = 0, pn (pns); n < sz_sect && pbuff[n] == (BYTE) pn (0); n++)
        ;
      if (n == sz_sect)
        {
          printf (" Data matched.\n");
        }
      else
        {
          printf ("Failed: Read data differs from the data written.\n");
          return 10;
        }
      pns++;

      printf ("**** Multiple sector write test ****\n");
      lba = 1;
      ns = sz_buff / sz_sect;
      if (ns > 4)
        ns = 4;
      for (n = 0, pn (pns); n < (UINT) (sz_sect * ns); n++)
        pbuff[n] = (BYTE) pn (0);
      printf (" disk_write(%p, 0x%X, %lu, %u)", pdrv, (UINT) pbuff, lba, ns);
      dr = disk_write (pdrv, pbuff, lba, ns);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 11;
        }
      printf (" disk_ioctl(%p, CTRL_SYNC, NULL)", pdrv);
      dr = disk_ioctl (pdrv, CTRL_SYNC, 0);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 12;
        }
      memset (pbuff, 0, sz_sect * ns);
      printf (" disk_read(%p, 0x%X, %lu, %u)", pdrv, (UINT) pbuff, lba, ns);
      dr = disk_read (pdrv, pbuff, lba, ns);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 13;
        }
      for (n = 0, pn (pns);
          n < (UINT) (sz_sect * ns) && pbuff[n] == (BYTE) pn (0); n++)
        ;
      if (n == (UINT) (sz_sect * ns))
        {
          printf (" Data matched.\n");
        }
      else
        {
          printf ("Failed: Read data differs from the data written.\n");
          return 14;
        }
      pns++;

      printf ("**** Single sector write test (misaligned address) ****\n");
      lba = 5;
      for (n = 0, pn (pns); n < sz_sect; n++)
        pbuff[n + 3] = (BYTE) pn (0);
      printf (" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT) (pbuff + 3), lba);
      dr = disk_write (pdrv, pbuff + 3, lba, 1);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 15;
        }
      printf (" disk_ioctl(%p, CTRL_SYNC, NULL)", pdrv);
      dr = disk_ioctl (pdrv, CTRL_SYNC, 0);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 16;
        }
      memset (pbuff + 5, 0, sz_sect);
      printf (" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT) (pbuff + 5), lba);
      dr = disk_read (pdrv, pbuff + 5, lba, 1);
      if (dr == RES_OK)
        {
          printf (" - ok.\n");
        }
      else
        {
          printf (" - failed.\n");
          return 17;
        }
      for (n = 0, pn (pns); n < sz_sect && pbuff[n + 5] == (BYTE) pn (0); n++)
        ;
      if (n == sz_sect)
        {
          printf (" Data matched.\n");
        }
      else
        {
          printf ("Failed: Read data differs from the data written.\n");
          return 18;
        }
      pns++;

      printf ("**** 4GB barrier test ****\n");
      if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2))
        {
          lba = 6;
          lba2 = lba + 0x80000000 / (sz_sect / 2);
          for (n = 0, pn (pns); n < (UINT) (sz_sect * 2); n++)
            pbuff[n] = (BYTE) pn (0);
          printf (" disk_write(%p, 0x%X, %lu, 1)", pdrv, (UINT) pbuff, lba);
          dr = disk_write (pdrv, pbuff, lba, 1);
          if (dr == RES_OK)
            {
              printf (" - ok.\n");
            }
          else
            {
              printf (" - failed.\n");
              return 19;
            }
          printf (" disk_write(%p, 0x%X, %lu, 1)", pdrv,
                  (UINT) (pbuff + sz_sect), lba2);
          dr = disk_write (pdrv, pbuff + sz_sect, lba2, 1);
          if (dr == RES_OK)
            {
              printf (" - ok.\n");
            }
          else
            {
              printf (" - failed.\n");
              return 20;
            }
          printf (" disk_ioctl(%p, CTRL_SYNC, NULL)", pdrv);
          dr = disk_ioctl (pdrv, CTRL_SYNC, 0);
          if (dr == RES_OK)
            {
              printf (" - ok.\n");
            }
          else
            {
              printf (" - failed.\n");
              return 21;
            }
          memset (pbuff, 0, sz_sect * 2);
          printf (" disk_read(%p, 0x%X, %lu, 1)", pdrv, (UINT) pbuff, lba);
          dr = disk_read (pdrv, pbuff, lba, 1);
          if (dr == RES_OK)
            {
              printf (" - ok.\n");
            }
          else
            {
              printf (" - failed.\n");
              return 22;
            }
          printf (" disk_read(%p, 0x%X, %lu, 1)", pdrv,
                  (UINT) (pbuff + sz_sect), lba2);
          dr = disk_read (pdrv, pbuff + sz_sect, lba2, 1);
          if (dr == RES_OK)
            {
              printf (" - ok.\n");
            }
          else
            {
              printf (" - failed.\n");
              return 23;
            }
          for (n = 0, pn (pns);
              pbuff[n] == (BYTE) pn (0) && n < (UINT) (sz_sect * 2); n++)
            ;
          if (n == (UINT) (sz_sect * 2))
            {
              printf (" Data matched.\n");
            }
          else
            {
              printf ("Failed: Read data differs from the data written.\n");
              return 24;
            }
        }
      else
        {
          printf (" Test skipped.\n");
        }
      pns++;

      printf (" disk_deinitalize(%p)", pdrv);
      ds = disk_deinitialize (pdrv);
      if (ds & STA_NOINIT)
        {
          printf (" - failed.\n");
          return 2;
        }
      else
        {
          printf (" - ok.\n");
        }

      printf ("**** Test cycle %u of %u completed ****\n\n", cc, ncyc);
    }

  return 0;
}

#endif // FILE_SYSTEM_TEST

#endif // FS_ENABLED
