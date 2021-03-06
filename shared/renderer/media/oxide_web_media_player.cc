// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "oxide_web_media_player.h"
#include "oxide_renderer_media_player_manager.h"

#include <limits>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/layers/video_layer.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_frame.h"
#include "media/base/key_systems.h"
#include "media/base/media_content_type.h"
#include "media/base/media_log.h"
#include "media/blink/webcontentdecryptionmodule_impl.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_util.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_frame.h"
#include "net/base/mime_util.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerSource.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "ui/gfx/image/image.h"
// #include "webkit/renderer/compositor_bindings/web_layer_impl.h"

static const uint32_t kGLTextureExternalOES = 0x8D65;

using blink::WebMediaPlayer;
using blink::WebSize;
using blink::WebString;
using blink::WebTimeRanges;
using blink::WebURL;
using media::VideoFrame;

namespace oxide {

WebMediaPlayer::WebMediaPlayer(
    blink::WebFrame* frame,
    blink::WebMediaPlayerClient* client,
    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
    RendererMediaPlayerManager* player_manager,
    media::MediaLog* media_log)
    : content::RenderFrameObserver(content::RenderFrame::FromWebFrame(frame)),
      frame_(frame),
      client_(client),
      delegate_(delegate),
      delegate_id_(0),
      buffered_(static_cast<size_t>(1)),
      ignore_metadata_duration_change_(false),
      pending_seek_(false),
      seeking_(false),
      did_loading_progress_(false),
      player_manager_(player_manager),
      network_state_(WebMediaPlayer::NetworkStateEmpty),
      ready_state_(WebMediaPlayer::ReadyStateHaveNothing),
      is_playing_(false),
      playing_started_(false),
      has_size_info_(false),
      has_media_metadata_(false),
      has_media_info_(false),
      pending_playback_(false),
      player_type_(MEDIA_PLAYER_TYPE_URL),
      current_time_(0),
      is_remote_(false),
      media_log_(media_log),
      weak_factory_(this) {
  DCHECK(player_manager_);
  DCHECK(main_thread_checker_.CalledOnValidThread());

  if (delegate_)
    delegate_id_ = delegate_->AddObserver(this);

  player_id_ = player_manager_->RegisterMediaPlayer(this);
}

WebMediaPlayer::~WebMediaPlayer() {
  client_->setWebLayer(nullptr);

  if (player_manager_) {
    player_manager_->DestroyPlayer(player_id_);
    player_manager_->UnregisterMediaPlayer(player_id_);
  }

  if (player_type_ == MEDIA_PLAYER_TYPE_MEDIA_SOURCE && delegate_) {
    delegate_->PlayerGone(delegate_id_);
  }
}

void WebMediaPlayer::load(LoadType load_type,
                          const blink::WebMediaPlayerSource& source,
                          CORSMode cors_mode) {
  switch (load_type) {
    case LoadTypeURL:
      player_type_ = MEDIA_PLAYER_TYPE_URL;
      break;

    case LoadTypeMediaSource:
      CHECK(false) << "WebMediaPlayer doesn't support MediaSource on "
                      "this platform";
      return;

    case LoadTypeMediaStream:
      CHECK(false) << "WebMediaPlayer doesn't support MediaStream on "
                      "this platform";
      return;
  }

  has_media_metadata_ = false;
  has_media_info_ = false;

  info_loader_.reset(
      new MediaInfoLoader(
          source.getAsURL(),
          cors_mode,
          base::Bind(&WebMediaPlayer::DidLoadMediaInfo,
                     weak_factory_.GetWeakPtr())));

  // The url might be redirected when android media player
  // requests the stream. As a result, we cannot guarantee there is only
  // a single origin. Remove the following line when b/12573548 is fixed.
  // Check http://crbug.com/334204.

  info_loader_->set_single_origin(false);
  info_loader_->Start(frame_);

  url_ = source.getAsURL();

  if (player_manager_) {
    GURL first_party_url = frame_->document().firstPartyForCookies();
    player_manager_->Initialize(
        player_type_, player_id_, url_, first_party_url);
  }

  UpdateNetworkState(WebMediaPlayer::NetworkStateLoading);
  UpdateReadyState(WebMediaPlayer::ReadyStateHaveNothing);
}

void WebMediaPlayer::DidLoadMediaInfo(MediaInfoLoader::Status status) {
  if (status == MediaInfoLoader::kFailed) {
    info_loader_.reset();
    UpdateNetworkState(WebMediaPlayer::NetworkStateNetworkError);
    return;
  }

  has_media_info_ = true;
  if (has_media_metadata_ &&
      ready_state_ != WebMediaPlayer::ReadyStateHaveEnoughData) {
    UpdateReadyState(WebMediaPlayer::ReadyStateHaveMetadata);
    UpdateReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
  }
  // Android doesn't start fetching resources until an implementation-defined
  // event (e.g. playback request) occurs. Sets the network state to IDLE
  // if play is not requested yet.
  if (!playing_started_) {
    UpdateNetworkState(WebMediaPlayer::NetworkStateIdle);
  }
}

void WebMediaPlayer::play() {
  if (paused()) {
    player_manager_->Start(player_id_);
  }

  UpdatePlayingState(true);
  UpdateNetworkState(WebMediaPlayer::NetworkStateLoading);

  playing_started_ = true;
}

void WebMediaPlayer::pause() {
  Pause(true);
}

void WebMediaPlayer::seek(double seconds) {
  player_manager_->Seek(player_id_, base::TimeDelta::FromSeconds(seconds));
}

bool WebMediaPlayer::supportsSave() const {
  return false;
}

void WebMediaPlayer::setRate(double rate) {
  player_manager_->SetRate(player_id_, rate);
}

void WebMediaPlayer::setSinkId(
      const blink::WebString& sinkId,
      const blink::WebSecurityOrigin& origin,
      blink::WebSetSinkIdCallbacks* callbacks) {
  NOTIMPLEMENTED();
}

void WebMediaPlayer::setVolume(double volume) {
  player_manager_->SetVolume(player_id_, volume);
}

bool WebMediaPlayer::hasVideo() const {
  if (has_size_info_) {
    return !natural_size_.isEmpty();
  }
  // We don't know whether the current media content has video unless
  // the player is prepared. If the player is not prepared, we fall back
  // to the mime-type. There may be no mime-type on a redirect URL.
  // In that case, we conservatively assume it contains video so that
  // enterfullscreen call will not fail.

  if (!url_.has_path()) {
    return false;
  }

  std::string mime;
  if (!net::GetMimeTypeFromFile(base::FilePath(url_.path()), &mime)) {
    return true;
  }

  return mime.find("audio/") == std::string::npos;
}

void WebMediaPlayer::setPoster(const blink::WebURL& poster) {
  //  player_manager_->SetPoster(player_id_, poster);
}

bool WebMediaPlayer::hasAudio() const {
  if (!url_.has_path()) {
    return false;
  }

  std::string mime;
  if (!net::GetMimeTypeFromFile(base::FilePath(url_.path()), &mime)) {
    return true;
  }

  if (mime.find("audio/") != std::string::npos ||
      mime.find("video/") != std::string::npos ||
      mime.find("application/ogg") != std::string::npos) {
    return true;
  }

  return false;
}

bool WebMediaPlayer::paused() const {
  return !is_playing_;
}

bool WebMediaPlayer::seeking() const {
  return seeking_;
}

double WebMediaPlayer::duration() const {
  // HTML5 spec requires duration to be NaN if readyState is HAVE_NOTHING
  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (duration_ == media::kInfiniteDuration) {
    return std::numeric_limits<double>::infinity();
  }
  return duration_.InSecondsF();
}

double WebMediaPlayer::timelineOffset() const {
  base::Time timeline_offset;

  if (timeline_offset.is_null()) {

    return std::numeric_limits<double>::quiet_NaN();
  }

  return timeline_offset.ToJsTime();
}

double WebMediaPlayer::currentTime() const {
  // If the player is processing a seek, return the seek time.
  // Blink may still query us if updatePlaybackState() occurs while seeking.
  if (seeking()) {
    return pending_seek_ ?
        pending_seek_time_.InSecondsF() : seek_time_.InSecondsF();
  }

  return current_time_;
}

WebSize WebMediaPlayer::naturalSize() const {
  return natural_size_;
}

WebMediaPlayer::NetworkState WebMediaPlayer::getNetworkState() const {
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayer::getReadyState() const {
  return ready_state_;
}

WebTimeRanges WebMediaPlayer::buffered() const {
  return buffered_;
}

WebTimeRanges WebMediaPlayer::seekable() const {
  const blink::WebTimeRange seekable_range(0.0, duration());
  return blink::WebTimeRanges(&seekable_range, 1);
}

double WebMediaPlayer::maxTimeSeekable() const {
  // If we haven't even gotten to ReadyStateHaveMetadata yet then just
  // return 0 so that the seekable range is empty.
  if (ready_state_ < WebMediaPlayer::ReadyStateHaveMetadata) {
    return 0.0;
  }

  return duration();
}

blink::WebString WebMediaPlayer::getErrorMessage() {
  return blink::WebString::fromUTF8(media_log_->GetLastErrorMessage());
}

bool WebMediaPlayer::didLoadingProgress() {
  bool ret = did_loading_progress_;
  did_loading_progress_ = false;
  return ret;
}

void WebMediaPlayer::paint(blink::WebCanvas* canvas,
                           const blink::WebRect& rect,
                           SkPaint&) {
  NOTIMPLEMENTED();
}

bool WebMediaPlayer::hasSingleSecurityOrigin() const {
  if (info_loader_) {
    return info_loader_->HasSingleOrigin();
  }

  // The info loader may have failed.
  if (player_type_ == MEDIA_PLAYER_TYPE_URL) {
    return false;
  }

  return true;
}

bool WebMediaPlayer::didPassCORSAccessCheck() const {
  if (info_loader_) {
    return info_loader_->DidPassCORSAccessCheck();
  }

  return false;
}

double WebMediaPlayer::mediaTimeForTimeValue(double timeValue) const {
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayer::decodedFrameCount() const {
  NOTIMPLEMENTED();

  return 0;
}

unsigned WebMediaPlayer::droppedFrameCount() const {
  NOTIMPLEMENTED();

  return 0;
}

size_t WebMediaPlayer::audioDecodedByteCount() const {
  NOTIMPLEMENTED();

  return 0;
}

size_t WebMediaPlayer::videoDecodedByteCount() const {
  NOTIMPLEMENTED();

  return 0;
}

void WebMediaPlayer::OnHidden() {
  // RendererMediaPlayerManager will not call SuspendAndReleaseResources() if we
  // were already in the paused state; thus notify the MediaWebContentsObserver
  // that we've been hidden so any lingering MediaSessions are released.
  if (delegate_)
    delegate_->PlayerGone(delegate_id_);
}

void WebMediaPlayer::OnShown() {}

bool WebMediaPlayer::OnSuspendRequested(bool must_suspend) {
  if (must_suspend && delegate_) {
    delegate_->PlayerGone(delegate_id_);
  }
  return true;
}

void WebMediaPlayer::OnPlay() {
  play();
  client_->playbackStateChanged();
}

void WebMediaPlayer::OnPause() {
  pause();
  client_->playbackStateChanged();
}

void WebMediaPlayer::OnVolumeMultiplierUpdate(double multiplier) {}

void WebMediaPlayer::OnMediaMetadataChanged(
    const base::TimeDelta& duration, int width, int height, bool success) {
  bool need_to_signal_duration_changed = false;

  if (url_.SchemeIs("file")) {
    UpdateNetworkState(WebMediaPlayer::NetworkStateLoaded);
  }

  // Update duration, if necessary, prior to ready state updates that may
  // cause duration() query.
  if (!ignore_metadata_duration_change_ && duration_ != duration) {
    duration_ = duration;

    // Client readyState transition from HAVE_NOTHING to HAVE_METADATA
    // already triggers a durationchanged event. If this is a different
    // transition, remember to signal durationchanged.
    // Do not ever signal durationchanged on metadata change in MSE case
    // because OnDurationChanged() handles this.
    if (ready_state_ > WebMediaPlayer::ReadyStateHaveNothing &&
        player_type_ != MEDIA_PLAYER_TYPE_MEDIA_SOURCE) {
      need_to_signal_duration_changed = true;
    }
  }

  has_media_metadata_ = true;
  if (has_media_info_ &&
      ready_state_ != WebMediaPlayer::ReadyStateHaveEnoughData) {
    UpdateReadyState(WebMediaPlayer::ReadyStateHaveMetadata);
    UpdateReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
  }

  // TODO(wolenetz): Should we just abort early and set network state to an
  // error if success == false? See http://crbug.com/248399
  if (success) {
    OnVideoSizeChanged(width, height);
  }

  if (need_to_signal_duration_changed) {
    client_->durationChanged();
  }
}

void WebMediaPlayer::OnPlaybackComplete() {
  // When playback is about to finish, android media player often stops
  // at a time which is smaller than the duration. This makes webkit never
  // know that the playback has finished. To solve this, we set the
  // current time to media duration when OnPlaybackComplete() get called.
  OnTimeUpdate(duration_);
  client_->timeChanged();

  // if the loop attribute is set, timeChanged() will update the current time
  // to 0. It will perform a seek to 0. As the requests to the renderer
  // process are sequential, the OnSeekComplete() will only occur
  // once OnPlaybackComplete() is done. As the playback can only be executed
  // upon completion of OnSeekComplete(), the request needs to be saved.
  is_playing_ = false;
  if (seeking_ && seek_time_ == base::TimeDelta()) {
    pending_playback_ = true;
  }
}

void WebMediaPlayer::OnBufferingUpdate(int percentage) {
  buffered_[0].end = duration() * percentage / 100;
  did_loading_progress_ = true;
}

void WebMediaPlayer::OnSeekRequest(const base::TimeDelta& time_to_seek) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  client_->requestSeek(time_to_seek.InSecondsF());
}

void WebMediaPlayer::OnSeekComplete(
    const base::TimeDelta& current_time) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  seeking_ = false;
  if (pending_seek_) {
    pending_seek_ = false;
    seek(pending_seek_time_.InSecondsF());
    return;
  }

  OnTimeUpdate(current_time);

  UpdateReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);

