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

package org.chromium.components.adblock;

import java.util.HashMap;
import java.util.Map;

public enum ElementHideAction {
    // Note. This has to be kept in sync with c++ enum.
    HIDE(1),
    REMOVE(2),
    INLINE_CSS(3);

    private final int action;

    private ElementHideAction(int action) {
        this.action = action;
    }

    private static final Map<Integer, ElementHideAction> intToElementHideActionMap =
            new HashMap<Integer, ElementHideAction>();

    static {
        for (ElementHideAction type : ElementHideAction.values()) {
            intToElementHideActionMap.put(type.action, type);
        }
    }

    public static ElementHideAction fromInt(int i) {
        return intToElementHideActionMap.get(i);
    }

    public int getValue() {
        return action;
    }
}
