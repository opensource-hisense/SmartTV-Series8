// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DataCue_h
#define DataCue_h

#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/DOMArrayBuffer.h"
#include "core/html/track/TextTrackCue.h"

namespace blink {

class Document;
class ExecutionContext;

class DataCue final : public TextTrackCue, public ContextLifecycleObserver {
    DEFINE_WRAPPERTYPEINFO();
    USING_GARBAGE_COLLECTED_MIXIN(DataCue);
public:
    static DataCue* create(ExecutionContext* context, double start, double end, DOMArrayBuffer* data)
    {
        return new DataCue(context, start, end, data);
    }

    virtual ~DataCue();

    DOMArrayBuffer* data() const;
    void setData(DOMArrayBuffer *);

    virtual CueType cueType() const override { return Data; }

    virtual ExecutionContext* getExecutionContext() const override;

#ifndef NDEBUG
    virtual String toString() const override;
#endif

    void updateDisplay(HTMLDivElement& container) override { }
    void updatePastAndFutureNodes(double movieTime) override { }
    void removeDisplayTree(RemovalNotification = NotifyRegion) override { }

    DECLARE_VIRTUAL_TRACE();

protected:
    DataCue(ExecutionContext* , double start, double end, DOMArrayBuffer* data);

private:
    Member<DOMArrayBuffer> m_data;
};

DEFINE_TYPE_CASTS(DataCue, TextTrackCue, cue, cue->cueType() == TextTrackCue::Data, cue.cueType() == TextTrackCue::Data);

} // namespace blink

#endif
