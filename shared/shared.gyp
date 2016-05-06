# Copyright (C) 2013-2016 Canonical Ltd.

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

{
  'variables': {
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/oxide',
  },
  'conditions': [
    ['enable_mediahub==1', {
      'targets': [
        {
          'target_name': 'mediahub_lib',
          'type': 'static_library',
          'hard_dependency': 1,
          'cflags_cc+': [ '-std=c++11', '-fexceptions' ],
          'cflags_cc!': [ '-fno-exceptions' ],
          'link_settings': {
            'libraries': [
              '-lmedia-hub-client',
            ],
          },
          'include_dirs': [
            '..',
          ],
          'sources': [
            'browser/media/mediahub_player_shim.cc',
            'browser/media/mediahub_player_shim.h',
          ],
        },
      ],
    }],
  ],
  'targets': [
    {
      'target_name': 'oxide_resources',
      'type': 'none',
      'actions': [
        {
          'action_name': 'oxide_resources',
          'variables': {
            'grit_grd_file': 'oxide_resources.grd'
          },
          'includes': [ '../../build/grit_action.gypi' ]
        }
      ],
      'includes': [ '../../build/grit_target.gypi' ],
    },
    {
      'target_name': 'packed_resources',
      'type': 'none',
      'variables': {
        'repack_path': '<(DEPTH)/tools/grit/grit/format/repack.py'
      },
      'dependencies': [
        'oxide_resources',
        '<(DEPTH)/content/app/resources/content_resources.gyp:content_resources',
        '<(DEPTH)/content/app/strings/content_strings.gyp:content_strings',
        '<(DEPTH)/content/browser/tracing/tracing_resources.gyp:tracing_resources',
        '<(DEPTH)/content/content.gyp:content_resources',
        '<(DEPTH)/content/browser/devtools/devtools_resources.gyp:devtools_resources',
        '<(DEPTH)/net/net.gyp:net_resources',
        '<(DEPTH)/third_party/WebKit/public/blink_resources.gyp:blink_image_resources',
        '<(DEPTH)/third_party/WebKit/public/blink_resources.gyp:blink_resources',
        '<(DEPTH)/ui/resources/ui_resources.gyp:ui_resources',
        '<(DEPTH)/ui/strings/ui_strings.gyp:ui_strings',
      ],
      'actions': [
        {
          'action_name': 'repack_oxide',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/blink/devtools_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/blink/public/resources/blink_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/browser/tracing/tracing_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/content_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/oxide/oxide_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/webui_resources.pak',
            ]
          },
          'inputs': [
            '<(repack_path)',
            '<@(pak_inputs)'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/repack/oxide.pak'
          ],
          'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)']
        },
        {
          'action_name': 'repack_oxide_100_percent',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/blink/public/resources/blink_image_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/app/resources/content_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/ui_resources_100_percent.pak',
            ]
          },
          'inputs': [
            '<(repack_path)',
            '<@(pak_inputs)'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/repack/oxide_100_percent.pak'
          ],
          'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)']
        },
        {
          'action_name': 'repack_oxide_200_percent',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/blink/public/resources/blink_image_resources_200_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/app/resources/content_resources_200_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/ui_resources_200_percent.pak',
            ]
          },
          'inputs': [
            '<(repack_path)',
            '<@(pak_inputs)'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/repack/oxide_200_percent.pak'
          ],
          'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)']
        },
        {
          'action_name': 'repack_locales',
          'variables': {
            'pak_locales': '<(locales)',
            'repack_locales_path': '../build/scripts/oxide_repack_locales.py',
          },
          'inputs': [
            '<(repack_locales_path)',
            '<!@pymod_do_main(oxide_repack_locales -i -p <(OS) -b chromium -g <(grit_out_dir) -s <(SHARED_INTERMEDIATE_DIR) -x <(SHARED_INTERMEDIATE_DIR) <(pak_locales))'
          ],
          'outputs': [
            '<!@pymod_do_main(oxide_repack_locales -o -p <(OS) -g <(grit_out_dir) -s <(SHARED_INTERMEDIATE_DIR) -x <(SHARED_INTERMEDIATE_DIR) <(pak_locales))'
          ],
          'action': [
            'python',
            '<(repack_locales_path)',
            '-b', 'chromium',
            '-p', '<(OS)',
            '-g', '<(grit_out_dir)',
            '-s', '<(SHARED_INTERMEDIATE_DIR)',
            '-x', '<(SHARED_INTERMEDIATE_DIR)/.',
            '<@(pak_locales)',
          ],
        },
      ],
      'copies': [
        {
          'destination': '<(PRODUCT_DIR)',
          'files': [
            '<(SHARED_INTERMEDIATE_DIR)/repack/oxide.pak'
          ],
        },
        {
          'destination': '<(PRODUCT_DIR)',
          'files': [
            '<(SHARED_INTERMEDIATE_DIR)/repack/oxide_100_percent.pak'
          ],
        },
        {
          'destination': '<(PRODUCT_DIR)',
          'files': [
            '<(SHARED_INTERMEDIATE_DIR)/repack/oxide_200_percent.pak'
          ],
        },
        {
          'destination': '<(PRODUCT_DIR)/locales',
          'files': [
            '<!@pymod_do_main(repack_locales -o -p <(OS) -g <(grit_out_dir) -s <(SHARED_INTERMEDIATE_DIR) -x <(SHARED_INTERMEDIATE_DIR) <(locales))'
          ],
        },
      ],
    },
    {
      'target_name': 'oxide_shared',
      'type': '<(component)',
      'hard_dependency': 1,
      'variables': {
        'chromium_code': 1,
        'oxide_subprocess_path%': '<(oxide_libexecdir)/<(oxide_renderer)',
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/oxide',
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/mojo/mojo_public.gyp:mojo_cpp_bindings',
        '<(DEPTH)/third_party/re2/re2.gyp:re2',
        '<(DEPTH)/ui/accessibility/accessibility.gyp:accessibility',
      ],
      'defines': [
        'OXIDE_SUBPROCESS_PATH=\"<(oxide_subprocess_path)\"',
        'OXIDE_GETTEXT_DOMAIN=\"<(oxide_gettext_domain)\"',
        'OXIDE_SHARED_IMPLEMENTATION',
      ],
      'dependencies': [
        'packed_resources',
        'oxide_resources',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/base.gyp:base_static',
        # Used via Singleton. Should base have this in export_dependent_settings?
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/build/linux/system.gyp:dbus',
        '<(DEPTH)/build/linux/system.gyp:fontconfig',
        '<(DEPTH)/build/linux/system.gyp:glib',
        '<(DEPTH)/build/linux/system.gyp:libpci',
        '<(DEPTH)/build/linux/system.gyp:xext',
        '<(DEPTH)/build/linux/system.gyp:x11',
        '<(DEPTH)/cc/cc.gyp:cc',
        '<(DEPTH)/cc/cc.gyp:cc_surfaces',
        '<(DEPTH)/components/components.gyp:devtools_http_handler',
        '<(DEPTH)/components/components.gyp:keyed_service_content',
        '<(DEPTH)/components/components.gyp:keyed_service_core',
        '<(DEPTH)/components/components.gyp:sessions_content',
        '<(DEPTH)/content/content.gyp:content_app_both',
        '<(DEPTH)/content/content.gyp:content_browser',
        '<(DEPTH)/content/content.gyp:content_child',
        '<(DEPTH)/content/content.gyp:content_common',
        '<(DEPTH)/content/content.gyp:content_gpu',
        '<(DEPTH)/content/content.gyp:content_ppapi_plugin',
        '<(DEPTH)/content/content.gyp:content_renderer',
        '<(DEPTH)/content/content.gyp:content_utility',
        '<(DEPTH)/crypto/crypto.gyp:crypto',
        '<(DEPTH)/dbus/dbus.gyp:dbus',
        '<(DEPTH)/device/vibration/vibration.gyp:device_vibration_mojo_bindings',
        '<(DEPTH)/gin/gin.gyp:gin',
        '<(DEPTH)/gpu/command_buffer/command_buffer.gyp:gles2_utils',
        '<(DEPTH)/gpu/gpu.gyp:command_buffer_common',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/mojo/mojo_base.gyp:mojo_common_lib',
        '<(DEPTH)/mojo/mojo_edk.gyp:mojo_system_impl',
        '<(DEPTH)/mojo/mojo_public.gyp:mojo_cpp_bindings',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/khronos/khronos.gyp:khronos_headers',
        '<(DEPTH)/third_party/libXNVCtrl/libXNVCtrl.gyp:libXNVCtrl',
        '<(DEPTH)/third_party/re2/re2.gyp:re2',
        '<(DEPTH)/third_party/WebKit/public/blink.gyp:blink',
        # Not used directly. Should content_browser have this in export_dependent_settings?
        '<(DEPTH)/ui/accessibility/accessibility.gyp:accessibility',
        '<(DEPTH)/ui/base/ui_base.gyp:ui_base',
        '<(DEPTH)/ui/events/events.gyp:events',
        '<(DEPTH)/ui/events/events.gyp:events_base',
        '<(DEPTH)/ui/events/events.gyp:gesture_detection',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
        '<(DEPTH)/ui/gfx/x/gfx_x11.gyp:gfx_x11',
        '<(DEPTH)/ui/gl/gl.gyp:gl',
        '<(DEPTH)/ui/native_theme/native_theme.gyp:native_theme',
        '<(DEPTH)/ui/ozone/ozone.gyp:ozone',
        '<(DEPTH)/ui/touch_selection/ui_touch_selection.gyp:ui_touch_selection',
        '<(DEPTH)/url/url.gyp:url_lib',
        '<(DEPTH)/url/ipc/url_ipc.gyp:url_ipc',
        '<(DEPTH)/v8/src/v8.gyp:v8',
        '../build/system.gyp:gdkpixbuf',
        '../build/system.gyp:libnotify',
      ],
      'include_dirs': [
        '..',
        '<(DEPTH)',
        '<(SHARED_INTERMEDIATE_DIR)/oxide',
      ],
      'sources': [
        'app/oxide_content_main_delegate.cc',
        'app/oxide_content_main_delegate.h',
        'app/oxide_main.cc',
        'app/oxide_main.h',
        'app/oxide_platform_delegate.h',
        'browser/compositor/oxide_compositing_mode.h',
        'browser/compositor/oxide_compositor.cc',
        'browser/compositor/oxide_compositor.h',
        'browser/compositor/oxide_compositor_client.h',
        'browser/compositor/oxide_compositor_frame_ack.cc',
        'browser/compositor/oxide_compositor_frame_ack.h',
        'browser/compositor/oxide_compositor_frame_collector.cc',
        'browser/compositor/oxide_compositor_frame_collector.h',
        'browser/compositor/oxide_compositor_frame_data.cc',
        'browser/compositor/oxide_compositor_frame_data.h',
        'browser/compositor/oxide_compositor_frame_handle.cc',
        'browser/compositor/oxide_compositor_frame_handle.h',
        'browser/compositor/oxide_compositor_gpu_shims.cc',
        'browser/compositor/oxide_compositor_gpu_shims.h',
        'browser/compositor/oxide_compositor_observer.cc',
        'browser/compositor/oxide_compositor_observer.h',
        'browser/compositor/oxide_compositor_output_surface.cc',
        'browser/compositor/oxide_compositor_output_surface.h',
        'browser/compositor/oxide_compositor_output_surface_gl.cc',
        'browser/compositor/oxide_compositor_output_surface_gl.h',
        'browser/compositor/oxide_compositor_output_surface_listener.h',
        'browser/compositor/oxide_compositor_output_surface_software.cc',
        'browser/compositor/oxide_compositor_output_surface_software.h',
        'browser/compositor/oxide_compositor_software_output_device.cc',
        'browser/compositor/oxide_compositor_software_output_device.h',
        'browser/compositor/oxide_compositor_utils.cc',
        'browser/compositor/oxide_compositor_utils.h',
        'browser/compositor/oxide_mailbox_buffer_map.cc',
        'browser/compositor/oxide_mailbox_buffer_map.h',
        'browser/input/oxide_ime_bridge.cc',
        'browser/input/oxide_ime_bridge.h',
        'browser/input/oxide_ime_bridge_impl.cc',
        'browser/input/oxide_ime_bridge_impl.h',
        'browser/input/oxide_input_method_context.cc',
        'browser/input/oxide_input_method_context.h',
        'browser/input/oxide_input_method_context_observer.cc',
        'browser/input/oxide_input_method_context_observer.h',
        'browser/media/oxide_media_capture_devices_context.cc',
        'browser/media/oxide_media_capture_devices_context.h',
        'browser/media/oxide_media_capture_devices_context_client.h',
        'browser/media/oxide_media_capture_devices_dispatcher.cc',
        'browser/media/oxide_media_capture_devices_dispatcher.h',
        'browser/media/oxide_media_capture_devices_dispatcher_observer.cc',
        'browser/media/oxide_media_capture_devices_dispatcher_observer.h',
        'browser/media/oxide_video_capture_device_factory_linux.cc',
        'browser/media/oxide_video_capture_device_factory_linux.h',
        'browser/notifications/oxide_notification_data.h',
        'browser/notifications/oxide_notification_delegate_proxy.cc',
        'browser/notifications/oxide_notification_delegate_proxy.h',
        'browser/notifications/oxide_notification_dispatcher.h',
        'browser/notifications/oxide_notification_id.h',
        'browser/notifications/oxide_platform_notification_service.cc',
        'browser/notifications/oxide_platform_notification_service.h',
        'browser/notifications/oxide_system_notification_dispatcher.h',
        'browser/notifications/oxide_system_notification_dispatcher_linux.cc',
        'browser/oxide_access_token_store.cc',
        'browser/oxide_access_token_store.h',
        'browser/oxide_browser_context.cc',
        'browser/oxide_browser_context.h',
        'browser/oxide_browser_context_delegate.h',
        'browser/oxide_browser_context_destroyer.cc',
        'browser/oxide_browser_context_destroyer.h',
        'browser/oxide_browser_context_observer.cc',
        'browser/oxide_browser_context_observer.h',
        'browser/oxide_browser_main_parts.cc',
        'browser/oxide_browser_main_parts.h',
        'browser/oxide_browser_platform_integration.cc',
        'browser/oxide_browser_platform_integration.h',
        'browser/oxide_browser_platform_integration_observer.cc',
        'browser/oxide_browser_platform_integration_observer.h',
        'browser/oxide_browser_process_main.cc',
        'browser/oxide_browser_process_main.h',
        'browser/oxide_content_browser_client.cc',
        'browser/oxide_content_browser_client.h',
        'browser/oxide_content_types.h',
        'browser/oxide_cookie_store_ui_proxy.cc',
        'browser/oxide_cookie_store_ui_proxy.h',
        'browser/oxide_devtools_http_handler_delegate.cc',
        'browser/oxide_devtools_http_handler_delegate.h',
        'browser/oxide_devtools_manager.cc',
        'browser/oxide_devtools_manager.h',
        'browser/oxide_download_manager_delegate.cc',
        'browser/oxide_download_manager_delegate.h',
        'browser/oxide_drag_source.h',
        'browser/oxide_drag_source_client.h',
        'browser/oxide_event_utils.cc',
        'browser/oxide_event_utils.h',
        'browser/oxide_favicon_helper.cc',
        'browser/oxide_favicon_helper.h',
        'browser/oxide_file_picker.cc',
        'browser/oxide_file_picker.h',
        'browser/oxide_find_controller.cc',
        'browser/oxide_find_controller.h',
        'browser/oxide_find_controller_client.h',
        'browser/oxide_form_factor_detection.cc',
        'browser/oxide_form_factor_detection.h',
        'browser/oxide_form_factor_detection_linux.cc',
        'browser/oxide_fullscreen_helper.cc',
        'browser/oxide_fullscreen_helper.h',
        'browser/oxide_fullscreen_helper_client.h',
        'browser/oxide_gesture_provider.cc',
        'browser/oxide_gesture_provider.h',
        'browser/oxide_gpu_info_collector_linux.cc',
        'browser/oxide_gpu_info_collector_linux.h',
        'browser/oxide_http_user_agent_settings.cc',
        'browser/oxide_http_user_agent_settings.h',
        'browser/oxide_io_thread.cc',
        'browser/oxide_io_thread.h',
        'browser/oxide_javascript_dialog.cc',
        'browser/oxide_javascript_dialog.h',
        'browser/oxide_javascript_dialog_manager.cc',
        'browser/oxide_javascript_dialog_manager.h',
        'browser/oxide_lifecycle_observer.cc',
        'browser/oxide_lifecycle_observer.h',
        'browser/oxide_message_pump.cc',
        'browser/oxide_message_pump.h',
        'browser/oxide_navigation_intercept_resource_throttle.cc',
        'browser/oxide_navigation_intercept_resource_throttle.h',
        'browser/oxide_network_delegate.cc',
        'browser/oxide_network_delegate.h',
        'browser/oxide_platform_run_loop.cc',
        'browser/oxide_platform_run_loop.h',
        'browser/oxide_power_save_blocker.cc',
        'browser/oxide_power_save_blocker.h',
        'browser/oxide_quota_permission_context.cc',
        'browser/oxide_quota_permission_context.h',
        'browser/oxide_redirection_intercept_throttle.cc',
        'browser/oxide_redirection_intercept_throttle.h',
        'browser/oxide_render_object_id.h',
        'browser/oxide_render_message_filter.cc',
        'browser/oxide_render_message_filter.h',
        'browser/oxide_render_process_initializer.cc',
        'browser/oxide_render_process_initializer.h',
        'browser/oxide_render_widget_host_view.cc',
        'browser/oxide_render_widget_host_view.h',
        'browser/oxide_render_widget_host_view_container.h',
        'browser/oxide_resource_dispatcher_host_delegate.cc',
        'browser/oxide_resource_dispatcher_host_delegate.h',
        'browser/oxide_resource_dispatcher_host_login_delegate.cc',
        'browser/oxide_resource_dispatcher_host_login_delegate.h',
        'browser/oxide_script_message_contents_helper.cc',
        'browser/oxide_script_message_contents_helper.h',
        'browser/oxide_script_message_impl_browser.cc',
        'browser/oxide_script_message_impl_browser.h',
        'browser/oxide_script_message_request_impl_browser.cc',
        'browser/oxide_script_message_request_impl_browser.h',
        'browser/oxide_script_message_target.h',
        'browser/oxide_mouse_event_state.cc',
        'browser/oxide_mouse_event_state.h',
        'browser/oxide_url_request_context.cc',
        'browser/oxide_url_request_context.h',
        'browser/oxide_url_request_delegated_job.cc',
        'browser/oxide_url_request_delegated_job.h',
        'browser/oxide_url_request_delegated_job_factory.cc',
        'browser/oxide_url_request_delegated_job_factory.h',
        'browser/oxide_user_agent_settings.cc',
        'browser/oxide_user_agent_settings.h',
        'browser/oxide_user_script_master.cc',
        'browser/oxide_user_script_master.h',
        'browser/oxide_web_contents_unloader.cc',
        'browser/oxide_web_contents_unloader.h',
        'browser/oxide_web_contents_view.cc',
        'browser/oxide_web_contents_view.h',
        'browser/oxide_web_contents_view_client.cc',
        'browser/oxide_web_contents_view_client.h',
        'browser/oxide_web_context_menu.cc',
        'browser/oxide_web_context_menu.h',
        'browser/oxide_web_frame.cc',
        'browser/oxide_web_frame.h',
        'browser/oxide_web_frame_tree.cc',
        'browser/oxide_web_frame_tree.h',
        'browser/oxide_web_frame_tree_observer.cc',
        'browser/oxide_web_frame_tree_observer.h',
        'browser/oxide_web_popup_menu.cc',
        'browser/oxide_web_popup_menu.h',
        'browser/oxide_web_preferences.cc',
        'browser/oxide_web_preferences.h',
        'browser/oxide_web_preferences_observer.cc',
        'browser/oxide_web_preferences_observer.h',
        'browser/oxide_web_view.cc',
        'browser/oxide_web_view.h',
        'browser/oxide_web_view_client.cc',
        'browser/oxide_web_view_client.h',
        'browser/oxide_web_view_contents_helper.cc',
        'browser/oxide_web_view_contents_helper.h',
        'browser/permissions/oxide_permission_manager.cc',
        'browser/permissions/oxide_permission_manager.h',
        'browser/permissions/oxide_permission_request.cc',
        'browser/permissions/oxide_permission_request.h',
        'browser/permissions/oxide_permission_request_dispatcher.cc',
        'browser/permissions/oxide_permission_request_dispatcher.h',
        'browser/permissions/oxide_permission_request_dispatcher_client.h',
        'browser/permissions/oxide_permission_request_response.h',
        'browser/permissions/oxide_temporary_saved_permission_context.cc',
        'browser/permissions/oxide_temporary_saved_permission_context.h',
        'browser/ssl/oxide_certificate_error.cc',
        'browser/ssl/oxide_certificate_error.h',
        'browser/ssl/oxide_certificate_error_dispatcher.cc',
        'browser/ssl/oxide_certificate_error_dispatcher.h',
        'browser/ssl/oxide_certificate_error_placeholder_page.cc',
        'browser/ssl/oxide_certificate_error_placeholder_page.h',
        'browser/ssl/oxide_certificate_error_proxy.cc',
        'browser/ssl/oxide_certificate_error_proxy.h',
        'browser/ssl/oxide_security_status.cc',
        'browser/ssl/oxide_security_status.h',
        'browser/ssl/oxide_security_types.h',
        'browser/ssl/oxide_ssl_config_service.cc',
        'browser/ssl/oxide_ssl_config_service.h',
        'browser/ssl/oxide_ssl_host_state_delegate.cc',
        'browser/ssl/oxide_ssl_host_state_delegate.h',
        'common/oxide_constants.cc',
        'common/oxide_constants.h',
        'common/oxide_content_client.cc',
        'common/oxide_content_client.h',
        'common/oxide_core_export.h',
        'common/oxide_cross_thread_data_stream.cc',
        'common/oxide_cross_thread_data_stream.h',
        'common/oxide_enum_flags.h',
        'common/oxide_export.h',
        'common/oxide_file_utils.cc',
        'common/oxide_file_utils.h',
        'common/oxide_form_factor.cc',
        'common/oxide_form_factor.h',
        'common/oxide_id_allocator.cc',
        'common/oxide_id_allocator.h',
        'common/oxide_message_enums.h',
        'common/oxide_message_generator.cc',
        'common/oxide_message_generator.h',
        'common/oxide_messages.h',
        'common/oxide_net_resource_provider.cc',
        'common/oxide_net_resource_provider.h',
        'common/oxide_param_traits.cc',
        'common/oxide_param_traits.h',
        'common/oxide_paths.cc',
        'common/oxide_paths.h',
        'common/oxide_script_message.cc',
        'common/oxide_script_message.h',
        'common/oxide_script_message_handler.cc',
        'common/oxide_script_message_handler.h',
        'common/oxide_script_message_params.cc',
        'common/oxide_script_message_params.h',
        'common/oxide_script_message_request.cc',
        'common/oxide_script_message_request.h',
        'common/oxide_shared_export.h',
        'common/oxide_unowned_user_data.h',
        'common/oxide_user_agent.cc',
        'common/oxide_user_agent.h',
        'common/oxide_user_agent_override_set.cc',
        'common/oxide_user_agent_override_set.h',
        'common/oxide_user_script.cc',
        'common/oxide_user_script.h',
        'gpu/oxide_gl_context_dependent.cc',
        'gpu/oxide_gl_context_dependend.h',
        'renderer/oxide_content_renderer_client.cc',
        'renderer/oxide_content_renderer_client.h',
        'renderer/oxide_isolated_world_map.cc',
        'renderer/oxide_isolated_world_map.h',
        'renderer/oxide_object_backed_native_handler.cc',
        'renderer/oxide_object_backed_native_handler.h',
        'renderer/oxide_renderer_user_agent_settings.cc',
        'renderer/oxide_renderer_user_agent_settings.h',
        'renderer/oxide_script_message_dispatcher_renderer.cc',
        'renderer/oxide_script_message_dispatcher_renderer.h',
        'renderer/oxide_script_message_handler_renderer.cc',
        'renderer/oxide_script_message_handler_renderer.h',
        'renderer/oxide_script_message_impl_renderer.cc',
        'renderer/oxide_script_message_impl_renderer.h',
        'renderer/oxide_script_message_manager.cc',
        'renderer/oxide_script_message_manager.h',
        'renderer/oxide_script_message_object_handler.cc',
        'renderer/oxide_script_message_object_handler.h',
        'renderer/oxide_script_message_request_impl_renderer.cc',
        'renderer/oxide_script_message_request_impl_renderer.h',
        'renderer/oxide_script_message_request_object_handler.cc',
        'renderer/oxide_script_message_request_object_handler.h',
        'renderer/oxide_script_referenced_object.cc',
        'renderer/oxide_script_referenced_object.h',
        'renderer/oxide_top_controls_handler.cc',
        'renderer/oxide_top_controls_handler.h',
        'renderer/oxide_user_script_scheduler.cc',
        'renderer/oxide_user_script_scheduler.h',
        'renderer/oxide_user_script_slave.cc',
        'renderer/oxide_user_script_slave.h',
        'renderer/oxide_v8_scoped_persistent.h',
        'renderer/oxide_web_content_settings_client.cc',
        'renderer/oxide_web_content_settings_client.h',
        '<(DEPTH)/extensions/common/constants.cc',
        '<(DEPTH)/extensions/common/constants.h',
        '<(DEPTH)/extensions/common/error_utils.cc',
        '<(DEPTH)/extensions/common/error_utils.h',
        '<(DEPTH)/extensions/common/url_pattern.cc',
        '<(DEPTH)/extensions/common/url_pattern.h',
        '<(DEPTH)/extensions/common/url_pattern_set.cc',
        '<(DEPTH)/extensions/common/url_pattern_set.h'
      ],
      'conditions': [
        ['enable_plugins==1', {
          'sources': [
            'browser/pepper/oxide_pepper_host_factory_browser.cc',
            'browser/pepper/oxide_pepper_host_factory_browser.h',
            'browser/pepper/oxide_pepper_flash_browser_host.cc',
            'browser/pepper/oxide_pepper_flash_browser_host.h',
            'renderer/pepper/oxide_pepper_renderer_host_factory.cc',
            'renderer/pepper/oxide_pepper_renderer_host_factory.h',
            'renderer/pepper/oxide_pepper_render_frame_observer.cc',
            'renderer/pepper/oxide_pepper_render_frame_observer.h',
            'renderer/pepper/oxide_pepper_flash_font_file_host.cc',
            'renderer/pepper/oxide_pepper_flash_font_file_host.h',
            'renderer/pepper/oxide_pepper_flash_fullscreen_host.cc',
            'renderer/pepper/oxide_pepper_flash_fullscreen_host.h',
            'renderer/pepper/oxide_pepper_flash_renderer_host.cc',
            'renderer/pepper/oxide_pepper_flash_renderer_host.h',
            'renderer/pepper/oxide_pepper_flash_menu_host.cc',
            'renderer/pepper/oxide_pepper_flash_menu_host.h',
            'renderer/pepper/oxide_pepper_shared_memory_message_filter.cc',
            'renderer/pepper/oxide_pepper_shared_memory_message_filter.h',
          ],
          'dependencies': [
            '<(DEPTH)/ppapi/ppapi_internal.gyp:ppapi_host',
            '<(DEPTH)/ppapi/ppapi_internal.gyp:ppapi_proxy',
            '<(DEPTH)/ppapi/ppapi_internal.gyp:ppapi_shared',
          ],
        }],
        ['enable_mediahub==1', {
          'defines': [
            'ENABLE_MEDIAHUB=1'
          ],
          'sources': [
            'browser/media/oxide_browser_media_player_manager.cc',
            'browser/media/oxide_browser_media_player_manager.h',
            'browser/media/oxide_player_media_hub.cc',
            'browser/media/oxide_player_media_hub.h',
            'browser/media/oxide_media_player.cc',
            'browser/media/oxide_media_player.h',
            'browser/media/oxide_media_web_contents_observer.cc',
            'browser/media/oxide_media_web_contents_observer.h',
            'renderer/media/oxide_renderer_media_player_manager.cc',
            'renderer/media/oxide_renderer_media_player_manager.h',
            'renderer/media/oxide_web_media_player.cc',
            'renderer/media/oxide_web_media_player.h',
            'renderer/media/oxide_media_info_loader.cc',
            'renderer/media/oxide_media_info_loader.h',
          ],
          'dependencies': [
            'mediahub_lib',
            '<(DEPTH)/media/media.gyp:media',
            '<(DEPTH)/media/blink/media_blink.gyp:media_blink',
          ],
        }],
        ['enable_hybris==1', {
          'defines': [
            'ENABLE_HYBRIS=1',
          ],
          'dependencies': [
            '../build/system.gyp:android-properties',
          ],
          'sources': [
            'browser/oxide_hybris_utils.cc',
            'browser/oxide_hybris_utils.h',
          ],
        }],
        ['enable_hybris_camera==1 and enable_hybris==1', {
          'defines': [
            'ENABLE_HYBRIS_CAMERA=1',
          ],
          'dependencies': [
            '../build/system.gyp:hybris-camera',
          ],
          'sources': [
            'browser/media/oxide_video_capture_device_hybris.cc',
            'browser/media/oxide_video_capture_device_hybris.h',
          ],
        }],
      ],
      'actions': [
        {
          'action_name': 'chrome_version_header',
          'inputs': [
            'common/chrome_version.h.in',
            '<(DEPTH)/chrome/VERSION'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/oxide/shared/common/chrome_version.h'
          ],
          'action': [
            'python',
            '<(DEPTH)/build/util/version.py',
            '-f', '<(DEPTH)/chrome/VERSION',
            '-i', '<@(_inputs)',
            '-o', '<@(_outputs)'
          ]
        },
      ],
    },
    {
      'target_name': 'oxide_shared_testutils',
      'type': 'static_library',
      'variables': {
        'chromium_code': 1,
      },
      'dependencies': [
        'oxide_shared',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/content/content.gyp:content_browser',
        '<(DEPTH)/content/content_shell_and_tests.gyp:test_support_content',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
      ],
      'include_dirs': [
        '..',
        '<(DEPTH)',
      ],
      'sources': [
        'test/oxide_mock_cert_store.cc',
        'test/oxide_mock_cert_store.h',
        'test/oxide_test_browser_thread_bundle.cc',
        'test/oxide_test_browser_thread_bundle.h',
        'test/oxide_test_suite.cc',
        'test/oxide_test_suite.h',
        'test/oxide_test_web_contents_view.cc',
        'test/oxide_test_web_contents_view.h',
      ],
    },
    {
      'target_name': 'oxide_shared_unittests',
      'type': 'executable',
      'variables': {
        'chromium_code': 1,
      },
      'dependencies': [
        'oxide_shared',
        'oxide_shared_testutils',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/content/content.gyp:content_browser',
        '<(DEPTH)/content/content.gyp:content_common',
        '<(DEPTH)/content/content_shell_and_tests.gyp:test_support_content',
        '<(DEPTH)/mojo/mojo_edk.gyp:mojo_system_impl',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/url/url.gyp:url_lib',
      ],
      'include_dirs': [
        '..',
        '<(DEPTH)',
      ],
      'sources': [
        'browser/ssl/oxide_certificate_error_unittest.cc',
        'browser/ssl/oxide_certificate_error_dispatcher_unittest.cc',
        'browser/ssl/oxide_security_status_unittest.cc',
        'browser/ssl/oxide_ssl_host_state_delegate_unittest.cc',
        'test/run_all_unittests.cc',
      ],
    },
  ]
}
