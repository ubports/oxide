// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2014 Canonical Ltd.

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

#include "oxide_url_request_delegated_job.h"

#include "base/logging.h"

namespace oxide {

void URLRequestDelegatedJob::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  DCHECK(!started_);
  extra_request_headers_.CopyFrom(headers);
}

void URLRequestDelegatedJob::SetPriority(net::RequestPriority priority) {
  priority_ = priority;
}

void URLRequestDelegatedJob::Start() {
  DCHECK(!started_);
  DCHECK(request());

  started_ = true;
  OnStart();
}

bool URLRequestDelegatedJob::GetMimeType(std::string* mime_type) const {
  if (mime_type_.empty()) {
    return false;
  }

  *mime_type = mime_type_;
  return true;
}

URLRequestDelegatedJob::URLRequestDelegatedJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate),
      started_(false),
      priority_(net::IDLE) {}

URLRequestDelegatedJob::~URLRequestDelegatedJob() {}

} // namespace oxide
