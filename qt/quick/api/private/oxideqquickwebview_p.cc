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

#include "qt/quick/api/oxideqquickwebview_p_p.h"
#include "qt/quick/api/oxideqquickwebview_p.h"

#include <QPointF>
#include <QQuickWindow>
#include <QSizeF>
#include <QString>
#include <QtQml>

#include "ui/gfx/size.h"
#include "url/gurl.h"

#include "qt/core/browser/oxide_qt_render_widget_host_view_qquick.h"
#include "qt/core/browser/oxide_qt_web_frame.h"
#include "qt/core/browser/oxide_qt_web_frame_tree.h"
#include "qt/core/browser/oxide_qt_web_popup_menu_qquick.h"

#include "qt/core/api/oxideqloadevent.h"

#include "qt/quick/api/oxideqquickmessagehandler_p_p.h"
#include "qt/quick/api/oxideqquickwebcontext_p.h"
#include "qt/quick/api/oxideqquickwebcontext_p_p.h"
#include "qt/quick/api/oxideqquickwebframe_p.h"

OxideQQuickWebViewPrivate::OxideQQuickWebViewPrivate(
    OxideQQuickWebView* view) :
    context(NULL),
    popup_menu(NULL),
    q_ptr(view),
    init_props_(new InitData()),
    weak_factory_(this) {}

void OxideQQuickWebViewPrivate::OnURLChanged() {
  Q_Q(OxideQQuickWebView);

  emit q->urlChanged();
}

void OxideQQuickWebViewPrivate::OnTitleChanged() {
  Q_Q(OxideQQuickWebView);

  emit q->titleChanged();
}

void OxideQQuickWebViewPrivate::OnCommandsUpdated() {
  Q_Q(OxideQQuickWebView);

  emit q->navigationHistoryChanged();
}

void OxideQQuickWebViewPrivate::OnRootFrameChanged() {
  Q_Q(OxideQQuickWebView);

  emit q->rootFrameChanged();
}

void OxideQQuickWebViewPrivate::OnLoadStarted(const GURL& validated_url,
                                              bool is_error_frame) {
  Q_Q(OxideQQuickWebView);

  OxideQLoadEvent event(QUrl(QString::fromStdString(validated_url.spec())),
                        OxideQLoadEvent::TypeStarted);
  emit q->loadingChanged(&event);
}

void OxideQQuickWebViewPrivate::OnLoadStopped(const GURL& validated_url) {
  Q_Q(OxideQQuickWebView);

  OxideQLoadEvent event(QUrl(QString::fromStdString(validated_url.spec())),
                        OxideQLoadEvent::TypeStopped);
  emit q->loadingChanged(&event);
}

void OxideQQuickWebViewPrivate::OnLoadFailed(
    const GURL& validated_url,
    int error_code,
    const std::string& error_description) {
  Q_Q(OxideQQuickWebView);

  OxideQLoadEvent event(QUrl(QString::fromStdString(validated_url.spec())),
                        OxideQLoadEvent::TypeFailed,
                        error_code,
                        QString::fromStdString(error_description));
  emit q->loadingChanged(&event);
}

void OxideQQuickWebViewPrivate::OnLoadSucceeded(const GURL& validated_url) {
  Q_Q(OxideQQuickWebView);

  OxideQLoadEvent event(QUrl(QString::fromStdString(validated_url.spec())),
                        OxideQLoadEvent::TypeSucceeded);
  emit q->loadingChanged(&event);
}

// static
OxideQQuickWebViewPrivate* OxideQQuickWebViewPrivate::Create(
    OxideQQuickWebView* view) {
  return new OxideQQuickWebViewPrivate(view);
}

OxideQQuickWebViewPrivate::~OxideQQuickWebViewPrivate() {
  // It's important that this goes away before our context
  DestroyWebContents();
}

size_t OxideQQuickWebViewPrivate::GetMessageHandlerCount() const {
  return message_handlers_.size();
}

oxide::MessageHandler* OxideQQuickWebViewPrivate::GetMessageHandlerAt(
    size_t index) const {
  return OxideQQuickMessageHandlerPrivate::get(
      message_handlers_.at(index))->handler();
}

void OxideQQuickWebViewPrivate::RootFrameCreated(oxide::WebFrame* root) {
  Q_Q(OxideQQuickWebView);

  oxide::qt::WebFrame* qroot = static_cast<oxide::qt::WebFrame *>(root);
  qroot->q_web_frame->setParent(q);
}

content::RenderWidgetHostView* OxideQQuickWebViewPrivate::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host) {
  Q_Q(OxideQQuickWebView);

  return new oxide::qt::RenderWidgetHostViewQQuick(render_widget_host, q);
}

