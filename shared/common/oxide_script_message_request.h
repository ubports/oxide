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

#ifndef _OXIDE_SHARED_COMMON_SCRIPT_MESSAGE_REQUEST_H_
#define _OXIDE_SHARED_COMMON_SCRIPT_MESSAGE_REQUEST_H_

#include <string>

#include "base/macros.h"

#include "shared/common/oxide_script_message_params.h"

namespace base {
class ListValue;
class Value;
}

namespace oxide {

class ScriptMessageRequest {
 public:
  virtual ~ScriptMessageRequest();

  int serial() const { return serial_; }

  bool IsWaitingForResponse() const;
  void OnReceiveResponse(base::ListValue* wrapped_payload,
                         ScriptMessageParams::Error error);

 protected:
  ScriptMessageRequest(int serial);

 private:
  virtual void OnReply(const base::Value& payload) = 0;
  virtual void OnError(ScriptMessageParams::Error error,
                       const base::Value& payload) = 0;

  int serial_;
  bool has_received_response_;

  DISALLOW_COPY_AND_ASSIGN(ScriptMessageRequest);
};

} // namespace oxide

#endif // _OXIDE_SHARED_COMMON_SCRIPT_MESSAGE_REQUEST_H_
