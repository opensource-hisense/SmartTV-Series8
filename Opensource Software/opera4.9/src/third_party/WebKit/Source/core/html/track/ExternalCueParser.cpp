/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/html/track/ExternalCueParser.h"

#include "core/html/track/html/HTMLCue.h"
#include "core/html/track/vtt/VTTCue.h"
#include "wtf/text/WTFString.h"

namespace blink {

ExternalCueParser::ExternalCueParser(ExternalCueParserClient* client, Document& document, std::unique_ptr<WebCueParser> cueParser)
    : m_client(client)
    , m_document(document)
    , m_cueParser(std::move(cueParser))
{
    m_cueParser->setClient(this);
}

ExternalCueParser::~ExternalCueParser()
{
}

void ExternalCueParser::parseBytes(const char* data, unsigned length)
{
    if (!m_cueParser)
        return;
    m_cueParser->parseBytes(data, length);
}

void ExternalCueParser::flush()
{
    if (!m_cueParser)
        return;
    m_cueParser->flush();
}

void ExternalCueParser::getNewCues(HeapVector<Member<TextTrackCue>>& cues)
{
    if (!m_cueParser)
        return;

    WebVector<WebVTTCue> vttCues;
    WebVector<WebHTMLCue> htmlCues;

    m_cueParser->getNewVTTCues(vttCues);

    for (unsigned i = 0; i < vttCues.size(); i++) {
        Member<VTTCue> cue = VTTCue::create(*m_document, vttCues[i].startTime, vttCues[i].endTime, vttCues[i].content);
        cue->setId(vttCues[i].id);
        cue->parseSettings(vttCues[i].settings);
        cues.append(cue);
    }

    m_cueParser->getNewHTMLCues(htmlCues);

    for (unsigned i = 0; i < htmlCues.size(); i++) {
        Member<HTMLCue> cue = HTMLCue::create(*m_document, htmlCues[i].startTime, htmlCues[i].endTime, htmlCues[i].content);
        cues.append(cue);
    }
}

void ExternalCueParser::newCuesParsed()
{
    m_client->externalNewCuesParsed();
}

void ExternalCueParser::fileFailedToParse()
{
    m_client->externalFileFailedToParse();
}

DEFINE_TRACE(ExternalCueParser)
{
    visitor->trace(m_client);
    visitor->trace(m_document);
}

} // namespace blink
