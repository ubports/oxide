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

#ifndef _OXIDE_QT_CORE_GLUE_PRIVATE_OUTGOING_MESSAGE_REQUEST_ADAPTER_H_
#define _OXIDE_QT_CORE_GLUE_PRIVATE_OUTGOING_MESSAGE_REQUEST_ADAPTER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"

#include "shared/browser/oxide_outgoing_message_request.h"

namespace oxide {
namespace qt {

class OutgoingMessageRequestAdapter;

class OutgoingMessageRequestAdapterPrivate FINAL {
 public:
  static OutgoingMessageRequestAdapterPrivate* Create(
      OutgoingMessageRequestAdapter* adapter);

  oxide::OutgoingMessageRequest& request() {
    return request_;
  }

  base::WeakPtr<OutgoingMessageRequestAdapter> GetWeakPtr();

 private:
  OutgoingMessageRequestAdapterPrivate(OutgoingMessageRequestAdapter* adapter);

  oxide::OutgoingMessageRequest request_;
  base::WeakPtrFactory<OutgoingMessageRequestAdapter> weak_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(OutgoingMessageRequestAdapterPrivate);
};

} // namespace qt
} // namespace oxide

#endif // _OXIDE_QT_CORE_GLUE_PRIVATE_OUTGOING_MESSAGE_REQUEST_ADAPTER_H_
