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

#ifndef _OXIDE_QT_QUICK_API_NAVIGATION_ITEM_P_H_
#define _OXIDE_QT_QUICK_API_NAVIGATION_ITEM_P_H_

#include <QExplicitlySharedDataPointer>
#include <QtGlobal>

#include "qt/quick/api/oxideqquickglobal.h"
#include "qt/quick/api/oxideqquicknavigationitem.h"

QT_BEGIN_NAMESPACE
class QDateTime;
class QString;
class QUrl;
QT_END_NAMESPACE

namespace oxide {
namespace qt {
class NavigationHistoryItem;
}
}

class OXIDE_QTQUICK_EXPORT OxideQQuickNavigationItemData {
 public:
  OxideQQuickNavigationItemData();
  OxideQQuickNavigationItemData(oxide::qt::NavigationHistoryItem* item);
  ~OxideQQuickNavigationItemData();

  static OxideQQuickNavigationItem createForTesting(const QUrl& url,
                                                    const QUrl& original_url,
                                                    const QString& title,
                                                    const QDateTime& timestamp);

  static OxideQQuickNavigationItemData* get(const OxideQQuickNavigationItem& q);

  oxide::qt::NavigationHistoryItem* item() const { return item_.data(); }

 private:
  friend class OxideQQuickNavigationItem;

  QExplicitlySharedDataPointer<oxide::qt::NavigationHistoryItem> item_;
};

#endif // _OXIDE_QT_QUICK_API_NAVIGATION_ITEM_P_H_
