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

#include "components/adblock/content/browser/factories/snippet_service_factory.h"

#include <memory>

#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "components/adblock/content/browser/factories/subscription_service_factory.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/adblock_prefs.h"
#include "components/adblock/core/common/task_scheduler_impl.h"
#include "components/adblock/core/net/adblock_resource_request_impl.h"
#include "components/adblock/core/snippet/snippet_downloader_impl.h"
#include "components/adblock/core/snippet/snippet_service_impl.h"
#include "components/adblock/core/snippet/snippet_storage_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace adblock {
namespace {

std::optional<base::TimeDelta> g_update_check_interval_for_testing;
std::optional<base::TimeDelta> g_initial_delay_for_testing;

base::TimeDelta GetUpdateCheckInterval() {
  static base::TimeDelta kCheckInterval =
      g_update_check_interval_for_testing
          ? g_update_check_interval_for_testing.value()
          : base::Hours(1);
  return kCheckInterval;
}

base::TimeDelta GetInitialDelay() {
  static base::TimeDelta kInitialDelay =
      g_initial_delay_for_testing ? g_initial_delay_for_testing.value()
                                  : base::Seconds(30);
  return kInitialDelay;
}

std::unique_ptr<TaskScheduler> MakeTaskScheduler() {
  return std::make_unique<TaskSchedulerImpl>(GetUpdateCheckInterval(),
                                             GetInitialDelay());
}
}  // namespace

// static
SnippetService* SnippetServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SnippetService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
SnippetServiceFactory* SnippetServiceFactory::GetInstance() {
  static base::NoDestructor<SnippetServiceFactory> instance;
  return instance.get();
}

// static
void SnippetServiceFactory::SetUpdateCheckIntervalForTesting(
    base::TimeDelta check_interval) {
  g_update_check_interval_for_testing = check_interval;
}

// static
void SnippetServiceFactory::SetInitialDelayForTesting(
    base::TimeDelta initial_delay) {
  g_initial_delay_for_testing = initial_delay;
}

SnippetServiceFactory::SnippetServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockSnippetUpdateService",
          BrowserContextDependencyManager::GetInstance()) {}

SnippetServiceFactory::~SnippetServiceFactory() = default;

std::unique_ptr<KeyedService>
SnippetServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto storage = std::make_unique<SnippetStorageImpl>(
      context->GetPath().AppendASCII("snippets"));
  auto* prefs = user_prefs::UserPrefs::Get(context);
  storage->LoadSnippets(prefs->GetString(common::prefs::kSnippetsSignature));
  auto downloader = std::make_unique<SnippetDownloaderImpl>(
      prefs, MakeTaskScheduler(),
      base::BindRepeating(&SubscriptionServiceFactory::MakeSubscriptionRequest,
                          context),
      SnippetLibraryUrl(), storage.get());

  return std::make_unique<SnippetUpdateServiceImpl>(std::move(storage),
                                                    std::move(downloader));
}

content::BrowserContext* SnippetServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

}  // namespace adblock
