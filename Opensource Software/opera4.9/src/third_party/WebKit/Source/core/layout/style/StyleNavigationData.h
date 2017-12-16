/*
 * Copyright (C) 2011 Kyounga Ra (kyounga.ra@gmail.com)
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

#ifndef StyleNavigationData_h
#define StyleNavigationData_h

#include "StyleNavigationValue.h"

namespace blink {

class StyleNavigationData {
public:
    enum NavigationDirection {
        NavigationDown,
        NavigationLeft,
        NavigationRight,
        NavigationUp
    };

    bool operator==(const StyleNavigationData& o) const
    {
        return m_up == o.m_up && m_down == o.m_down && m_left == o.m_left && m_right == o.m_right;
    }

    bool operator!=(const StyleNavigationData& o) const
    {
        return !(*this == o);
    }

    const StyleNavigationValue& up() const { return m_up; }
    const StyleNavigationValue& down() const { return m_down; }
    const StyleNavigationValue& left() const { return m_left; }
    const StyleNavigationValue& right() const { return m_right; }

    void setUp(const StyleNavigationValue& value) { m_up = value; }
    void setDown(const StyleNavigationValue& value) { m_down = value; }
    void setLeft(const StyleNavigationValue& value) { m_left = value; }
    void setRight(const StyleNavigationValue& value) { m_right = value; }

    void setProperty(NavigationDirection, const StyleNavigationValue&);

private:
    StyleNavigationValue m_up;
    StyleNavigationValue m_down;
    StyleNavigationValue m_left;
    StyleNavigationValue m_right;
};

} // namespace blink

#endif // StyleNavigationData_h
