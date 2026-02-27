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

#ifndef CHROME_BROWSER_EXTENSIONS_API_EYEO_FILTERING_PRIVATE_EYEO_FILTERING_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_EYEO_FILTERING_PRIVATE_EYEO_FILTERING_PRIVATE_API_H_

#include "base/memory/raw_ptr.h"
#include "chrome/common/extensions/api/eyeo_filtering_private.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class EyeoFilteringPrivateAPI : public BrowserContextKeyedAPI,
                                public EventRouter::Observer {
 public:
  static BrowserContextKeyedAPIFactory<EyeoFilteringPrivateAPI>*
  GetFactoryInstance();

  static EyeoFilteringPrivateAPI* Get(content::BrowserContext* context);

  explicit EyeoFilteringPrivateAPI(content::BrowserContext* context);
  ~EyeoFilteringPrivateAPI() override;
  friend class BrowserContextKeyedAPIFactory<EyeoFilteringPrivateAPI>;

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "EyeoFilteringPrivateAPI"; }
  static const bool kServiceRedirectedInIncognito = true;
  static const bool kServiceIsCreatedWithBrowserContext = true;
  void Shutdown() override;

  // EventRouter::Observer:
  void OnListenerAdded(const extensions::EventListenerInfo& details) override;

 private:
  const raw_ptr<content::BrowserContext> context_;
  class EyeoFilteringAPIEventRouter;
  std::unique_ptr<EyeoFilteringAPIEventRouter> event_router_;
};

template <>
void BrowserContextKeyedAPIFactory<
    EyeoFilteringPrivateAPI>::DeclareFactoryDependencies();

namespace api {

class EyeoFilteringPrivateCreateConfigurationFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.createConfiguration",
                             UNKNOWN)
  EyeoFilteringPrivateCreateConfigurationFunction();

 private:
  ~EyeoFilteringPrivateCreateConfigurationFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateCreateConfigurationFunction(
      const EyeoFilteringPrivateCreateConfigurationFunction&) = delete;
  EyeoFilteringPrivateCreateConfigurationFunction& operator=(
      const EyeoFilteringPrivateCreateConfigurationFunction&) = delete;
};

class EyeoFilteringPrivateRemoveConfigurationFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.removeConfiguration",
                             UNKNOWN)
  EyeoFilteringPrivateRemoveConfigurationFunction();

 private:
  ~EyeoFilteringPrivateRemoveConfigurationFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateRemoveConfigurationFunction(
      const EyeoFilteringPrivateRemoveConfigurationFunction&) = delete;
  EyeoFilteringPrivateRemoveConfigurationFunction& operator=(
      const EyeoFilteringPrivateRemoveConfigurationFunction&) = delete;
};

class EyeoFilteringPrivateGetConfigurationsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.getConfigurations", UNKNOWN)
  EyeoFilteringPrivateGetConfigurationsFunction();

 private:
  ~EyeoFilteringPrivateGetConfigurationsFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateGetConfigurationsFunction(
      const EyeoFilteringPrivateGetConfigurationsFunction&) = delete;
  EyeoFilteringPrivateGetConfigurationsFunction& operator=(
      const EyeoFilteringPrivateGetConfigurationsFunction&) = delete;
};

class EyeoFilteringPrivateSetAutoInstallEnabledFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.setAutoInstallEnabled",
                             UNKNOWN)
  EyeoFilteringPrivateSetAutoInstallEnabledFunction();

 private:
  ~EyeoFilteringPrivateSetAutoInstallEnabledFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateSetAutoInstallEnabledFunction(
      const EyeoFilteringPrivateSetAutoInstallEnabledFunction&) = delete;
  EyeoFilteringPrivateSetAutoInstallEnabledFunction& operator=(
      const EyeoFilteringPrivateSetAutoInstallEnabledFunction&) = delete;
};

