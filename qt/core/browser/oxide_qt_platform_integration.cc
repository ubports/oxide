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

#include "oxide_qt_platform_integration.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QString>
#include <QTouchDevice>
#include <QUrl>
#include <QtGui/qpa/qplatformnativeinterface.h>

#include "url/gurl.h"

#include "qt/core/base/oxide_qt_screen_utils.h"

namespace oxide {
namespace qt {

PlatformIntegration::PlatformIntegration() {}

bool PlatformIntegration::LaunchURLExternally(const GURL& url) {
  return QDesktopServices::openUrl(QUrl(QString::fromStdString(url.spec())));
}

bool PlatformIntegration::IsTouchSupported() {
  // XXX: Is there a way to get notified if a touch device is added?
  return QTouchDevice::devices().size() > 0;
}

intptr_t PlatformIntegration::GetNativeDisplay() {
  QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
  if (!pni) {
    return 0;
  }

  return reinterpret_cast<intptr_t>(
      pni->nativeResourceForScreen("display",
                                   QGuiApplication::primaryScreen()));
}

blink::WebScreenInfo PlatformIntegration::GetDefaultScreenInfo() {
  return GetWebScreenInfoFromQScreen(QGuiApplication::primaryScreen());
}

} // namespace qt
} // namespace oxide
