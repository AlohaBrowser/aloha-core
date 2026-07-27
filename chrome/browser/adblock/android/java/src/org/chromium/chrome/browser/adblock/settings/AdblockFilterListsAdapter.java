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

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.TextView;

import org.chromium.chrome.browser.adblock.R;
import org.chromium.components.adblock.AdblockController;

import java.net.URL;
import java.util.List;

public class AdblockFilterListsAdapter extends BaseAdapter implements OnClickListener {
    private final LayoutInflater mLayoutInflater;
    private final AdblockController mController;

    public AdblockFilterListsAdapter(Context context, AdblockController controller) {
        mLayoutInflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        this.mController = controller;
    }

    public void start() {
        notifyDataSetChanged();
    }

    // BaseAdapter:

    @Override
    public int getCount() {
        return mController.getRecommendedSubscriptions().size();
    }

    @Override
    public Object getItem(int pos) {
        return mController.getRecommendedSubscriptions().get(pos);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (convertView == null) {
            view = mLayoutInflater.inflate(R.layout.adblock_filter_lists_list_item, null);
        }

        AdblockController.Subscription item = (AdblockController.Subscription) getItem(position);
        view.setOnClickListener(this);
        view.setTag(item.url());

        CheckBox checkBox = view.findViewById(R.id.checkbox);
        final List<AdblockController.Subscription> subscriptions =
                mController.getInstalledSubscriptions();
        boolean subscribed = false;
        boolean autoinstalled = false;
        for (final AdblockController.Subscription subscription : subscriptions) {
            if (subscription.url().equals(item.url())) {
                subscribed = true;
                autoinstalled = subscription.autoinstalled();
                break;
            }
        }
        checkBox.setChecked(subscribed);
        checkBox.setEnabled(!autoinstalled);
        checkBox.setContentDescription(item.title() + "filer list item checkbox");

        TextView description = view.findViewById(R.id.name);
        description.setText(item.title());
        description.setContentDescription(item.title() + "filer list item title text");
        return view;
    }

    @Override
    public void onClick(View view) {
        URL url = (URL) view.getTag();

        CheckBox checkBox = view.findViewById(R.id.checkbox);
        if (checkBox.isChecked()) {
            mController.uninstallSubscription(url);
        } else {
            mController.installSubscription(url);
        }

        notifyDataSetChanged();
    }
}
