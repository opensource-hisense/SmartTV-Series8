// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera TV AS. All rights reserved.
//
// This file is an original work developed by Opera TV AS.

#ifndef WebCueParser_h
#define WebCueParser_h

#include "public/platform/WebString.h"
#include "public/platform/WebVector.h"

namespace blink {

struct WebHTMLCue {
    double startTime;
    double endTime;
    WebString content;
};

struct WebVTTCue {
    double startTime;
    double endTime;
    WebString id;
    WebString content;
    WebString settings;
};

class WebCueParserClient {
public:
    virtual void newCuesParsed() = 0;
    virtual void fileFailedToParse() = 0;
protected:
    virtual ~WebCueParserClient() {}
};

class WebCueParser {
public:
    virtual ~WebCueParser() {}
    virtual void setClient(WebCueParserClient*) = 0;
    virtual WebCueParserClient* client() = 0;
    virtual void parseBytes(const char* data, unsigned length) = 0;
    virtual void flush() = 0;
    virtual void getNewHTMLCues(blink::WebVector<blink::WebHTMLCue>&) = 0;
    virtual void getNewVTTCues(blink::WebVector<blink::WebVTTCue>&) = 0;
};

} // namespace blink


#endif

