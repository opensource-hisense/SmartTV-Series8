/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTCIceCandidateEvent_h
#define RTCIceCandidateEvent_h

#include "modules/EventModules.h"
#include "wtf/text/AtomicString.h"

namespace blink {
class RTCIceCandidate;

class RTCIceCandidateEvent final : public Event {
    DEFINE_WRAPPERTYPEINFO();
public:
    ~RTCIceCandidateEvent() override;

    static RTCIceCandidateEvent* create();
    static RTCIceCandidateEvent* create(bool canBubble, bool cancelable, RTCIceCandidate*);

    RTCIceCandidate* candidate() const;

    const AtomicString& interfaceName() const override;

    DECLARE_VIRTUAL_TRACE();

private:
    RTCIceCandidateEvent();
    RTCIceCandidateEvent(bool canBubble, bool cancelable, RTCIceCandidate*);

    Member<RTCIceCandidate> m_candidate;
};

} // namespace blink

#endif // RTCIceCandidateEvent_h
