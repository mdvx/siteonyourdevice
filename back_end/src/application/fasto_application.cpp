/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "application/fasto_application.h"

#include <stdlib.h>

#include <string>

#include "common/file_system.h"

#if defined(BUILD_CONSOLE)
  #include "fasto_remote_application.h"
#else
  #if defined(OS_WIN)
      #include "platform/windows/gui_fasto_application.h"
  #elif defined(OS_MACOSX)
      #include "platform/macosx/gui_fasto_application.h"
  #elif defined(OS_ANDROID)
      #include "fasto_remote_application.h"
  #else
      #include "platform/linux/gui_fasto_application.h"
  #endif
#endif

namespace {

fasto::siteonyourdevice::application::IFastoApplicationImpl* createImpl(int argc, char *argv[]) {
#if defined(BUILD_CONSOLE)
  return new fasto::siteonyourdevice::application::FastoRemoteApplication(argc, argv);
#else
#if defined(OS_WIN)
  return new fasto::siteonyourdevice::application::WinGuiFastoRemoteApplication(argc, argv);
#elif defined(OS_MACOSX)
  return new fasto::siteonyourdevice::application::MacOSXGuiFastoRemoteApplication(argc, argv);
#elif defined(OS_ANDROID)
  return new fasto::siteonyourdevice::application::FastoRemoteApplication(argc, argv);
#else
  return new fasto::siteonyourdevice::application::GtkGuiFastoRemoteApplication(argc, argv);
#endif
#endif
}

}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace application {

IFastoApplicationImpl::IFastoApplicationImpl(int argc, char *argv[]) {
}

IFastoApplicationImpl::~IFastoApplicationImpl() {
}

FastoApplication* FastoApplication::self_ = NULL;

FastoApplication::FastoApplication(int argc, char *argv[])
  : argc_(argc), argv_(argv), impl_(createImpl(argc, argv)) {
  CHECK(!self_);
  if (!self_) {
      self_ = this;
  }
}

int FastoApplication::argc() const {
  return argc_;
}

char **FastoApplication::argv() const {
  return argv_;
}

FastoApplication::~FastoApplication() {
  self_ = NULL;
}

FastoApplication *FastoApplication::instance() {
  return self_;
}

std::string FastoApplication::appPath() const {
  return argv_[0];
}

std::string FastoApplication::appDir() const {
#ifdef OS_MACOSX
  const std::string appP = common::file_system::pwd();
#else
  const std::string appP = appPath();
#endif
  return common::file_system::get_dir_path(appP);
}

int FastoApplication::exec() {
  int res = impl_->preExec();
  if (res == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  res = impl_->exec();
  if (res == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }
  return impl_->postExec();
}

void FastoApplication::exit(int result) {
  if (!self_) {
    return;
  }

  self_->impl_->exit(result);
}

}  // namespace application
}  // namespace siteonyourdevice
}  // namespace fasto
