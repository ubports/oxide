// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2013-2016 Canonical Ltd.

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

#include "oxide_web_view.h"

#include <queue>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/supports_user_data.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/file_chooser_file_info.h"
#include "content/public/common/file_chooser_params.h"
#include "content/public/common/menu_item.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/result_codes.h"
#include "content/public/common/url_constants.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/net_errors.h"
#include "net/ssl/ssl_info.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/event.h"
#include "ui/gl/gl_implementation.h"
#include "ui/shell_dialogs/selected_file_info.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#include "shared/browser/javascript_dialogs/javascript_dialog_contents_helper.h"
#include "shared/browser/media/oxide_media_capture_devices_dispatcher.h"
#include "shared/browser/permissions/oxide_permission_request_dispatcher.h"
#include "shared/browser/permissions/oxide_temporary_saved_permission_context.h"
#include "shared/browser/ssl/oxide_certificate_error_dispatcher.h"
#include "shared/browser/ssl/oxide_security_status.h"
#include "shared/common/oxide_content_client.h"
#include "shared/common/oxide_enum_flags.h"
#include "shared/common/oxide_messages.h"
#include "shared/common/oxide_unowned_user_data.h"

#include "navigation_controller_observer.h"
#include "oxide_browser_context.h"
#include "oxide_browser_process_main.h"
#include "oxide_content_browser_client.h"
#include "oxide_event_utils.h"
#include "oxide_favicon_helper.h"
#include "oxide_file_picker.h"
#include "oxide_find_controller.h"
#include "oxide_fullscreen_helper.h"
#include "oxide_render_widget_host_view.h"
#include "oxide_script_message_contents_helper.h"
#include "oxide_user_agent_settings.h"
#include "oxide_web_contents_view.h"
#include "oxide_web_contents_view_client.h"
#include "oxide_web_frame_tree.h"
#include "oxide_web_frame.h"
#include "oxide_web_view_client.h"
#include "web_contents_client.h"
#include "web_contents_helper.h"
#include "web_process_status_monitor.h"

#if defined(ENABLE_MEDIAHUB)
#include "shared/browser/media/oxide_media_web_contents_observer.h"
#endif

#define DCHECK_VALID_SOURCE_CONTENTS DCHECK_EQ(source, web_contents());

