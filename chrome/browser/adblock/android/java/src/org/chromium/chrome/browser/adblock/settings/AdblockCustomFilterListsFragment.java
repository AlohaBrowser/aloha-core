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

package org.chromium.chrome.browser.adblock.settings;

import android.os.Bundle;
import android.view.Gravity;
import android.webkit.URLUtil;
import android.widget.Toast;

import org.chromium.base.Log;
import org.chromium.chrome.browser.adblock.R;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.adblock.AdblockController;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class AdblockCustomFilterListsFragment extends AdblockCustomItemFragment {
    private static final String TAG = AdblockCustomFilterListsFragment.class.getSimpleName();

    public AdblockCustomFilterListsFragment() {}

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getActivity().setTitle(R.string.fragment_adblock_more_options_custom_filter_lists_title);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mPageTitle.set(getString(R.string.fragment_adblock_more_options_custom_filter_lists_title));
    }

    @Override
    protected List<String> getItems() {
        final List<AdblockController.Subscription> installed =
                AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                        .getInstalledSubscriptions();
        final List<AdblockController.Subscription> recommended =
                AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                        .getRecommendedSubscriptions();
        final List<String> customStrings = new ArrayList<String>();
        for (final AdblockController.Subscription subscription : installed) {
            if (recommended.contains(subscription)) {
                continue;
            }
            // FIXME(kzlomek): Remove this after DPD-1613
            if (subscription
                    .url()
                    .toString()
                    .equals("https://easylist-downloads.adblockplus.org/exceptionrules.txt")) {
                continue;
            }
            customStrings.add(subscription.url().toString());
        }

        return customStrings;
    }

    @Override
    protected String getCustomItemTextViewText() {
        return getString(R.string.fragment_adblock_settings_filter_lists_title);
    }

    @Override
    protected String getCustomItemEditTextHint() {
        return getString(R.string.fragment_adblock_more_options_add_custom_filter_list);
    }

    @Override
    protected String getCustomItemTextViewContentDescription() {
        return new String("Add custom filter list text field");
    }

    @Override
    protected String getCustomItemAddButtonContentDescription() {
        return new String("Add custom filter list add button");
    }

    @Override
    protected String getCustomItemRemoveButtonContentDescription() {
        return new String("Add custom filter list remove button");
    }

    @Override
    protected void addItemImpl(String url) {
        if (!URLUtil.isHttpsUrl(url)) {
            Toast toast =
                    Toast.makeText(getActivity(), "Url must be https format", Toast.LENGTH_SHORT);
            toast.setGravity(Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL, 0, 0);
            toast.show();
            Log.e(TAG, "Invalid url: " + url);
            return;
        }
        try {
            AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                    .installSubscription(new URL(URLUtil.guessUrl(url)));
        } catch (MalformedURLException ex) {
            Toast.makeText(getActivity(), "Error parsing url", Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Error parsing url: " + url);
        }
    }

    @Override
    protected void removeItemImpl(String url) {
        try {
            AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                    .uninstallSubscription(new URL(URLUtil.guessUrl(url)));
        } catch (MalformedURLException ex) {
            Log.e(TAG, "Error parsing url: " + url);
        }
    }
}
