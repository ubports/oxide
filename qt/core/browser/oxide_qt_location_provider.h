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

#ifndef _OXIDE_QT_CORE_BROWSER_LOCATION_PROVIDER_H_
#define _OXIDE_QT_CORE_BROWSER_LOCATION_PROVIDER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "device/geolocation/geoposition.h"
#include "device/geolocation/location_provider.h"

namespace oxide {
namespace qt {

class LocationSourceProxy;

class LocationProvider : public device::LocationProvider,
                         public base::NonThreadSafe,
                         public base::SupportsWeakPtr<LocationProvider> {
 public:
  LocationProvider();
  ~LocationProvider();

 private:
  friend class LocationSourceProxy;

  // device::LocationProvider implementation
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  bool StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const device::Geoposition& GetPosition() override;
  void OnPermissionGranted() override;

  void NotifyPositionUpdated(const device::Geoposition& position);

  LocationProviderUpdateCallback callback_;
  bool running_;
  bool is_permission_granted_;
  scoped_refptr<LocationSourceProxy> source_;
  device::Geoposition position_;

  DISALLOW_COPY_AND_ASSIGN(LocationProvider);
};

} // namespace qt
} // namespace oxide

#endif // _OXIDE_QT_CORE_BROWSER_LOCATION_PROVIDER_H_
