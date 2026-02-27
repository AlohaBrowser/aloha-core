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

public enum ContentType {
    // Note. This has to be kept in sync with c++ enum so some values
    // are skipped
    CONTENT_TYPE_OTHER(1 << 0),
    CONTENT_TYPE_SCRIPT(1 << 1),
    CONTENT_TYPE_IMAGE(1 << 2),
    CONTENT_TYPE_STYLESHEET(1 << 3),
    CONTENT_TYPE_OBJECT(1 << 4),
    CONTENT_TYPE_SUBDOCUMENT(1 << 5),
    CONTENT_TYPE_WEBSOCKET(1 << 7),
    CONTENT_TYPE_WEBRTC(1 << 8),
    CONTENT_TYPE_PING(1 << 10),
    CONTENT_TYPE_XMLHTTPREQUEST(1 << 11),
    CONTENT_TYPE_MEDIA(1 << 14),
    CONTENT_TYPE_FONT(1 << 15);

    private final int contentType;

    private ContentType(int contentType) {
        this.contentType = contentType;
    }

    private static final Map<Integer, ContentType> intToContentTypeMap =
            new HashMap<Integer, ContentType>();

    static {
        for (ContentType type : ContentType.values()) {
            intToContentTypeMap.put(type.contentType, type);
        }
    }

    public static ContentType fromInt(int i) {
        return intToContentTypeMap.get(i);
    }

    public int getValue() {
        return contentType;
    }
}
