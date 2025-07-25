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

import org.chromium.chrome.browser.adblock.R;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.adblock.AdblockController;

import java.util.List;

public class AdblockCustomFiltersFragment extends AdblockCustomItemFragment {
    public AdblockCustomFiltersFragment() {}

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getActivity()
                .setTitle(getString(R.string.fragment_adblock_more_options_custom_filters_title));
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mPageTitle.set(getString(R.string.fragment_adblock_more_options_custom_filters_title));
    }

    @Override
    protected List<String> getItems() {
        return AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                .getCustomFilters();
    }

    @Override
    protected String getCustomItemTextViewText() {
        return getString(R.string.fragment_adblock_more_options_custom_filters_title);
    }

    @Override
    protected String getCustomItemEditTextHint() {
        return getString(R.string.fragment_adblock_more_options_custom_filters_hint);
    }

    @Override
    protected String getCustomItemTextViewContentDescription() {
        return "Add custom filter text field";
    }

    @Override
    protected String getCustomItemAddButtonContentDescription() {
        return "Custom filter add button";
    }

    @Override
    protected String getCustomItemRemoveButtonContentDescription() {
        return "Custom filter remove button";
    }

    @Override
    protected void addItemImpl(String value) {
        AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                .addCustomFilter(value);
    }

    @Override
    protected void removeItemImpl(String value) {
        AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile())
                .removeCustomFilter(value);
    }
}
