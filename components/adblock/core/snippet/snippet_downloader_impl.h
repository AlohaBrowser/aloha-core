/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/core/snippet/snippet_downloader.h"
#include "components/prefs/pref_service.h"

#ifndef COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_DOWNLOADER_IMPL_H_
#define COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_DOWNLOADER_IMPL_H_

#include "components/adblock/core/common/task_scheduler.h"
#include "components/adblock/core/net/adblock_resource_request.h"
#include "components/adblock/core/snippet/snippet_storage.h"

namespace adblock {

class SnippetDownloaderImpl final : public SnippetDownloader {
 public:
  // Used to create AdblockResourceRequest to implement HEAD and
  // GET requests for snippets library.
  using SnippetsRequestMaker =
      base::RepeatingCallback<std::unique_ptr<AdblockResourceRequest>()>;

  SnippetDownloaderImpl(PrefService* prefs,
                        std::unique_ptr<TaskScheduler> scheduler,
                        SnippetsRequestMaker request_maker,
                        GURL url,
                        SnippetStorage* storage);
  ~SnippetDownloaderImpl() final;

  void StartSchedule() final;

 private:
  void RunUpdateCheck();
  void OnVersionInfo(const GURL& url,
                     base::FilePath downloaded_file,
                     scoped_refptr<net::HttpResponseHeaders> headers);
  void MakeGetRequest(const std::string& etag);
  void OnDownloaded(const GURL& url,
                    base::FilePath downloaded_file,
                    scoped_refptr<net::HttpResponseHeaders> headers);
  void MaybeUpdatePrefs(const std::string& etag, const std::string& signature);
  void OnSnippetsRead(const std::string& etag,
                      const std::string& snippets_content);

  raw_ptr<PrefService> prefs_;
  std::unique_ptr<TaskScheduler> scheduler_;
  SnippetsRequestMaker request_maker_;
  std::unique_ptr<AdblockResourceRequest> ongoing_request_;
  GURL url_;
  raw_ptr<SnippetStorage> storage_;
  base::WeakPtrFactory<SnippetDownloaderImpl> weak_ptr_factory_{this};
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SNIPPET_SNIPPET_DOWNLOADER_IMPL_H_
