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

#ifndef ExternalCueParser_h
#define ExternalCueParser_h

#include "core/html/track/TextTrackCue.h"
#include "platform/heap/Handle.h"
#include "public/platform/WebCueParser.h"
#include <memory>

namespace blink {

class Document;

class ExternalCueParserClient : public GarbageCollectedMixin {
public:
    virtual ~ExternalCueParserClient() { }

    virtual void externalNewCuesParsed() = 0;
    virtual void externalFileFailedToParse() = 0;

    DEFINE_INLINE_VIRTUAL_TRACE() { }
};

class ExternalCueParser final : public GarbageCollectedFinalized<ExternalCueParser>, public WebCueParserClient {
public:
    static ExternalCueParser* create(ExternalCueParserClient* client, Document& document, std::unique_ptr<WebCueParser> cueParser)
    {
        return new ExternalCueParser(client, document, std::move(cueParser));
    }

    ~ExternalCueParser() override;

    // Input data to the parser to parse.
    void parseBytes(const char* data, unsigned length);
    void flush();

    // Transfers ownership of last parsed cues to caller.
    void getNewCues(HeapVector<Member<TextTrackCue>>&);

    // WebCueParserClient
    void newCuesParsed() override;
    void fileFailedToParse() override;

    DECLARE_TRACE();

    // Oilpan: need to promptly release m_cueParser.
    EAGERLY_FINALIZE();

private:
    explicit ExternalCueParser(ExternalCueParserClient*, Document&, std::unique_ptr<WebCueParser>);

    Member<ExternalCueParserClient> m_client;
    Member<Document> m_document;
    std::unique_ptr<WebCueParser> m_cueParser;
};

} // namespace blink

#endif
