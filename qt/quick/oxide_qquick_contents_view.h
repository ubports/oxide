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

#ifndef _OXIDE_QQUICK_CONTENTS_VIEW_H_
#define _OXIDE_QQUICK_CONTENTS_VIEW_H_

#include <QPointer>
#include <QtGlobal>

#include "qt/core/glue/oxide_qt_contents_view_proxy.h"
#include "qt/core/glue/oxide_qt_contents_view_proxy_client.h"

QT_BEGIN_NAMESPACE
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QQmlComponent;
class QQuickItem;
class QSGNode;
class QTouchEvent;
class QWheelEvent;
QT_END_NAMESPACE

class OxideQQuickTouchSelectionController;

namespace oxide {
namespace qquick {

class ContentsView : public oxide::qt::ContentsViewProxyClient {
 public:
  ContentsView(QQuickItem* item);
  ~ContentsView() override;

  void set_touch_selection_controller(
      OxideQQuickTouchSelectionController* controller) {
    touch_selection_controller_ = controller;
  }

  void handleKeyPressEvent(QKeyEvent* event);
  void handleKeyReleaseEvent(QKeyEvent* event);
  void handleMousePressEvent(QMouseEvent* event);
  void handleMouseMoveEvent(QMouseEvent* event);
  void handleMouseReleaseEvent(QMouseEvent* event);
  void handleWheelEvent(QWheelEvent* event);
  void handleTouchEvent(QTouchEvent* event);
  void handleHoverEnterEvent(QHoverEvent* event);
  void handleHoverMoveEvent(QHoverEvent* event);
  void handleHoverLeaveEvent(QHoverEvent* event);
  void handleDragEnterEvent(QDragEnterEvent* event);
  void handleDragMoveEvent(QDragMoveEvent* event);
  void handleDragLeaveEvent(QDragLeaveEvent* event);
  void handleDropEvent(QDropEvent* event);

  QSGNode* updatePaintNode(QSGNode* old_node);

  // XXX(chrisccoulson): Remove these UI component accessors from here in
  //  future. Instead, we should have an auxilliary UI client class. There'll
  //  be a WebView impl of this for the custom UI component APIs, and also an
  //  Ubuntu UI Toolkit impl
  QQmlComponent* contextMenu() const { return context_menu_; }
  void setContextMenu(QQmlComponent* context_menu) {
    context_menu_ = context_menu;
  }

  QQmlComponent* popupMenu() const { return popup_menu_; }
  void setPopupMenu(QQmlComponent* popup_menu) {
    popup_menu_ = popup_menu;
  }

 private:
  friend class UpdatePaintNodeScope;

  void handleKeyEvent(QKeyEvent* event);
  void handleMouseEvent(QMouseEvent* event);
  void handleHoverEvent(QHoverEvent* event);

  void didUpdatePaintNode();

  // oxide::qt::ContentsViewProxyClient implementation
  QScreen* GetScreen() const override;
  bool IsVisible() const override;
  bool HasFocus() const override;
  QRect GetBoundsPix() const override;
  void ScheduleUpdate() override;
  void EvictCurrentFrame() override;
  void UpdateCursor(const QCursor& cursor) override;
  oxide::qt::WebContextMenuProxy* CreateWebContextMenu(
      oxide::qt::WebContextMenuProxyClient* client) override;
  oxide::qt::WebPopupMenuProxy* CreateWebPopupMenu(
      oxide::qt::WebPopupMenuProxyClient* client) override;
  oxide::qt::TouchHandleDrawableProxy* CreateTouchHandleDrawable() override;
  void TouchSelectionChanged(bool active, const QRectF& bounds) override;

  QPointer<QQuickItem> item_;
  QPointer<OxideQQuickTouchSelectionController> touch_selection_controller_;

  QPointer<QQmlComponent> context_menu_;
  QPointer<QQmlComponent> popup_menu_;
  QPointer<QQmlComponent> touch_handle_;

  bool received_new_compositor_frame_;
  bool frame_evicted_;
  oxide::qt::CompositorFrameHandle::Type last_composited_frame_type_;

  Q_DISABLE_COPY(ContentsView)
};

} // namespace qquick
} // namespace oxide

#endif // _OXIDE_QQUICK_CONTENTS_VIEW_H_