class EyeoFilteringPrivateIsAutoInstallEnabledFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.isAutoInstallEnabled",
                             UNKNOWN)
  EyeoFilteringPrivateIsAutoInstallEnabledFunction();

 private:
  ~EyeoFilteringPrivateIsAutoInstallEnabledFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateIsAutoInstallEnabledFunction(
      const EyeoFilteringPrivateIsAutoInstallEnabledFunction&) = delete;
  EyeoFilteringPrivateIsAutoInstallEnabledFunction& operator=(
      const EyeoFilteringPrivateIsAutoInstallEnabledFunction&) = delete;
};

class EyeoFilteringPrivateSetEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.setEnabled", UNKNOWN)
  EyeoFilteringPrivateSetEnabledFunction();

 private:
  ~EyeoFilteringPrivateSetEnabledFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateSetEnabledFunction(
      const EyeoFilteringPrivateSetEnabledFunction&) = delete;
  EyeoFilteringPrivateSetEnabledFunction& operator=(
      const EyeoFilteringPrivateSetEnabledFunction&) = delete;
};

class EyeoFilteringPrivateIsEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.isEnabled", UNKNOWN)
  EyeoFilteringPrivateIsEnabledFunction();

 private:
  ~EyeoFilteringPrivateIsEnabledFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateIsEnabledFunction(
      const EyeoFilteringPrivateIsEnabledFunction&) = delete;
  EyeoFilteringPrivateIsEnabledFunction& operator=(
      const EyeoFilteringPrivateIsEnabledFunction&) = delete;
};

class EyeoFilteringPrivateGetAcceptableAdsUrlFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.getAcceptableAdsUrl",
                             UNKNOWN)
  EyeoFilteringPrivateGetAcceptableAdsUrlFunction();

 private:
  ~EyeoFilteringPrivateGetAcceptableAdsUrlFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  EyeoFilteringPrivateGetAcceptableAdsUrlFunction(
      const EyeoFilteringPrivateGetAcceptableAdsUrlFunction&) = delete;
  EyeoFilteringPrivateGetAcceptableAdsUrlFunction& operator=(
      const EyeoFilteringPrivateGetAcceptableAdsUrlFunction&) = delete;
};

class EyeoFilteringPrivateAddFilterListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.addFilterList", UNKNOWN)
  EyeoFilteringPrivateAddFilterListFunction();

 private:
  ~EyeoFilteringPrivateAddFilterListFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateAddFilterListFunction(
      const EyeoFilteringPrivateAddFilterListFunction&) = delete;
  EyeoFilteringPrivateAddFilterListFunction& operator=(
      const EyeoFilteringPrivateAddFilterListFunction&) = delete;
};

class EyeoFilteringPrivateRemoveFilterListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.removeFilterList", UNKNOWN)
  EyeoFilteringPrivateRemoveFilterListFunction();

 private:
  ~EyeoFilteringPrivateRemoveFilterListFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateRemoveFilterListFunction(
      const EyeoFilteringPrivateRemoveFilterListFunction&) = delete;
  EyeoFilteringPrivateRemoveFilterListFunction& operator=(
      const EyeoFilteringPrivateRemoveFilterListFunction&) = delete;
};

class EyeoFilteringPrivateGetFilterListsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.getFilterLists", UNKNOWN)
  EyeoFilteringPrivateGetFilterListsFunction();

 private:
  ~EyeoFilteringPrivateGetFilterListsFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateGetFilterListsFunction(
      const EyeoFilteringPrivateGetFilterListsFunction&) = delete;
  EyeoFilteringPrivateGetFilterListsFunction& operator=(
      const EyeoFilteringPrivateGetFilterListsFunction&) = delete;
};

class EyeoFilteringPrivateAddAllowedDomainFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.addAllowedDomain", UNKNOWN)
  EyeoFilteringPrivateAddAllowedDomainFunction();

 private:
  ~EyeoFilteringPrivateAddAllowedDomainFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateAddAllowedDomainFunction(
      const EyeoFilteringPrivateAddAllowedDomainFunction&) = delete;
  EyeoFilteringPrivateAddAllowedDomainFunction& operator=(
      const EyeoFilteringPrivateAddAllowedDomainFunction&) = delete;
};

