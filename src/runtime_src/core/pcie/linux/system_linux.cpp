/**
 * Copyright (C) 2019 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "system_linux.h"
#include "device_linux.h"
#include "gen/version.h"
#include "scan.h"
#include "core/common/time.h"

#include <boost/property_tree/ini_parser.hpp>

#include <fstream>
#include <memory>
#include <vector>
#include <map>

#include <sys/utsname.h>
#include <gnu/libc-version.h>

namespace {

// Singleton registers with base class xrt_core::system
// during static global initialization.  If statically
// linking with libxrt_core, then explicit initialiation
// is required
static xrt_core::system_linux*
singleton_instance()
{
  static xrt_core::system_linux singleton;
  return &singleton;
}

// Dynamic linking automatically constructs the singleton
struct X
{
  X() { singleton_instance(); }
} x;

static std::string
driver_version(const std::string& driver)
{
  std::string line("unknown");
  std::string path("/sys/module/");
  path += driver;
  path += "/version";
  std::ifstream ver(path);
  if (ver.is_open()) {
    getline(ver, line);
  }

  return line;
}

static std::vector<std::weak_ptr<xrt_core::device_linux>> mgmtpf_devices(16); // fix size
static std::vector<std::weak_ptr<xrt_core::device_linux>> userpf_devices(16); // fix size
static std::map<xrt_core::device::handle_type, std::weak_ptr<xrt_core::device_linux>> userpf_device_map;

}

namespace xrt_core {


void 
system_linux::
get_xrt_info(boost::property_tree::ptree &pt)
{
  pt.put("version",   xrt_build_version);
  pt.put("hash",      xrt_build_version_hash);
  pt.put("date",      xrt_build_version_date);
  pt.put("branch",    xrt_build_version_branch);
  pt.put("xocl",      driver_version("xocl"));
  pt.put("xclmgmt",   driver_version("xclmgmt"));
}


void 
system_linux::
get_os_info(boost::property_tree::ptree &pt)
{
  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    pt.put("sysname",   sysinfo.sysname);
    pt.put("release",   sysinfo.release);
    pt.put("version",   sysinfo.version);
    pt.put("machine",   sysinfo.machine);
  }

  pt.put("glibc", gnu_get_libc_version());

  // The file is a requirement as per latest Linux standards
  // https://www.freedesktop.org/software/systemd/man/os-release.html
  std::ifstream ifs("/etc/os-release");
  if ( ifs.good() ) {
      boost::property_tree::ptree opt;
      boost::property_tree::ini_parser::read_ini(ifs, opt);
      std::string val = opt.get<std::string>("PRETTY_NAME", "");
      if (val.length()) {
          if ((val.front() == '"') && (val.back() == '"')) {
              val.erase(0, 1);
              val.erase(val.size()-1);
          }
          pt.put("linux", val);
      }
      ifs.close();
  }

  pt.put("now", xrt_core::timestamp());
}

std::pair<device::id_type, device::id_type>
system_linux::
get_total_devices(bool is_user) const
{
  return std::make_pair(pcidev::get_dev_total(is_user), pcidev::get_dev_ready(is_user));
}

void
system_linux::
scan_devices(bool verbose, bool json) const
{
  std::cout << "TO-DO: scan_devices\n";
  verbose = verbose;
  json = json;
}

std::shared_ptr<device>
system_linux::
get_userpf_device(device::id_type id) const
{
  // deliberately not using std::make_shared (used with weak_ptr)
  return std::shared_ptr<device_linux>(new device_linux(id,true));
}

std::shared_ptr<device>
system_linux::
get_userpf_device(device::handle_type handle, device::id_type id) const
{
  // deliberately not using std::make_shared (used with weak_ptr)
  return std::shared_ptr<device_linux>(new device_linux(handle, id));
}

std::shared_ptr<device>
system_linux::
get_mgmtpf_device(device::id_type id) const
{
  // deliberately not using std::make_shared (used with weak_ptr)
  return std::shared_ptr<device_linux>(new device_linux(id,false));
}

namespace pcie_linux {

std::shared_ptr<device>
get_userpf_device(device::handle_type device_handle, device::id_type id)
{
  singleton_instance(); // force loading if necessary
  return xrt_core::get_userpf_device(device_handle, id);
}

} // pcie_linux

} // xrt_core
