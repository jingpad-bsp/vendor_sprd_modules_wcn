//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "bluetooth_address.h"

#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utils/Log.h>
#include <sys/time.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

void BluetoothAddress::bytes_to_string(const uint8_t* addr, char* addr_str) {
  sprintf(addr_str, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
          addr[3], addr[4], addr[5]);
}

bool BluetoothAddress::string_to_bytes(const char* addr_str, uint8_t* addr) {
  if (addr_str == NULL) return false;
  if (strnlen(addr_str, kStringLength) != kStringLength) return false;
  unsigned char trailing_char = '\0';

  return (sscanf(addr_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx%1c",
                 &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5],
                 &trailing_char) == kBytes);
}

bool BluetoothAddress::generate_local_address(uint8_t* local_addr, char* file_name) {
  uint8_t bdaddr[kBytes] = {0x40, 0x45, 0xDA, 0, 0, 0};
  char bdstring[19] = {0};
  int fd, ret, randseed, mac_rd;

  ALOGD("%s", __func__);

  fd = open("/dev/urandom", O_RDONLY);

  if (fd < 0){
    ALOGE("%s: open urandom fail", __func__);
  } else {
    ret = read(fd, &randseed, sizeof(randseed));
    ALOGE("%s urandom:0x%08X", __func__, randseed);
    close(fd);
  }

  if (fd < 0 || ret < 0) {
    struct timeval tt;
    if (gettimeofday(&tt, (struct timezone *)0) > 0) {
      randseed = (unsigned int) tt.tv_usec;
    } else {
      randseed = (unsigned int) time(NULL);
    }
    ALOGW("%s urandom fail, using system time for randseed", __func__);
  }

  srand(randseed);
  mac_rd = rand();

  bdaddr[3] = (uint8_t)(mac_rd & 0xFF);
  bdaddr[4] = (uint8_t)((mac_rd >> 8) & 0xFF);
  bdaddr[5] = (uint8_t)((mac_rd >> 16) & 0xFF);

  ALOGI("%s: [%02X:%02X:%02X:%02X:%02X:%02X]",__func__, bdaddr[0], bdaddr[1],
    bdaddr[2], bdaddr[3], bdaddr[4], bdaddr[5]);

  memcpy(local_addr, bdaddr, sizeof(bdaddr));

  fd = open(file_name, O_CREAT|O_RDWR|O_TRUNC, 0664);
  if (fd < 0) {
    ALOGE("%s open %s error: %s", __func__, file_name, strerror(errno));
    return false;
  }

  sprintf(bdstring, "%02x:%02x:%02x:%02x:%02x:%02x\n", bdaddr[0], bdaddr[1], bdaddr[2],
    bdaddr[3], bdaddr[4], bdaddr[5]);
  ret = write(fd, bdstring, sizeof(bdstring));
  if (ret < 0) {
    ALOGI("%s write %s error: %s", __func__, file_name, strerror(errno));
  }
  ret = fsync(fd);
  if (ret < 0) {
    ALOGI("%s fsync %s error: %s", __func__, file_name, strerror(errno));
  }
  close(fd);

  memcpy(local_addr, bdaddr, sizeof(bdaddr));

  return true;
}

bool BluetoothAddress::write_local_address(char* addr_str, char* file_name){
  int fd,ret;
  char address[kStringLength + 1] = {0};
  fd = open(file_name, O_CREAT | O_RDWR , 0664);
  if (fd < 0) {
    ALOGE("%s open %s error: %s", __func__, file_name, strerror(errno));
    return false;
  }
  ret = read(fd, address, kStringLength);
  if (ret < -1) {
    ALOGE("%s: Error reading address from %s: %s", __func__, file_name,
    strerror(errno));
  }
  if (memcmp(addr_str,address,kStringLength) != 0) {
    ALOGD("%s", __func__);
    ret = lseek(fd , 0 , SEEK_SET);
    if (ret < 0) {
      ALOGE("%s lseek %s error: %s", __func__, file_name, strerror(errno));
    }
    ret = write(fd, addr_str, kStringLength + 1);
    if (ret < 0) {
      ALOGE("%s write %s error: %s", __func__, file_name, strerror(errno));
    }
    ret = fsync(fd);
    if (ret < 0) {
      ALOGE("%s fsync %s error: %s", __func__, file_name, strerror(errno));
    }
  }
  close(fd);
  return true;
}

bool BluetoothAddress::get_local_address(uint8_t* local_addr) {
  char property[PROPERTY_VALUE_MAX] = {0};
  int addr_fd,bytes_read;
  // Get local bdaddr storage path from a system property.
  if (property_get(PROPERTY_BT_BDADDR_PATH, property, NULL)) {
    ALOGD("%s: Trying %s", __func__, PRODUCT_BT_BDADDR_PATH);
    char address[kStringLength + 1] = {0};
    addr_fd = open(PRODUCT_BT_BDADDR_PATH, O_RDONLY);
    if (addr_fd != -1) {
      bytes_read = read(addr_fd, address, kStringLength);
      if (bytes_read == -1) {
        ALOGE("%s: Error reading address from %s: %s", __func__, property,
          strerror(errno));
      }
      close(addr_fd);
      address[kStringLength] = '\n';

      //write the address to local bdaddr storage path
      bool addr_ret = write_local_address(address, property);
      if (addr_ret == false) {
        ALOGE("%s: Error writing address to %s: %s", __func__, property,
          strerror(errno));
      }
    } else {
      addr_fd = open(property, O_RDONLY);
      if(addr_fd != -1){
        bytes_read = read(addr_fd, address, kStringLength);
        if (bytes_read == -1) {
          ALOGE("%s: Error reading address from %s: %s", __func__, property,
            strerror(errno));
        }
        close(addr_fd);
      } else {
        return generate_local_address(local_addr, property);
      }
    }

    // Null terminate the string.
    address[kStringLength] = '\0';
    // If the address is not all zeros, then use it.
    const uint8_t zero_bdaddr[kBytes] = {0, 0, 0, 0, 0, 0};
    if ((string_to_bytes(address, local_addr)) &&
        (memcmp(local_addr, zero_bdaddr, kBytes) != 0)) {
      ALOGD("%s: Got Factory BDA %s", __func__, address);
      return true;
    } else {
      ALOGE("%s: Got Invalid BDA '%s' from %s", __func__, address, property);
    }
  }

  // No BDADDR found in the file. Look for BDA in a factory property.
  if (property_get(FACTORY_BDADDR_PROPERTY, property, NULL) &&
    string_to_bytes(property, local_addr)) {
    return true;
  }

  // No factory BDADDR found. Look for a previously stored BDA.
  if (property_get(PERSIST_BDADDR_PROPERTY, property, NULL) &&
    string_to_bytes(property, local_addr)) {
    return true;
  }
  return false;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
