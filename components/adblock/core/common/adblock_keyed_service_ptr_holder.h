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

#ifndef COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_KEYED_SERVICE_PTR_HOLDER_H_
#define COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_KEYED_SERVICE_PTR_HOLDER_H_

#include <memory>
#include <type_traits>

#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"

class KeyedService;

namespace adblock {

// An utility class to store a KeyedService ptr in an object supporting
// base::SupportsUserData interface (typically content::BrowserContext).
// This is used to overcome a dependency problem in a code which needs to
// to access a KeyedService instance where referring to a KeyedServiceFactory
// is not allowed.
template <class KeyedServiceDerived>
struct KeyedServicePtrHolder final : public base::SupportsUserData::Data {
  static_assert(std::is_base_of<KeyedService, KeyedServiceDerived>::value,
                "KeyedServiceDerived must derive from KeyedService.");

  ~KeyedServicePtrHolder() override {}

  static KeyedServiceDerived* Get(base::SupportsUserData* container,
                                  const char* key) {
    auto* ptr_holder =
        static_cast<KeyedServicePtrHolder*>(container->GetUserData(key));
    if (ptr_holder) {
      return ptr_holder->ptr_;
    }
    return nullptr;
  }

  static void Set(base::SupportsUserData* container,
                  const char* key,
                  KeyedServiceDerived* data) {
    container->SetUserData(key, std::unique_ptr<KeyedServicePtrHolder>(
                                    new KeyedServicePtrHolder(data)));
  }

 private:
  explicit KeyedServicePtrHolder(KeyedServiceDerived* ptr) : ptr_(ptr) {}
  raw_ptr<KeyedServiceDerived, DanglingUntriaged> ptr_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_COMMON_ADBLOCK_KEYED_SERVICE_PTR_HOLDER_H_