  client_->timeChanged();

  if (pending_playback_) {
    play();
    pending_playback_ = false;
  }
}

void WebMediaPlayer::OnMediaError(int error_type) {
  client_->repaint();
}

void WebMediaPlayer::OnVideoSizeChanged(int width, int height) {
  has_size_info_ = true;
  if (natural_size_.width == width && natural_size_.height == height) {
    return;
  }

  natural_size_.width = width;
  natural_size_.height = height;

  // TODO(qinmin): This is a hack. We need the media element to stop showing the
  // poster image by forcing it to call setDisplayMode(video). Should move the
  // logic into HTMLMediaElement.cpp.
  client_->timeChanged();
}

void WebMediaPlayer::OnTimeUpdate(const base::TimeDelta& current_time) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  current_time_ = current_time.InSecondsF();
}

void WebMediaPlayer::OnDidEnterFullscreen() {
  NOTIMPLEMENTED();
}

void WebMediaPlayer::OnDidExitFullscreen() {
  NOTIMPLEMENTED();
}

void WebMediaPlayer::OnMediaPlayerPlay() {
  UpdateNetworkState(WebMediaPlayer::NetworkStateLoaded);
  UpdatePlayingState(true);
  client_->playbackStateChanged();
}

void WebMediaPlayer::OnMediaPlayerPause() {
  UpdatePlayingState(false);
  client_->playbackStateChanged();
}

