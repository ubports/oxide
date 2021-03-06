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

#ifndef _OXIDE_SHARED_BROWSER_CLIPBOARD_CLIPBOARD_DUMMY_IMPL_H_
#define _OXIDE_SHARED_BROWSER_CLIPBOARD_CLIPBOARD_DUMMY_IMPL_H_

#include "base/macros.h"

#include "shared/browser/clipboard/oxide_clipboard.h"

namespace oxide {

class ClipboardDummyImpl : public Clipboard {
 public:
  ClipboardDummyImpl();
  ~ClipboardDummyImpl() override;

 private:
  // ui::Clipboard implementation
  void OnPreShutdown() override;
  uint64_t GetSequenceNumber(ui::ClipboardType type) const override;
  bool IsFormatAvailable(const FormatType& format,
                         ui::ClipboardType type) const override;
  void Clear(ui::ClipboardType type) override;
  void ReadAvailableTypes(ui::ClipboardType type,
                          std::vector<base::string16>* types,
                          bool* contains_filenames) const override;
  void ReadText(ui::ClipboardType type, base::string16* result) const override;
  void ReadAsciiText(ui::ClipboardType type,
                     std::string* result) const override;
  void ReadHTML(ui::ClipboardType type,
                base::string16* markup,
                std::string* src_url,
                uint32_t* fragment_start,
                uint32_t* fragment_end) const override;
  void ReadRTF(ui::ClipboardType type, std::string* result) const override;
  SkBitmap ReadImage(ui::ClipboardType type) const override;
  void ReadCustomData(ui::ClipboardType clipboard_type,
                      const base::string16& type,
                      base::string16* result) const override;
  void ReadBookmark(base::string16* title, std::string* url) const override;
  void ReadData(const FormatType& format, std::string* result) const override;
  void WriteObjects(ui::ClipboardType type, const ObjectMap& objects) override;
  void WriteText(const char* text_data, size_t text_len) override;
  void WriteHTML(const char* markup_data,
                 size_t markup_len,
                 const char* url_data,
                 size_t url_len) override;
  void WriteRTF(const char* rtf_data, size_t data_len) override;
  void WriteBookmark(const char* title_data,
                     size_t title_len,
                     const char* url_data,
                     size_t url_len) override;
  void WriteWebSmartPaste() override;
  void WriteBitmap(const SkBitmap& bitmap) override;
  void WriteData(const FormatType& format,
                 const char* data_data,
                 size_t data_len) override;

  DISALLOW_COPY_AND_ASSIGN(ClipboardDummyImpl);
};

} // namespace oxide

#endif // _OXIDE_SHARED_BROWSER_CLIPBOARD_CLIPBOARD_DUMMY_IMPL_H_
