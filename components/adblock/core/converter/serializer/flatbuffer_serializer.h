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

#ifndef COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_FLATBUFFER_SERIALIZER_H_
#define COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_FLATBUFFER_SERIALIZER_H_

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/converter/parser/content_filter.h"
#include "components/adblock/core/converter/parser/metadata.h"
#include "components/adblock/core/converter/parser/snippet_filter.h"
#include "components/adblock/core/converter/parser/url_filter.h"
#include "components/adblock/core/converter/parser/url_filter_options.h"
#include "components/adblock/core/converter/serializer/serializer.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "url/gurl.h"

namespace adblock {

class FlatbufferSerializer final : public Serializer {
 public:
  explicit FlatbufferSerializer(GURL subscription_url, bool allow_privileged);
  ~FlatbufferSerializer() override;

  std::unique_ptr<FlatbufferData> GetSerializedSubscription();

  void SerializeMetadata(const Metadata metadata) override;
  void SerializeContentFilter(const ContentFilter content_filter) override;
  void SerializeSnippetFilter(const SnippetFilter snippet_filter) override;
  void SerializeUrlFilter(const UrlFilter url_filter) override;

 private:
  using UrlFilterIndex =
      std::map<std::string, std::vector<flatbuffers::Offset<flat::UrlFilter>>>;
  using ElemhideIndex = std::unordered_map<
      std::string,
      std::vector<flatbuffers::Offset<flat::ElemHideFilter>>>;
  using SnippetIndex =
      std::map<std::string,
               std::vector<flatbuffers::Offset<flat::SnippetFilter>>>;
  using RemoveFilterIndex =
      std::map<std::string,
               std::vector<flatbuffers::Offset<flat::RemoveFilter>>>;
  using InlineCssFilterIndex =
      std::map<std::string,
               std::vector<flatbuffers::Offset<flat::InlineCssFilter>>>;

  void AddUrlFilterToIndex(UrlFilterIndex& index,
                           absl::optional<std::string_view> pattern_text,
                           flatbuffers::Offset<flat::UrlFilter> filter);
  void AddElemhideFilterForDomains(
      ElemhideIndex& index,
      const std::vector<std::string>& include_domains,
      flatbuffers::Offset<flat::ElemHideFilter> filter) const;
  void AddRemoveFilterForDomains(
      RemoveFilterIndex& index,
      const std::vector<std::string>& include_domains,
      flatbuffers::Offset<flat::RemoveFilter> filter) const;
  void AddInlineCssFilterForDomains(
      InlineCssFilterIndex& index,
      const std::vector<std::string>& include_domains,
      flatbuffers::Offset<flat::InlineCssFilter> filter) const;
  void AddSnippetFilterForDomains(
      SnippetIndex& index,
      const std::vector<std::string>& domains,
      flatbuffers::Offset<flat::SnippetFilter> filter) const;

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>>
  CreateVectorOfSharedStrings(const std::vector<std::string>& strings);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>>
  CreateVectorOfSharedStringsFromSitekeys(const std::vector<SiteKey>& sitekeys);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::UrlFiltersByKeyword>>>
  WriteUrlFilterIndex(const UrlFilterIndex& index);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::ElemHideFiltersByDomain>>>
  WriteElemhideFilterIndex(const ElemhideIndex& index);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::RemoveFiltersByDomain>>>
  WriteRemoveFilterIndex(const RemoveFilterIndex& index);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::InlineCssFiltersByDomain>>>
  WriteInlineCssFilterIndex(const InlineCssFilterIndex& index);

  flatbuffers::Offset<
      flatbuffers::Vector<flatbuffers::Offset<flat::SnippetFiltersByDomain>>>
  WriteSnippetFilterIndex(const SnippetIndex& index);

  std::string FindCandidateKeyword(UrlFilterIndex& index,
                                   std::string_view value);

  flatbuffers::Offset<flat::ElemHideFilter> CreateElemHideFilter(
      const ContentFilter& content_filter);
  flatbuffers::Offset<flat::RemoveFilter> CreateRemoveFilter(
      const ContentFilter& content_filter);
  flatbuffers::Offset<flat::InlineCssFilter> CreateInlineCssFilter(
      const ContentFilter& content_filter);

  static std::string EscapeSelector(const std::string_view& value);

  static flat::ThirdParty ThirdPartyOptionToFb(
      UrlFilterOptions::ThirdPartyOption option);
  static flat::AbpResource RewriteOptionToFb(
      UrlFilterOptions::RewriteOption option);

  GURL subscription_url_;
  bool allow_privileged_ = false;
  flatbuffers::FlatBufferBuilder builder_;
  flatbuffers::Offset<flat::SubscriptionMetadata> metadata_;
  UrlFilterIndex url_subresource_block_;
  UrlFilterIndex url_subresource_allow_;
  UrlFilterIndex url_popup_block_;
  UrlFilterIndex url_popup_allow_;
  UrlFilterIndex url_document_allow_;
  UrlFilterIndex url_elemhide_allow_;
  UrlFilterIndex url_generichide_allow_;
  UrlFilterIndex url_genericblock_allow_;
  UrlFilterIndex url_csp_block_;
  UrlFilterIndex url_csp_allow_;
  UrlFilterIndex url_rewrite_block_;
  UrlFilterIndex url_rewrite_allow_;
  UrlFilterIndex url_header_allow_;
  UrlFilterIndex url_header_block_;
  ElemhideIndex elemhide_exception_index_;
  ElemhideIndex elemhide_index_;
  ElemhideIndex elemhide_emulation_index_;
  RemoveFilterIndex remove_index_;
  InlineCssFilterIndex inline_css_index_;
  SnippetIndex snippet_index_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_CONVERTER_SERIALIZER_FLATBUFFER_SERIALIZER_H_
