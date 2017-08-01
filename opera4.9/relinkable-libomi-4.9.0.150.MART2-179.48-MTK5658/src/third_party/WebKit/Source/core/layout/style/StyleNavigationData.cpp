/*
 * Copyright (C) 2014 Opera TV AS. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "core/layout/style/StyleNavigationData.h"

#include "wtf/Assertions.h"

namespace blink {

void StyleNavigationData::setProperty(StyleNavigationData::NavigationDirection direction, const StyleNavigationValue& value)
{
    switch (direction) {
    case NavigationDown:
        setDown(value);
        return;
    case NavigationLeft:
        setLeft(value);
        return;
    case NavigationRight:
        setRight(value);
        return;
    case NavigationUp:
        setUp(value);
        return;
    default:
        ASSERT_NOT_REACHED();
    }
}

} // namespace blink