void WebMediaPlayer::OnRequestFullscreen() {
  NOTIMPLEMENTED();
}

void WebMediaPlayer::OnDurationChanged(const base::TimeDelta& duration) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  // Only MSE |player_type_| registers this callback.
  DCHECK_EQ(player_type_, MEDIA_PLAYER_TYPE_MEDIA_SOURCE);

  // Cache the new duration value and trust it over any subsequent duration
  // values received in OnMediaMetadataChanged().
  duration_ = duration;
  ignore_metadata_duration_change_ = true;

  // Notify MediaPlayerClient that duration has changed, if > HAVE_NOTHING.
  if (ready_state_ > WebMediaPlayer::ReadyStateHaveNothing) {
    client_->durationChanged();
  }
}

void WebMediaPlayer::UpdateNetworkState(
    WebMediaPlayer::NetworkState state) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing &&
      (state == WebMediaPlayer::NetworkStateNetworkError ||
       state == WebMediaPlayer::NetworkStateDecodeError)) {
    // Any error that occurs before reaching ReadyStateHaveMetadata should
    // be considered a format error.
    network_state_ = WebMediaPlayer::NetworkStateFormatError;
  } else {
    network_state_ = state;
  }
  client_->networkStateChanged();
}

void WebMediaPlayer::UpdateReadyState(
    WebMediaPlayer::ReadyState state) {
  ready_state_ = state;
  client_->readyStateChanged();
}

