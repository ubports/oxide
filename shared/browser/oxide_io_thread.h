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

#ifndef _OXIDE_SHARED_BROWSER_IO_THREAD_H_
#define _OXIDE_SHARED_BROWSER_IO_THREAD_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/non_thread_safe.h"
#include "content/public/browser/browser_thread_delegate.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace net {

class CertVerifier;
class CTPolicyEnforcer;
class CTVerifier;
class HostResolver;
class HttpAuthHandlerFactory;
class NetLog;
class ProxyService;
class URLRequestContextGetter;
class URLRequestThrottlerManager;

} // namespace net

namespace oxide {

class URLRequestContext;
class URLRequestContextGetter;

// This object manages the lifetime of objects that are tied to the
// IO thread
class IOThread final : public content::BrowserThreadDelegate {
 public:

  class Globals final : public base::NonThreadSafe {
   public:
    net::HostResolver* host_resolver() const;
    net::CertVerifier* cert_verifier() const;
    net::HttpAuthHandlerFactory* http_auth_handler_factory() const;
    net::ProxyService* proxy_service() const;
    net::URLRequestThrottlerManager* throttler_manager() const;
    net::CTVerifier* cert_transparency_verifier() const;
    net::CTPolicyEnforcer* ct_policy_enforcer() const;

    URLRequestContext* system_request_context() const;

   private:
    friend class IOThread;

    Globals();
    ~Globals();

    // host_resolver_ needs to outlive http_auth_handler_factory_
    std::unique_ptr<net::HostResolver> host_resolver_;
    std::unique_ptr<net::CertVerifier> cert_verifier_;
    std::unique_ptr<net::HttpAuthHandlerFactory> http_auth_handler_factory_;
    std::unique_ptr<net::ProxyService> proxy_service_;
    std::unique_ptr<net::URLRequestThrottlerManager> throttler_manager_;
    std::unique_ptr<net::CTVerifier> cert_transparency_verifier_;
    std::unique_ptr<net::CTPolicyEnforcer> ct_policy_enforcer_;

    std::unique_ptr<URLRequestContext> system_request_context_;

    DISALLOW_COPY_AND_ASSIGN(Globals);
  };

  static IOThread* instance();

  IOThread();
  ~IOThread();

  net::NetLog* net_log() const;
  Globals* globals() const;

  net::URLRequestContextGetter* GetSystemURLRequestContext();

 private:
  void InitSystemRequestContext();
  void InitSystemRequestContextOnIOThread();

  // Called on the IO thread
  void Init() final;
  void CleanUp() final;

  std::unique_ptr<net::NetLog> net_log_;
  Globals* globals_;
  scoped_refptr<URLRequestContextGetter> system_request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(IOThread);
};

} // namespace oxide

#endif // _OXIDE_SHARED_BROWSER_IO_THREAD_H_