namespace oxide {

namespace {

int kUserDataKey;

void FillLoadURLParamsFromOpenURLParams(
    content::NavigationController::LoadURLParams* load_params,
    const content::OpenURLParams& params) {
  load_params->transition_type = params.transition;
  load_params->frame_tree_node_id = params.frame_tree_node_id;
  load_params->referrer = params.referrer;
  load_params->redirect_chain = params.redirect_chain;
  load_params->extra_headers = params.extra_headers;
  load_params->is_renderer_initiated = params.is_renderer_initiated;
  load_params->should_replace_current_entry =
      params.should_replace_current_entry;

  if (params.uses_post) {
    load_params->load_type = content::NavigationController::LOAD_TYPE_HTTP_POST;
    load_params->post_data = params.post_data;
  }
}

void CreateHelpers(content::WebContents* contents) {
  CertificateErrorDispatcher::CreateForWebContents(contents);
  SecurityStatus::CreateForWebContents(contents);
  PermissionRequestDispatcher::CreateForWebContents(contents);
  ScriptMessageContentsHelper::CreateForWebContents(contents);
#if defined(ENABLE_MEDIAHUB)
  new MediaWebContentsObserver(contents);
#endif
  FindController::CreateForWebContents(contents);
  WebFrameTree::CreateForWebContents(contents);
  FaviconHelper::CreateForWebContents(contents);
  FullscreenHelper::CreateForWebContents(contents);
  WebProcessStatusMonitor::CreateForWebContents(contents);
  JavaScriptDialogContentsHelper::CreateForWebContents(contents);
}

OXIDE_MAKE_ENUM_BITWISE_OPERATORS(ui::PageTransition)
OXIDE_MAKE_ENUM_BITWISE_OPERATORS(ContentType)

}

WebView::CommonParams::CommonParams() = default;

WebView::CommonParams::~CommonParams() = default;

WebView::CreateParams::CreateParams()
    : context(nullptr),
      incognito(false),
      restore_type(content::RestoreType::NONE),
      restore_index(0) {}

WebView::CreateParams::~CreateParams() {}

WebView::WebView(WebViewClient* client)
    : client_(client),
      blocked_content_(CONTENT_TYPE_NONE),
      edit_flags_(blink::WebContextMenuData::CanDoNone),
      weak_factory_(this) {
  CHECK(client) << "Didn't specify a client";
}

void WebView::CommonInit(WebContentsUniquePtr contents,
                         WebContentsViewClient* view_client,
                         WebContentsClient* contents_client) {
  web_contents_ = std::move(contents);

  // Attach ourself to the WebContents
  web_contents_->SetDelegate(this);
  web_contents_->SetUserData(&kUserDataKey,
                             new UnownedUserData<WebView>(this));

  content::WebContentsObserver::Observe(web_contents_.get());

  WebContentsView* view = WebContentsView::FromWebContents(web_contents_.get());
  view->SetClient(view_client);
  view->set_editing_capabilities_changed_callback(
      base::Bind(&WebView::EditingCapabilitiesChanged,
                 base::Unretained(this)));
  swap_compositor_frame_subscription_ =
      view->AddSwapCompositorFrameCallback(
          base::Bind(&WebView::OnSwapCompositorFrame,
                     base::Unretained(this)));

  DCHECK(GetWebContentsHelper());
  GetWebContentsHelper()->SetClient(contents_client);
}

RenderWidgetHostView* WebView::GetRenderWidgetHostView() const {
  content::RenderWidgetHostView* rwhv =
      web_contents_->GetFullscreenRenderWidgetHostView();
  if (!rwhv) {
    rwhv = web_contents_->GetRenderWidgetHostView();
  }

  return static_cast<RenderWidgetHostView *>(rwhv);
}

content::RenderWidgetHost* WebView::GetRenderWidgetHost() const {
  RenderWidgetHostView* rwhv = GetRenderWidgetHostView();
  if (!rwhv) {
    return nullptr;
  }

  return rwhv->GetRenderWidgetHost();
}

gfx::Size WebView::GetViewSizeDip() const {
  return GetViewBoundsDip().size();
}

gfx::Rect WebView::GetViewBoundsDip() const {
  return WebContentsView::FromWebContents(web_contents_.get())->GetBounds();
}

bool WebView::IsFullscreen() const {
  if (!web_contents_) {
    // We're called in the constructor via GetViewSizeDip
    return false;
  }

  return FullscreenHelper::FromWebContents(web_contents_.get())->IsFullscreen();
}

void WebView::DispatchLoadFailed(const GURL& validated_url,
                                 int error_code,
                                 const base::string16& error_description,
                                 bool is_provisional_load) {
  if (error_code == net::ERR_ABORTED) {
    client_->LoadStopped(validated_url);
  } else {
    content::NavigationEntry* entry =
      web_contents_->GetController().GetLastCommittedEntry();
    client_->LoadFailed(validated_url,
                        error_code,
                        base::UTF16ToUTF8(error_description),
                        is_provisional_load ?
                            0 : entry->GetHttpStatusCode());
  }
}

void WebView::OnDidBlockDisplayingInsecureContent() {
  if (blocked_content_ & CONTENT_TYPE_MIXED_DISPLAY) {
    return;
  }

  blocked_content_ |= CONTENT_TYPE_MIXED_DISPLAY;

  client_->ContentBlocked();
}

void WebView::OnDidBlockRunningInsecureContent() {
  if (blocked_content_ & CONTENT_TYPE_MIXED_SCRIPT) {
    return;
  }

  blocked_content_ |= CONTENT_TYPE_MIXED_SCRIPT;

  client_->ContentBlocked();
}

void WebView::DispatchPrepareToCloseResponse(bool proceed) {
  client_->PrepareToCloseResponseReceived(proceed);
}

void WebView::MaybeCancelFullscreenMode() {
  if (IsFullscreen()) {
    // The application might have granted fullscreen by now
    return;
  }

  web_contents_->ExitFullscreen(false);
}

void WebView::OnSwapCompositorFrame(
    const CompositorFrameData* data,
    const cc::CompositorFrameMetadata& metadata) {
  cc::CompositorFrameMetadata old = std::move(compositor_frame_metadata_);
  compositor_frame_metadata_ = metadata.Clone();

  client_->FrameMetadataUpdated(old);
}

void WebView::EditingCapabilitiesChanged() {
  blink::WebContextMenuData::EditFlags flags =
      WebContentsView::FromWebContents(web_contents_.get())
          ->GetEditingCapabilities();

  if (flags != edit_flags_) {
    edit_flags_ = flags;
    client_->OnEditingCapabilitiesChanged();
  }
}

size_t WebView::GetScriptMessageHandlerCount() const {
  return client_->GetScriptMessageHandlerCount();
}

const ScriptMessageHandler* WebView::GetScriptMessageHandlerAt(
    size_t index) const {
  return client_->GetScriptMessageHandlerAt(index);
}

content::WebContents* WebView::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  DCHECK_VALID_SOURCE_CONTENTS