gfx::Rect OxideQQuickWebViewPrivate::GetContainerBounds() {
  Q_Q(OxideQQuickWebView);

  QPointF pos(q->mapToScene(QPointF(0,0)));
  if (q->window()) {
    // We could be called before being added to a scene
    pos += q->window()->position();
  }

  return gfx::Rect(qRound(pos.x()),
                   qRound(pos.y()),
                   qRound(q->width()),
                   qRound(q->height()));
}

oxide::WebPopupMenu* OxideQQuickWebViewPrivate::CreatePopupMenu() {
  Q_Q(OxideQQuickWebView);

  return new oxide::qt::WebPopupMenuQQuick(q, web_contents());
}

void OxideQQuickWebViewPrivate::UpdateVisibility() {
  Q_Q(OxideQQuickWebView);

  if (init_props_) {
    return;
  }

  if (q->isVisible()) {
    Shown();
  } else {
    Hidden();
  }  
}


oxide::qt::WebFrameTree* OxideQQuickWebViewPrivate::CreateWebFrameTree(
    content::RenderViewHost* rvh) {
  return new oxide::qt::WebFrameTree(rvh);
}

void OxideQQuickWebViewPrivate::componentComplete() {
  Q_Q(OxideQQuickWebView);

  Q_ASSERT(init_props_);

  if (!context) {
    // Ok, we handle the default context a bit differently. If our context
    // comes from setContext(), then we don't hold a strong reference to it
    // because it will be owned by someone else in the QML object
    // hierarchy. However, the default context is not in this hierarchy and
    // has no QObject parent, so we use reference counting for it instead to
    // ensure that it is freed once all webviews are closed
    default_context_.reset(OxideQQuickWebContext::defaultContext());
    context = default_context_.data();
  }

  Init(OxideQQuickWebContextPrivate::get(context)->GetContext(),
       init_props_->incognito,
       gfx::Size(qRound(q->width()), qRound(q->height())));

  if (!init_props_->url.isEmpty()) {
    SetURL(GURL(init_props_->url.toString().toStdString()));
  }

  init_props_.reset();

  UpdateVisibility();
}

// static
void OxideQQuickWebViewPrivate::messageHandler_append(
    QQmlListProperty<OxideQQuickMessageHandler>* prop,
    OxideQQuickMessageHandler* message_handler) {
  if (!message_handler) {
    return;
  }

  OxideQQuickWebView* web_view =
      static_cast<OxideQQuickWebView* >(prop->object);

  web_view->addMessageHandler(message_handler);
}

// static
int OxideQQuickWebViewPrivate::messageHandler_count(
    QQmlListProperty<OxideQQuickMessageHandler>* prop) {
  OxideQQuickWebViewPrivate* p = OxideQQuickWebViewPrivate::get(
        static_cast<OxideQQuickWebView *>(prop->object));

  return p->message_handlers().size();
}

// static
OxideQQuickMessageHandler* OxideQQuickWebViewPrivate::messageHandler_at(
    QQmlListProperty<OxideQQuickMessageHandler>* prop,
    int index) {
  OxideQQuickWebViewPrivate* p = OxideQQuickWebViewPrivate::get(
        static_cast<OxideQQuickWebView *>(prop->object));

  if (index >= p->message_handlers().size()) {
    return NULL;
  }

  return p->message_handlers().at(index);
}

// static
void OxideQQuickWebViewPrivate::messageHandler_clear(
    QQmlListProperty<OxideQQuickMessageHandler>* prop) {
  OxideQQuickWebView* web_view =
      static_cast<OxideQQuickWebView *>(prop->object);
  OxideQQuickWebViewPrivate* p = OxideQQuickWebViewPrivate::get(web_view);

  p->message_handlers().clear();

  emit web_view->messageHandlersChanged();
}

// static
OxideQQuickWebViewPrivate* OxideQQuickWebViewPrivate::get(
    OxideQQuickWebView* view) {
  return view->d_func();
}

void OxideQQuickWebViewPrivate::addAttachedPropertyTo(QObject* object) {
  Q_Q(OxideQQuickWebView);

  OxideQQuickWebViewAttached* attached =
      qobject_cast<OxideQQuickWebViewAttached *>(
        qmlAttachedPropertiesObject<OxideQQuickWebView>(object));
  attached->setView(q);
}

void OxideQQuickWebViewPrivate::updateSize(const QSizeF& size) {
  UpdateSize(gfx::Size(qRound(size.width()), qRound(size.height())));
}

QUrl OxideQQuickWebViewPrivate::url() const {
  return QUrl(QString::fromStdString(GetURL().spec()));
}

void OxideQQuickWebViewPrivate::setUrl(const QUrl& url) {
  SetURL(GURL(url.toString().toStdString()));
}
