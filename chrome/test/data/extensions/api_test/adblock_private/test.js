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

'use strict';

const urlParams = new URLSearchParams(window.location.search);

// This class binds 1st argument for chrome.eyeoFilteringPrivate to 'adblock'
// configuration and redirects methods renamed in chrome.eyeoFilteringPrivate
// (eg. `onAdAllowed` => `onRequestAllowed`) which allows test code to
// seamlessly call the same methods on chrome.adblockPrivate and on
// chrome.eyeoFilteringPrivate.
class FilteringPrivateBoundWithAdblock {
  constructor() {
    this.delegate = chrome.eyeoFilteringPrivate;
    this.configuration = 'adblock';
    this.getSessionAllowedAdsCount =
        this.delegate.getSessionAllowedRequestsCount;
    this.getSessionBlockedAdsCount =
        this.delegate.getSessionBlockedRequestsCount;
    this.onAdAllowed = this.delegate.onRequestAllowed;
    this.onAdBlocked = this.delegate.onRequestBlocked;
    this.onAllowedDomainsChanged = this.delegate.onAllowedDomainsChanged;
    this.onCustomFiltersChanged = this.delegate.onCustomFiltersChanged;
    this.onEnabledStateChanged = this.delegate.onEnabledStateChanged;
    this.onFilterListsChanged = this.delegate.onFilterListsChanged;
    this.onSubscriptionUpdated = this.delegate.onSubscriptionUpdated;
    const methodsToBind = [
      'isEnabled', 'setEnabled', 'getAllowedDomains', 'addAllowedDomain',
      'removeAllowedDomain', 'getCustomFilters', 'addCustomFilter',
      'removeCustomFilter'
    ];
    for (const method of methodsToBind) {
      this[method] = function() {
        const args = Array.from(arguments);
        args.unshift(this.configuration);
        this.delegate[method].apply(this.delegate, args);
      }
    }
    const methodsToBindWithoutConfigurationParam = [
      'isAutoInstallEnabled', 'setAutoInstallEnabled'
    ];
    for (const method of methodsToBindWithoutConfigurationParam) {
      this[method] = function() {
        const args = Array.from(arguments);
        this.delegate[method].apply(this.delegate, args);
      }
    }
    const methodsToBindRenamed = new Map([
      ['installSubscription', 'addFilterList'],
      ['uninstallSubscription', 'removeFilterList'],
      ['getInstalledSubscriptions', 'getFilterLists'],
    ]);
    methodsToBindRenamed.forEach((value, key) => {
      this[key] =
          function() {
            const args = Array.from(arguments);
            args.unshift(this.configuration);
            this.delegate[value].apply(this.delegate, args);
          }
    });
  }
};

// Set API object for tests, defaults to chrome.adblockPrivate
let apiObject = chrome.adblockPrivate;
if (urlParams.get('api') === 'eyeoFilteringPrivate') {
  apiObject = new FilteringPrivateBoundWithAdblock;
}

function findEnglishEasyList(item) {
  console.log(item.title + ' ' + item.url + ' ' + item.languages);
  return item.title.toLowerCase().includes('easylist') &&
      item.url.toLowerCase().includes('easylist') &&
      item.languages.includes('en');
}

function containsSubscription(subscriptions, url) {
  for (const subscription of subscriptions) {
    if (subscription.url === url) {
      return true;
    }
  }
  return false;
}

// Checks that session stats data contains expected filter list url entry with a
// single hit
function verifySessionStats(sessionStats) {
  let index = -1;
  for (let i = 0; i < sessionStats.length; i++) {
    if (sessionStats[i].url === 'adblock:custom') {
      index = i;
      break;
    }
  }
  if (index == -1) {
    chrome.test.fail('Failed: Missing entry in sessionStats');
    return;
  }
  if (sessionStats[index].count != 1) {
    chrome.test.fail(
        'Failed: Expected a single subscription hit number, got: ' +
        sessionStats[index].count);
  }
}

