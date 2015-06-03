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

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/browser/resource_dispatcher_host_login_delegate.h"

namespace content {
class ResourceContext;
class ResourceThrottle;
struct Referrer;
}

namespace net {
class URLRequest;
class CookieStore;
}

namespace oxide {

class ResourceDispatcherHostDelegate;
class WebView;

class ResourceDispatcherHostLoginDelegate
    : public content::ResourceDispatcherHostLoginDelegate {
 public:
    ResourceDispatcherHostLoginDelegate(net::AuthChallengeInfo* auth_info,
                                        net::URLRequest* request);
    ~ResourceDispatcherHostLoginDelegate() override;
    void OnRequestCancelled() override;

    void Deny();
    void Allow(std::string username, std::string password);

    void SetCancelledCallback(const base::Closure& cancelled_callback);

private:
    friend class ResourceDispatcherHostDelegate;
    void DispatchRequest();
    WebView* GetWebView(net::URLRequest* request);

    net::URLRequest* request_;
    base::Closure cancelled_callback_;
};

class ResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:

  ResourceDispatcherHostDelegate();
  ~ResourceDispatcherHostDelegate() override;

  void DownloadStarting(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      int child_id,
      int route_id,
      int request_id,
      bool is_content_initiated,
      bool must_download,
      const std::string& suggested_filename,
      ScopedVector<content::ResourceThrottle>* throttles) override;

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
                               content::ResourceContext* resource_context,
                               net::URLRequest* url_request);

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
                              int route_id,
                              bool is_main_frame,
                              ui::PageTransition page_transition,
                              bool has_user_gesture) override;

  content::ResourceDispatcherHostLoginDelegate* CreateLoginDelegate(
      net::AuthChallengeInfo* auth_info,
      net::URLRequest* request) override;

private:
  DISALLOW_COPY_AND_ASSIGN(ResourceDispatcherHostDelegate);
};

} // namespace oxide

#endif // OXIDE_SHARED_BROWSER_RESOURCE_DISPATCHER_HOST_DELEGATE

