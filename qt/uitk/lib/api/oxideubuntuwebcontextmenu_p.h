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

#ifndef _OXIDE_UITK_LIB_API_WEB_CONTEXT_MENU_P_H_
#define _OXIDE_UITK_LIB_API_WEB_CONTEXT_MENU_P_H_

#include <set>

#include <QList>
#include <QtGlobal>
#include <QVariantList>

#include "qt/uitk/lib/api/oxideubuntuwebcontextmenuitem.h"

class OxideUbuntuWebContextMenu;

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class OxideUbuntuWebContextMenuPrivate {
  Q_DISABLE_COPY(OxideUbuntuWebContextMenuPrivate)
  Q_DECLARE_PUBLIC(OxideUbuntuWebContextMenu)

 public:
  ~OxideUbuntuWebContextMenuPrivate();

  static OxideUbuntuWebContextMenuPrivate* get(OxideUbuntuWebContextMenu* q);

  void AppendItem(OxideUbuntuWebContextMenuItem* item);

  QVariantList GetItemsAsVariantList() const;

 private:
  OxideUbuntuWebContextMenuPrivate(OxideUbuntuWebContextMenu* q);

  static int items_Count(QQmlListProperty<OxideUbuntuWebContextMenuItem>* prop);
  static OxideUbuntuWebContextMenuItem* items_At(
      QQmlListProperty<OxideUbuntuWebContextMenuItem>* prop,
      int index);

  bool PrepareToAddItem(OxideUbuntuWebContextMenuItem* item);
  bool InsertItemAt(int index, OxideUbuntuWebContextMenuItem* item);
  void DiscardCreatedItem(OxideUbuntuWebContextMenuItem* item);

  OxideUbuntuWebContextMenu* q_ptr;

  std::set<OxideUbuntuWebContextMenuItem::Section> sections_;

  QList<OxideUbuntuWebContextMenuItem*> items_;
};

#endif // _OXIDE_UITK_LIB_API_WEB_CONTEXT_MENU_P_H_