void WebMediaPlayer::OnPlayerReleased() {
  if (is_playing_) {
    OnMediaPlayerPause();
  }
}

void WebMediaPlayer::ReleaseMediaResources() {
  switch (network_state_) {
    // Pause the media player and inform WebKit if the player is in a good
    // shape.
    case WebMediaPlayer::NetworkStateIdle:
    case WebMediaPlayer::NetworkStateLoading:
    case WebMediaPlayer::NetworkStateLoaded:
      Pause(false);
      client_->playbackStateChanged();
      break;
    // If a WebMediaPlayer instance has entered into one of these states,
    // the internal network state in HTMLMediaElement could be set to empty.
    // And calling playbackStateChanged() could get this object deleted.
    case WebMediaPlayer::NetworkStateEmpty:
    case WebMediaPlayer::NetworkStateFormatError:
    case WebMediaPlayer::NetworkStateNetworkError:
    case WebMediaPlayer::NetworkStateDecodeError:
      break;
  }
  player_manager_->ReleaseResources(player_id_);
  OnPlayerReleased();
}

void WebMediaPlayer::OnDestruct() {
  if (player_manager_) {
    player_manager_->UnregisterMediaPlayer(player_id_);
  }
  Detach();
}

void WebMediaPlayer::Detach() {
  is_remote_ = false;
  player_manager_ = nullptr;
}

void WebMediaPlayer::Pause(bool is_media_related_action) {
  if (player_manager_) {
    player_manager_->Pause(player_id_, is_media_related_action);
  }
  UpdatePlayingState(false);
}

void WebMediaPlayer::UpdatePlayingState(bool is_playing) {
  is_playing_ = is_playing;
  if (!delegate_) {
    return;
  }
  if (is_playing) {
    delegate_->DidPlay(delegate_id_,
                       hasVideo(),
                       !hasVideo(),
                       isRemote(),
                       media::DurationToMediaContentType(duration_));
  } else {
    delegate_->DidPause(delegate_id_,
                        !pending_seek_ || currentTime() >= duration());
  }
}

void WebMediaPlayer::enterFullscreen() {
  NOTIMPLEMENTED();
}

void WebMediaPlayer::exitFullscreen() {
  NOTIMPLEMENTED();
}

bool WebMediaPlayer::canEnterFullscreen() const {
  return false;
}

}  // namespace oxide
