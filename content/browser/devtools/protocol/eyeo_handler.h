// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_EYEO_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_EYEO_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "components/adblock/content/browser/resource_classification_runner.h"  // nogncheck
#include "components/adblock/core/configuration/filtering_configuration.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/browser/devtools/protocol/eyeo.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {

class DevToolsAgentHostImpl;
class RenderFrameHostImpl;

namespace protocol {

class EyeoHandler final
    : public DevToolsDomainHandler,
      public Eyeo::Backend,
      public adblock::FilteringConfiguration::Observer,
      public adblock::ResourceClassificationRunner::Observer {
 public:
  EyeoHandler();

  EyeoHandler(const EyeoHandler&) = delete;
  EyeoHandler& operator=(const EyeoHandler&) = delete;

  ~EyeoHandler() override;

  static std::vector<EyeoHandler*> ForAgentHost(DevToolsAgentHostImpl* host);

  // DevToolsDomainHandler implementation.
  void SetRenderer(int process_host_id,
                   RenderFrameHostImpl* frame_host) override;
  void Wire(UberDispatcher* dispatcher) override;

  // Eyeo::Backend implementation.
  Response Disable() override;
  Response Enable() override;
  Response GetConfigurations(
      std::unique_ptr<protocol::Array<std::string>>* lists) override;
  Response GetDownloadStats(
      std::optional<std::string> configuration,
      std::unique_ptr<protocol::Array<std::string>>* result) override;
  Response GetSessionStats(
      std::optional<std::string> configuration,
      std::unique_ptr<protocol::Array<std::string>>* result) override;
  Response GetFilterLists(
      std::optional<std::string> configuration,
      std::unique_ptr<protocol::Array<std::string>>* lists) override;
  Response AddFilterList(std::optional<std::string> configuration,
                         const std::string& url) override;
  Response RemoveFilterList(std::optional<std::string> configuration,
                            const std::string& url) override;
  Response GetCustomFilters(
      std::optional<std::string> configuration,
      std::unique_ptr<protocol::Array<std::string>>* filters) override;
  Response AddCustomFilter(std::optional<std::string> configuration,
                           const std::string& filter) override;
  Response RemoveCustomFilter(std::optional<std::string> configuration,
                              const std::string& filter) override;
  Response GetAllowedDomains(
      std::optional<std::string> configuration,
      std::unique_ptr<protocol::Array<std::string>>* domains) override;
  Response AddAllowedDomain(std::optional<std::string> configuration,
                            const std::string& domain) override;
  Response RemoveAllowedDomain(std::optional<std::string> configuration,
                               const std::string& domain) override;
  Response GetTelemetryDebugInfo(protocol::String* result) override;

  // adblock::FilteringConfiguration::Observer implementation.
  void OnFilterListsChanged(adblock::FilteringConfiguration* config) override;
  void OnCustomFiltersChanged(adblock::FilteringConfiguration* config) override;

  // ResourceClassificationRunner::Observer
  void OnRequestMatched(const GURL& url,
                        adblock::FilterMatchResult match_result,
                        const std::vector<GURL>& parent_frame_urls,
                        adblock::ContentType content_type,
                        content::RenderFrameHost* render_frame_host,
                        const GURL& subscription,
                        const std::string& configuration_name) override;
  void OnPageAllowed(const GURL& url,
                     content::RenderFrameHost* render_frame_host,
                     const GURL& subscription,
                     const std::string& configuration_name) override;
  void OnPopupMatched(const GURL& url,
                      adblock::FilterMatchResult match_result,
                      const GURL& opener_url,
                      content::RenderFrameHost* render_frame_host,
                      const GURL& subscription,
                      const std::string& configuration_name) override;

 private:
  void OnMatchedInternal(const GURL& url,
                         adblock::FilterMatchResult match_result,
                         adblock::ContentType content_type,
                         content::RenderFrameHost* render_frame_host,
                         const GURL& subscription);
  std::unique_ptr<Eyeo::Frontend> frontend_;
  bool enabled_ = false;
  raw_ptr<RenderFrameHostImpl> rfh_;
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_EYEO_HANDLER_H_
