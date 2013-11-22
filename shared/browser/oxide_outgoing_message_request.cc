// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2013 Canonical Ltd.

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

#include "oxide_outgoing_message_request.h"

namespace oxide {

OutgoingMessageRequest::OutgoingMessageRequest() :
    serial_(-1),
    had_response_(false) {}

bool OutgoingMessageRequest::IsWaiting() const {
  return !had_response_;
}

void OutgoingMessageRequest::SetReplyCallback(
    const ReplyCallback& callback) {
  reply_callback_ = callback;
}

void OutgoingMessageRequest::SetErrorCallback(
    const ErrorCallback& callback) {
  error_callback_ = callback;
}

void OutgoingMessageRequest::OnReceiveResponse(
    const std::string& args,
    OxideMsg_SendMessage_Error::Value error) {
  if (had_response_) {
    return;
  }

  had_response_ = true;

  if (error != OxideMsg_SendMessage_Error::OK) {
    if (!error_callback_.is_null()) {
      error_callback_.Run(error, args);
    }
  } else {
    if (!reply_callback_.is_null()) {
      reply_callback_.Run(args);
    }
  }
}

} // namespace oxide
