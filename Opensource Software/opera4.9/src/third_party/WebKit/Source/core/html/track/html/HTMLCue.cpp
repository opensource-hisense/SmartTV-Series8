// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera TV AS. All rights reserved.
//
// This file is an original work developed by Opera TV AS.

#include "core/html/track/html/HTMLCue.h"

#include "core/dom/DocumentFragment.h"

namespace blink {

HTMLCue::HTMLCue(Document& document, double startTime, double endTime, const String& content)
    : TextTrackCue(startTime, endTime)
    , m_container(HTMLDivElement::create(document))
{
    Member<DocumentFragment> documentFragment = DocumentFragment::create(this->document());
    documentFragment->parseHTML(content, m_container.get(), DisallowScriptingAndPluginContent);
    m_container->appendChild(documentFragment);
    m_container->setInlineStyleProperty(CSSPropertyOverflow, CSSValueHidden);
    m_container->setInlineStyleProperty(CSSPropertyDisplay, CSSValueBlock);
}


HTMLCue::~HTMLCue()
{
}

void HTMLCue::updateDisplay(HTMLDivElement& container)
{
    container.appendChild(m_container);
}

void HTMLCue::updatePastAndFutureNodes(double movieTime)
{
}

void HTMLCue::removeDisplayTree(RemovalNotification)
{
    m_container->remove(ASSERT_NO_EXCEPTION);
}

ExecutionContext* HTMLCue::getExecutionContext() const
{
    return m_container->getExecutionContext();
}

#ifndef NDEBUG
String HTMLCue::toString() const
{
    return "";
}
#endif

Document& HTMLCue::document() const
{
    return m_container->document();
}

DEFINE_TRACE(HTMLCue)
{
    visitor->trace(m_container);
    TextTrackCue::trace(visitor);
}

} // namespace blink