  if (params.disposition != WindowOpenDisposition::CURRENT_TAB &&
      params.disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB &&
      params.disposition != WindowOpenDisposition::NEW_BACKGROUND_TAB &&
      params.disposition != WindowOpenDisposition::NEW_POPUP &&
      params.disposition != WindowOpenDisposition::NEW_WINDOW) {
    return nullptr;
  }

  // Block popups
  UserAgentSettings* ua_settings =
      UserAgentSettings::Get(web_contents_->GetBrowserContext());
  if ((params.disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
       params.disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB ||
       params.disposition == WindowOpenDisposition::NEW_WINDOW ||
       params.disposition == WindowOpenDisposition::NEW_POPUP) &&
      !params.user_gesture && ua_settings->IsPopupBlockerEnabled()) {
    return nullptr;
  }

  WindowOpenDisposition disposition = params.disposition;

  WebContentsClient* contents_client = GetWebContentsHelper()->client();

  // If we can't create new windows, this should be a CURRENT_TAB navigation
  // in the top-level frame
  if ((!contents_client || !contents_client->CanCreateWindows()) &&
      disposition != WindowOpenDisposition::CURRENT_TAB) {
    disposition = WindowOpenDisposition::CURRENT_TAB;
  }

  // Handle the CURRENT_TAB case now
  if (disposition == WindowOpenDisposition::CURRENT_TAB) {
    // XXX: We have no way to propagate OpenURLParams::user_gesture here, so
    // ResourceRequestInfo::HasUserGesture will always return false in
    // NavigationInterceptResourceThrottle
    // See https://launchpad.net/bugs/1499434
    content::NavigationController::LoadURLParams load_params(params.url);
    FillLoadURLParamsFromOpenURLParams(&load_params, params);

    web_contents_->GetController().LoadURLWithParams(load_params);

    return web_contents_.get();
  }

  // At this point, we expect all navigations to be for the main frame
  DCHECK_EQ(params.frame_tree_node_id, -1);

  // Coerce all non CURRENT_TAB navigations that don't come from a user
  // gesture to NEW_POPUP
  if (!params.user_gesture) {
    disposition = WindowOpenDisposition::NEW_POPUP;
  }

  // If there's no WebContentsClient, then we can't open a new view
  if (!contents_client) {
    return nullptr;
  }

  // Give the application a chance to block the navigation if it is
  // renderer initiated
  if (params.is_renderer_initiated &&
      !contents_client->ShouldCreateNewWebContents(params.url,
                                                   disposition,
                                                   params.user_gesture)) {
    return nullptr;
  }

  // XXX(chrisccoulson): Is there a way to tell when the opener shouldn't
  // be suppressed?
  bool opener_suppressed = true;

  content::WebContents::CreateParams contents_params(
      GetBrowserContext(),
      opener_suppressed ? nullptr : web_contents_->GetSiteInstance());
  contents_params.initial_size = GetViewSizeDip();
  contents_params.initially_hidden =
      disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB;
  contents_params.opener_render_process_id =
      web_contents_->GetRenderProcessHost()->GetID();
  // XXX(chrisccoulson): This is probably wrong, but we're going to revisit
  //  navigations anyway, and opener_suppressed is currently always true so
  //  this is ignored
  contents_params.opener_render_frame_id =
      web_contents_->GetMainFrame()->GetRoutingID();
  contents_params.opener_suppressed = opener_suppressed;

  WebContentsUniquePtr contents =
      WebContentsHelper::CreateWebContents(contents_params);
  CreateHelpers(contents.get());

  WebContentsHelper* helper = WebContentsHelper::FromWebContents(contents.get());

  bool created =
      GetWebContentsHelper()->client()->AdoptNewWebContents(GetViewBoundsDip(),
                                                            disposition,
                                                            std::move(contents));
  if (!created) {
    return nullptr;
  }

  content::NavigationController::LoadURLParams load_params(params.url);
  FillLoadURLParamsFromOpenURLParams(&load_params, params);

  helper->GetWebContents()->GetController().LoadURLWithParams(load_params);

  return helper->GetWebContents();
}

void WebView::NavigationStateChanged(content::WebContents* source,
                                     content::InvalidateTypes changed_flags) {
  DCHECK_VALID_SOURCE_CONTENTS

  NavigationControllerObserver::NotifyNavigationStateChanged(
      web_contents_->GetController(),
      changed_flags);

  if (changed_flags & content::INVALIDATE_TYPE_URL) {
    client_->URLChanged();
  }

  if (changed_flags & content::INVALIDATE_TYPE_TITLE) {
    client_->TitleChanged();
  }
}

