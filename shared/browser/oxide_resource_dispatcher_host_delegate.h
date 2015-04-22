// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2014-2015 Canonical Ltd.

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

#ifndef _OXIDE_SHARED_BROWSER_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define _OXIDE_SHARED_BROWSER_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include "base/macros.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace content {
class ResourceContext;
struct Referrer;
}

namespace net {
class URLRequest;
class CookieStore;
}

namespace oxide {

class ResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  ResourceDispatcherHostDelegate();
  ~ResourceDispatcherHostDelegate() override;

 private:
  struct DownloadRequestParams;

  void DispatchDownloadRequest(const GURL& url,
                               const GURL& first_party_url,
                               bool is_content_initiated,
                               const base::string16& suggested_name,
                               const bool use_prompt,
                               const content::Referrer& referrer,
                               const std::string& mime_type,
                               int render_process_id,
                               int render_view_id,
                               content::ResourceContext* resource_context);

  static void DispatchDownloadRequestWithCookies(
      const DownloadRequestParams& params,
      const std::string& cookies);

  net::CookieStore* GetCookieStoreForContext(
      content::ResourceContext* resource_context);

  // content::ResourceDispatcherHostDelegate implementation
  void RequestBeginning(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::AppCacheService* appcache_service,
      content::ResourceType resource_type,
      ScopedVector<content::ResourceThrottle>* throttles) override;
  bool HandleExternalProtocol(const GURL& url,
                              int child_id,
                              int route_id) override;
  bool ShouldDownloadUrl(const GURL& url,
                         const GURL& first_party_url,
                         bool is_content_initiated,
                         const base::string16& suggested_name,
                         const bool use_prompt,
                         const content::Referrer& referrer,
                         const std::string& mime_type,
                         int render_process_id,
                         int render_view_id,
                         content::ResourceContext* resource_context) override;

  DISALLOW_COPY_AND_ASSIGN(ResourceDispatcherHostDelegate);
};

} // namespace oxide

#endif // OXIDE_SHARED_BROWSER_RESOURCE_DISPATCHER_HOST_DELEGATE

