// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "TestInterface2OrUint8Array.h"

#include "bindings/core/v8/ToV8.h"
#include "bindings/core/v8/V8ArrayBufferView.h"
#include "bindings/core/v8/V8TestInterface2.h"
#include "bindings/core/v8/V8Uint8Array.h"
#include "core/dom/FlexibleArrayBufferView.h"

namespace blink {

TestInterface2OrUint8Array::TestInterface2OrUint8Array()
    : m_type(SpecificTypeNone)
{
}

TestInterface2* TestInterface2OrUint8Array::getAsTestInterface2() const
{
    ASSERT(isTestInterface2());
    return m_testInterface2;
}

void TestInterface2OrUint8Array::setTestInterface2(TestInterface2* value)
{
    ASSERT(isNull());
    m_testInterface2 = value;
    m_type = SpecificTypeTestInterface2;
}

TestInterface2OrUint8Array TestInterface2OrUint8Array::fromTestInterface2(TestInterface2* value)
{
    TestInterface2OrUint8Array container;
    container.setTestInterface2(value);
    return container;
}

DOMUint8Array* TestInterface2OrUint8Array::getAsUint8Array() const
{
    ASSERT(isUint8Array());
    return m_uint8Array;
}

void TestInterface2OrUint8Array::setUint8Array(DOMUint8Array* value)
{
    ASSERT(isNull());
    m_uint8Array = value;
    m_type = SpecificTypeUint8Array;
}

TestInterface2OrUint8Array TestInterface2OrUint8Array::fromUint8Array(DOMUint8Array* value)
{
    TestInterface2OrUint8Array container;
    container.setUint8Array(value);
    return container;
}

TestInterface2OrUint8Array::TestInterface2OrUint8Array(const TestInterface2OrUint8Array&) = default;
TestInterface2OrUint8Array::~TestInterface2OrUint8Array() = default;
TestInterface2OrUint8Array& TestInterface2OrUint8Array::operator=(const TestInterface2OrUint8Array&) = default;

DEFINE_TRACE(TestInterface2OrUint8Array)
{
    visitor->trace(m_testInterface2);
    visitor->trace(m_uint8Array);
}

void V8TestInterface2OrUint8Array::toImpl(v8::Isolate* isolate, v8::Local<v8::Value> v8Value, TestInterface2OrUint8Array& impl, UnionTypeConversionMode conversionMode, ExceptionState& exceptionState)
{
    if (v8Value.IsEmpty())
        return;

    if (conversionMode == UnionTypeConversionMode::Nullable && isUndefinedOrNull(v8Value))
        return;

    if (V8TestInterface2::hasInstance(v8Value, isolate)) {
        TestInterface2* cppValue = V8TestInterface2::toImpl(v8::Local<v8::Object>::Cast(v8Value));
        impl.setTestInterface2(cppValue);
        return;
    }

    if (v8Value->IsUint8Array()) {
        DOMUint8Array* cppValue = V8Uint8Array::toImpl(v8::Local<v8::Object>::Cast(v8Value));
        impl.setUint8Array(cppValue);
        return;
    }

    exceptionState.throwTypeError("The provided value is not of type '(TestInterface2 or Uint8Array)'");
}

v8::Local<v8::Value> toV8(const TestInterface2OrUint8Array& impl, v8::Local<v8::Object> creationContext, v8::Isolate* isolate)
{
    switch (impl.m_type) {
    case TestInterface2OrUint8Array::SpecificTypeNone:
        return v8::Null(isolate);
    case TestInterface2OrUint8Array::SpecificTypeTestInterface2:
        return toV8(impl.getAsTestInterface2(), creationContext, isolate);
    case TestInterface2OrUint8Array::SpecificTypeUint8Array:
        return toV8(impl.getAsUint8Array(), creationContext, isolate);
    default:
        ASSERT_NOT_REACHED();
    }
    return v8::Local<v8::Value>();
}

TestInterface2OrUint8Array NativeValueTraits<TestInterface2OrUint8Array>::nativeValue(v8::Isolate* isolate, v8::Local<v8::Value> value, ExceptionState& exceptionState)
{
    TestInterface2OrUint8Array impl;
    V8TestInterface2OrUint8Array::toImpl(isolate, value, impl, UnionTypeConversionMode::NotNullable, exceptionState);
    return impl;
}

} // namespace blink
