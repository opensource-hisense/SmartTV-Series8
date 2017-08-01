/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/html/track/html/HTMLTrackContainer.h"

#include "core/html/HTMLVideoElement.h"
#include "core/html/track/CueTimeline.h"
#include "core/layout/LayoutHTMLTrackContainer.h"

namespace blink {

HTMLTrackContainer::HTMLTrackContainer(Document& document)
    : HTMLDivElement(document)
{
}

HTMLTrackContainer* HTMLTrackContainer::create(Document& document)
{
    HTMLTrackContainer* element = new HTMLTrackContainer(document);
    element->setShadowPseudoId(AtomicString("-internal-media-html-track-container"));
    return element;
}

LayoutObject* HTMLTrackContainer::createLayoutObject(const ComputedStyle&)
{
    return new LayoutHTMLTrackContainer(this);
}

void HTMLTrackContainer::updateDisplay(HTMLMediaElement& mediaElement, ExposingControls exposingControls)
{
    if (!mediaElement.textTracksVisible()) {
        removeChildren();
        return;
    }

    if (isHTMLAudioElement(mediaElement))
        return;

    HTMLVideoElement& video = toHTMLVideoElement(mediaElement);
    bool reset = exposingControls == DidStartExposingControls;

    const CueList& activeCues = video.cueTimeline().currentlyActiveCues();

    if (reset)
        removeChildren();

    double movieTime = video.currentTime();
    for (size_t i = 0; i < activeCues.size(); ++i) {
        TextTrackCue* cue = activeCues[i].data();

        ASSERT(cue->isActive());
        if (!cue->track() || !cue->track()->isRendered() || !cue->isActive())
            continue;
        if (cue->cueType() != TextTrackCue::HTML)
            continue;

        cue->updateDisplay(*this);
        cue->updatePastAndFutureNodes(movieTime);
    }
}

} // namespace blink
