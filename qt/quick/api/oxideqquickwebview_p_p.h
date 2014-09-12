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

#ifndef _OXIDE_QT_QUICK_API_WEB_VIEW_P_P_H_
#define _OXIDE_QT_QUICK_API_WEB_VIEW_P_P_H_

#include <QSharedPointer>
#include <QtGlobal>
#include <QUrl>

#include "qt/core/glue/oxide_qt_web_view_adapter.h"

#include "oxideqquicknavigationhistory_p.h"

class OxideQQuickScriptMessageHandler;
class OxideQQuickWebContext;
class OxideQQuickWebContextPrivate;
class OxideQQuickWebView;

QT_BEGIN_NAMESPACE
class QQmlComponent;
template <typename T> class QQmlListProperty;
class QQuickItem;
class QQuickWindow;
QT_END_NAMESPACE

namespace oxide {
namespace qt {
class CompositorFrameHandle;
}
}

class OxideQQuickWebViewPrivate Q_DECL_FINAL :
     public oxide::qt::WebViewAdapter {
  Q_DECLARE_PUBLIC(OxideQQuickWebView)

 public:
  ~OxideQQuickWebViewPrivate();

  static OxideQQuickWebViewPrivate* get(OxideQQuickWebView* web_view);

  void addAttachedPropertyTo(QObject* object);

 private:
  friend class UpdatePaintNodeScope;

  OxideQQuickWebViewPrivate(OxideQQuickWebView* view);

  oxide::qt::WebPopupMenuDelegate* CreateWebPopupMenuDelegate() Q_DECL_FINAL;
  oxide::qt::JavaScriptDialogDelegate* CreateJavaScriptDialogDelegate(
      oxide::qt::JavaScriptDialogDelegate::Type type) Q_DECL_FINAL;
  oxide::qt::JavaScriptDialogDelegate* CreateBeforeUnloadDialogDelegate() Q_DECL_FINAL;
  oxide::qt::FilePickerDelegate* CreateFilePickerDelegate() Q_DECL_FINAL;

  void OnInitialized(bool orig_incognito,
                     oxide::qt::WebContextAdapter* orig_context) Q_DECL_FINAL;

  void URLChanged() Q_DECL_FINAL;
  void TitleChanged() Q_DECL_FINAL;
  void IconChanged(QUrl icon) Q_DECL_FINAL;
  void CommandsUpdated() Q_DECL_FINAL;

  void LoadProgressChanged(double progress) Q_DECL_FINAL;

  void LoadEvent(OxideQLoadEvent* event) Q_DECL_FINAL;
  
  void NavigationEntryCommitted() Q_DECL_FINAL;
  void NavigationListPruned(bool from_front, int count) Q_DECL_FINAL;
  void NavigationEntryChanged(int index) Q_DECL_FINAL;

  oxide::qt::WebFrameAdapter* CreateWebFrame() Q_DECL_FINAL;

  QScreen* GetScreen() const Q_DECL_FINAL;
  QSize GetViewSizePix() const Q_DECL_FINAL;
  bool IsVisible() const Q_DECL_FINAL;
  bool HasFocus() const Q_DECL_FINAL;

  void AddMessageToConsole(int level,
			   const QString& message,
			   int line_no,
			   const QString& source_id) Q_DECL_FINAL;

  void ToggleFullscreenMode(bool enter) Q_DECL_FINAL;

  void OnWebPreferencesChanged() Q_DECL_FINAL;

  void FrameAdded(oxide::qt::WebFrameAdapter* frame) Q_DECL_FINAL;
  void FrameRemoved(oxide::qt::WebFrameAdapter* frame) Q_DECL_FINAL;

  bool CanCreateWindows() const Q_DECL_FINAL;

  void UpdateCursor(const QCursor& cursor) Q_DECL_FINAL;

  void NavigationRequested(OxideQNavigationRequest* request) Q_DECL_FINAL;
  void NewViewRequested(OxideQNewViewRequest* request) Q_DECL_FINAL;

  void RequestGeolocationPermission(
      OxideQGeolocationPermissionRequest* request) Q_DECL_FINAL;

  void HandleUnhandledKeyboardEvent(QKeyEvent *event) Q_DECL_FINAL;

  void FrameMetadataUpdated(
      oxide::qt::FrameMetadataChangeFlags flags) Q_DECL_FINAL;

  void ScheduleUpdate() Q_DECL_FINAL;
  void EvictCurrentFrame() Q_DECL_FINAL;

  void SetInputMethodEnabled(bool enabled) Q_DECL_FINAL;

  void DownloadRequested(OxideQDownloadRequest* downloadRequest) Q_DECL_FINAL;

  void CertificateError(OxideQCertificateError* cert_error) Q_DECL_FINAL;
  void ContentBlocked() Q_DECL_FINAL;

  void completeConstruction();

  static void messageHandler_append(
      QQmlListProperty<OxideQQuickScriptMessageHandler>* prop,
      OxideQQuickScriptMessageHandler* message_handler);
  static int messageHandler_count(
      QQmlListProperty<OxideQQuickScriptMessageHandler>* prop);
  static OxideQQuickScriptMessageHandler* messageHandler_at(
      QQmlListProperty<OxideQQuickScriptMessageHandler>* prop,
      int index);
  static void messageHandler_clear(
      QQmlListProperty<OxideQQuickScriptMessageHandler>* prop);

  void contextConstructed();
  void contextWillBeDestroyed();
  void attachContextSignals(OxideQQuickWebContextPrivate* context);
  void detachContextSignals(OxideQQuickWebContextPrivate* context);

  void didUpdatePaintNode();

  bool constructed_;
  int load_progress_;
  QUrl icon_;
  OxideQQuickNavigationHistory navigation_history_;
  QQmlComponent* popup_menu_;
  QQmlComponent* alert_dialog_;
  QQmlComponent* confirm_dialog_;
  QQmlComponent* prompt_dialog_;
  QQmlComponent* before_unload_dialog_;
  QQmlComponent* file_picker_;

  QQuickItem* input_area_;

  bool received_new_compositor_frame_;
  bool frame_evicted_;
  oxide::qt::CompositorFrameHandle::Type last_composited_frame_type_;
  QSharedPointer<oxide::qt::CompositorFrameHandle> compositor_frame_handle_;
};

#endif // _OXIDE_QT_QUICK_API_WEB_VIEW_P_P_H_
