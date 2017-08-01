// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Modified by Opera TV AS.
// @copied-from chromium/src/third_party/WebKit/Source/core/layout/LayoutTextTrackContainer.cpp
// @last-synchronized 9474aeb50d0396bc5c86e118a88f6feb49372fcc
// @maintainer pgorszkowski
// @component Video

#include "core/layout/LayoutHTMLTrackContainer.h"

#include "bindings/core/v8/ExceptionStatePlaceholder.h"
#include "core/frame/DeprecatedScheduleStyleRecalcDuringLayout.h"
#include "core/layout/LayoutVideo.h"

namespace blink {

LayoutHTMLTrackContainer::LayoutHTMLTrackContainer(Element* element)
    : LayoutBlockFlow(element)
{
}

void LayoutHTMLTrackContainer::layout()
{
    LayoutBlockFlow::layout();
    if (style()->display() == NONE)
        return;

    DeprecatedScheduleStyleRecalcDuringLayout marker(node()->document().lifecycle());

    LayoutObject* mediaLayoutObject = parent();
    if (!mediaLayoutObject || !mediaLayoutObject->isVideo())
        return;

    if (updateSizes(toLayoutVideo(*mediaLayoutObject))) {
        IgnorableExceptionState exceptionState;
        // This sets a CSS variable video-height(with current height of video)
        // which can be used in TTML cue to calculate a cell height(in pixels)
        String videoDimensions = String::format("--video-height: %fpx;--video-width: %fpx;", m_videoSize.height().toFloat(), m_videoSize.width().toFloat());
        toElement(node())->style()->setCSSText(videoDimensions, exceptionState);
    }
}

bool LayoutHTMLTrackContainer::updateSizes(const LayoutVideo& videoLayoutObject)
{
    // FIXME: The video size is used to calculate the font size (a workaround
    // for lack of per-spec vh/vw support) but the whole media element is used
    // for cue rendering. This is inconsistent. See also the somewhat related
    // spec bug: https://www.w3.org/Bugs/Public/show_bug.cgi?id=28105
    LayoutSize videoSize = videoLayoutObject.replacedContentRect().size();

    if (videoSize != m_videoSize) {
        m_videoSize = videoSize;
        return true;
    }

    return false;
}

} // namespace blink

