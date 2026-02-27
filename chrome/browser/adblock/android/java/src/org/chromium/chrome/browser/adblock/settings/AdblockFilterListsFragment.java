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
import android.view.View;
import android.widget.ListView;

import androidx.fragment.app.ListFragment;

import org.chromium.chrome.browser.adblock.R;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.browser_ui.settings.SettingsFragment;

public class AdblockFilterListsFragment extends ListFragment implements SettingsFragment {
    private AdblockFilterListsAdapter mFilterListsAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.fragment_adblock_settings_filter_lists_title);
        mFilterListsAdapter =
                new AdblockFilterListsAdapter(
                        getActivity(),
                        AdblockController.getInstance(ProfileManager.getLastUsedRegularProfile()));
        setListAdapter(mFilterListsAdapter);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        ListView listView = getListView();
        listView.setDivider(null);
        listView.setItemsCanFocus(true);
    }

    @Override
    public void onStart() {
        super.onStart();
        mFilterListsAdapter.start();
    }

    @Override
    public @AnimationType int getAnimationType() {
        return AnimationType.PROPERTY;
    }
}
