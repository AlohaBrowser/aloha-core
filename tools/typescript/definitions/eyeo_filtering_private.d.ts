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

/** @fileoverview Type definitions for chrome.eyeoFilteringPrivate API. */

import {ChromeEvent} from './chrome_event.js';

declare global {
  export namespace chrome {
    export namespace eyeoFilteringPrivate {

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

      export interface RequestInfo {
        url: string;
        parent_frame_urls: string[];
        subscription: string;
        configuration_name: string;
        content_type: string;
        tab_id: number;
        window_id: number;
      }

      export const onRequestAllowed: ChromeEvent<(
        info: RequestInfo,
      ) => void>;

      export const onRequestBlocked: ChromeEvent<(
        info: RequestInfo,
      ) => void>;

      export const onPageAllowed: ChromeEvent<(
        info: RequestInfo,
      ) => void>;

      export const onPopupBlocked: ChromeEvent<(
        info: RequestInfo,
      ) => void>;

      export const onPopupAllowed: ChromeEvent<(
        info: RequestInfo,
      ) => void>;

      export const onSubscriptionUpdated: ChromeEvent<(
        subscription_url: string,
      ) => void>;

      export const onEnabledStateChanged: ChromeEvent<(
        config_name: string,
      ) => void>;

      export const onFilterListsChanged: ChromeEvent<(
        config_name: string,
      ) => void>;

      export const onAllowedDomainsChanged: ChromeEvent<(
        config_name: string,
      ) => void>;

      export const onCustomFiltersChanged: ChromeEvent<(
        config_name: string,
      ) => void>;

      export function createConfiguration(
        config_name: string,
      ): void;

      export function removeConfiguration(
        config_name: string,
      ): void;

      export function getConfigurations(): Promise<string[]>;

      export function getConfigurations(
        callback?: (
          result: string[],
        ) => void,
      ): void;

      export function setEnabled(
        configuration: string,
        enabled: boolean,
      ): Promise<void>;

      export function setEnabled(
        configuration: string,
        enabled: boolean,
        status?: () => void,
      ): void;

      export function isEnabled(
        configuration: string,
      ): Promise<boolean>;

      export function isEnabled(
        configuration: string,
        callback?: (
          result: boolean,
        ) => void,
      ): void;

      export function setAutoInstallEnabled(
        enabled: boolean,
      ): Promise<void>;

      export function setAutoInstallEnabled(
        enabled: boolean,
        status?: () => void,
      ): void;

      export function isAutoInstallEnabled(): Promise<boolean>;

      export function isAutoInstallEnabled(
        callback?: (
          result: boolean,
        ) => void,
      ): void;

      export function addAllowedDomain(
        configuration: string,
        domain: string,
      ): Promise<void>;

      export function addAllowedDomain(
        configuration: string,
        domain: string,
        status?: () => void,
      ): void;

      export function removeAllowedDomain(
        configuration: string,
        domain: string,
      ): Promise<void>;

      export function removeAllowedDomain(
        configuration: string,
        domain: string,
        status?: () => void,
      ): void;

      export function getAllowedDomains(
        configuration: string,
      ): Promise<string[]>;

      export function getAllowedDomains(
        configuration: string,
        callback?: (
          result: string[],
        ) => void,
      ): void;

      export function addCustomFilter(
        configuration: string,
        filter: string,
      ): Promise<void>;

      export function addCustomFilter(
        configuration: string,
        filter: string,
        status?: () => void,
      ): void;

      export function removeCustomFilter(
        configuration: string,
        filter: string,
      ): Promise<void>;

      export function removeCustomFilter(
        configuration: string,
        filter: string,
        status?: () => void,
      ): void;

      export function getCustomFilters(
        configuration: string,
      ): Promise<string[]>;

      export function getCustomFilters(
        configuration: string,
        callback?: (
          result: string[],
        ) => void,
      ): void;

      export function getFilterLists(
        configuration: string,
      ): Promise<Subscription[]>;

      export function getFilterLists(
        configuration: string,
        callback?: (
          result: Subscription[],
        ) => void,
      ): void;

      export function addFilterList(
        configuration: string,
        url: string,
      ): Promise<void>;

      export function addFilterList(
        configuration: string,
        url: string,
        status?: () => void,
      ): void;

      export function removeFilterList(
        configuration: string,
        url: string,
      ): Promise<void>;

      export function removeFilterList(
        configuration: string,
        url: string,
        status?: () => void,
      ): void;

      export function getSessionAllowedRequestsCount(): Promise<SessionStatsEntry[]>;

      export function getSessionAllowedRequestsCount(
        callback?: (
          result: SessionStatsEntry[],
        ) => void,
      ): void;

      export function getSessionBlockedRequestsCount(): Promise<SessionStatsEntry[]>;

      export function getSessionBlockedRequestsCount(
        callback?: (
          result: SessionStatsEntry[],
        ) => void,
      ): void;

      export function getAcceptableAdsUrl(): Promise<string>;

      export function getAcceptableAdsUrl(
        callback?: (
          result: string,
        ) => void,
      ): void;
    }
  }
}
