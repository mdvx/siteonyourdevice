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

#include <common/file_system.h>

#include "fasto_remote_application.h"

namespace {

fasto::siteonyourdevice::application::IFastoApplicationImpl* createImpl(int argc, char* argv[]) {
  return new fasto::siteonyourdevice::application::FastoRemoteApplication(argc, argv);
}

}  // namespace

namespace fasto {
namespace siteonyourdevice {
namespace application {

IFastoApplicationImpl::IFastoApplicationImpl(int argc, char* argv[]) {}

IFastoApplicationImpl::~IFastoApplicationImpl() {}

FastoApplication* FastoApplication::self_ = nullptr;

FastoApplication::FastoApplication(int argc, char* argv[])
    : argc_(argc), argv_(argv), impl_(createImpl(argc, argv)) {
  CHECK(!self_);
  if (!self_) {
    self_ = this;
  }
}

int FastoApplication::argc() const {
  return argc_;
}

char** FastoApplication::argv() const {
  return argv_;
}

FastoApplication::~FastoApplication() {
  self_ = nullptr;
}

FastoApplication* FastoApplication::instance() {
  return self_;
}

std::string FastoApplication::appPath() const {
  return argv_[0];
}

std::string FastoApplication::appDir() const {
#ifdef OS_MACOSX
  std::string appP = common::file_system::pwd();
#else
  std::string appP = appPath();
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
