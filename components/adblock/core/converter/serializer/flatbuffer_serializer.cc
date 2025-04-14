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

#include "components/adblock/core/converter/serializer/flatbuffer_serializer.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/regex_filter_pattern.h"
#include "components/adblock/core/converter/parser/filter_classifier.h"
#include "components/adblock/core/converter/serializer/filter_keyword_extractor.h"

namespace adblock {

class Buffer : public FlatbufferData {
 public:
  explicit Buffer(flatbuffers::DetachedBuffer&& buffer)
      : buffer_(std::move(buffer)) {}

  const uint8_t* data() const override { return buffer_.data(); }

  size_t size() const override { return buffer_.size(); }

  const base::span<const uint8_t> span() const override {
    return base::as_byte_span(buffer_);
  }

 private:
  flatbuffers::DetachedBuffer buffer_;
};

FlatbufferSerializer::FlatbufferSerializer(GURL subscription_url,
                                           bool allow_privileged)
    : subscription_url_(subscription_url), allow_privileged_(allow_privileged) {
  SerializeMetadata(Metadata::Default());
}

FlatbufferSerializer::~FlatbufferSerializer() = default;

std::unique_ptr<FlatbufferData>
FlatbufferSerializer::GetSerializedSubscription() {
  auto subscription = flat::CreateSubscription(
      builder_, metadata_, WriteUrlFilterIndex(url_subresource_block_),
      WriteUrlFilterIndex(url_subresource_allow_),
      WriteUrlFilterIndex(url_popup_block_),
      WriteUrlFilterIndex(url_popup_allow_),
      WriteUrlFilterIndex(url_document_allow_),
      WriteUrlFilterIndex(url_elemhide_allow_),
      WriteUrlFilterIndex(url_generichide_allow_),
      WriteUrlFilterIndex(url_genericblock_allow_),
      WriteUrlFilterIndex(url_csp_block_), WriteUrlFilterIndex(url_csp_allow_),
      WriteUrlFilterIndex(url_rewrite_block_),
      WriteUrlFilterIndex(url_rewrite_allow_),
      WriteUrlFilterIndex(url_header_block_),
      WriteUrlFilterIndex(url_header_allow_),
      WriteElemhideFilterIndex(elemhide_index_),
      WriteElemhideFilterIndex(elemhide_emulation_index_),
      WriteElemhideFilterIndex(elemhide_exception_index_),
      WriteRemoveFilterIndex(remove_index_),
      WriteInlineCssFilterIndex(inline_css_index_),
      WriteSnippetFilterIndex(snippet_index_));

  builder_.Finish(subscription, flat::SubscriptionIdentifier());
  return std::make_unique<Buffer>(builder_.Release());
}

void FlatbufferSerializer::SerializeMetadata(const Metadata metadata) {
  metadata_ = flat::CreateSubscriptionMetadata(
      builder_, builder_.CreateString(CurrentSchemaVersion()),
      builder_.CreateString(subscription_url_.spec()),
      builder_.CreateString(metadata.homepage),
      builder_.CreateString(metadata.title),
      builder_.CreateString(metadata.version),
      metadata.expires.InMilliseconds());
}

void FlatbufferSerializer::SerializeContentFilter(
    const ContentFilter content_filter) {
  switch (content_filter.type) {
    case FilterType::ElemHide:
      AddElemhideFilterForDomains(elemhide_index_,
                                  content_filter.domains.GetIncludeDomains(),
                                  CreateElemHideFilter(content_filter));
      break;
    case FilterType::ElemHideException:
      AddElemhideFilterForDomains(elemhide_exception_index_,
                                  content_filter.domains.GetIncludeDomains(),
                                  CreateElemHideFilter(content_filter));
      break;
    case FilterType::ElemHideEmulation:
      AddElemhideFilterForDomains(elemhide_emulation_index_,
                                  content_filter.domains.GetIncludeDomains(),
                                  CreateElemHideFilter(content_filter));
      break;
    case FilterType::Remove:
      AddRemoveFilterForDomains(remove_index_,
                                content_filter.domains.GetIncludeDomains(),
                                CreateRemoveFilter(content_filter));
      break;
    case FilterType::InlineCss:
      if (!allow_privileged_) {
        VLOG(1) << "[eyeo] Inline CSS filters not allowed";
        break;
      }
      AddInlineCssFilterForDomains(inline_css_index_,
                                   content_filter.domains.GetIncludeDomains(),
                                   CreateInlineCssFilter(content_filter));
      break;
    default:
      break;
  }
}

void FlatbufferSerializer::SerializeSnippetFilter(
    const SnippetFilter snippet_filter) {
  if (!allow_privileged_) {
    VLOG(1) << "[eyeo] Snippet filters not allowed";
    return;
  }

  std::vector<flatbuffers::Offset<adblock::flat::SnippetFunctionCall>> offsets;
  offsets.reserve(snippet_filter.snippet_script.size());
  for (const auto& cur : snippet_filter.snippet_script) {
    offsets.push_back(flat::CreateSnippetFunctionCall(
        builder_, builder_.CreateSharedString(cur.front()),
        builder_.CreateVectorOfStrings(++cur.begin(), cur.end())));
  }

  auto offset = flat::CreateSnippetFilter(
      builder_, {},
      CreateVectorOfSharedStrings(snippet_filter.domains.GetExcludeDomains()),
      builder_.CreateVector(offsets));
  AddSnippetFilterForDomains(
      snippet_index_, snippet_filter.domains.GetIncludeDomains(), offset);
}

void FlatbufferSerializer::SerializeUrlFilter(const UrlFilter url_filter) {
  const auto& options = url_filter.options;
  if (!allow_privileged_ && options.Headers().has_value()) {
    VLOG(1) << "[eyeo] Header filters not allowed";
    return;
  }

  auto offset = flat::CreateUrlFilter(
      builder_, builder_.CreateString(url_filter.original_filter_text),
      builder_.CreateString(url_filter.pattern), options.IsMatchCase(),
      options.ContentTypes(), ThirdPartyOptionToFb(options.ThirdParty()),
      CreateVectorOfSharedStringsFromSitekeys(options.Sitekeys()),
      CreateVectorOfSharedStrings(options.Domains().GetIncludeDomains()),
      CreateVectorOfSharedStrings(options.Domains().GetExcludeDomains()),
      options.Rewrite().has_value()
          ? flat::CreateRewrite(builder_,
                                RewriteOptionToFb(options.Rewrite().value()))
          : flatbuffers::Offset<flat::Rewrite>(),
      options.Csp().has_value()
          ? builder_.CreateSharedString(options.Csp().value())
          : flatbuffers::Offset<flatbuffers::String>(),
      options.Headers().has_value()
          ? builder_.CreateSharedString(options.Headers().value())
          : flatbuffers::Offset<flatbuffers::String>());

  const absl::optional<std::string_view> keyword_pattern =
      ExtractRegexFilterFromPattern(url_filter.pattern).has_value()
          ? absl::optional<std::string_view>()
          : url_filter.pattern;

  if (options.Headers().has_value()) {
    AddUrlFilterToIndex(
        url_filter.is_allowing ? url_header_allow_ : url_header_block_,
        keyword_pattern, offset);
    return;
  }

  if (options.IsPopup()) {
    AddUrlFilterToIndex(
        url_filter.is_allowing ? url_popup_allow_ : url_popup_block_,
        keyword_pattern, offset);
  }

  if (options.Csp().has_value()) {
    AddUrlFilterToIndex(
        url_filter.is_allowing ? url_csp_allow_ : url_csp_block_,
        keyword_pattern, offset);
  }

  if (options.Rewrite().has_value()) {
    AddUrlFilterToIndex(
        url_filter.is_allowing ? url_rewrite_allow_ : url_rewrite_block_,
        keyword_pattern, offset);
  }

  if (options.IsSubresource()) {
    AddUrlFilterToIndex(url_filter.is_allowing ? url_subresource_allow_
                                               : url_subresource_block_,
                        keyword_pattern, offset);
  }

  for (auto exception_type : options.ExceptionTypes()) {
    switch (exception_type) {
      case UrlFilterOptions::ExceptionType::Genericblock:
        AddUrlFilterToIndex(url_genericblock_allow_, keyword_pattern, offset);
        break;
      case UrlFilterOptions::ExceptionType::Generichide:
        AddUrlFilterToIndex(url_generichide_allow_, keyword_pattern, offset);
        break;
      case UrlFilterOptions::ExceptionType::Document:
        AddUrlFilterToIndex(url_document_allow_, keyword_pattern, offset);
        break;
      case UrlFilterOptions::ExceptionType::Elemhide:
        AddUrlFilterToIndex(url_elemhide_allow_, keyword_pattern, offset);
        break;
      default:
        break;
    }
  }
}

void FlatbufferSerializer::AddUrlFilterToIndex(
    UrlFilterIndex& index,
    absl::optional<std::string_view> pattern_text,
    flatbuffers::Offset<flat::UrlFilter> filter) {
  const auto keyword =
      pattern_text ? FindCandidateKeyword(index, *pattern_text) : "";
  index[keyword].push_back(filter);
}

void FlatbufferSerializer::AddElemhideFilterForDomains(
    ElemhideIndex& index,
    const std::vector<std::string>& include_domains,
    flatbuffers::Offset<flat::ElemHideFilter> filter) const {
  if (include_domains.empty()) {
    // This is a generic filter, we add those under "" index.
    index[""].push_back(filter);
  } else {
    // Index this filter under each domain it is included for.
    for (const auto& domain : include_domains) {
      index[domain].push_back(filter);
    }
  }
}

void FlatbufferSerializer::AddRemoveFilterForDomains(
    RemoveFilterIndex& index,
    const std::vector<std::string>& include_domains,
    flatbuffers::Offset<flat::RemoveFilter> filter) const {
  // Include domains can not be empty for remove filters
  DCHECK(!include_domains.empty());
  for (const auto& domain : include_domains) {
    index[domain].push_back(filter);
  }
}

void FlatbufferSerializer::AddInlineCssFilterForDomains(
    InlineCssFilterIndex& index,
    const std::vector<std::string>& include_domains,
    flatbuffers::Offset<flat::InlineCssFilter> filter) const {
  // Include domains can not be empty for inline CSS filters
  DCHECK(!include_domains.empty());
  for (const auto& domain : include_domains) {
    index[domain].push_back(filter);
  }
}

void FlatbufferSerializer::AddSnippetFilterForDomains(
    SnippetIndex& index,
    const std::vector<std::string>& domains,
    flatbuffers::Offset<flat::SnippetFilter> filter) const {
  for (const auto& domain : domains) {
    index[domain].push_back(filter);
  }
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>>
FlatbufferSerializer::CreateVectorOfSharedStrings(
    const std::vector<std::string>& strings) {
  std::vector<flatbuffers::Offset<flatbuffers::String>> shared_strings;
  std::transform(
      strings.begin(), strings.end(), std::back_inserter(shared_strings),
      [&](const std::string& s) { return builder_.CreateSharedString(s); });
  return builder_.CreateVector(std::move(shared_strings));
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>>
FlatbufferSerializer::CreateVectorOfSharedStringsFromSitekeys(
    const std::vector<SiteKey>& sitekeys) {
  std::vector<flatbuffers::Offset<flatbuffers::String>> shared_strings;
  std::transform(
      sitekeys.begin(), sitekeys.end(), std::back_inserter(shared_strings),
      [&](const SiteKey& s) { return builder_.CreateSharedString(s.value()); });
  return builder_.CreateVector(std::move(shared_strings));
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>>>
FlatbufferSerializer::WriteUrlFilterIndex(const UrlFilterIndex& index) {
  std::vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateUrlFiltersByKeyword(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }

  return builder_.CreateVector(offsets);
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::ElemHideFiltersByDomain>>>
FlatbufferSerializer::WriteElemhideFilterIndex(const ElemhideIndex& index) {
  std::vector<flatbuffers::Offset<flat::ElemHideFiltersByDomain>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateElemHideFiltersByDomain(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }
  // Filters must be sorted (by domain), in order for LookupByKey() to work
  // correctly. This can be also achieved by making ElemhideIndex an ordered
  // map, but profiling shows sorting an unordered_map at the end is faster by
  // about 15% (on exceptionrules.txt).
  return builder_.CreateVectorOfSortedTables(offsets.data(), offsets.size());
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::RemoveFiltersByDomain>>>
FlatbufferSerializer::WriteRemoveFilterIndex(const RemoveFilterIndex& index) {
  std::vector<flatbuffers::Offset<flat::RemoveFiltersByDomain>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateRemoveFiltersByDomain(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }

  return builder_.CreateVector(offsets);
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::InlineCssFiltersByDomain>>>
FlatbufferSerializer::WriteInlineCssFilterIndex(
    const InlineCssFilterIndex& index) {
  std::vector<flatbuffers::Offset<flat::InlineCssFiltersByDomain>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateInlineCssFiltersByDomain(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }

  return builder_.CreateVector(offsets);
}

flatbuffers::Offset<
    flatbuffers::Vector<flatbuffers::Offset<flat::SnippetFiltersByDomain>>>
FlatbufferSerializer::WriteSnippetFilterIndex(const SnippetIndex& index) {
  std::vector<flatbuffers::Offset<flat::SnippetFiltersByDomain>> offsets;
  offsets.reserve(index.size());

  for (const auto& cur : index) {
    offsets.push_back(flat::CreateSnippetFiltersByDomain(
        builder_, builder_.CreateSharedString(cur.first),
        builder_.CreateVector(cur.second)));
  }
  return builder_.CreateVector(offsets);
}

std::string FlatbufferSerializer::FindCandidateKeyword(UrlFilterIndex& index,
                                                       std::string_view value) {
  FilterKeywordExtractor keyword_extractor(value);
  size_t last_size = std::numeric_limits<size_t>::max();
  std::string keyword;
  while (auto current_keyword = keyword_extractor.GetNextKeyword()) {
    std::string candidate = *current_keyword;
    auto it = index.find(candidate);
    auto size = it != index.end() ? it->second.size() : 0;

    if (size < last_size ||
        (size == last_size && candidate.size() > keyword.size())) {
      last_size = size;
      keyword = candidate;
    }
  }
  return keyword;
}

flatbuffers::Offset<flat::ElemHideFilter>
FlatbufferSerializer::CreateElemHideFilter(
    const ContentFilter& content_filter) {
  return flat::CreateElemHideFilter(
      builder_, {},
      content_filter.type == FilterType::ElemHideEmulation
          ? builder_.CreateString(content_filter.selector.data(),
                                  content_filter.selector.size())
          : builder_.CreateString(EscapeSelector(content_filter.selector)),
      CreateVectorOfSharedStrings(content_filter.domains.GetExcludeDomains()));
}

flatbuffers::Offset<flat::RemoveFilter>
FlatbufferSerializer::CreateRemoveFilter(const ContentFilter& content_filter) {
  return flat::CreateRemoveFilter(
      builder_, {},
      builder_.CreateString(content_filter.selector.data(),
                            content_filter.selector.size()),
      CreateVectorOfSharedStrings(content_filter.domains.GetExcludeDomains()));
}

flatbuffers::Offset<flat::InlineCssFilter>
FlatbufferSerializer::CreateInlineCssFilter(
    const ContentFilter& content_filter) {
  return flat::CreateInlineCssFilter(
      builder_, {},
      builder_.CreateString(content_filter.selector.data(),
                            content_filter.selector.size()),
      builder_.CreateString(content_filter.modifier.data(),
                            content_filter.modifier.size()),
      CreateVectorOfSharedStrings(content_filter.domains.GetExcludeDomains()));
}

// static
std::string FlatbufferSerializer::EscapeSelector(
    const std::string_view& value) {
  std::string escaped;
  base::ReplaceChars(value, "{", "\\7b ", &escaped);
  base::ReplaceChars(escaped, "}", "\\7d ", &escaped);
  return escaped;
}

// static
flat::ThirdParty FlatbufferSerializer::ThirdPartyOptionToFb(
    UrlFilterOptions::ThirdPartyOption option) {
  if (option == UrlFilterOptions::ThirdPartyOption::ThirdPartyOnly) {
    return flat::ThirdParty_ThirdPartyOnly;
  }
  if (option == UrlFilterOptions::ThirdPartyOption::FirstPartyOnly) {
    return flat::ThirdParty_FirstPartyOnly;
  }
  return flat::ThirdParty_Ignore;
}

// static
flat::AbpResource FlatbufferSerializer::RewriteOptionToFb(
    UrlFilterOptions::RewriteOption option) {
  switch (option) {
    case UrlFilterOptions::RewriteOption::AbpResource_BlankText:
      return flat::AbpResource_BlankText;
    case UrlFilterOptions::RewriteOption::AbpResource_BlankCss:
      return flat::AbpResource_BlankCss;
    case UrlFilterOptions::RewriteOption::AbpResource_BlankJs:
      return flat::AbpResource_BlankJs;
    case UrlFilterOptions::RewriteOption::AbpResource_BlankHtml:
      return flat::AbpResource_BlankHtml;
    case UrlFilterOptions::RewriteOption::AbpResource_BlankMp3:
      return flat::AbpResource_BlankMp3;
    case UrlFilterOptions::RewriteOption::AbpResource_BlankMp4:
      return flat::AbpResource_BlankMp4;
    case UrlFilterOptions::RewriteOption::AbpResource_TransparentGif1x1:
      return flat::AbpResource_TransparentGif1x1;
    case UrlFilterOptions::RewriteOption::AbpResource_TransparentPng2x2:
      return flat::AbpResource_TransparentPng2x2;
    case UrlFilterOptions::RewriteOption::AbpResource_TransparentPng3x2:
      return flat::AbpResource_TransparentPng3x2;
    case UrlFilterOptions::RewriteOption::AbpResource_TransparentPng32x32:
      return flat::AbpResource_TransparentPng32x32;
    default:
      NOTREACHED();
  }
}

}  // namespace adblock
