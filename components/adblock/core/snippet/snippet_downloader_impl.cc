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

#include "components/adblock/core/snippet/snippet_downloader_impl.h"

#include <fstream>

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/converter/parser/metadata.h"
#include "crypto/obsolete/md5.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace adblock {

std::string Md5AsHexForSnippetLibrary(const std::string& data) {
  auto hash = crypto::obsolete::Md5::Hash(data);
  return base::ToLowerASCII(base::HexEncode(hash));
}

namespace {
AdblockResourceRequest::HeadersMap GetRequestHeaders(PrefService* prefs) {
  return prefs->GetString(common::prefs::kSnippetsVersion).empty()
             ? AdblockResourceRequest::HeadersMap()
             : AdblockResourceRequest::HeadersMap{
                   {net::HttpRequestHeaders::kIfNoneMatch,
                    prefs->GetString(common::prefs::kSnippetsVersion)}};
}

AdblockResourceRequest::HeadersMap GetHeadHeaders() {
  return {{net::HttpRequestHeaders::kAcceptEncoding, "plain"}};
}

bool StoredLibraryMatchesVersion(const std::string& stored_library,
                                 const std::string& new_version) {
  auto stored_library_checksum = Md5AsHexForSnippetLibrary(stored_library);
  VLOG(2) << "[eyeo] Stored library version: " << stored_library_checksum;
  return base::StartsWith(new_version, stored_library_checksum);
}

std::string EtagFromHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& headers) {
  std::string new_etag{headers->EnumerateHeader(nullptr, "ETag").value_or("")};
  base::TrimString(new_etag, "\"", &new_etag);
  return new_etag;
}

inline void RemoveFile(base::FilePath path) {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(base::IgnoreResult(&base::DeleteFile), path));
}

std::string ReadDownloadedSnippets(PrefService* prefs,
                                   base::FilePath downloaded_file) {
  std::string result;
  std::ifstream input_stream(downloaded_file.AsUTF8Unsafe());

  if (!input_stream.is_open() || !input_stream.good()) {
    LOG(WARNING) << "[eyeo] Bad stream of snippet library";
  } else {
    result = std::string(std::istreambuf_iterator<char>(input_stream), {});
    input_stream.close();
  }
  RemoveFile(downloaded_file);
  return result;
}

}  // namespace

SnippetDownloaderImpl::SnippetDownloaderImpl(
    PrefService* prefs,
    std::unique_ptr<TaskScheduler> scheduler,
    SnippetsRequestMaker request_maker,
    GURL url,
    SnippetStorage* storage)
    : prefs_(prefs),
      scheduler_(std::move(scheduler)),
      request_maker_(std::move(request_maker)),
      url_(url),
      storage_(storage) {}

SnippetDownloaderImpl::~SnippetDownloaderImpl() = default;

void SnippetDownloaderImpl::StartSchedule() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          adblock::switches::kDisableEyeoSnippetsLibraryUpdate)) {
    scheduler_->StartSchedule(
        base::BindRepeating(&SnippetDownloaderImpl::RunUpdateCheck,
                            weak_ptr_factory_.GetWeakPtr()));
  }
}

void SnippetDownloaderImpl::RunUpdateCheck() {
  VLOG(2) << "[eyeo] Start snippet update check";
  if (ongoing_request_) {
    VLOG(2) << "[eyeo] Snippet update request already in progress";
    return;
  }
  ongoing_request_ = request_maker_.Run();
  if (prefs_->GetString(common::prefs::kSnippetsVersion).empty()) {
    ongoing_request_->Start(
        url_, AdblockResourceRequest::Method::HEAD,
        base::BindRepeating(&SnippetDownloaderImpl::OnVersionInfo,
                            weak_ptr_factory_.GetWeakPtr()),
        AdblockResourceRequest::RetryPolicy::DoNotRetry, "", GetHeadHeaders());
  } else {
    ongoing_request_->Start(
        url_, AdblockResourceRequest::Method::GET,
        base::BindRepeating(&SnippetDownloaderImpl::OnDownloaded,
                            weak_ptr_factory_.GetWeakPtr()),
        AdblockResourceRequest::RetryPolicy::DoNotRetry, "",
        GetRequestHeaders(prefs_));
  }
}

void SnippetDownloaderImpl::MakeGetRequest(const std::string& etag) {
  VLOG(2) << "[eyeo] Calling snippet library update";
  ongoing_request_ = request_maker_.Run();
  ongoing_request_->Start(
      url_, AdblockResourceRequest::Method::GET,
      base::BindRepeating(&SnippetDownloaderImpl::OnDownloaded,
                          weak_ptr_factory_.GetWeakPtr()),
      AdblockResourceRequest::RetryPolicy::DoNotRetry, "",
      GetRequestHeaders(prefs_));
}

void SnippetDownloaderImpl::OnVersionInfo(
    const GURL& url,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  ongoing_request_.reset();
  if (!headers) {
    LOG(WARNING) << "[eyeo] Missing headers, skipping response processing";
    return;
  }
  auto new_etag = EtagFromHeaders(headers);
  if (new_etag.empty()) {
    LOG(WARNING) << "[eyeo] No ETag for snippet library";
    return;
  }
  VLOG(2) << "[eyeo] Got snippet library version: " << new_etag;
  if (StoredLibraryMatchesVersion(storage_->GetLoadedSnippets(), new_etag)) {
    // Set the version pref so next checks continue via GET request
    prefs_->SetString(common::prefs::kSnippetsVersion, new_etag);
  } else {
    MakeGetRequest(new_etag);
  }
}
void SnippetDownloaderImpl::OnSnippetsRead(
    const std::string& etag,
    const std::string& snippets_content) {
  storage_->StoreSnippets(
      snippets_content, base::BindOnce(&SnippetDownloaderImpl::MaybeUpdatePrefs,
                                       weak_ptr_factory_.GetWeakPtr(), etag));
}

void SnippetDownloaderImpl::MaybeUpdatePrefs(const std::string& etag,
                                             const std::string& signature) {
  if (signature.empty()) {
    LOG(WARNING) << "[eyeo] Failed to store snippet library";
  } else {
    VLOG(2) << "[eyeo] Stored new snippet library";
    DCHECK(!etag.empty());
    prefs_->SetString(common::prefs::kSnippetsVersion, etag);
    prefs_->SetString(common::prefs::kSnippetsSignature, signature);
  }
}

void SnippetDownloaderImpl::OnDownloaded(
    const GURL& url,
    base::FilePath downloaded_file,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  ongoing_request_.reset();
  VLOG(2) << "[eyeo] Downloaded new snippet library";
  if (!headers) {
    LOG(WARNING) << "[eyeo] Missing headers, skipping response processing";
    RemoveFile(downloaded_file);
    return;
  }
  auto new_etag = EtagFromHeaders(headers);
  if (new_etag.empty()) {
    LOG(WARNING) << "[eyeo] No ETag for snippet library";
    RemoveFile(downloaded_file);
    return;
  }
  auto current_etag = prefs_->GetString(common::prefs::kSnippetsVersion);
  if (new_etag == current_etag) {
    VLOG(2) << "[eyeo] No new snippet library, same ETag";
    RemoveFile(downloaded_file);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadDownloadedSnippets, prefs_, downloaded_file),
      base::BindOnce(&SnippetDownloaderImpl::OnSnippetsRead,
                     weak_ptr_factory_.GetWeakPtr(), new_etag));
}

}  // namespace adblock