void WebView::VisibleSecurityStateChanged(content::WebContents* source) {
  DCHECK_VALID_SOURCE_CONTENTS

  SecurityStatus::FromWebContents(web_contents_.get())
      ->VisibleSecurityStateChanged();
}

bool WebView::ShouldCreateWebContents(
    content::WebContents* source,
    content::SiteInstance* source_site_instance,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace,
    WindowOpenDisposition disposition,
    bool user_gesture) {
  DCHECK_VALID_SOURCE_CONTENTS

  if (disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB &&
      disposition != WindowOpenDisposition::NEW_BACKGROUND_TAB &&
      disposition != WindowOpenDisposition::NEW_POPUP &&
      disposition != WindowOpenDisposition::NEW_WINDOW) {
    return false;
  }

  // Note that popup blocking was done on the IO thread

  if (!GetWebContentsHelper()->client()) {
    return false;
  }

  if (!GetWebContentsHelper()->client()->CanCreateWindows()) {
    return false;
  }

  return GetWebContentsHelper()->client()->ShouldCreateNewWebContents(
      target_url,
      user_gesture ? disposition : WindowOpenDisposition::NEW_POPUP,
      user_gesture);
}

void WebView::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  DCHECK_VALID_SOURCE_CONTENTS

  WebContentsView::FromWebContents(web_contents_.get())
      ->client()
      ->UnhandledKeyboardEvent(event);
}

void WebView::WebContentsCreated(content::WebContents* source,
                                 int opener_render_process_id,
                                 int opener_render_frame_id,
                                 const std::string& frame_name,
                                 const GURL& target_url,
                                 content::WebContents* new_contents) {
  DCHECK_VALID_SOURCE_CONTENTS
  DCHECK(!WebView::FromWebContents(new_contents));

  WebContentsHelper::CreateForWebContents(new_contents, web_contents_.get());
  CreateHelpers(new_contents);
}

void WebView::RendererUnresponsive(
    content::WebContents* source,
    const content::WebContentsUnresponsiveState& unresponsive_state) {
  DCHECK_VALID_SOURCE_CONTENTS
  WebProcessStatusMonitor::FromWebContents(web_contents_.get())
      ->RendererIsUnresponsive();
}

void WebView::RendererResponsive(content::WebContents* source) {
  DCHECK_VALID_SOURCE_CONTENTS
  WebProcessStatusMonitor::FromWebContents(web_contents_.get())
      ->RendererIsResponsive();
}

void WebView::AddNewContents(content::WebContents* source,
                             content::WebContents* new_contents,
                             WindowOpenDisposition disposition,
                             const gfx::Rect& initial_pos,
                             bool user_gesture,
                             bool* was_blocked) {
  DCHECK_VALID_SOURCE_CONTENTS
  DCHECK(disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
         disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB ||
         disposition == WindowOpenDisposition::NEW_POPUP ||
         disposition == WindowOpenDisposition::NEW_WINDOW) <<
             "Invalid disposition";
  DCHECK_EQ(GetBrowserContext(),
            BrowserContext::FromContent(new_contents->GetBrowserContext()));

  if (was_blocked) {
    *was_blocked = true;
  }

  WebContentsUniquePtr contents(new_contents);

  if (!GetWebContentsHelper()->client()) {
    return;
  }

  bool created =
      GetWebContentsHelper()->client()->AdoptNewWebContents(
          initial_pos,
          user_gesture ? disposition : WindowOpenDisposition::NEW_POPUP,
          std::move(contents));
  if (!created) {
    return;
  }

  if (was_blocked) {
    *was_blocked = false;
  }
}

void WebView::LoadProgressChanged(content::WebContents* source,
                                  double progress) {
  DCHECK_VALID_SOURCE_CONTENTS

  // XXX: Is there a way to update this when we adopt a WebContents?
  client_->LoadProgressChanged(progress);
}

void WebView::CloseContents(content::WebContents* source) {
  DCHECK_VALID_SOURCE_CONTENTS

  client_->CloseRequested();
}

void WebView::UpdateTargetURL(content::WebContents* source, const GURL& url) {
  DCHECK_VALID_SOURCE_CONTENTS

  if (url != target_url_) {
    target_url_ = url;
    client_->TargetURLChanged();
  }
}

bool WebView::DidAddMessageToConsole(content::WebContents* source,
                                     int32_t level,
                                     const base::string16& message,
                                     int32_t line_no,
                                     const base::string16& source_id) {
  DCHECK_VALID_SOURCE_CONTENTS

  return client_->AddMessageToConsole(level, message, line_no, source_id);
}

