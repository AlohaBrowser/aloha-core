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

/** @fileoverview Type definitions for chrome.adblockPrivate API. */

import {ChromeEvent} from './chrome_event.js';

declare global {
  export namespace chrome {
    export namespace adblockPrivate {

      export interface BuiltInSubscription {
        url: string;
        title: string;
        languages: string[];
      }

      export interface Subscription {
        url: string;
        installation_state: string;
        title: string;
        current_version: string;
        last_installation_time: string;
      }

      export interface SessionStatsEntry {
        url: string;
        count: number;
      }

      export interface AdInfo {
        url: string;
        parent_frame_urls: string[];
        subscription: string;
        configuration_name: string;
        content_type: string;
        tab_id: number;
        window_id: number;
      }

      export const onAdAllowed: ChromeEvent<(
        info: AdInfo,
      ) => void>;

      export const onAdBlocked: ChromeEvent<(
        info: AdInfo,
      ) => void>;

      export const onPageAllowed: ChromeEvent<(
        info: AdInfo,
      ) => void>;

      export const onPopupBlocked: ChromeEvent<(
        info: AdInfo,
      ) => void>;

      export const onPopupAllowed: ChromeEvent<(
        info: AdInfo,
      ) => void>;

      export const onSubscriptionUpdated: ChromeEvent<(
        subscription_url: string,
      ) => void>;

      export const onEnabledStateChanged: ChromeEvent<(
      ) => void>;

      export const onFilterListsChanged: ChromeEvent<(
      ) => void>;

      export const onAllowedDomainsChanged: ChromeEvent<(
      ) => void>;

      export const onCustomFiltersChanged: ChromeEvent<(
      ) => void>;

      export function setEnabled(
        enabled: boolean,
      ): void;

      export function isEnabled(
        callback: (
          result: boolean,
        ) => void,
      ): void;

      export function setAcceptableAdsEnabled(
        enabled: boolean,
      ): void;

      export function isAcceptableAdsEnabled(
        callback: (
          result: boolean,
        ) => void,
      ): void;

      export function setAutoInstallEnabled(
        enabled: boolean,
      ): void;

      export function isAutoInstallEnabled(
        callback: (
          result: boolean,
        ) => void,
      ): void;

      export function getBuiltInSubscriptions(
        callback: (
          result: BuiltInSubscription[],
        ) => void,
      ): void;

      export function addAllowedDomain(
        domain: string,
      ): void;

      export function removeAllowedDomain(
        domain: string,
      ): void;

      export function getAllowedDomains(
        callback: (
          result: string[],
        ) => void,
      ): void;

      export function addCustomFilter(
        filter: string,
      ): void;

      export function removeCustomFilter(
        filter: string,
      ): void;

      export function getCustomFilters(
        callback: (
          result: string[],
        ) => void,
      ): void;

      export function getSessionAllowedAdsCount(
        callback: (
          result: SessionStatsEntry[],
        ) => void,
      ): void;

      export function getSessionBlockedAdsCount(
        callback: (
          result: SessionStatsEntry[],
        ) => void,
      ): void;

      export function getInstalledSubscriptions(
        callback: (
          result: Subscription[],
        ) => void,
      ): void;

      export function installSubscription(
        url: string,
        feedback?: () => void,
      ): void;

      export function uninstallSubscription(
        url: string,
        feedback?: () => void,
      ): void;
    }
  }
}
