// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2013-2015 Canonical Ltd.

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef _OXIDE_SHARED_BROWSER_MESSAGE_PUMP_H_
#define _OXIDE_SHARED_BROWSER_MESSAGE_PUMP_H_

#include "base/macros.h"
#include "base/message_loop/message_pump.h"

#include "shared/browser/oxide_platform_run_loop.h"
#include "shared/common/oxide_shared_export.h"

namespace oxide {

class OXIDE_SHARED_EXPORT MessagePump : public base::MessagePump {
 public:
  static MessagePump* Get();

  MessagePump();
  virtual ~MessagePump();

  void Start();
  void Stop();

 private:
  virtual void OnStart() = 0;

  PlatformRunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(MessagePump);
};

} // namespace oxide

#endif // _OXIDE_SHARED_BROWSER_MESSAGE_PUMP_H_
