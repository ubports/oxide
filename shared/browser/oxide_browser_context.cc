// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2013-2016 Canonical Ltd.

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

#include "oxide_browser_context.h"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>
#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/synchronization/lock.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/worker_pool.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h" // nogncheck
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/host_mapping_rules.h"
#include "net/base/net_errors.h"
#include "net/cookies/cookie_monster.h"
#include "net/extras/sqlite/sqlite_channel_id_store.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/transport_security_persister.h"
#include "net/http/transport_security_state.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/channel_id_store.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"

#include "shared/browser/net/oxide_cookie_store_proxy.h"
#include "shared/browser/permissions/oxide_permission_manager.h"
#include "shared/browser/permissions/oxide_temporary_saved_permission_context.h"
#include "shared/browser/ssl/oxide_ssl_config_service.h"
#include "shared/browser/ssl/oxide_ssl_host_state_delegate.h"
#include "shared/common/oxide_constants.h"
#include "shared/common/oxide_content_client.h"

#include "oxide_browser_context_delegate.h"
#include "oxide_browser_context_destroyer.h"
#include "oxide_browser_process_main.h"
#include "oxide_download_manager_delegate.h"
#include "oxide_http_user_agent_settings.h"
#include "oxide_io_thread.h"
#include "oxide_network_delegate.h"
#include "oxide_url_request_context.h"
#include "oxide_url_request_delegated_job_factory.h"
#include "oxide_user_agent_settings.h"

namespace oxide {

namespace {

base::LazyInstance<std::set<BrowserContext*>>::Leaky g_all_contexts;

// Cache was used for the default blockfile backend (CACHE_BACKEND_BLOCKFILE),
// Cache2 is used since the switch to the simple backend (CACHE_BACKEND_SIMPLE).
const base::FilePath::CharType kCacheDirname[] = FILE_PATH_LITERAL("Cache");
const base::FilePath::CharType kCacheDirname2[] = FILE_PATH_LITERAL("Cache2");

const base::FilePath::CharType kCookiesFilename[] =
    FILE_PATH_LITERAL("cookies.sqlite");
const base::FilePath::CharType kChannelIDFilename[] =
    FILE_PATH_LITERAL("ChannelID");

const char kDataScheme[] = "data";
const char kFileScheme[] = "file";
const char kFtpScheme[] = "ftp";

void CleanupOldCacheDir(const base::FilePath& path) {
  if (!base::DirectoryExists(path)) {
    return;
  }

  base::FileEnumerator traversal(
      path, false,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  for (base::FilePath current = traversal.Next(); !current.empty();
       current = traversal.Next()) {
    if (traversal.GetInfo().IsDirectory()) {
      return;
    }

    base::FilePath::StringType name = traversal.GetInfo().GetName().value();
    if (name != FILE_PATH_LITERAL("index") &&
        name.compare(0, 5, FILE_PATH_LITERAL("data_")) != 0 &&
        name.compare(0, 2, FILE_PATH_LITERAL("f_")) != 0) {
      return;
    }
  }

  base::DeleteFile(path, true);
}

} // namespace

class MainURLRequestContextGetter : public URLRequestContextGetter {
 public:
  MainURLRequestContextGetter(
      BrowserContextIOData* context,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) :
      context_(context),
      request_interceptors_(std::move(request_interceptors)) {
    std::swap(protocol_handlers_, *protocol_handlers);
  }

 private:
  // URLRequestContextGetter implementation
  net::URLRequestContext* GetURLRequestContext() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

    if (!url_request_context_) {
      DCHECK(context_);
      url_request_context_ = context_->CreateMainRequestContext(
          protocol_handlers_, std::move(request_interceptors_))->AsWeakPtr();
      context_ = nullptr;
    }

    return url_request_context_.get();
  }

 private:
  BrowserContextIOData* context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;

  base::WeakPtr<URLRequestContext> url_request_context_;
};

class ResourceContext : public content::ResourceContext {
 public:
  ResourceContext(BrowserContextIOData* io_data)
      : io_data_(io_data),
        request_context_(nullptr) {}

 private:
  friend class BrowserContextIOData; // for setting request_context_ and
                                     // accessing io_data_

  // content::ResourceContext implementation
  net::HostResolver* GetHostResolver() override {
    return IOThread::instance()->globals()->host_resolver();
  }

