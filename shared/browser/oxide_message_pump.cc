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

#include "oxide_message_pump.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/threading/thread_local.h"

namespace oxide {

namespace {

base::LazyInstance<base::ThreadLocalPointer<MessagePump>>::Leaky g_lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

}

// static
MessagePump* MessagePump::Get() {
  return g_lazy_tls.Pointer()->Get();
}

MessagePump::MessagePump() {
  CHECK(!Get());
  g_lazy_tls.Pointer()->Set(this);
}

MessagePump::~MessagePump() {
  g_lazy_tls.Pointer()->Set(nullptr);
}

void MessagePump::Start() {
  CHECK(!base::MessageLoop::current()->is_running()) <<
      "Called Start() more than once or whilst inside a RunLoop";

  run_loop_.Enter();
  OnStart();
}

void MessagePump::Stop() {
  if (!base::MessageLoop::current()->is_running()) {
    return;
  }

  run_loop_.Exit();
}

} // namespace oxide