// Checks that even contains no error and expected filter list url
function verifyEventData(event) {
  if (event.tab_id < 1) {
    chrome.test.fail('Failed: Wrong tab_id value');
  }
  if (event.window_id < 1) {
    chrome.test.fail('Failed: Wrong window_id value');
  }
  if (event.content_type !== 'XMLHTTPREQUEST') {
    chrome.test.fail('Failed: Wrong content_type value: ' + event.content_type);
  }
  if (event.subscription !== 'adblock:custom') {
    chrome.test.fail('Failed: Wrong subscription value: ' + event.subscription);
  }
}

function arrayEquals(a, b) {
  if (a === b)
    return true;
  if (a === null || b === null)
    return false;
  if (a.length !== b.length)
    return false;
  for (var i = 0; i < a.length; i++) {
    if (a[i] !== b[i])
      return false;
  }
  return true;
};

const availableTests = [
  function setEnabled_isEnabled() {
    apiObject.setEnabled(false);
    apiObject.isEnabled(function(enabled) {
      if (enabled)
        chrome.test.fail('Failed: ad blocking should NOT be enabled');
    });
    apiObject.setEnabled(true);
    apiObject.isEnabled(function(enabled) {
      if (enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: ad blocking should be enabled');
    });
  },
  function setAAEnabled_isAAEnabled() {
    chrome.adblockPrivate.setAcceptableAdsEnabled(false);
    chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
      if (enabled)
        chrome.test.fail('Failed: AA should NOT be enabled');
    });
    chrome.adblockPrivate.setAcceptableAdsEnabled(true);
    chrome.adblockPrivate.getInstalledSubscriptions(function(selected) {
      if (!containsSubscription(
              selected,
              'https://easylist-downloads.adblockplus.org/exceptionrules.txt')) {
        chrome.test.fail('Failed: AA subscription should be on the list');
      }
    });
    chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
      if (enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: AA should be enabled');
    });
  },
  function setAAEnabled_isAAEnabled_newAPI() {
    const default_config = 'adblock';
    chrome.eyeoFilteringPrivate.getAcceptableAdsUrl(function(aa_url) {
      chrome.eyeoFilteringPrivate.removeFilterList(
          default_config, aa_url);
      chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
        if (enabled)
          chrome.test.fail('Failed: AA should NOT be enabled');
      });
      chrome.eyeoFilteringPrivate.getAcceptableAdsUrl(function(aa_url) {
        chrome.eyeoFilteringPrivate.addFilterList(
            default_config, aa_url);
        chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
          if (enabled)
            chrome.test.succeed();
          else
            chrome.test.fail('Failed: AA should be enabled');
        });
      });
    });
  },
  function getBuiltInSubscriptions() {
    chrome.adblockPrivate.getBuiltInSubscriptions(function(recommended) {
      const found = recommended.find(findEnglishEasyList);
      if (found) {
        chrome.test.succeed();
      } else {
        chrome.test.fail('Failed: invalid built-in subscriptions');
      }
    });
  },
  // This test works because at the beginning getInstalledSubscriptions returns
  // just default entries from recommended subscriptions.
  function installedSubscriptionsDataSchema() {
    const disabled = !!urlParams.get('disabled');
    if (disabled) {
      apiObject.setEnabled(false);
    }
    apiObject.getInstalledSubscriptions(function(installed) {
      for (const subscription of installed) {
        if (!subscription.url)
          chrome.test.fail('Failed: Must contain "url" property');
        if (!(subscription.installation_state == 'Unknown' && disabled ||
              subscription.installation_state != 'Unknown' && !disabled))
          chrome.test.fail(
              'Failed: Must contain valid "installation_state" property');
        if (disabled)
          continue;
        const pathname = new URL(subscription.url).pathname;
        if (subscription.title != pathname)
          chrome.test.fail('Failed: Must contain expected "title" property');
        if (subscription.current_version != pathname.length)
          chrome.test.fail('Failed: Must contain expected "current_version" property');
        if (!subscription.last_installation_time)
          chrome.test.fail(
              'Failed: Installed subscription must contain "last_installation_time" property');
      }
      chrome.test.succeed();
    });
  },
  function installSubscriptionInvalidURL() {
    apiObject.installSubscription('http://', function() {
      if (!chrome.runtime.lastError)
        chrome.test.fail('Failed: invalid input accepted');
      else
        chrome.test.succeed();
    });
  },
  function uninstallSubscriptionInvalidURL() {
    apiObject.uninstallSubscription('http://', function() {
      if (!chrome.runtime.lastError)
        chrome.test.fail('Failed: invalid input accepted');
      else
        chrome.test.succeed();
    });
  },
  function subscriptionsManagement() {
    const kEasylist = urlParams.get('easylist');
    const kExceptionrules = urlParams.get('exceptions');
    const kABPFilters = urlParams.get('snippets');
    const kCustom = 'https://example.com/subscription.txt';

    function containsDefaultSubscriptions(subscriptions) {
      return containsSubscription(subscriptions, kEasylist) &&
          containsSubscription(subscriptions, kExceptionrules) &&
          containsSubscription(subscriptions, kABPFilters);
    }

    if (urlParams.get('disabled')) {
      apiObject.setEnabled(false);
    }

    apiObject.getInstalledSubscriptions(function(installed) {
      if (installed.length) {
        if (!containsDefaultSubscriptions(installed)) {
          chrome.test.fail('Failed: Should contain all default subscriptions');
        }
        for (const subscription of installed) {
          apiObject.uninstallSubscription(subscription.url);
        }
      } else {
        chrome.test.fail('Failed: Should contain default subscriptions');
      }
      apiObject.getInstalledSubscriptions(function(installed) {
        if (installed.length) {
          chrome.test.fail(
              'Failed: There shouldn\'t be any installed subscriptions');
        }
        apiObject.installSubscription(kEasylist);
        apiObject.installSubscription(kExceptionrules);
        apiObject.installSubscription(kABPFilters);
        apiObject.getInstalledSubscriptions(function(installed) {
          if (installed.length) {
            if (!containsDefaultSubscriptions(installed)) {
              chrome.test.fail(
                  'Failed: Should contain all default subscriptions');
            }
          } else {
            chrome.test.fail('Failed: Should contain default subscriptions');
          }
          apiObject.installSubscription(kCustom);
          apiObject.getInstalledSubscriptions(function(installed) {
            if (!containsSubscription(installed, kCustom)) {
              chrome.test.fail('Failed: Should contain custom subscription');
            }
            apiObject.uninstallSubscription(kCustom);
            apiObject.getInstalledSubscriptions(function(installed) {
              if (containsSubscription(installed, kCustom)) {
                chrome.test.fail(
                    'Failed: Should not contain custom subscription');
              } else {
                chrome.test.succeed();
              }
            });
          });
        });
      });
    });
  },
  function allowedDomainsManagement() {
    apiObject.getAllowedDomains(function(domains) {
      if (domains.length) {
        for (const domain of domains)
          apiObject.removeAllowedDomain(domain);
      }

      apiObject.getAllowedDomains(function(domains) {
        if (domains.length) {
          chrome.test.fail('Failed: There shouldn\'t be any allowed domains');
          return;
        }

        apiObject.addAllowedDomain('foo.bar');
        apiObject.addAllowedDomain('bar.baz');
        apiObject.getAllowedDomains(function(domains) {
          if (domains.length != 2) {
            chrome.test.fail('Failed: There should be 2 allowed domains');
            return;
          }

          if (domains.indexOf('foo.bar') == -1 ||
              domains.indexOf('bar.baz') == -1) {
            chrome.test.fail('Failed: Didn\'t find expected allowed domains');
            return;
          }

          apiObject.removeAllowedDomain('foo.bar');
          apiObject.removeAllowedDomain('bar.baz');

          apiObject.getAllowedDomains(function(domains) {
            if (domains.length)
              chrome.test.fail('Failed: Still have allowed domains');
            else
              chrome.test.succeed();
          });
        });
      });
    });
  },
  function customFiltersManagement() {
    apiObject.getCustomFilters(function(filters) {
      if (filters.length) {
        for (const filter of filters)
          apiObject.removeCustomFilter(filter);
      }

      apiObject.getCustomFilters(function(filters) {
        if (filters.length) {
          chrome.test.fail('Failed: There shouldn\'t be any custom filters');
          return;
        }

        apiObject.addCustomFilter('foo.bar');
        apiObject.addCustomFilter('bar.baz');
        apiObject.getCustomFilters(function(filters) {
          if (filters.length != 2) {
            chrome.test.fail('Failed: There should be 2 custom filters');
            return;
          }

          if (filters.indexOf('foo.bar') == -1 ||
              filters.indexOf('bar.baz') == -1) {
            chrome.test.fail('Failed: Didn\'t find expected custom filters');
            return;
          }

          apiObject.removeCustomFilter('foo.bar');
          apiObject.removeCustomFilter('bar.baz');

          apiObject.getCustomFilters(function(filters) {
            if (filters.length)
              chrome.test.fail('Failed: Still have custom filters');
            else
              chrome.test.succeed();
          });
        });
      });
    });
  },
  function adBlockedEvents() {
    let expected_filters_count = 1;
    apiObject.onCustomFiltersChanged.addListener(function(filter) {
      if (--expected_filters_count)
        return;

      apiObject.onAdBlocked.addListener(function(e) {
        verifyEventData(e);
        if (e.url.includes('test1.png')) {
          chrome.test.succeed();
        }
      });
      // External request to delay triggering test request
      const delaying_wrapper_request = new XMLHttpRequest();
      const delaying_wrapper_request_handler = function() {
        // Internal request expected by test logic
        const blocked_url_request = new XMLHttpRequest();
        const blocked_url_request_handler = function() {};
        blocked_url_request.onload = blocked_url_request_handler;
        blocked_url_request.onerror = blocked_url_request_handler;
        blocked_url_request.open('GET', 'https://example.com/test1.png', true);
        blocked_url_request.send();
      };
      delaying_wrapper_request.onload = delaying_wrapper_request_handler;
      delaying_wrapper_request.onerror = delaying_wrapper_request_handler;
      delaying_wrapper_request.open('GET', 'https://example.com/', true);
      delaying_wrapper_request.send();
    });
    apiObject.addCustomFilter('test1.png');
  },
  function adAllowedEvents() {
    let expected_filters_count = 2;
    apiObject.onCustomFiltersChanged.addListener(function(filter) {
      if (--expected_filters_count)
        return;

      apiObject.onAdAllowed.addListener(function(e) {
        verifyEventData(e);
        if (e.url.includes('test2.png')) {
          chrome.test.succeed();
        }
      });
      // External request to delay triggering test request
      const delaying_wrapper_request = new XMLHttpRequest();
      const delaying_wrapper_request_handler = function() {
        // Internal request expected by test logic
        const allowed_url_request = new XMLHttpRequest();
        const allowed_url_request_handler = function() {};
        allowed_url_request.onload = allowed_url_request_handler;
        allowed_url_request.onerror = allowed_url_request_handler;
        allowed_url_request.open('GET', 'https://example.com/test2.png', true);
        allowed_url_request.send();
      };
      delaying_wrapper_request.onload = delaying_wrapper_request_handler;
      delaying_wrapper_request.onerror = delaying_wrapper_request_handler;
      delaying_wrapper_request.open('GET', 'https://example.com/', true);
      delaying_wrapper_request.send();
    });
    apiObject.addCustomFilter('test2.png');
    apiObject.addCustomFilter('@@test2.png');
  },
  function sessionStats() {
    let expected_filters_count = 3;
    apiObject.onCustomFiltersChanged.addListener(function(filter) {
      if (--expected_filters_count)
        return;

      const blocked_request_handler = function() {
        apiObject.getSessionBlockedAdsCount(function(sessionStats) {
          verifySessionStats(sessionStats);
          const allowed_request_handler = function() {
            apiObject.getSessionAllowedAdsCount(function(sessionStats) {
              verifySessionStats(sessionStats);
              chrome.test.succeed();
            });
          };
          const request = new XMLHttpRequest();
          request.onload = allowed_request_handler;
          request.onerror = allowed_request_handler;
          request.open('GET', 'https://example.com/test4.png', true);
          request.send();
        });
      };
      const request = new XMLHttpRequest();
      request.onload = blocked_request_handler;
      request.onerror = blocked_request_handler;
      request.open('GET', 'https://example.com/test3.png', true);
      request.send();
    });
    apiObject.addCustomFilter('test3.png');
    apiObject.addCustomFilter('test4.png');
    apiObject.addCustomFilter('@@test4.png');
  },
  function allowedDomainsEvent() {
    const domain = 'domain.com';
    let data = [domain];
    let attempts = 2;
    apiObject.onAllowedDomainsChanged.addListener(function() {
      apiObject.getAllowedDomains(function(domains) {
        if (!arrayEquals(data, domains)) {
          chrome.test.fail('Unexpected domain list');
        }
        if (--attempts == 0) {
          chrome.test.succeed();
        }
      });
    });
    apiObject.addAllowedDomain(domain);
    data = [];
    apiObject.removeAllowedDomain(domain);
  },
  function enabledStateEvent() {
    let state = false;
    let attempts = 2;
    apiObject.onEnabledStateChanged.addListener(function() {
      apiObject.isEnabled(function(enabled) {
        if (enabled !== state) {
          chrome.test.fail('Unexpected enabled state');
        }
        if (--attempts == 0) {
          chrome.test.succeed();
        }
      });
    });
    apiObject.setEnabled(false);
    state = true;
    apiObject.setEnabled(true);
  },
  function filterListsEvent() {
    const kCustom = 'https://example.com/subscription.txt';
    let data = [kCustom];
    let attempts = 2;
    apiObject.onFilterListsChanged.addListener(function() {
      apiObject.getInstalledSubscriptions(function(custom) {
        if (!(data.length + 3, custom.length)) {
          chrome.test.fail('Unexpected subscription list');
        }
        if (--attempts == 0) {
          chrome.test.succeed();
        }
        data = [];
        apiObject.uninstallSubscription(kCustom);
      });
    });
    apiObject.installSubscription(kCustom);
  },
  function customFiltersEvent() {
    const filter = 'foo.bar';
    let data = [filter];
    let attempts = 2;
    apiObject.onCustomFiltersChanged.addListener(function() {
      apiObject.getCustomFilters(function(filters) {
        if (!arrayEquals(data, filters)) {
          chrome.test.fail('Unexpected custom filter list');
        }
        if (--attempts == 0) {
          chrome.test.succeed();
        }
      });
    });
    apiObject.addCustomFilter(filter);
    data = [];
    apiObject.removeCustomFilter(filter);
  },
  function disableGeolocation() {
    const geoLocatedList1 = urlParams.get('geolocated_list_1');
    const geoLocatedList2 = urlParams.get('geolocated_list_2');
    if (!geoLocatedList1 || !geoLocatedList2) {
      chrome.test.fail('Failed: missing expected filter list param(s)');
    }
    apiObject.isAutoInstallEnabled(function(enabled) {
      if (!enabled) {
        chrome.test.fail('Failed: auto installed FL feature should be enabled by default');
      }
      apiObject.getInstalledSubscriptions(function(installed) {
        if (!containsSubscription(installed, geoLocatedList1)
            || !containsSubscription(installed, geoLocatedList2)) {
          chrome.test.fail('Failed: Should contain expected auto installed filter lists');
        }
        // Make geoLocatedList1 not auto-installed
        apiObject.uninstallSubscription(geoLocatedList1);
        apiObject.installSubscription(geoLocatedList1);
        apiObject.setAutoInstallEnabled(false);
        apiObject.isAutoInstallEnabled(function(enabled) {
          if (enabled) {
            chrome.test.fail('Failed: auto installed FL feature should be disabled');
          }
          apiObject.getInstalledSubscriptions(function(installed) {
            if (!containsSubscription(installed, geoLocatedList1)) {
              chrome.test.fail('Failed: Should contain manually installed filter list');
            }
            if (containsSubscription(installed, geoLocatedList2)) {
              chrome.test.fail('Failed: Should NOT contain auto installed filter list');
            }
            chrome.test.succeed();
          });
        });
      });
    });
  },
];

chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == urlParams.get('subtest');
}));