  net::URLRequestContext* GetRequestContext() override {
    CHECK(request_context_);
    return request_context_;
  }

  BrowserContextIOData* io_data_;
  net::URLRequestContext* request_context_;

  DISALLOW_COPY_AND_ASSIGN(ResourceContext);
};

struct BrowserContextSharedIOData {
  BrowserContextSharedIOData(BrowserContextIOData* context,
                             const BrowserContext::Params& params)
      : path(params.path),
        cache_path(params.cache_path),
        max_cache_size_hint(params.max_cache_size_hint),
        cookie_policy(net::StaticCookiePolicy::ALLOW_ALL_COOKIES),
        session_cookie_mode(params.session_cookie_mode),
        popup_blocker_enabled(true),
        host_mapping_rules(params.host_mapping_rules),
        user_agent_settings(new UserAgentSettingsIOData(context)) {}

  mutable base::Lock lock;

  base::FilePath path;
  base::FilePath cache_path;
  int max_cache_size_hint;

  net::StaticCookiePolicy::Type cookie_policy;
  content::CookieStoreConfig::SessionCookieMode session_cookie_mode;
  bool popup_blocker_enabled;

  std::vector<std::string> host_mapping_rules;

  std::unique_ptr<UserAgentSettingsIOData> user_agent_settings;

  scoped_refptr<BrowserContextDelegate> delegate;
};

// static
void BrowserContextDelegateTraits::Destruct(const BrowserContextDelegate* x) {
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    delete x;
    return;
  }

  if (!content::BrowserThread::DeleteSoon(content::BrowserThread::UI,
                                          FROM_HERE, x)) {
    LOG(ERROR) <<
        "BrowserContextDelegate won't be deleted. This could be due to it "
        "being leaked until after Chromium shutdown has begun";
  }
}

class BrowserContextIODataImpl : public BrowserContextIOData {
 public:
  BrowserContextIODataImpl(const BrowserContext::Params& params)
      : data_(this, params) {}

  BrowserContextSharedIOData& GetSharedData() override {
    return data_;
  }
  const BrowserContextSharedIOData& GetSharedData() const override {
    return data_;
  }

 private:
  content::CookieStoreConfig::SessionCookieMode
  GetSessionCookieMode() const override {
    return GetPath().empty() ?
        content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES :
        data_.session_cookie_mode;
  }
 
  bool IsOffTheRecord() const override {
    return false;
  }

  BrowserContextSharedIOData data_;
};

class OTRBrowserContextIODataImpl : public BrowserContextIOData {
 public:
  OTRBrowserContextIODataImpl(BrowserContextIODataImpl* original)
      : original_io_data_(original) {}

 private:
  content::CookieStoreConfig::SessionCookieMode
  GetSessionCookieMode() const override {
    return content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES;
  }

  bool IsOffTheRecord() const override {
    return true;
  }

  BrowserContextSharedIOData& GetSharedData() override {
    return original_io_data_->GetSharedData();
  }
  const BrowserContextSharedIOData& GetSharedData() const override {
    return original_io_data_->GetSharedData();
  }

  BrowserContextIODataImpl* original_io_data_;
};

BrowserContextIOData::BrowserContextIOData()
    : resource_context_(new ResourceContext(this)),
      temporary_saved_permission_context_(
        new TemporarySavedPermissionContext()) {}

BrowserContextIOData::~BrowserContextIOData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

// static
BrowserContextIOData* BrowserContextIOData::FromResourceContext(
    content::ResourceContext* resource_context) {
  return static_cast<ResourceContext*>(resource_context)->io_data_;
}

scoped_refptr<BrowserContextDelegate> BrowserContextIOData::GetDelegate() {
  BrowserContextSharedIOData& data = GetSharedData();
  base::AutoLock lock(data.lock);
  return data.delegate;
}

net::StaticCookiePolicy::Type BrowserContextIOData::GetCookiePolicy() const {
  const BrowserContextSharedIOData& data = GetSharedData();
  base::AutoLock lock(data.lock);
  return data.cookie_policy;
}

base::FilePath BrowserContextIOData::GetPath() const {
  return GetSharedData().path;
}

base::FilePath BrowserContextIOData::GetCachePath() const {
  const BrowserContextSharedIOData& data = GetSharedData();
  if (data.cache_path.empty()) {
    return GetPath();
  }

  return data.cache_path;
}

