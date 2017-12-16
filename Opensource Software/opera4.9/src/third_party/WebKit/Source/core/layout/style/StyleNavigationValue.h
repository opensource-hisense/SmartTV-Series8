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

#ifndef StyleNavigationValue_h
#define StyleNavigationValue_h

#include "wtf/text/WTFString.h"

namespace blink {

class StyleNavigationValue {
public:
    StyleNavigationValue()
        : m_auto(true)
        , m_id("")
        , m_target("current")
    { }

    StyleNavigationValue(const AtomicString& id, const AtomicString& target = "current")
        : m_auto(false)
        , m_id(id)
        , m_target(target)
    { }

    StyleNavigationValue(const String& id, const String& target = "current")
        : m_auto(false)
        , m_id(AtomicString(id))
        , m_target(AtomicString(target))
    { }

    bool isAuto() const
    {
        return m_auto;
    }

    bool operator==(const StyleNavigationValue& o) const
    {
        if (m_auto)
            return o.m_auto;
        return m_id == o.m_id && m_target == o.m_target;
    }

    bool operator!=(const StyleNavigationValue& o) const
    {
        return !(*this == o);
    }

    void operator=(const StyleNavigationValue& o)
    {
        m_auto = o.m_auto;
        m_id = o.m_id;
        m_target = o.m_target;
    }

    const AtomicString& id() const { return m_id; }
    const AtomicString& target() const { return m_target; }

private:
    bool m_auto;
    AtomicString m_id;
    AtomicString m_target;
};

} // namespace blink

#endif // StyleNavigationValue_h