void WebView::BeforeUnloadFired(content::WebContents* source,
                                bool proceed,
                                bool* proceed_to_fire_unload) {
  DCHECK_VALID_SOURCE_CONTENTS

  // We take care of the unload handler on deletion
  *proceed_to_fire_unload = false;

  client_->PrepareToCloseResponseReceived(proceed);
}

content::JavaScriptDialogManager* WebView::GetJavaScriptDialogManager(
    content::WebContents* source) {
  DCHECK_VALID_SOURCE_CONTENTS
  return JavaScriptDialogContentsHelper::FromWebContents(web_contents_.get());
}

void WebView::RunFileChooser(content::RenderFrameHost* render_frame_host,
                             const content::FileChooserParams& params) {
  FilePicker* file_picker = client_->CreateFilePicker(render_frame_host);
  if (!file_picker) {
    std::vector<content::FileChooserFileInfo> empty;
    render_frame_host->FilesSelectedInChooser(empty, params.mode);
    return;
  }

  file_picker->Run(params);
}

bool WebView::EmbedsFullscreenWidget() const {
  return true;
}

void WebView::EnterFullscreenModeForTab(content::WebContents* source,
                                        const GURL& origin) {
  DCHECK_VALID_SOURCE_CONTENTS

  FullscreenHelper::FromWebContents(
      web_contents_.get())->EnterFullscreenMode(origin);
}

void WebView::ExitFullscreenModeForTab(content::WebContents* source) {
  DCHECK_VALID_SOURCE_CONTENTS

  FullscreenHelper::FromWebContents(
      web_contents_.get())->ExitFullscreenMode();
}

bool WebView::IsFullscreenForTabOrPending(
    const content::WebContents* source) const {
  DCHECK_VALID_SOURCE_CONTENTS

  return IsFullscreen();
}

void WebView::FindReply(content::WebContents* source,
                        int request_id,
                        int number_of_matches,
                        const gfx::Rect& selection_rect,
                        int active_match_ordinal,
                        bool final_update) {
  DCHECK_VALID_SOURCE_CONTENTS

  FindController::FromWebContents(web_contents_.get())->HandleFindReply(
      request_id, number_of_matches, active_match_ordinal);
}

void WebView::RequestMediaAccessPermission(
    content::WebContents* source,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  DCHECK_VALID_SOURCE_CONTENTS

  MediaCaptureDevicesDispatcher::GetInstance()->RequestMediaAccessPermission(
      request,
      callback);
}

bool WebView::CheckMediaAccessPermission(content::WebContents* source,
                                         const GURL& security_origin,
                                         content::MediaStreamType type) {
  DCHECK_VALID_SOURCE_CONTENTS
  DCHECK(type == content::MEDIA_DEVICE_VIDEO_CAPTURE ||
         type == content::MEDIA_DEVICE_AUDIO_CAPTURE);

  TemporarySavedPermissionType permission =
      type == content::MEDIA_DEVICE_VIDEO_CAPTURE ?
        TEMPORARY_SAVED_PERMISSION_TYPE_MEDIA_DEVICE_CAMERA :
        TEMPORARY_SAVED_PERMISSION_TYPE_MEDIA_DEVICE_MIC;

  TemporarySavedPermissionStatus status =
      GetBrowserContext()->GetTemporarySavedPermissionContext()
        ->GetPermissionStatus(permission,
                              security_origin,
                              web_contents_->GetLastCommittedURL().GetOrigin());
  return status == TEMPORARY_SAVED_PERMISSION_STATUS_ALLOWED;
}

void WebView::DidStartLoading() {
  client_->LoadingChanged();
}

void WebView::DidStopLoading() {
  client_->LoadingChanged();
}

void WebView::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                            const GURL& validated_url) {
  if (render_frame_host->GetParent()) {
    return;
  }

  if (validated_url.spec() == content::kUnreachableWebDataURL) {
    return;
  }

  content::NavigationEntry* entry =
      web_contents_->GetController().GetLastCommittedEntry();
  // Some transient about:blank navigations dont have navigation entries.
  client_->LoadSucceeded(
      validated_url,
      entry ? entry->GetHttpStatusCode() : 0);
}

void WebView::DidFailLoad(content::RenderFrameHost* render_frame_host,
                          const GURL& validated_url,
                          int error_code,
                          const base::string16& error_description,
                          bool was_ignored_by_handler) {
  if (render_frame_host->GetParent()) {
    return;
  }

  DispatchLoadFailed(validated_url, error_code, error_description);
}

