// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera TV AS. All rights reserved.
//
// This file is an original work developed by Opera TV AS.

#ifndef HTMLCue_h
#define HTMLCue_h

#include "core/html/track/TextTrackCue.h"
#include "platform/heap/Handle.h"

namespace blink {

class Document;
class DocumentFragment;

class HTMLCue final : public TextTrackCue {
public:
    static HTMLCue* create(Document& document, double startTime, double endTime, const String& content)
    {
        return new HTMLCue(document, startTime, endTime, content);
    }
    DECLARE_VIRTUAL_TRACE();

    ~HTMLCue() override;

    void updateDisplay(HTMLDivElement& container) override;
    void updatePastAndFutureNodes(double movieTime) override;
    void removeDisplayTree(RemovalNotification) override;
    CueType cueType() const override { return HTML; }
    ExecutionContext* getExecutionContext() const override;

#ifndef NDEBUG
    String toString() const override;
#endif

    Document& document() const;

private:
    HTMLCue(Document&, double startTime, double endTime, const String& content);

    Member<HTMLDivElement> m_container;
};

} // namespace blink

#endif // HTMLCue_h
