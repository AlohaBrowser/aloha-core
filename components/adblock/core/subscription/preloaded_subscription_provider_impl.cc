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

#include "components/adblock/core/subscription/preloaded_subscription_provider_impl.h"

#include <string_view>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/pattern.h"
#include "base/trace_event/trace_event.h"
#include "components/adblock/core/common/adblock_utils.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/subscription.h"
#include "components/adblock/core/subscription/subscription_config.h"

namespace adblock {
namespace {

bool HasSubscriptionWithMatchingUrl(const std::vector<GURL>& collection,
                                    std::string_view pattern) {
  return std::find_if(collection.begin(), collection.end(),
                      [pattern](const GURL& url) {
                        return base::MatchPattern(url.spec(), pattern);
                      }) != collection.end();
}

}  // namespace

class PreloadedSubscriptionProviderImpl::SingleSubscriptionProvider {
 public:
  explicit SingleSubscriptionProvider(PreloadedSubscriptionInfo info)
      : info_(info) {}

  void UpdatePreloadedSubscription(
      const std::vector<GURL>& installed_subscriptions,
      const std::vector<GURL>& pending_subscriptions) {
    const bool needs_subscription =
        HasSubscriptionWithMatchingUrl(pending_subscriptions,
                                       info_.url_pattern) &&
        !HasSubscriptionWithMatchingUrl(installed_subscriptions,
                                        info_.url_pattern);
    if (needs_subscription && !subscription_) {
      TRACE_EVENT1("eyeo", "Creating preloaded subscription", "url_pattern",
                   info_.url_pattern);
      subscription_ = base::MakeRefCounted<InstalledSubscriptionImpl>(
          utils::MakeFlatbufferDataFromResourceBundle(
              info_.flatbuffer_resource_id),
          Subscription::InstallationState::Preloaded, base::Time());
      VLOG(1) << "[eyeo] Preloaded subscription now in use: "
              << subscription_->GetSourceUrl();
    } else if (!needs_subscription && subscription_) {
      VLOG(1) << "[eyeo] Preloaded subscription no longer in use: "
              << subscription_->GetSourceUrl();
      subscription_.reset();
    }
  }

  scoped_refptr<InstalledSubscription> subscription() const {
    return subscription_;
  }

  void Reset() { subscription_.reset(); }

 private:
  PreloadedSubscriptionInfo info_;
  scoped_refptr<InstalledSubscription> subscription_;
};

PreloadedSubscriptionProviderImpl::~PreloadedSubscriptionProviderImpl() =
    default;
PreloadedSubscriptionProviderImpl::PreloadedSubscriptionProviderImpl() {
  for (const auto& info : config::GetPreloadedSubscriptionConfiguration()) {
    providers_.emplace_back(info);
  }
}

void PreloadedSubscriptionProviderImpl::UpdateSubscriptions(
    std::vector<GURL> installed_subscriptions,
    std::vector<GURL> pending_subscriptions) {
  installed_subscriptions_ = std::move(installed_subscriptions);
  pending_subscriptions_ = std::move(pending_subscriptions);
  UpdateSubscriptionsInternal();
}

std::vector<scoped_refptr<InstalledSubscription>>
PreloadedSubscriptionProviderImpl::GetCurrentPreloadedSubscriptions() const {
  std::vector<scoped_refptr<InstalledSubscription>> result;
  for (const auto& provider : providers_) {
    auto sub = provider.subscription();
    if (sub) {
      result.push_back(sub);
    }
  }
  return result;
}

void PreloadedSubscriptionProviderImpl::UpdateSubscriptionsInternal() {
  for (auto& provider : providers_) {
    provider.UpdatePreloadedSubscription(installed_subscriptions_,
                                         pending_subscriptions_);
  }
}

}  // namespace adblock