void WebView::DidStartProvisionalLoadForFrame(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    bool is_error_frame) {
  if (is_error_frame) {
    return;
  }

  if (render_frame_host->GetParent()) {
    return;
  }

  client_->LoadStarted(validated_url);
}

void WebView::DidCommitProvisionalLoadForFrame(
    content::RenderFrameHost* render_frame_host,
    const GURL& url,
    ui::PageTransition transition_type) {
  if (render_frame_host->GetParent()) {
    return;
  }

  content::NavigationEntry* entry =
      web_contents_->GetController().GetLastCommittedEntry();
  client_->LoadCommitted(url,
                         entry->GetPageType() == content::PAGE_TYPE_ERROR,
                         entry->GetHttpStatusCode());
}

void WebView::DidFailProvisionalLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description,
    bool was_ignored_by_handler) {
  if (render_frame_host->GetParent()) {
    return;
  }

  if (validated_url.spec() == content::kUnreachableWebDataURL) {
    return;
  }

  DispatchLoadFailed(validated_url, error_code, error_description, true);
}

void WebView::DidNavigateMainFrame(
    const content::LoadCommittedDetails& details,
    const content::FrameNavigateParams& params) {
  if (details.is_navigation_to_different_page()) {
    blocked_content_ = CONTENT_TYPE_NONE;
    client_->ContentBlocked();
  }
}

void WebView::DidGetRedirectForResourceRequest(
      const content::ResourceRedirectDetails& details) {
  if (details.resource_type != content::RESOURCE_TYPE_MAIN_FRAME) {
    return;
  }

  client_->LoadRedirected(details.new_url,
                          details.original_url,
                          details.http_response_code);
}

void WebView::DidShowFullscreenWidget() {
  if (IsFullscreen()) {
    return;
  }

  // If the application didn't grant us fullscreen, schedule a task to cancel
  // the fullscreen. We do this as we'll have a fullscreen view that the
  // application can't get rid of.
  // We do this asynchronously to avoid a UAF in
  // WebContentsImpl::ShowCreatedWidget
  // See https://launchpad.net/bugs/1510973
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&WebView::MaybeCancelFullscreenMode, AsWeakPtr()));
}

bool WebView::OnMessageReceived(const IPC::Message& msg,
                                content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebView, msg)
    IPC_MESSAGE_HANDLER(OxideHostMsg_DidBlockDisplayingInsecureContent,
                        OnDidBlockDisplayingInsecureContent)
    IPC_MESSAGE_HANDLER(OxideHostMsg_DidBlockRunningInsecureContent,
                        OnDidBlockRunningInsecureContent)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebView::ClipboardDataChanged(ui::ClipboardType type) {
  if (type != ui::CLIPBOARD_TYPE_COPY_PASTE) {
    return;
  }

  EditingCapabilitiesChanged();
}

WebView::WebView(const CommonParams& common_params,
                 const CreateParams& create_params)
    : WebView(common_params.client) {
  CHECK(create_params.context) << "Didn't specify a BrowserContext";

  BrowserContext* context = create_params.incognito ?
      create_params.context->GetOffTheRecordContext() :
      create_params.context->GetOriginalContext();

  content::WebContents::CreateParams content_params(context);
  content_params.initial_size =
      gfx::ToEnclosingRect(common_params.view_client->GetBounds()).size();
  content_params.initially_hidden = !common_params.view_client->IsVisible();

  WebContentsUniquePtr contents =
      WebContentsHelper::CreateWebContents(content_params);

  CreateHelpers(contents.get());
  CommonInit(std::move(contents),
             common_params.view_client,
             common_params.contents_client);

  if (create_params.restore_entries.size() > 0) {
    std::vector<std::unique_ptr<content::NavigationEntry>> entries =
        sessions::ContentSerializedNavigationBuilder::ToNavigationEntries(
            create_params.restore_entries, context);
    web_contents_->GetController().Restore(
        create_params.restore_index,
        create_params.restore_type,
        &entries);
    web_contents_->GetController().LoadIfNecessary();
  }
}

WebView::WebView(const CommonParams& common_params,
                 WebContentsUniquePtr contents)
    : WebView(common_params.client) {
  CHECK(contents);
  DCHECK(contents->GetBrowserContext()) <<
         "Specified WebContents doesn't have a BrowserContext";
  CHECK(!FromWebContents(contents.get())) <<
        "Specified WebContents already belongs to a WebView";

  CommonInit(std::move(contents),
             common_params.view_client,
             common_params.contents_client);
}

