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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.preference.PreferenceFragmentCompat;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.chrome.browser.adblock.R;
import org.chromium.components.browser_ui.settings.EmbeddableSettingsPage;
import org.chromium.components.browser_ui.settings.SettingsFragment;

import java.util.List;

public abstract class AdblockCustomItemFragment extends PreferenceFragmentCompat
        implements EmbeddableSettingsPage {
    private EditText mItem;
    private ImageView mAddButton;
    private ListView mListView;
    private Adapter mAdapter;

    protected final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    public AdblockCustomItemFragment() {}

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.adblock_custom_item_settings, container, false);
        TextView textView = rootView.findViewById(R.id.fragment_adblock_custom_item_text);
        textView.setText(getCustomItemTextViewText());
        textView.setContentDescription(getCustomItemTextViewContentDescription());

        bindControls(rootView);

        return rootView;
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public @SettingsFragment.AnimationType int getAnimationType() {
        return SettingsFragment.AnimationType.PROPERTY;
    }

    @Override
    public void onResume() {
        super.onResume();
        initControls();
    }

    private void bindControls(View rootView) {
        mItem = rootView.findViewById(R.id.fragment_adblock_custom_item_add_label);
        mItem.setHint(getCustomItemEditTextHint());
        mAddButton = rootView.findViewById(R.id.fragment_adblock_custom_item_add_button);
        mAddButton.setContentDescription(getCustomItemAddButtonContentDescription());
        mListView = rootView.findViewById(R.id.fragment_adblock_custom_item_listview);
    }

    protected abstract void addItemImpl(String item);

    protected abstract void removeItemImpl(String item);

    protected abstract List<String> getItems();

    protected abstract String getCustomItemTextViewText();

    protected abstract String getCustomItemTextViewContentDescription();

    protected abstract String getCustomItemAddButtonContentDescription();

    protected abstract String getCustomItemRemoveButtonContentDescription();

    protected abstract String getCustomItemEditTextHint();

    // Holder for listview items
    private class Holder {
        final TextView mItem;
        final ImageView mRemoveButton;

        Holder(View rootView) {
            mItem = rootView.findViewById(R.id.fragment_adblock_custom_item_title);
            mRemoveButton = rootView.findViewById(R.id.fragment_adblock_custom_item_remove);
            mRemoveButton.setContentDescription(
                    AdblockCustomItemFragment.this.getCustomItemRemoveButtonContentDescription());
        }
    }

    private final View.OnClickListener removeItemClickListener =
            new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    String item = (String) v.getTag();
                    removeItemImpl(item);
                    mAdapter.notifyDataSetChanged();
                }
            };

    private class Adapter extends BaseAdapter {
        @Override
        public int getCount() {
            return AdblockCustomItemFragment.this.getItems().size();
        }

        @Override
        public Object getItem(int position) {
            return AdblockCustomItemFragment.this.getItems().get(position);
        }

        @Override
        public long getItemId(int position) {
            return getItem(position).hashCode();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                LayoutInflater inflater = LayoutInflater.from(getActivity());
                convertView = inflater.inflate(R.layout.adblock_custom_item, parent, false);
                convertView.setTag(new Holder(convertView));
            }

            String item = (String) getItem(position);
            Holder holder = (Holder) convertView.getTag();
            holder.mItem.setText(item.toString());
            holder.mRemoveButton.setOnClickListener(removeItemClickListener);
            holder.mRemoveButton.setTag(item.toString());

            return convertView;
        }
    }

    private void initControls() {
        mAddButton.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        String preparedItem = prepareItem(mItem.getText().toString());

                        if (preparedItem != null && preparedItem.length() > 0) {
                            addItem(preparedItem);
                        }
                    }
                });

        mAdapter = new Adapter();
        mListView.setAdapter(mAdapter);
    }

    private String prepareItem(String item) {
        return item.trim();
    }

    public void addItem(String newItem) {
        addItemImpl(newItem);
        mAdapter.notifyDataSetChanged();
        mItem.getText().clear();
        mItem.clearFocus();
    }
}
