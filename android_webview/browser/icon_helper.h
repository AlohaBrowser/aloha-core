// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_ICON_HELPER_H_
#define ANDROID_WEBVIEW_BROWSER_ICON_HELPER_H_

#include <stdint.h>

#include <string>
#include <unordered_set>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"  // ALOHA https://app.clickup.com/t/86exgxd2t
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/mojom/favicon/favicon_url.mojom-forward.h"
#include "url/gurl.h"

class SkBitmap;

namespace gfx {
class Size;
}

namespace android_webview {

// A helper that observes favicon changes for Webview.
// Lifetime: WebView
class IconHelper : public content::WebContentsObserver {
 public:
  class Listener {
   public:
    virtual bool ShouldDownloadFavicon() = 0;
    virtual void OnReceivedIcon(const GURL& icon_url,
                                const SkBitmap& bitmap) = 0;
    virtual void OnReceivedTouchIconUrl(const std::string& url,
                                        const bool precomposed) = 0;
   protected:
    virtual ~Listener() {}
  };

  explicit IconHelper(content::WebContents* web_contents);

  IconHelper(const IconHelper&) = delete;
  IconHelper& operator=(const IconHelper&) = delete;

  ~IconHelper() override;

  void SetListener(Listener* listener);

  // From WebContentsObserver
  void DidUpdateFaviconURL(
      content::RenderFrameHost* render_frame_host,
      const std::vector<blink::mojom::FaviconURLPtr>& candidates,
      blink::mojom::FaviconUpdateReason reason) override;
  void DidStartNavigation(content::NavigationHandle* navigation) override;
  // ALOHA https://app.clickup.com/t/86exgxd2t
  void DidFinishNavigation(content::NavigationHandle* navigation) override;

  // ALOHA https://app.clickup.com/t/86exgxd2t : |document_scoped| dies on the
  // next cross-document commit, |lifetime| only with the IconHelper (404s).
  static void DownloadFaviconCallback(
      base::WeakPtr<IconHelper> document_scoped,
      base::WeakPtr<IconHelper> lifetime,
      int id,
      int http_status_code,
      const GURL& image_url,
      const std::vector<SkBitmap>& bitmaps,
      const std::vector<gfx::Size>& original_bitmap_sizes);

 private:
  // ALOHA https://app.clickup.com/t/86exgxd2t
  void OnFaviconDownloaded(const GURL& image_url,
                           const std::vector<SkBitmap>& bitmaps,
                           const std::vector<gfx::Size>& original_bitmap_sizes);
  void MarkUnableToDownloadFavicon(const GURL& icon_url);
  bool WasUnableToDownloadFavicon(const GURL& icon_url) const;
  void ClearUnableToDownloadFavicons();

  raw_ptr<Listener> listener_;

  using MissingFaviconURLHash = size_t;
  std::unordered_set<MissingFaviconURLHash> missing_favicon_urls_;

  // ALOHA https://app.clickup.com/t/86exgxd2t : |weak_factory_| is invalidated
  // on every cross-document commit in the primary main frame (stale favicon
  // downloads); |lifetime_weak_factory_| never is (404 negative cache).
  base::WeakPtrFactory<IconHelper> weak_factory_{this};
  base::WeakPtrFactory<IconHelper> lifetime_weak_factory_{this};
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_ICON_HELPER_H_
