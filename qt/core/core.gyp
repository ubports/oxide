# Copyright (C) 2013 Canonical Ltd.

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
  'targets': [
    {
      'target_name': 'OxideQtCore',
      'type': 'shared_library',
      'shared_library_version': '<(oxide_qt_libversion)',
      'dependencies': [
        'system.gyp:Qt5Core',
        'system.gyp:Qt5Gui',
        '../../shared/shared.gyp:oxide_shared',
        'OxideQtCore_public',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/content/content.gyp:content_browser',
        '<(DEPTH)/content/content.gyp:content_renderer',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/ui/ui.gyp:ui',
        '<(DEPTH)/url/url.gyp:url_lib'
      ],
      'include_dirs': [
        '../..',
        '<(DEPTH)'
      ],
      'sources': [
        '<(INTERMEDIATE_DIR)/oxide_qt_web_popup_menu_qquick.moc',
        'api/private/oxideqincomingmessage_p.cc',
        'api/private/oxideqincomingmessage_p.h',
        'api/private/oxideqloadevent_p.cc',
        'api/private/oxideqloadevent_p.h',
        'browser/oxide_qt_backing_store.cc',
        'browser/oxide_qt_backing_store.h',
        'browser/oxide_qt_content_browser_client.cc',
        'browser/oxide_qt_content_browser_client.h',
        'browser/oxide_qt_message_pump.cc',
        'browser/oxide_qt_message_pump.h',
        'browser/oxide_qt_render_widget_host_view.cc',
        'browser/oxide_qt_render_widget_host_view.h',
        'browser/oxide_qt_web_frame.cc',
        'browser/oxide_qt_web_frame.h',
        'browser/oxide_qt_web_popup_menu.cc',
        'browser/oxide_qt_web_popup_menu.h',
        'common/oxide_qt_content_main_delegate.cc',
        'common/oxide_qt_content_main_delegate.h',
        'glue/private/oxide_qt_message_handler_adapter_p.cc',
        'glue/private/oxide_qt_message_handler_adapter_p.h',
        'glue/private/oxide_qt_outgoing_message_request_adapter_p.cc',
        'glue/private/oxide_qt_outgoing_message_request_adapter_p.h',
        'glue/private/oxide_qt_user_script_adapter_p.cc',
        'glue/private/oxide_qt_user_script_adapter_p.h',
        'glue/private/oxide_qt_web_context_adapter_p.cc',
        'glue/private/oxide_qt_web_context_adapter_p.h',
        'glue/private/oxide_qt_web_frame_adapter_p.cc',
        'glue/private/oxide_qt_web_frame_adapter_p.h',
        'glue/private/oxide_qt_web_view_adapter_p.cc',
        'glue/private/oxide_qt_web_view_adapter_p.h'
      ],
      'variables': {
        'chromium_code': 1
      }
    },
    {
      'target_name': 'OxideQtCore_public',
      'type': 'static_library',
      'cflags_cc!': [ '-fno-rtti' ],
      'dependencies': [
        'system.gyp:Qt5Core',
        'system.gyp:Qt5Gui',
        '../../shared/shared.gyp:oxide_shared',
        '<(DEPTH)/skia/skia.gyp:skia',
      ],
      'export_dependent_settings': [
        '<(DEPTH)/skia/skia.gyp:skia'
      ],
      'include_dirs': [
        '../..',
        '<(INTERMEDIATE_DIR)',
        '<(DEPTH)'
      ],
      'sources': [
        '<(INTERMEDIATE_DIR)/moc_oxideqincomingmessage.cc',
        '<(INTERMEDIATE_DIR)/moc_oxideqloadevent.cc',
        'api/oxideqincomingmessage.cc',
        'api/oxideqincomingmessage.h',
        'api/oxideqloadevent.cc',
        'api/oxideqloadevent.h',
        'glue/oxide_qt_adapter_base.h',
        'glue/oxide_qt_message_handler_adapter.cc',
        'glue/oxide_qt_message_handler_adapter.h',
        'glue/oxide_qt_outgoing_message_request_adapter.cc',
        'glue/oxide_qt_outgoing_message_request_adapter.h',
        'glue/oxide_qt_render_widget_host_view_delegate.cc',
        'glue/oxide_qt_render_widget_host_view_delegate.h',
        'glue/oxide_qt_user_script_adapter.cc',
        'glue/oxide_qt_user_script_adapter.h',
        'glue/oxide_qt_web_context_adapter.cc',
        'glue/oxide_qt_web_context_adapter.h',
        'glue/oxide_qt_web_frame_adapter.cc',
        'glue/oxide_qt_web_frame_adapter.h',
        'glue/oxide_qt_web_popup_menu_delegate.cc',
        'glue/oxide_qt_web_popup_menu_delegate.h',
        'glue/oxide_qt_web_view_adapter.cc',
        'glue/oxide_qt_web_view_adapter.h',
      ],
      'actions': [
        {
          'action_name': 'moc_oxideqincomingmessage.cc',
          'moc_input': 'api/oxideqincomingmessage.h',
          'includes': [ 'moc.gypi' ]
        },
        {
          'action_name': 'moc_oxideqloadevent.cc',
          'moc_input': 'api/oxideqloadevent.h',
          'includes': [ 'moc.gypi' ]
        },
      ]
    }
  ]
}
