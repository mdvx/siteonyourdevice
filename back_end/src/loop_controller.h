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

#pragma once

#include <memory>

#include <common/threads/thread.h>

#include <common/libev/io_loop.h>

namespace fasto {
namespace siteonyourdevice {

class ILoopController {
 public:
  ILoopController();
  virtual ~ILoopController();

  void start();
  int exec();
  void stop();

 protected:
  common::libev::IoLoop* loop_;
  common::libev::IoLoopObserver* handler_;

 private:
  virtual common::libev::IoLoopObserver* CreateHandler() = 0;
  virtual common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) = 0;
  virtual void started() = 0;
  virtual void stoped() = 0;
};

class ILoopThreadController : public ILoopController {
 public:
  ILoopThreadController();
  virtual ~ILoopThreadController();

  int join();

 private:
  using ILoopController::exec;

  virtual common::libev::IoLoopObserver* CreateHandler() override = 0;
  virtual common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) override = 0;

  virtual void started() override;
  virtual void stoped() override;

  std::shared_ptr<common::threads::Thread<int>> loop_thread_;
};

}  // namespace siteonyourdevice
}  // namespace fasto
