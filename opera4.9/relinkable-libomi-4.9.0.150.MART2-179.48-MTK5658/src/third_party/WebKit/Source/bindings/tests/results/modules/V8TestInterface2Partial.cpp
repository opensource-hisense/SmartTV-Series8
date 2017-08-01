// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "V8TestInterface2Partial.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8DOMConfiguration.h"
#include "bindings/core/v8/V8GCController.h"
#include "bindings/core/v8/V8ObjectConstructor.h"
#include "bindings/core/v8/V8TestInterface2.h"
#include "bindings/tests/idls/modules/TestInterface2Partial.h"
#include "bindings/tests/idls/modules/TestInterface2Partial2.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace blink {

namespace TestInterface2PartialV8Internal {

static void voidMethodPartial1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        V8ThrowException::throwException(createMinimumArityTypeErrorForMethod(info.GetIsolate(), "voidMethodPartial1", "TestInterface2", 1, info.Length()), info.GetIsolate());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toImpl(info.Holder());
    V8StringResource<> value;
    {
        value = info[0];
        if (!value.prepare())
            return;
    }
    TestInterface2Partial::voidMethodPartial1(*impl, value);
}

static void voidMethodPartial1MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterface2PartialV8Internal::voidMethodPartial1Method(info);
}

static void voidMethodPartial2Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        V8ThrowException::throwException(createMinimumArityTypeErrorForMethod(info.GetIsolate(), "voidMethodPartial2", "TestInterface2", 1, info.Length()), info.GetIsolate());
        return;
    }
    TestInterface2* impl = V8TestInterface2::toImpl(info.Holder());
    V8StringResource<> value;
    {
        value = info[0];
        if (!value.prepare())
            return;
    }
    TestInterface2Partial2::voidMethodPartial2(*impl, value);
}

static void voidMethodPartial2MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterface2PartialV8Internal::voidMethodPartial2Method(info);
}

} // namespace TestInterface2PartialV8Internal

const V8DOMConfiguration::MethodConfiguration V8TestInterface2Methods[] = {
    {"voidMethodPartial2", TestInterface2PartialV8Internal::voidMethodPartial2MethodCallback, 0, 1, v8::None, V8DOMConfiguration::ExposedToAllScripts, V8DOMConfiguration::OnPrototype},
};

void V8TestInterface2Partial::installV8TestInterface2Template(v8::Isolate* isolate, const DOMWrapperWorld& world, v8::Local<v8::FunctionTemplate> interfaceTemplate)
{
    // Initialize the interface object's template.
    V8TestInterface2::installV8TestInterface2Template(isolate, world, interfaceTemplate);
    v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interfaceTemplate);
    ALLOW_UNUSED_LOCAL(signature);
    v8::Local<v8::ObjectTemplate> instanceTemplate = interfaceTemplate->InstanceTemplate();
    ALLOW_UNUSED_LOCAL(instanceTemplate);
    v8::Local<v8::ObjectTemplate> prototypeTemplate = interfaceTemplate->PrototypeTemplate();
    ALLOW_UNUSED_LOCAL(prototypeTemplate);
    // Register DOM constants, attributes and operations.
    V8DOMConfiguration::installMethods(isolate, world, instanceTemplate, prototypeTemplate, interfaceTemplate, signature, V8TestInterface2Methods, WTF_ARRAY_LENGTH(V8TestInterface2Methods));

    if (RuntimeEnabledFeatures::interface2PartialFeatureNameEnabled()) {
        const V8DOMConfiguration::MethodConfiguration voidMethodPartial1MethodConfiguration = {"voidMethodPartial1", TestInterface2PartialV8Internal::voidMethodPartial1MethodCallback, 0, 1, v8::None, V8DOMConfiguration::ExposedToAllScripts, V8DOMConfiguration::OnPrototype};
        V8DOMConfiguration::installMethod(isolate, world, instanceTemplate, prototypeTemplate, interfaceTemplate, signature, voidMethodPartial1MethodConfiguration);
    }
}

void V8TestInterface2Partial::preparePrototypeAndInterfaceObject(v8::Local<v8::Context> context, const DOMWrapperWorld& world, v8::Local<v8::Object> prototypeObject, v8::Local<v8::Function> interfaceObject, v8::Local<v8::FunctionTemplate> interfaceTemplate)
{
    V8TestInterface2::preparePrototypeAndInterfaceObject(context, world, prototypeObject, interfaceObject, interfaceTemplate);
}

void V8TestInterface2Partial::initialize()
{
    // Should be invoked from ModulesInitializer.
    V8TestInterface2::updateWrapperTypeInfo(
        &V8TestInterface2Partial::installV8TestInterface2Template,
        &V8TestInterface2Partial::preparePrototypeAndInterfaceObject);
}

} // namespace blink
