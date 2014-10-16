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

#include "oxide_qt_content_main_delegate.h"

#include <QGuiApplication>
#include <QtGui/qpa/qplatformnativeinterface.h>

#include "base/lazy_instance.h"
#include "base/logging.h"

#include "qt/core/browser/oxide_qt_content_browser_client.h"
#include "qt/core/gl/oxide_qt_shared_gl_context.h"
#include "qt/core/glue/oxide_qt_init.h"

namespace oxide {
namespace qt {

namespace {
base::LazyInstance<ContentBrowserClient> g_content_browser_client =
    LAZY_INSTANCE_INITIALIZER;
}

ContentMainDelegate::ContentMainDelegate(const base::FilePath& nss_db_path)
    : is_browser_(true)
#if defined(USE_NSS)
      , nss_db_path_(nss_db_path) {
#else
{
  DCHECK(nss_db_path.empty());
#endif
  QOpenGLContext* qcontext = oxide::qt::GetSharedGLContext();
  if (qcontext) {
    scoped_refptr<SharedGLContext> context(new SharedGLContext(qcontext));
    if (context->GetHandle()) {
      shared_gl_context_ = context;
    } else {
      DLOG(WARNING) << "Could not determine native handle for shared GL context";
    }
  }
}

oxide::SharedGLContext* ContentMainDelegate::GetSharedGLContext() const {
  return shared_gl_context_.get();
}

bool ContentMainDelegate::GetNativeDisplay(intptr_t* handle) const {
  if (!is_browser_) {
    return false;
  }

  QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
  if (!pni) {
    return false;
  }

  *handle = reinterpret_cast<intptr_t>(
      pni->nativeResourceForScreen("display",
                                   QGuiApplication::primaryScreen()));

  return true;
}

#if defined(USE_NSS)
base::FilePath ContentMainDelegate::GetNSSDbPath() const {
  DCHECK(is_browser_);
  return nss_db_path_;
}
#endif

content::ContentBrowserClient*
ContentMainDelegate::CreateContentBrowserClient() {
  return g_content_browser_client.Pointer();
}

ContentMainDelegate::ContentMainDelegate()
    : is_browser_(false) {}

ContentMainDelegate::~ContentMainDelegate() {}

// static
ContentMainDelegate* ContentMainDelegate::CreateForBrowser(
    const base::FilePath& nss_db_path) {
  return new ContentMainDelegate(nss_db_path);
}

} // namespace qt
} // namespace oxide
