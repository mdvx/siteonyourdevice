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

#include <common/smart_ptr.h>
#include <common/thread/thread.h>

namespace fasto {
namespace siteonyourdevice {

namespace tcp {
class ITcpLoop;
class ITcpLoopObserver;
}

class ILoopController {
 public:
  ILoopController();
  virtual ~ILoopController();

  void start();
  int exec();
  void stop();

 protected:
  tcp::ITcpLoop* loop_;
  tcp::ITcpLoopObserver* handler_;

 private:
  virtual tcp::ITcpLoopObserver* createHandler() = 0;
  virtual tcp::ITcpLoop* createServer(tcp::ITcpLoopObserver* handler) = 0;
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

  virtual tcp::ITcpLoopObserver* createHandler() = 0;
  virtual tcp::ITcpLoop* createServer(tcp::ITcpLoopObserver* handler) = 0;

  virtual void started();
  virtual void stoped();

  common::shared_ptr<common::thread::Thread<int> > loop_thread_;
};

}  // namespace siteonyourdevice
}  // namespace fasto