class EyeoFilteringPrivateRemoveAllowedDomainFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.removeAllowedDomain",
                             UNKNOWN)
  EyeoFilteringPrivateRemoveAllowedDomainFunction();

 private:
  ~EyeoFilteringPrivateRemoveAllowedDomainFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateRemoveAllowedDomainFunction(
      const EyeoFilteringPrivateRemoveAllowedDomainFunction&) = delete;
  EyeoFilteringPrivateRemoveAllowedDomainFunction& operator=(
      const EyeoFilteringPrivateRemoveAllowedDomainFunction&) = delete;
};

class EyeoFilteringPrivateGetAllowedDomainsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.getAllowedDomains", UNKNOWN)
  EyeoFilteringPrivateGetAllowedDomainsFunction();

 private:
  ~EyeoFilteringPrivateGetAllowedDomainsFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateGetAllowedDomainsFunction(
      const EyeoFilteringPrivateGetAllowedDomainsFunction&) = delete;
  EyeoFilteringPrivateGetAllowedDomainsFunction& operator=(
      const EyeoFilteringPrivateGetAllowedDomainsFunction&) = delete;
};

class EyeoFilteringPrivateAddCustomFilterFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.addCustomFilter", UNKNOWN)
  EyeoFilteringPrivateAddCustomFilterFunction();

 private:
  ~EyeoFilteringPrivateAddCustomFilterFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateAddCustomFilterFunction(
      const EyeoFilteringPrivateAddCustomFilterFunction&) = delete;
  EyeoFilteringPrivateAddCustomFilterFunction& operator=(
      const EyeoFilteringPrivateAddCustomFilterFunction&) = delete;
};

class EyeoFilteringPrivateRemoveCustomFilterFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.removeCustomFilter", UNKNOWN)
  EyeoFilteringPrivateRemoveCustomFilterFunction();

 private:
  ~EyeoFilteringPrivateRemoveCustomFilterFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateRemoveCustomFilterFunction(
      const EyeoFilteringPrivateRemoveCustomFilterFunction&) = delete;
  EyeoFilteringPrivateRemoveCustomFilterFunction& operator=(
      const EyeoFilteringPrivateRemoveCustomFilterFunction&) = delete;
};

class EyeoFilteringPrivateGetCustomFiltersFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("eyeoFilteringPrivate.getCustomFilters", UNKNOWN)
  EyeoFilteringPrivateGetCustomFiltersFunction();

 private:
  ~EyeoFilteringPrivateGetCustomFiltersFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateGetCustomFiltersFunction(
      const EyeoFilteringPrivateGetCustomFiltersFunction&) = delete;
  EyeoFilteringPrivateGetCustomFiltersFunction& operator=(
      const EyeoFilteringPrivateGetCustomFiltersFunction&) = delete;
};

class EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "eyeoFilteringPrivate.getSessionAllowedRequestsCount",
      UNKNOWN)
  EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction();

 private:
  ~EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction(
      const EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction&) =
      delete;
  EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction& operator=(
      const EyeoFilteringPrivateGetSessionAllowedRequestsCountFunction&) =
      delete;
};

class EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "eyeoFilteringPrivate.getSessionBlockedRequestsCount",
      UNKNOWN)
  EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction();

 private:
  ~EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction() override;

  ResponseAction Run() override;

  EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction(
      const EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction&) =
      delete;
  EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction& operator=(
      const EyeoFilteringPrivateGetSessionBlockedRequestsCountFunction&) =
      delete;
};

}  // namespace api

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_EYEO_FILTERING_PRIVATE_EYEO_FILTERING_PRIVATE_API_H_
