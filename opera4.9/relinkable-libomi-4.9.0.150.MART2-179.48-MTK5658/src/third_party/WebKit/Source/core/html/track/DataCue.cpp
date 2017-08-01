// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/track/DataCue.h"

namespace blink {

DataCue::DataCue(ExecutionContext* context, double startTime, double endTime, DOMArrayBuffer* data)
    : TextTrackCue(startTime, endTime)
    , ContextLifecycleObserver(context)
{
    setData(data);
}

DataCue::~DataCue()
{
}

#ifndef NDEBUG
String DataCue::toString() const
{
    return String::format("%p id=%s interval=%f-->%f)", this, id().utf8().data(), startTime(), endTime());
}
#endif

DOMArrayBuffer* DataCue::data() const
{
    ASSERT(m_data);
    return m_data;
}

void DataCue::setData(DOMArrayBuffer* data)
{
    m_data = data;
}

ExecutionContext* DataCue::getExecutionContext() const
{
    return ContextLifecycleObserver::getExecutionContext();
}

DEFINE_TRACE(DataCue)
{
    visitor->trace(m_data);
    ContextLifecycleObserver::trace(visitor);
    TextTrackCue::trace(visitor);
}

} // namespace blink
