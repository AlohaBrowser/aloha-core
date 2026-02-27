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

const custom_config = 'custom';

async function pollUntil(predicate, pollEveryMs) {
  return new Promise(r => {
    const id = setInterval(() => {
      let ret;
      if (ret = predicate()) {
        clearInterval(id);
        r(ret);
      }
    }, pollEveryMs);
  });
}

function containsSubscription(subscriptions, url) {
  for (const subscription of subscriptions) {
    if (subscription.url === url) {
      return true;
    }
  }
  return false;
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
  function createRemoveAndGetConfigurations() {
    chrome.eyeoFilteringPrivate.getConfigurations(function(configs) {
      if (configs.includes(custom_config)) {
        chrome.test.fail('Failed: There should NOT be a custom configuration');
      }
      chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
      chrome.eyeoFilteringPrivate.getConfigurations(function(configs) {
        if (!configs.includes(custom_config)) {
          chrome.test.fail('Failed: There should be a custom configuration');
        }
        chrome.eyeoFilteringPrivate.removeConfiguration(custom_config);
        chrome.eyeoFilteringPrivate.getConfigurations(function(configs) {
          if (configs.includes(custom_config)) {
            chrome.test.fail('Failed: There should NOT be a custom configuration');
          }
          chrome.test.succeed();
        });
      });
    });
  },
  async function createRemoveAndGetConfigurationsWithPromises() {
    let configs = await chrome.eyeoFilteringPrivate.getConfigurations();
    if (configs.includes(custom_config)) {
      chrome.test.fail('Failed: There should NOT be a custom configuration');
    }
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    configs = await chrome.eyeoFilteringPrivate.getConfigurations();
    if (!configs.includes(custom_config)) {
      chrome.test.fail('Failed: There should be a custom configuration');
    }
    chrome.eyeoFilteringPrivate.removeConfiguration(custom_config);
    configs = await chrome.eyeoFilteringPrivate.getConfigurations();
    if (configs.includes(custom_config)) {
      chrome.test.fail('Failed: There should NOT be a custom configuration');
    } else {
      chrome.test.succeed();
    }
  },
  function enableAndDisableConfiguration() {
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.isEnabled(custom_config, function(enabled) {
      if (!enabled) {
        chrome.test.fail('Failed: Configuration should be enabled');
      }
      chrome.eyeoFilteringPrivate.setEnabled(custom_config, false);
      chrome.eyeoFilteringPrivate.isEnabled(custom_config, function(enabled) {
        if (enabled) {
          chrome.test.fail('Failed: Configuration should NOT be enabled');
        }
        chrome.eyeoFilteringPrivate.setEnabled(custom_config, true);
        chrome.eyeoFilteringPrivate.isEnabled(custom_config, function(enabled) {
          if (!enabled) {
            chrome.test.fail('Failed: Configuration should be enabled');
          }
          chrome.test.succeed();
        });
      });
    });
  },
  function enableAndDisableConfigurationWithPromises() {
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.isEnabled(custom_config)
      .then(enabled => {
        if (!enabled) {
          throw 'Failed: Configuration should be enabled';
        }
        chrome.eyeoFilteringPrivate.setEnabled(custom_config, false);
      })
      .then(() => chrome.eyeoFilteringPrivate.isEnabled(custom_config))
      .then(enabled => {
        if (enabled) {
          throw 'Failed: Configuration should NOT be enabled';
        }
        chrome.eyeoFilteringPrivate.setEnabled(custom_config, true);
      })
      .then(() => chrome.eyeoFilteringPrivate.isEnabled(custom_config))
      .then(enabled => {
        if (!enabled) {
          throw 'Failed: Configuration should be enabled';
        }
        chrome.test.succeed();
      })
      .catch(message => chrome.test.fail(message))
  },
  function addAllowedDomainToCustomConfiguration() {
    const domain = 'domain.com';
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addAllowedDomain(custom_config, domain);
    chrome.eyeoFilteringPrivate.getAllowedDomains(
        custom_config, function(domains) {
      if (domains.length != 1 || domains.indexOf(domain) == -1) {
        chrome.test.fail('Failed: There should be a custom domain');
      }
      chrome.eyeoFilteringPrivate.removeAllowedDomain(custom_config, domain);
      chrome.eyeoFilteringPrivate.getAllowedDomains(
          custom_config, function(domains) {
        if (domains.length) {
          chrome.test.fail('Failed: Still have custom domain(s)');
        }
        chrome.test.succeed();
      });
    });
  },
  function addAllowedDomainToCustomConfigurationWithPromises() {
    const domain = 'domain.com';
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addAllowedDomain(custom_config, domain)
      .then(() => chrome.eyeoFilteringPrivate.getAllowedDomains(custom_config))
      .then(domains => {
        if (domains.length != 1 || domains.indexOf(domain) == -1) {
          throw 'Failed: There should be a custom domain';
        }
        chrome.eyeoFilteringPrivate.removeAllowedDomain(custom_config, domain);
      })
      .then(() => chrome.eyeoFilteringPrivate.getAllowedDomains(custom_config))
      .then(domains => {
        if (domains.length) {
          throw 'Failed: Still have custom domain(s)';
        }
        chrome.test.succeed();
      })
      .catch(message => chrome.test.fail(message))
  },
  function addCustomFilterToCustomConfiguration() {
    const filter = '||foo.bar';
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addCustomFilter(custom_config, filter);
    chrome.eyeoFilteringPrivate.getCustomFilters(
        custom_config, function(filters) {
      if (filters.length != 1 || filters.indexOf(filter) == -1) {
        chrome.test.fail('Failed: There should be a custom filter');
      }
      chrome.eyeoFilteringPrivate.removeCustomFilter(custom_config, filter);
      chrome.eyeoFilteringPrivate.getCustomFilters(
          custom_config, function(filters) {
        if (filters.length) {
          chrome.test.fail('Failed: Still have custom filter(s)');
        }
        chrome.test.succeed();
      });
    });
  },
  function addCustomFilterToCustomConfigurationWithPromises() {
    const filter = '||foo.bar';
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addCustomFilter(custom_config, filter)
      .then(() => chrome.eyeoFilteringPrivate.getCustomFilters(custom_config))
      .then(filters => {
        if (filters.length != 1 || filters.indexOf(filter) == -1) {
          throw 'Failed: There should be a custom filter';
        }
        chrome.eyeoFilteringPrivate.removeCustomFilter(custom_config, filter);
      })
      .then(() => chrome.eyeoFilteringPrivate.getCustomFilters(custom_config))
      .then(filters => {
        if (filters.length) {
          throw 'Failed: Still have custom filter(s)';
        }
        chrome.test.succeed();
      })
      .catch(message => chrome.test.fail(message));
  },
  function addFilterListInCustomConfiguration() {
    const subscription = 'https://example.com/subscription.txt';
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addFilterList(custom_config, subscription);
    chrome.eyeoFilteringPrivate.getFilterLists(
        custom_config, function(subscriptions) {
      if (subscriptions.length != 1 || !containsSubscription(subscriptions, subscription)) {
        chrome.test.fail('Failed: There should be a single custom subscription');
      }
      chrome.eyeoFilteringPrivate.removeFilterList(custom_config, subscription);
      chrome.eyeoFilteringPrivate.getFilterLists(
          custom_config, function(subscriptions) {
        if (subscriptions.length) {
          chrome.test.fail('Failed: Still have custom subscription(s)');
        }
        chrome.test.succeed();
      });
    });
  },
  function addFilterListInCustomConfigurationWithPromises() {
    const subscription = 'https://example.com/subscription.txt';
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addFilterList(custom_config, subscription)
      .then(() => chrome.eyeoFilteringPrivate.getFilterLists(custom_config))
      .then(subscriptions => {
        if (subscriptions.length != 1 || !containsSubscription(subscriptions, subscription)) {
          throw 'Failed: There should be a single custom subscription';
        }
        chrome.eyeoFilteringPrivate.removeFilterList(custom_config, subscription);
      })
      .then(() => chrome.eyeoFilteringPrivate.getFilterLists(custom_config))
      .then(subscriptions => {
        if (subscriptions.length) {
          throw 'Failed: Still have custom subscription(s)';
        }
        chrome.test.succeed();
      })
      .catch(message => chrome.test.fail(message));
  },
  async function missingConfiguration() {
    const input = 'https://dummy.com';
    const expectedError = 'Configuration with name \'custom\' does not exist!';
    const setters = [
      'addFilterList', 'removeFilterList', 'addAllowedDomain',
      'removeAllowedDomain', 'addCustomFilter', 'removeCustomFilter'
    ];
    const getters = [
      'isEnabled', 'getFilterLists', 'getAllowedDomains', 'getCustomFilters'
    ];
    const allMethodsCount = 1 + setters.length + getters.length;
    let counter = 0;
    chrome.eyeoFilteringPrivate.setEnabled(custom_config, false, function() {
      if (!chrome.runtime.lastError) {
        chrome.test.fail('Failed: missing configuration accepted');
      }
      chrome.test.assertEq(expectedError, chrome.runtime.lastError.message);
      ++counter;
    });
    for (const method of setters) {
      chrome.eyeoFilteringPrivate[method](custom_config, input, function() {
        if (!chrome.runtime.lastError) {
          chrome.test.fail('Failed: missing configuration accepted');
        }
        chrome.test.assertEq(expectedError, chrome.runtime.lastError.message);
        ++counter;
      });
    }
    for (const method of getters) {
      chrome.eyeoFilteringPrivate[method](custom_config, function(result) {
        if (!chrome.runtime.lastError) {
          chrome.test.fail('Failed: missing configuration accepted');
        }
        chrome.test.assertEq(expectedError, chrome.runtime.lastError.message);
        ++counter;
      });
    }
    await pollUntil(() => counter === allMethodsCount, 100);
    chrome.test.succeed();
  },
  async function missingConfigurationWithPromises() {
    const input = 'https://dummy.com';
    const expectedError =
        'Error: Configuration with name \'custom\' does not exist!'
    const setters = [
      'addFilterList', 'removeFilterList', 'addAllowedDomain',
      'removeAllowedDomain', 'addCustomFilter', 'removeCustomFilter'
    ];
    const getters = [
      'isEnabled', 'getFilterLists', 'getAllowedDomains', 'getCustomFilters'
    ];
    const allMethodsCount = 1 + setters.length + getters.length;
    let counter = 0;
    const errorHandler = function(error) {
      chrome.test.assertEq(expectedError, error.toString());
      ++counter;
    };
    await chrome.eyeoFilteringPrivate.setEnabled(custom_config, false)
        .catch(error => errorHandler(error));
    for (const method of setters) {
      await chrome.eyeoFilteringPrivate[method](custom_config, input)
          .catch(error => errorHandler(error));
    }
    for (const method of getters) {
      await chrome.eyeoFilteringPrivate[method](custom_config)
          .catch(error => errorHandler(error));
    }
    if (counter == allMethodsCount) {
      chrome.test.succeed();
    } else {
      chrome.test.fail('Failed: expected missing configuration for every call');
    }
  },
  function allowedDomainsEvent() {
    const domain = 'domain.com';
    let data = [domain];
    let attempts = 2;
    chrome.eyeoFilteringPrivate.onAllowedDomainsChanged.addListener(function(
        config_name) {
      if (config_name != custom_config) {
        chrome.test.fail('Failed: Wrong config name');
      }
      chrome.eyeoFilteringPrivate.getAllowedDomains(
          custom_config, function(domains) {
            if (!arrayEquals(data, domains)) {
              chrome.test.fail('Unexpected domain list');
            }
            if (--attempts == 0) {
              chrome.test.succeed();
            }
          });
    });
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addAllowedDomain(custom_config, domain);
    data = [];
    chrome.eyeoFilteringPrivate.removeAllowedDomain(custom_config, domain);
  },
  function enabledStateEvent() {
    let state = false;
    let attempts = 2;
    chrome.eyeoFilteringPrivate.onEnabledStateChanged.addListener(function(
        config_name) {
      if (config_name != custom_config) {
        chrome.test.fail('Failed: Wrong config name');
      }
      chrome.eyeoFilteringPrivate.isEnabled(custom_config, function(enabled) {
        if (enabled !== state) {
          chrome.test.fail('Unexpected enabled state');
        }
        if (--attempts == 0) {
          chrome.test.succeed();
        }
      });
    });
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.setEnabled(custom_config, false);
    state = true;
    chrome.eyeoFilteringPrivate.setEnabled(custom_config, true);
  },
  function filterListsEvent() {
    const list = 'http://example.com/list.txt';
    let data = [list];
    let attempts = 2;
    chrome.eyeoFilteringPrivate.onFilterListsChanged.addListener(function(
        config_name) {
      if (config_name != custom_config) {
        chrome.test.fail('Failed: Wrong config name');
      }
      chrome.eyeoFilteringPrivate.getFilterLists(
          custom_config, function(custom) {
            if (!arrayEquals(data, custom)) {
              chrome.test.fail('Unexpected subscription list');
            }
            if (--attempts == 0) {
              chrome.test.succeed();
            }
          });
    });
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addFilterList(custom_config, list);
    data = [];
    chrome.eyeoFilteringPrivate.removeFilterList(custom_config, list);
  },
  function customFiltersEvent() {
    const filter = 'foo.bar';
    let data = [filter];
    let attempts = 2;
    chrome.eyeoFilteringPrivate.onCustomFiltersChanged.addListener(function(
        config_name) {
      if (config_name != custom_config) {
        chrome.test.fail('Failed: Wrong config name');
      }
      chrome.eyeoFilteringPrivate.getCustomFilters(
          custom_config, function(filters) {
            if (!arrayEquals(data, filters)) {
              chrome.test.fail('Unexpected custom filter list');
            }
            if (--attempts == 0) {
              chrome.test.succeed();
            }
          });
    });
    chrome.eyeoFilteringPrivate.createConfiguration(custom_config);
    chrome.eyeoFilteringPrivate.addCustomFilter(custom_config, filter);
    data = [];
    chrome.eyeoFilteringPrivate.removeCustomFilter(custom_config, filter);
  },
];

const urlParams = new URLSearchParams(window.location.search);
chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == urlParams.get('subtest');
}));
