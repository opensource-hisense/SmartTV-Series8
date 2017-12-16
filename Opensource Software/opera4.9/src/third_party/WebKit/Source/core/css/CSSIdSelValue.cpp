// Copyright 2015 Opera TV. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/CSSIdSelValue.h"

#include "core/css/CSSMarkup.h"
#include "wtf/text/WTFString.h"

namespace blink {

CSSIdSelValue::CSSIdSelValue(const String& str)
    : CSSValue(IdSelClass)
    , m_idsel(str) { }

String CSSIdSelValue::customCSSText() const
{
    return serializeIdSel(m_idsel);
}

DEFINE_TRACE_AFTER_DISPATCH(CSSIdSelValue)
{
    CSSValue::traceAfterDispatch(visitor);
}

} // namespace blink