int BrowserContextIOData::GetMaxCacheSizeHint() const {
  int max_cache_size_hint = GetSharedData().max_cache_size_hint;
  // max_cache_size_hint is expressed in MB, let’s check that
  // converting it to bytes won’t trigger an integer overflow
  static int upper_limit = std::numeric_limits<int>::max() / (1024 * 1024);
  DCHECK_LE(max_cache_size_hint, upper_limit);
  return max_cache_size_hint;
}

URLRequestContext* BrowserContextIOData::CreateMainRequestContext(
    content::ProtocolHandlerMap& protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(!main_request_context_);

  std::unique_ptr<net::CookieStore> cookie_store;
  scoped_refptr<net::SQLiteChannelIDStore> channel_id_store;
  if (!IsOffTheRecord() && !GetPath().empty()) {
    cookie_store =
        content::CreateCookieStore(
            content::CookieStoreConfig(GetPath().Append(kCookiesFilename),
                                       GetSessionCookieMode(),
                                       nullptr, nullptr));
    channel_id_store =
        new net::SQLiteChannelIDStore(
            GetPath().Append(kChannelIDFilename),
            content::BrowserThread::GetBlockingPool()->GetSequencedTaskRunner(
                base::SequencedWorkerPool::GetSequenceToken()));
  } else {
    cookie_store = content::CreateCookieStore(content::CookieStoreConfig());
  }

  cookie_store_owner_->set_store(std::move(cookie_store));

  IOThread::Globals* io_thread_globals = IOThread::instance()->globals();

  ssl_config_service_ = new SSLConfigService();
  http_user_agent_settings_.reset(new HttpUserAgentSettings(this));

  // TODO: We want persistent storage here (for non-incognito), but the
  //       persistent implementation used in Chrome uses the preferences
  //       system, which we don't want. We need our own implementation,
  //       backed either by sqlite or a text file
  http_server_properties_.reset(new net::HttpServerPropertiesImpl());

  network_delegate_.reset(new NetworkDelegate(this));
  transport_security_state_.reset(new net::TransportSecurityState());
  transport_security_persister_.reset(
      new net::TransportSecurityPersister(
        transport_security_state_.get(),
        GetPath(),
        content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::FILE),
        IsOffTheRecord()));

  host_mapping_rules_.reset(new net::HostMappingRules());
  const std::vector<std::string>& host_mapping_rules =
      GetSharedData().host_mapping_rules;
  for (std::vector<std::string>::const_iterator it =
           host_mapping_rules.begin();
       it != host_mapping_rules.end(); ++it) {
    host_mapping_rules_->AddRuleFromString(*it);
  }

  main_request_context_.reset(new URLRequestContext());
  URLRequestContext* context = main_request_context_.get();
  net::URLRequestContextStorage* storage = context->storage();

  storage->set_ssl_config_service(ssl_config_service_.get());
  context->set_network_delegate(network_delegate_.get());
  context->set_http_user_agent_settings(http_user_agent_settings_.get());

  storage->set_channel_id_service(
      base::MakeUnique<net::ChannelIDService>(
          new net::DefaultChannelIDStore(channel_id_store.get())));

  context->set_http_server_properties(http_server_properties_.get());

  context->set_cookie_store(cookie_store_owner_->store());

  context->set_transport_security_state(transport_security_state_.get());

  // Run-once code that is run when upgrading oxide to
  // a version that uses the simple cache backend.
  content::BrowserThread::PostTask(
      content::BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&CleanupOldCacheDir, GetCachePath().Append(kCacheDirname)));

  std::unique_ptr<net::HttpCache::BackendFactory> cache_backend;
  if (IsOffTheRecord() || GetCachePath().empty()) {
    cache_backend = net::HttpCache::DefaultBackend::InMemory(0);
  } else {
    cache_backend.reset(new net::HttpCache::DefaultBackend(
          net::DISK_CACHE,
          net::CACHE_BACKEND_SIMPLE,
          GetCachePath().Append(kCacheDirname2),
          GetMaxCacheSizeHint() * 1024 * 1024, // MB -> bytes
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::CACHE)));
  }

  net::HttpNetworkSession::Params session_params;
  session_params.host_resolver = context->host_resolver();
  session_params.cert_verifier = context->cert_verifier();
  session_params.channel_id_service = context->channel_id_service();
  session_params.transport_security_state =
      context->transport_security_state();
  session_params.cert_transparency_verifier =
      context->cert_transparency_verifier();
  session_params.ct_policy_enforcer = context->ct_policy_enforcer();
  session_params.proxy_service = context->proxy_service();
  session_params.ssl_config_service = context->ssl_config_service();
  session_params.http_auth_handler_factory =
      context->http_auth_handler_factory();
  session_params.http_server_properties =
      context->http_server_properties();
  session_params.net_log = context->net_log();
  session_params.host_mapping_rules = host_mapping_rules_.get();

  http_network_session_ =
      base::WrapUnique(new net::HttpNetworkSession(session_params));

  {
    // Calls QuickStreamFactory constructor which uses base::CPU
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    storage->set_http_transaction_factory(
        base::WrapUnique(
            new net::HttpCache(http_network_session_.get(),
                               std::move(cache_backend),
                               true)));
  }

  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
      new net::URLRequestJobFactoryImpl());

  bool set_protocol = false;
  for (content::ProtocolHandlerMap::iterator it = protocol_handlers.begin();
       it != protocol_handlers.end();
       ++it) {
    set_protocol =
        job_factory->SetProtocolHandler(it->first,
                                        base::WrapUnique(it->second.release()));
    DCHECK(set_protocol);
  }
  protocol_handlers.clear();

  set_protocol = job_factory->SetProtocolHandler(
      oxide::kFileScheme,
      base::WrapUnique(
          new net::FileProtocolHandler(
              content::BrowserThread::GetTaskRunnerForThread(
                  content::BrowserThread::FILE))));
  DCHECK(set_protocol);
  set_protocol = job_factory->SetProtocolHandler(
      oxide::kDataScheme,
      base::WrapUnique(new net::DataProtocolHandler()));
  DCHECK(set_protocol);

  set_protocol = job_factory->SetProtocolHandler(
      oxide::kFtpScheme,
      net::FtpProtocolHandler::Create(io_thread_globals->host_resolver()));
  DCHECK(set_protocol);

  std::unique_ptr<net::URLRequestJobFactory> top_job_factory(
      new URLRequestDelegatedJobFactory(std::move(job_factory),
                                        this));

  for (auto& interceptor : request_interceptors) {
    top_job_factory =
        base::MakeUnique<net::URLRequestInterceptingJobFactory>(
            std::move(top_job_factory),
            std::move(interceptor));
  }

  storage->set_job_factory(std::move(top_job_factory));

  resource_context_->request_context_ = context;
  return main_request_context_.get();
}