WebView::~WebView() {
  WebContentsView* view = WebContentsView::FromWebContents(web_contents_.get());
  view->SetClient(nullptr);
  view->set_editing_capabilities_changed_callback(base::Closure());

  GetWebContentsHelper()->SetClient(nullptr);

  // Stop WebContents from calling back in to us
  content::WebContentsObserver::Observe(nullptr);

  // It's time we split the WebContentsDelegate implementation from WebView,
  // given that a lot of functionality that is interested in it lives outside
  // now
  web_contents_->SetDelegate(nullptr);

  web_contents_->RemoveUserData(&kUserDataKey);
}

void WebView::SetJavaScriptDialogFactory(JavaScriptDialogFactory* factory) {
  JavaScriptDialogContentsHelper::FromWebContents(web_contents_.get())
      ->set_factory(factory);
}

// static
WebView* WebView::FromWebContents(const content::WebContents* web_contents) {
  UnownedUserData<WebView>* data =
      static_cast<UnownedUserData<WebView>*>(
        web_contents->GetUserData(&kUserDataKey));
  if (!data) {
    return nullptr;
  }

  return data->get();
}

// static
WebView* WebView::FromRenderViewHost(content::RenderViewHost* rvh) {
  content::WebContents* contents =
      content::WebContents::FromRenderViewHost(rvh);
  if (!contents) {
    return nullptr;
  }

  return FromWebContents(contents);
}

// static
WebView* WebView::FromRenderFrameHost(content::RenderFrameHost* rfh) {
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (!contents) {
    return nullptr;
  }

  return FromWebContents(contents);
}

const GURL& WebView::GetURL() const {
  return web_contents_->GetVisibleURL();
}

void WebView::SetURL(const GURL& url) {
  if (!url.is_valid()) {
    return;
  }

  content::NavigationController::LoadURLParams params(url);
  params.transition_type =
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_API;

  web_contents_->GetController().LoadURLWithParams(params);
}

std::vector<sessions::SerializedNavigationEntry> WebView::GetState() const {
  std::vector<sessions::SerializedNavigationEntry> entries;
  const content::NavigationController& controller =
      web_contents_->GetController();
  const int pending_index = controller.GetPendingEntryIndex();
  int entry_count = controller.GetEntryCount();
  if (entry_count == 0 && pending_index == 0) {
    entry_count++;
  }
  entries.resize(entry_count);
  for (int i = 0; i < entry_count; ++i) {
    content::NavigationEntry* entry = (i == pending_index) ?
        controller.GetPendingEntry() : controller.GetEntryAtIndex(i);
    entries[i] =
        sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
          i, *entry);
  }
  return entries;
}

void WebView::LoadData(const std::string& encoded_data,
                       const std::string& mime_type,
                       const GURL& base_url) {
  std::string url("data:");
  url.append(mime_type);
  url.append(",");
  url.append(encoded_data);

  content::NavigationController::LoadURLParams params((GURL(url)));
  params.transition_type =
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_API;
  params.load_type = content::NavigationController::LOAD_TYPE_DATA;
  params.base_url_for_data_url = base_url;
  params.virtual_url_for_data_url =
      base_url.is_empty() ? GURL(url::kAboutBlankURL) : base_url;
  params.can_load_local_resources = true;

  web_contents_->GetController().LoadURLWithParams(params);
}

std::string WebView::GetTitle() const {
  return base::UTF16ToUTF8(web_contents_->GetTitle());
}

const GURL& WebView::GetFaviconURL() const {
  return FaviconHelper::FromWebContents(web_contents_.get())->GetFaviconURL();
}

bool WebView::CanGoBack() const {
  return web_contents_->GetController().CanGoBack();
}

bool WebView::CanGoForward() const {
  return web_contents_->GetController().CanGoForward();
}

void WebView::GoBack() {
  if (!CanGoBack()) {
    return;
  }

  web_contents_->GetController().GoBack();
}

void WebView::GoForward() {
  if (!CanGoForward()) {
    return;
  }

  web_contents_->GetController().GoForward();
}

void WebView::Stop() {
  web_contents_->Stop();
}

void WebView::Reload() {
  web_contents_->GetController().Reload(content::ReloadType::NORMAL, true);
}

bool WebView::IsIncognito() const {
  return GetBrowserContext()->IsOffTheRecord();
}

bool WebView::IsLoading() const {
  return web_contents_->IsLoading();
}

BrowserContext* WebView::GetBrowserContext() const {
  return BrowserContext::FromContent(web_contents_->GetBrowserContext());
}

content::WebContents* WebView::GetWebContents() const {
  return web_contents_.get();
}

