// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera TV AS. All rights reserved.
//
// This file is an original work developed by Opera TV AS.

#ifndef ScrollbarThemeInvisible_h
#define ScrollbarThemeInvisible_h

#include "platform/scroll/ScrollbarThemeOverlay.h"

namespace blink {

class PLATFORM_EXPORT ScrollbarThemeInvisible : public ScrollbarThemeOverlay {
public:
    ScrollbarThemeInvisible() : ScrollbarThemeOverlay(0, 0, DisallowHitTest) { }

    bool paint(const Scrollbar& scrollbar, GraphicsContext& context, const CullRect& damageRect) override
    {
        return false;
    }
};

} // namespace blink
#endif // ScrollbarThemeInvisible_h
