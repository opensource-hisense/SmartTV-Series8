// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutHTMLTrackContainer_h
#define LayoutHTMLTrackContainer_h

#include "core/layout/LayoutBlockFlow.h"
#include "platform/geometry/LayoutSize.h"

namespace blink {

class Element;
class LayoutVideo;

class LayoutHTMLTrackContainer final : public LayoutBlockFlow {
public:
    LayoutHTMLTrackContainer(Element*);

private:
    void layout() override;
    bool updateSizes(const LayoutVideo&);

    LayoutSize m_videoSize;
};

} // namespace blink

#endif // LayoutHTMLTrackContainer_h
