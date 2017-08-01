// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "V8TestNode.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8DOMConfiguration.h"
#include "bindings/core/v8/V8ObjectConstructor.h"
#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace blink {

// Suppress warning: global constructors, because struct WrapperTypeInfo is trivial
// and does not depend on another global objects.
#if defined(COMPONENT_BUILD) && defined(WIN32) && COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
const WrapperTypeInfo V8TestNode::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestNode::domTemplate, V8TestNode::trace, V8TestNode::traceWrappers, 0, 0, V8TestNode::preparePrototypeAndInterfaceObject, nullptr, "TestNode", &V8Node::wrapperTypeInfo, WrapperTypeInfo::WrapperTypeObjectPrototype, WrapperTypeInfo::NodeClassId, WrapperTypeInfo::InheritFromEventTarget, WrapperTypeInfo::Dependent };
#if defined(COMPONENT_BUILD) && defined(WIN32) && COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

// This static member must be declared by DEFINE_WRAPPERTYPEINFO in TestNode.h.
// For details, see the comment of DEFINE_WRAPPERTYPEINFO in
// bindings/core/v8/ScriptWrappable.h.
const WrapperTypeInfo& TestNode::s_wrapperTypeInfo = V8TestNode::wrapperTypeInfo;

namespace TestNodeV8Internal {

static void hrefAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    TestNode* impl = V8TestNode::toImpl(holder);
    v8SetReturnValueString(info, impl->href(), info.GetIsolate());
}

static void hrefAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestNodeV8Internal::hrefAttributeGetter(info);
}

static void hrefAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    TestNode* impl = V8TestNode::toImpl(holder);
    V8StringResource<> cppValue = v8Value;
    if (!cppValue.prepare())
        return;
    impl->setHref(cppValue);
}

static void hrefAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Value> v8Value = info[0];
    TestNodeV8Internal::hrefAttributeSetter(v8Value, info);
}

static void hrefThrowsAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    TestNode* impl = V8TestNode::toImpl(holder);
    v8SetReturnValueString(info, impl->hrefThrows(), info.GetIsolate());
}

static void hrefThrowsAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestNodeV8Internal::hrefThrowsAttributeGetter(info);
}

static void hrefThrowsAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    ExceptionState exceptionState(ExceptionState::SetterContext, "hrefThrows", "TestNode", holder, info.GetIsolate());
    TestNode* impl = V8TestNode::toImpl(holder);
    V8StringResource<> cppValue = v8Value;
    if (!cppValue.prepare())
        return;
    impl->setHrefThrows(cppValue, exceptionState);
    exceptionState.throwIfNeeded();
}

static void hrefThrowsAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Value> v8Value = info[0];
    TestNodeV8Internal::hrefThrowsAttributeSetter(v8Value, info);
}

static void hrefCallWithAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    TestNode* impl = V8TestNode::toImpl(holder);
    v8SetReturnValueString(info, impl->hrefCallWith(), info.GetIsolate());
}

static void hrefCallWithAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestNodeV8Internal::hrefCallWithAttributeGetter(info);
}

static void hrefCallWithAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    TestNode* impl = V8TestNode::toImpl(holder);
    V8StringResource<> cppValue = v8Value;
    if (!cppValue.prepare())
        return;
    ExecutionContext* executionContext = currentExecutionContext(info.GetIsolate());
    impl->setHrefCallWith(executionContext, currentDOMWindow(info.GetIsolate()), enteredDOMWindow(info.GetIsolate()), cppValue);
}

static void hrefCallWithAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Value> v8Value = info[0];
    TestNodeV8Internal::hrefCallWithAttributeSetter(v8Value, info);
}

static void hrefByteStringAttributeGetter(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    TestNode* impl = V8TestNode::toImpl(holder);
    v8SetReturnValueString(info, impl->hrefByteString(), info.GetIsolate());
}

static void hrefByteStringAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestNodeV8Internal::hrefByteStringAttributeGetter(info);
}

static void hrefByteStringAttributeSetter(v8::Local<v8::Value> v8Value, const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Object> holder = info.Holder();
    ExceptionState exceptionState(ExceptionState::SetterContext, "hrefByteString", "TestNode", holder, info.GetIsolate());
    TestNode* impl = V8TestNode::toImpl(holder);
    V8StringResource<> cppValue = toByteString(info.GetIsolate(), v8Value, exceptionState);
    if (exceptionState.throwIfNeeded())
        return;
    impl->setHrefByteString(cppValue);
}

