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

#include "oxide_qt_screen_utils.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QString>
#include <QtGui/qpa/qplatformnativeinterface.h>

#include "third_party/WebKit/public/platform/modules/screen_orientation/WebScreenOrientationType.h"
#include "third_party/WebKit/public/platform/WebRect.h"

namespace oxide {
namespace qt {

namespace {

blink::WebScreenOrientationType GetOrientationTypeFromScreenOrientation(
    Qt::ScreenOrientation orientation) {
  switch (orientation) {
    case Qt::PortraitOrientation:
      return blink::WebScreenOrientationPortraitPrimary;
    case Qt::LandscapeOrientation:
      return blink::WebScreenOrientationLandscapePrimary;
    case Qt::InvertedPortraitOrientation:
      return blink::WebScreenOrientationPortraitSecondary;
    case Qt::InvertedLandscapeOrientation:
      return blink::WebScreenOrientationLandscapeSecondary;
    default:
      NOTREACHED();
      return blink::WebScreenOrientationUndefined;
  }
}

}

float GetDeviceScaleFactorFromQScreen(QScreen* screen) {
  // Because it only supports integer scale factors, the Ubuntu QPA plugin
  // doesn't use the default Qt devicePixelRatio but a custom scaling system
  // based on grid units. The relationship between grid units and device pixels
  // is set by the "GRID_UNIT_PX" environment variable. On a screen with a scale
  // factor of 1, GRID_UNIT_PX is set to 8, and 1 grid unit == 8 device pixels.
  // If the Ubuntu backend is used, we retrieve scale factors from the QPA
  // plugin. For old versions that don't expose these factors, we deduce the
  // scale factor by reading the "GRID_UNIT_PX" environment variable.
  //
  // XXX: There are 2 scenarios where this is completely broken:
  //      1) Any apps not using the Ubuntu SDK but running with the Ubuntu
  //         QPA plugin. In this case, we derive a DPR from GRID_UNIT_PX if
  //         set, and the application probably uses QScreen::devicePixelRatio,
  //         which is always 1.0f
  //      2) Any apps using the Ubuntu SDK but not running with the Ubuntu
  //         QPA plugin. In this case, we get the DPR from
  //         QScreen::devicePixelRatio, and the application uses GRID_UNIX_PX
  //         if set
  //      I think it would be better if the Ubuntu QPA plugin did override
  //      QScreen::devicePixelRatio (it could still get that from GRID_UNIT_PX),
  //      and the Ubuntu SDK used this to convert between grid units and device
  //      pixels, then we could just use QScreen::devicePixelRatio here

  // Allow an override for testing
  {
    QByteArray force_dpr(qgetenv("OXIDE_FORCE_DPR"));
    bool ok;
    float scale = force_dpr.toFloat(&ok);
    if (ok) {
      return scale;
    }
  }

  QString platform = QGuiApplication::platformName();
  if (platform.startsWith("ubuntu") || platform == "mirserver") {

    QPlatformNativeInterface* interface =
        QGuiApplication::platformNativeInterface();
    void* data =
        interface->nativeResourceForScreen(QByteArray("scale"), screen);
    if (data) {
      return *reinterpret_cast<float*>(data);
    }

    QByteArray grid_unit_px(qgetenv("GRID_UNIT_PX"));
    bool ok;
    float scale = grid_unit_px.toFloat(&ok);
    if (ok) {
      return scale / 8;
    }
  }

  return float(screen->devicePixelRatio());
}

blink::WebScreenInfo GetWebScreenInfoFromQScreen(QScreen* screen) {
  blink::WebScreenInfo result;

  result.depth = 24;
  result.depthPerComponent = 8; // XXX: Copied the GTK impl here
  result.isMonochrome = result.depth == 1;
  result.deviceScaleFactor = GetDeviceScaleFactorFromQScreen(screen);

  QRect rect = screen->geometry();
  result.rect = blink::WebRect(rect.x(),
                               rect.y(),
                               rect.width(),
                               rect.height());

  QRect availableRect = screen->availableGeometry();
  result.availableRect = blink::WebRect(availableRect.x(),
                                        availableRect.y(),
                                        availableRect.width(),
                                        availableRect.height());

  result.orientationType =
      GetOrientationTypeFromScreenOrientation(screen->orientation());

  result.orientationAngle =
      screen->angleBetween(screen->orientation(),
                           screen->nativeOrientation());

  return result;
}

} // namespace qt
} // namespace oxide