content::ResourceContext* BrowserContextIOData::GetResourceContext() {
  return resource_context_.get();
}

bool BrowserContextIOData::CanAccessCookies(const GURL& url,
                                            const GURL& first_party_url,
                                            bool write) {
  net::StaticCookiePolicy policy(GetCookiePolicy());
  return policy.CanAccessCookies(url, first_party_url) == net::OK;
}

TemporarySavedPermissionContext*
BrowserContextIOData::GetTemporarySavedPermissionContext() const {
  return temporary_saved_permission_context_.get();
}

UserAgentSettingsIOData* BrowserContextIOData::GetUserAgentSettings() const {
  return GetSharedData().user_agent_settings.get();
}

net::CookieStore* BrowserContextIOData::GetCookieStore() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return cookie_store_owner_->store();
}

class BrowserContextImpl;

class OTRBrowserContextImpl : public BrowserContext {
 public:
  OTRBrowserContextImpl(BrowserContextImpl* original,
                        BrowserContextIODataImpl* original_io_data);
  ~OTRBrowserContextImpl() override;

 private:
  BrowserContext* GetOffTheRecordContext() override { return this; }

  BrowserContext* GetOriginalContext() override;

  bool HasOffTheRecordContext() const override { return true; }

  BrowserContextImpl* original_context_;
};

class BrowserContextImpl : public BrowserContext {
 public:
  BrowserContextImpl(const BrowserContext::Params& params);

 private:
  friend class BrowserContext;

  ~BrowserContextImpl() override;

  BrowserContext* GetOffTheRecordContext() override;

  BrowserContext* GetOriginalContext() override { return this; }

  bool HasOffTheRecordContext() const override {
    return otr_context_ != nullptr;
  }

  std::unique_ptr<OTRBrowserContextImpl> otr_context_;
};