static void hrefByteStringAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::Value> v8Value = info[0];
    TestNodeV8Internal::hrefByteStringAttributeSetter(v8Value, info);
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestNode* impl = TestNode::create();
    v8::Local<v8::Object> wrapper = info.Holder();
    wrapper = impl->associateWithWrapper(info.GetIsolate(), &V8TestNode::wrapperTypeInfo, wrapper);
    v8SetReturnValue(info, wrapper);
}

} // namespace TestNodeV8Internal

const V8DOMConfiguration::AccessorConfiguration V8TestNodeAccessors[] = {
    {"href", TestNodeV8Internal::hrefAttributeGetterCallback, TestNodeV8Internal::hrefAttributeSetterCallback, 0, 0, 0, v8::DEFAULT, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::ExposedToAllScripts, V8DOMConfiguration::OnPrototype, V8DOMConfiguration::CheckHolder},
    {"hrefThrows", TestNodeV8Internal::hrefThrowsAttributeGetterCallback, TestNodeV8Internal::hrefThrowsAttributeSetterCallback, 0, 0, 0, v8::DEFAULT, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::ExposedToAllScripts, V8DOMConfiguration::OnPrototype, V8DOMConfiguration::CheckHolder},
    {"hrefCallWith", TestNodeV8Internal::hrefCallWithAttributeGetterCallback, TestNodeV8Internal::hrefCallWithAttributeSetterCallback, 0, 0, 0, v8::DEFAULT, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::ExposedToAllScripts, V8DOMConfiguration::OnPrototype, V8DOMConfiguration::CheckHolder},
    {"hrefByteString", TestNodeV8Internal::hrefByteStringAttributeGetterCallback, TestNodeV8Internal::hrefByteStringAttributeSetterCallback, 0, 0, 0, v8::DEFAULT, static_cast<v8::PropertyAttribute>(v8::None), V8DOMConfiguration::ExposedToAllScripts, V8DOMConfiguration::OnPrototype, V8DOMConfiguration::CheckHolder},
};

void V8TestNode::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (!info.IsConstructCall()) {
        V8ThrowException::throwTypeError(info.GetIsolate(), ExceptionMessages::constructorNotCallableAsFunction("TestNode"));
        return;
    }

    if (ConstructorMode::current(info.GetIsolate()) == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    TestNodeV8Internal::constructor(info);
}

static void installV8TestNodeTemplate(v8::Isolate* isolate, const DOMWrapperWorld& world, v8::Local<v8::FunctionTemplate> interfaceTemplate)
{
    // Initialize the interface object's template.
    V8DOMConfiguration::initializeDOMInterfaceTemplate(isolate, interfaceTemplate, V8TestNode::wrapperTypeInfo.interfaceName, V8Node::domTemplate(isolate, world), V8TestNode::internalFieldCount);
    interfaceTemplate->SetCallHandler(V8TestNode::constructorCallback);
    interfaceTemplate->SetLength(0);
    v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interfaceTemplate);
    ALLOW_UNUSED_LOCAL(signature);
    v8::Local<v8::ObjectTemplate> instanceTemplate = interfaceTemplate->InstanceTemplate();
    ALLOW_UNUSED_LOCAL(instanceTemplate);
    v8::Local<v8::ObjectTemplate> prototypeTemplate = interfaceTemplate->PrototypeTemplate();
    ALLOW_UNUSED_LOCAL(prototypeTemplate);
    // Register DOM constants, attributes and operations.
    V8DOMConfiguration::installAccessors(isolate, world, instanceTemplate, prototypeTemplate, interfaceTemplate, signature, V8TestNodeAccessors, WTF_ARRAY_LENGTH(V8TestNodeAccessors));
}

v8::Local<v8::FunctionTemplate> V8TestNode::domTemplate(v8::Isolate* isolate, const DOMWrapperWorld& world)
{
    return V8DOMConfiguration::domClassTemplate(isolate, world, const_cast<WrapperTypeInfo*>(&wrapperTypeInfo), installV8TestNodeTemplate);
}


bool V8TestNode::hasInstance(v8::Local<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, v8Value);
}

v8::Local<v8::Object> V8TestNode::findInstanceInPrototypeChain(v8::Local<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->findInstanceInPrototypeChain(&wrapperTypeInfo, v8Value);
}

TestNode* V8TestNode::toImplWithTypeCheck(v8::Isolate* isolate, v8::Local<v8::Value> value)
{
    return hasInstance(value, isolate) ? toImpl(v8::Local<v8::Object>::Cast(value)) : nullptr;
}

} // namespace blink
