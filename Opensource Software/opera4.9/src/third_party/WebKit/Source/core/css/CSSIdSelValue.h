// Copyright 2015 Opera TV AS. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSIdSelValue_h
#define CSSIdSelValue_h

#include "core/css/CSSValue.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"

namespace blink {

class CSSIdSelValue : public CSSValue {
public:
    static CSSIdSelValue* create(const String& str)
    {
        return new CSSIdSelValue(str);
    }

    String value() const { return m_idsel; }

    String customCSSText() const;

    bool equals(const CSSIdSelValue& other) const
    {
        return m_idsel == other.m_idsel;
    }

    DECLARE_TRACE_AFTER_DISPATCH();

private:
    CSSIdSelValue(const String&);

    String m_idsel;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSIdSelValue, isIdSelValue());

} // namespace blink

#endif // CSSStringValue_h