WebContentsHelper* WebView::GetWebContentsHelper() const {
  return WebContentsHelper::FromWebContents(web_contents_.get());
}

WebFrame* WebView::GetRootFrame() const {
  return WebFrameTree::FromWebContents(web_contents_.get())->root_frame();
}

gfx::Point WebView::GetCompositorFrameScrollOffset() {
  // See https://launchpad.net/bugs/1336730
  const gfx::SizeF& viewport_size =
      compositor_frame_metadata().scrollable_viewport_size;
  float x_scale = compositor_frame_metadata().page_scale_factor *
                  viewport_size.width() / std::round(viewport_size.width());
  float y_scale = compositor_frame_metadata().page_scale_factor *
                  viewport_size.height() / std::round(viewport_size.height());

  gfx::Vector2dF offset =
      gfx::ScaleVector2d(compositor_frame_metadata().root_scroll_offset,
                         x_scale, y_scale);

  return gfx::Point(std::round(offset.x()), std::round(offset.y()));
}

gfx::Size WebView::GetCompositorFrameContentSize() {
  // See https://launchpad.net/bugs/1336730
  const gfx::SizeF& viewport_size =
      compositor_frame_metadata().scrollable_viewport_size;
  float x_scale = compositor_frame_metadata().page_scale_factor *
                  viewport_size.width() / std::round(viewport_size.width());
  float y_scale = compositor_frame_metadata().page_scale_factor *
                  viewport_size.height() / std::round(viewport_size.height());

  gfx::SizeF size =
      gfx::ScaleSize(compositor_frame_metadata().root_layer_size,
                     x_scale, y_scale);

  return gfx::Size(std::round(size.width()), std::round(size.height()));
}

gfx::Size WebView::GetCompositorFrameViewportSize() {
  gfx::SizeF size =
      gfx::ScaleSize(compositor_frame_metadata().scrollable_viewport_size,
                     compositor_frame_metadata().page_scale_factor);

  return gfx::Size(std::round(size.width()), std::round(size.height()));
}

void WebView::SetCanTemporarilyDisplayInsecureContent(bool allow) {
  if (!(blocked_content_ & CONTENT_TYPE_MIXED_DISPLAY) && allow) {
    return;
  }

  if (allow) {
    blocked_content_ &= ~CONTENT_TYPE_MIXED_DISPLAY;
    client_->ContentBlocked();
  }

  web_contents_->SendToAllFrames(
      new OxideMsg_SetAllowDisplayingInsecureContent(MSG_ROUTING_NONE, allow));
  web_contents_->GetMainFrame()->Send(
      new OxideMsg_ReloadFrame(web_contents_->GetMainFrame()->GetRoutingID()));
}

void WebView::SetCanTemporarilyRunInsecureContent(bool allow) {
  if (!web_contents_) {
    return;
  }

  if (!(blocked_content_ & CONTENT_TYPE_MIXED_SCRIPT) && allow) {
    return;
  }

  if (allow) {
    blocked_content_ &= ~CONTENT_TYPE_MIXED_DISPLAY;
    blocked_content_ &= ~CONTENT_TYPE_MIXED_SCRIPT;
    client_->ContentBlocked();
  }

  web_contents_->SendToAllFrames(
      new OxideMsg_SetAllowRunningInsecureContent(MSG_ROUTING_NONE, allow));
  web_contents_->GetMainFrame()->Send(
      new OxideMsg_ReloadFrame(web_contents_->GetMainFrame()->GetRoutingID()));
}

void WebView::PrepareToClose() {
  base::Closure no_before_unload_handler_response_task =
      base::Bind(&WebView::DispatchPrepareToCloseResponse,
                 AsWeakPtr(), true);

  if (!web_contents_->NeedToFireBeforeUnload()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&WebView::DispatchPrepareToCloseResponse,
                   AsWeakPtr(), true));
    return;
  }

  // This is ok to call multiple times - RFHI tracks whether a response
  // is pending and won't dispatch another event if it is
  web_contents_->DispatchBeforeUnload();
}

blink::WebContextMenuData::EditFlags WebView::GetEditFlags() const {
  return edit_flags_;
}

void WebView::TerminateWebProcess() {
  content::RenderProcessHost* host = web_contents_->GetRenderProcessHost();
  if (!host) {
    return;
  }

  bool is_hung =
      WebProcessStatusMonitor::FromWebContents(web_contents_.get())
          ->GetStatus() == WebProcessStatusMonitor::Status::Unresponsive;

  host->Shutdown(is_hung ?
                     content::RESULT_CODE_HUNG : content::RESULT_CODE_KILLED,
                 false);
}

} // namespace oxide
