// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2016 Canonical Ltd.

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

#ifndef _OXIDE_QT_CORE_GLUE_LEGACY_EXTERNAL_TOUCH_EDITING_MENU_CONTROLLER_H_
#define _OXIDE_QT_CORE_GLUE_LEGACY_EXTERNAL_TOUCH_EDITING_MENU_CONTROLLER_H_

#include "qt/core/api/oxideqglobal.h"

namespace oxide {
namespace qt {

class LegacyExternalTouchEditingMenuControllerDelegate;

class OXIDE_QTCORE_EXPORT LegacyExternalTouchEditingMenuController {
 public:
  virtual ~LegacyExternalTouchEditingMenuController();

  virtual void HideAndDisallowShowingAutomatically() = 0;

 protected:
  LegacyExternalTouchEditingMenuController(
      LegacyExternalTouchEditingMenuControllerDelegate* delegate);

  friend class LegacyExternalTouchEditingMenuControllerDelegate;
  void ClearDelegate();

  LegacyExternalTouchEditingMenuControllerDelegate* delegate() const {
    return delegate_;
  }

 private:
  LegacyExternalTouchEditingMenuControllerDelegate* delegate_;
};

} // namespace qt
} // namespace oxide

#endif // _OXIDE_QT_CORE_GLUE_LEGACY_EXTERNAL_TOUCH_EDITING_MENU_CONTROLLER_H_
