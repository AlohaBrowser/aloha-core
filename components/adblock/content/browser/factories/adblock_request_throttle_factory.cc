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

#include "components/adblock/content/browser/factories/adblock_request_throttle_factory.h"

#include <memory>

#include "base/command_line.h"
#include "base/time/time.h"
#include "components/adblock/core/common/adblock_switches.h"
#include "components/adblock/core/net/adblock_request_throttle_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

// static
AdblockRequestThrottle* AdblockRequestThrottleFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<AdblockRequestThrottle*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
AdblockRequestThrottleFactory* AdblockRequestThrottleFactory::GetInstance() {
  static base::NoDestructor<AdblockRequestThrottleFactory> instance;
  return instance.get();
}

// ALOHA https://app.clickup.com/t/86er9wwca
inline static base::TimeDelta g_delay_init{base::Seconds(10)};
void  AdblockRequestThrottleFactory::SetInitialDelay(base::TimeDelta delay) {
  g_delay_init = delay;
}

AdblockRequestThrottleFactory::AdblockRequestThrottleFactory()
    : BrowserContextKeyedServiceFactory(
          "AdblockRequestThrottle",
          BrowserContextDependencyManager::GetInstance()) {}

AdblockRequestThrottleFactory::~AdblockRequestThrottleFactory() = default;

std::unique_ptr<KeyedService>
AdblockRequestThrottleFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto throttle = std::make_unique<AdblockRequestThrottleImpl>();
  const auto initial_delay =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          adblock::switches::kDisableEyeoRequestThrottling)
          ? base::TimeDelta()
          : g_delay_init; // ALOHA https://app.clickup.com/t/86er9a3fz
  throttle->AllowRequestsAfter(initial_delay);
  return std::move(throttle);
}

content::BrowserContext* AdblockRequestThrottleFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

}  // namespace adblock
