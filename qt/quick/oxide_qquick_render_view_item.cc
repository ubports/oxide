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

#include "oxide_qquick_render_view_item.h"

#include <QQuickWindow>
#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include "qt/quick/api/oxideqquickwebview_p.h"

#include "oxide_qquick_painted_render_view_node.h"

namespace oxide {
namespace qquick {

void RenderViewItem::ScheduleUpdate(const QRect& rect) {
  if (rect.isNull() && !dirty_rect_.isNull()) {
    dirty_rect_ = QRectF(0, 0, width(), height()).toAlignedRect();
  } else {
    dirty_rect_ |= (QRectF(0, 0, width(), height()) & rect).toAlignedRect();
  }

  update();
  polish();
}

RenderViewItem::RenderViewItem(
    OxideQQuickWebView* webview) :
    QQuickItem(webview),
    backing_store_(NULL) {
  setFlag(QQuickItem::ItemHasContents);

  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
}

void RenderViewItem::Blur() {
  setFocus(false);
}

void RenderViewItem::Focus() {
  setFocus(true);
}

bool RenderViewItem::HasFocus() {
  return hasFocus();
}

void RenderViewItem::Show() {
  setVisible(true);
}

void RenderViewItem::Hide() {
  setVisible(false);
}

bool RenderViewItem::IsShowing() {
  return isVisible();
}

QRect RenderViewItem::GetViewBounds() {
  QPointF pos(mapToScene(QPointF(0, 0)));
  if (window()) {
    pos += window()->position();
  }

  return QRect(qRound(pos.x()), qRound(pos.y()),
               qRound(width()), qRound(height()));
}

QRect RenderViewItem::GetBoundsInRootWindow() {
  if (!window()) {
    return GetViewBounds();
  }

  return window()->frameGeometry();
}

void RenderViewItem::SetSize(const QSize& size) {
  setSize(QSizeF(size));
  polish();
}

QScreen* RenderViewItem::GetScreen() {
  if (!window()) {
    return NULL;
  }

  return window()->screen();
}

void RenderViewItem::focusInEvent(QFocusEvent* event) {
  Q_ASSERT(event->gotFocus());
  ForwardFocusEvent(event);
}

void RenderViewItem::focusOutEvent(QFocusEvent* event) {
  Q_ASSERT(event->lostFocus());
  ForwardFocusEvent(event);
}

void RenderViewItem::keyPressEvent(QKeyEvent* event) {
  ForwardKeyEvent(event);
}

void RenderViewItem::keyReleaseEvent(QKeyEvent* event) {
  ForwardKeyEvent(event);
}

void RenderViewItem::mouseDoubleClickEvent(QMouseEvent* event) {
  ForwardMouseEvent(event);
}

void RenderViewItem::mouseMoveEvent(QMouseEvent* event) {
  ForwardMouseEvent(event);
}

void RenderViewItem::mousePressEvent(QMouseEvent* event) {
  ForwardMouseEvent(event);
  setFocus(true);
}

void RenderViewItem::mouseReleaseEvent(QMouseEvent* event) {
  ForwardMouseEvent(event);
}

void RenderViewItem::wheelEvent(QWheelEvent* event) {
  ForwardWheelEvent(event);
}

void RenderViewItem::hoverMoveEvent(QHoverEvent* event) {
  // QtQuick gives us a hover event unless we have a grab (which
  // happens implicitly on button press). As Chromium doesn't
  // distinguish between the 2, just give it a mouse event
  QPointF window_pos = mapToScene(event->posF());
  QMouseEvent me(QEvent::MouseMove,
                 event->posF(),
                 window_pos,
                 window_pos + window()->position(),
                 Qt::NoButton,
                 Qt::NoButton,
                 event->modifiers());

  ForwardMouseEvent(&me);

  event->setAccepted(me.isAccepted());
}

void RenderViewItem::updatePolish() {
  backing_store_ = GetBackingStore();
}

QSGNode* RenderViewItem::updatePaintNode(
    QSGNode* oldNode,
    UpdatePaintNodeData* data) {
  Q_UNUSED(data);

  if (width() <= 0 || height() <= 0) {
    delete oldNode;
    return NULL;
  }

  PaintedRenderViewNode* node = static_cast<PaintedRenderViewNode *>(oldNode);
  if (!node) {
    node = new PaintedRenderViewNode();
  }

  node->setSize(QSizeF(width(), height()).toSize());
  node->markDirtyRect(dirty_rect_);
  node->setBackingStore(backing_store_);

  dirty_rect_ = QRect();

  return node;
}

} // namespace qquick
} // namespace oxide