BrowserContext* OTRBrowserContextImpl::GetOriginalContext() {
  return original_context_;
}

OTRBrowserContextImpl::~OTRBrowserContextImpl() {}

OTRBrowserContextImpl::OTRBrowserContextImpl(
    BrowserContextImpl* original,
    BrowserContextIODataImpl* original_io_data)
    : BrowserContext(new OTRBrowserContextIODataImpl(original_io_data)),
      original_context_(original) {
  BrowserContextDependencyManager::GetInstance()
      ->CreateBrowserContextServices(this);
}

BrowserContext* BrowserContextImpl::GetOffTheRecordContext() {
  if (!otr_context_) {
    otr_context_ =
        base::MakeUnique<OTRBrowserContextImpl>(
            this,
            static_cast<BrowserContextIODataImpl*>(io_data()));
  }

  return otr_context_.get();
}

BrowserContextImpl::~BrowserContextImpl() {
  CHECK(!otr_context_);
}

BrowserContextImpl::BrowserContextImpl(const BrowserContext::Params& params)
    : BrowserContext(new BrowserContextIODataImpl(params)) {
  if (!GetPath().empty()) {
    base::FilePath gpu_cache = GetPath().Append(FILE_PATH_LITERAL("GPUCache"));
    content::BrowserThread::PostTask(
        content::BrowserThread::FILE,
        FROM_HERE,
        base::Bind(&CleanupOldCacheDir, gpu_cache));
  }

  BrowserContextDependencyManager::GetInstance()
      ->CreateBrowserContextServices(this);
}

void BrowserContext::Deleter::operator()(BrowserContext* context) {
  CHECK(!context->IsOffTheRecord());
  BrowserContextDestroyer::DestroyContext(base::WrapUnique(context));
}

std::unique_ptr<content::ZoomLevelDelegate>
BrowserContext::CreateZoomLevelDelegate(
    const base::FilePath& partition_path) {
  return nullptr;
}

content::DownloadManagerDelegate*
BrowserContext::GetDownloadManagerDelegate() {
  return DownloadManagerDelegate::Get(this);
}

content::BrowserPluginGuestManager* BrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy* BrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

content::PushMessagingService* BrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate* BrowserContext::GetSSLHostStateDelegate() {
  if (!ssl_host_state_delegate_) {
    ssl_host_state_delegate_.reset(new SSLHostStateDelegate());
  }

  return ssl_host_state_delegate_.get();
}

content::PermissionManager* BrowserContext::GetPermissionManager() {
  if (!permission_manager_) {
    permission_manager_.reset(new PermissionManager(this));
  }

  return permission_manager_.get();
}

content::BackgroundSyncController*
BrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

net::URLRequestContextGetter* BrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  DCHECK(CalledOnValidThread());
  DCHECK(!main_request_context_getter_.get());

  main_request_context_getter_ =
      new MainURLRequestContextGetter(io_data(),
                                      protocol_handlers,
                                      std::move(request_interceptors));

  return main_request_context_getter_.get();
}

net::URLRequestContextGetter*
BrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  // We don't return any storage partition names from
  // GetStoragePartitionConfigForSite(), so it's a bug to hit this
  NOTREACHED() << "Invalid request for request context for storage partition";
  return nullptr;
}

net::URLRequestContextGetter* BrowserContext::CreateMediaRequestContext() {
  DCHECK(CalledOnValidThread());
  DCHECK(main_request_context_getter_.get());
  return main_request_context_getter_.get();
}

net::URLRequestContextGetter*
BrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  // We don't return any storage partition names from
  // ContentBrowserClient::GetStoragePartitionConfigForSite(), so it's a
  // bug to hit this
  NOTREACHED() << "Invalid request for request context for storage partition";
  return nullptr;
}

BrowserContext::BrowserContext(BrowserContextIOData* io_data)
    : io_data_(io_data) {
  CHECK(BrowserProcessMain::GetInstance()->IsRunning()) <<
      "The main browser process components must be started before " <<
      "creating a context";

  g_all_contexts.Get().insert(this);

  io_data->cookie_store_owner_ = base::MakeUnique<CookieStoreOwner>();
  cookie_store_ =
      base::MakeUnique<CookieStoreProxy>(
          io_data->cookie_store_owner_->GetWeakPtr(),
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::UI),
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::IO));

  content::BrowserContext::Initialize(this, io_data->GetPath());
  content::BrowserContext::EnsureResourceContextInitialized(this);
}

