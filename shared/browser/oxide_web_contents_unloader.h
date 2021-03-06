// vim:expandtab:shiftwidth=2:tabstop=2:
// Copyright (C) 2015 Canonical Ltd.

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

#ifndef _OXIDE_SHARED_BROWSER_WEB_CONTENTS_UNLOADER_H_
#define _OXIDE_SHARED_BROWSER_WEB_CONTENTS_UNLOADER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "content/public/browser/web_contents_delegate.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace content {
class WebContents;
}

namespace oxide {

// Singleton class that provides a mechanism to correctly unload WebContents,
// by running the unload handler after its WebView has been destroyed and
// ensuring that its render process lives until the handler has completed
class WebContentsUnloader : public content::WebContentsDelegate {
 public:
  ~WebContentsUnloader();

  // Return the WebContentsUnloader singleton
  static WebContentsUnloader* GetInstance();

  // Run the unload handler for the current page in |contents|. This method
  // takes ownership of |contents| and deletes it when complete
  void Unload(std::unique_ptr<content::WebContents> contents);

  // Delete all WebContents that are currently closing without waiting for
  // unload handlers to complete
  void Shutdown();

  // Determine whether |contents| is currently unloading
  bool IsUnloadInProgress(content::WebContents* contents);

 private:
  friend class base::DefaultSingletonTraits<WebContentsUnloader>;

  WebContentsUnloader();

  // content::WebContentsDelegate implementation
  void CloseContents(content::WebContents* contents) override;

  // The WebContents for which we are waiting to unload
  std::vector<std::unique_ptr<content::WebContents>> contents_unloading_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsUnloader);
};

} // namespace oxide

#endif // _OXIDE_SHARED_BROWSER_WEB_CONTENTS_UNLOADER_H_