BrowserContext::~BrowserContext() {
  DCHECK(CalledOnValidThread());

  size_t erased = g_all_contexts.Get().erase(this);
  DCHECK_EQ(erased, 1);

  BrowserContextDependencyManager::GetInstance()
      ->DestroyBrowserContextServices(this);

  ShutdownStoragePartitions();

  // Schedule io_data_ to be destroyed on the IO thread
  content::BrowserThread::DeleteSoon(content::BrowserThread::IO,
                                     FROM_HERE, io_data_);
  io_data_ = nullptr;
}

// static
BrowserContext* BrowserContext::FromContent(content::BrowserContext* context) {
  BrowserContext* result = static_cast<BrowserContext*>(context);
  if (g_all_contexts.Get().find(result) != g_all_contexts.Get().end()) {
    return result;
  }

  return nullptr;
}

// static
BrowserContext::UniquePtr BrowserContext::Create(const Params& params) {
  return BrowserContext::UniquePtr(new BrowserContextImpl(params));
}

// static
void BrowserContext::ForEach(const BrowserContextCallback& callback) {
  for (auto context : g_all_contexts.Get()) {
    callback.Run(context);
  }
}

// static
void BrowserContext::AssertNoContextsExist() {
  CHECK_EQ(g_all_contexts.Get().size(), static_cast<size_t>(0))
      << "BrowserContexts still exist at shutdown! This is normally the result "
      << "of an application leak, but it's possible that there might be an "
      << "Oxide bug too";
}

BrowserContextID BrowserContext::GetID() const {
  return reinterpret_cast<BrowserContextID>(this);
}

BrowserContextDelegate* BrowserContext::GetDelegate() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetDelegate().get();
}

void BrowserContext::SetDelegate(BrowserContextDelegate* delegate) {
  DCHECK(CalledOnValidThread());

  BrowserContextSharedIOData& data = io_data()->GetSharedData();
  base::AutoLock lock(data.lock);
  data.delegate = delegate;
}

// static
void BrowserContext::DestroyOffTheRecordContextForContext(
    BrowserContext* context) {
  CHECK(!context->IsOffTheRecord() && context->HasOffTheRecordContext());
  BrowserContextDestroyer::DestroyContext(
      std::move(static_cast<BrowserContextImpl*>(context)->otr_context_));
}

bool BrowserContext::IsOffTheRecord() const {
  DCHECK(CalledOnValidThread());
  return io_data()->IsOffTheRecord();
}

bool BrowserContext::IsSameContext(BrowserContext* other) const {
  DCHECK(CalledOnValidThread());
  return other->GetOriginalContext() == this ||
         (other->HasOffTheRecordContext() &&
          other->GetOffTheRecordContext() == this);
}

base::FilePath BrowserContext::GetPath() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetPath();
}

base::FilePath BrowserContext::GetCachePath() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetCachePath();
}

int BrowserContext::GetMaxCacheSizeHint() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetMaxCacheSizeHint();
}

net::StaticCookiePolicy::Type BrowserContext::GetCookiePolicy() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetCookiePolicy();
}

void BrowserContext::SetCookiePolicy(net::StaticCookiePolicy::Type policy) {
  DCHECK(CalledOnValidThread());

  BrowserContextSharedIOData& data = io_data()->GetSharedData();
  base::AutoLock lock(data.lock);
  data.cookie_policy = policy;
}

content::CookieStoreConfig::SessionCookieMode
BrowserContext::GetSessionCookieMode() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetSessionCookieMode();
}

const std::vector<std::string>& BrowserContext::GetHostMappingRules() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetSharedData().host_mapping_rules;
}

content::ResourceContext* BrowserContext::GetResourceContext() {
  DCHECK(CalledOnValidThread());
  return io_data()->GetResourceContext();
}

net::CookieStore* BrowserContext::GetCookieStore() const {
  return cookie_store_.get();
}

TemporarySavedPermissionContext*
BrowserContext::GetTemporarySavedPermissionContext() const {
  DCHECK(CalledOnValidThread());
  return io_data()->GetTemporarySavedPermissionContext();
}

BrowserContextIOData* BrowserContext::GetIOData() const {
  DCHECK(CalledOnValidThread());
  return io_data_;
}

} // namespace oxide
