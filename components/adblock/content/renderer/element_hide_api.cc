/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/content/renderer/element_hide_api.h"

#include "content/public/renderer/v8_value_converter.h"
#include "gin/function_template.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"

namespace adblock {

namespace {

constexpr const char* kApiObjectName = "adblockEhApi";

v8::Local<v8::Object> GetOrCreateApiObject(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> object;
  v8::Local<v8::Value> value;
  auto id = gin::StringToV8(isolate, kApiObjectName);
  if (!global->Get(context, id).ToLocal(&value) || !value->IsObject()) {
    object = v8::Object::New(isolate);
    global->Set(context, id, object).Check();
  } else {
    object = v8::Local<v8::Object>::Cast(value);
  }
  return object;
}

template <typename Sig>
void BindFunctionToObject(v8::Isolate* isolate,
                          v8::Local<v8::Object> object,
                          const std::string& name,
                          const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

}  // namespace

ElementHideApi::ElementHideApi(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

ElementHideApi::~ElementHideApi() = default;

void ElementHideApi::LogSelectors(int action,
                                  std::vector<std::string> selectors) {
  element_hide_api_handler_->LogSelectors(
      static_cast<mojom::ElementHideAction>(action), selectors);
}

void ElementHideApi::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty()) {
    return;
  }

  EnsureServiceConnected();

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> api = GetOrCreateApiObject(isolate, context);
  BindFunctionToObject(isolate, api, "logSelectors",
                       base::BindRepeating(&ElementHideApi::LogSelectors,
                                           base::Unretained(this)));
}

void ElementHideApi::EnsureServiceConnected() {
  if (!element_hide_api_handler_) {
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
        element_hide_api_handler_.BindNewPipeAndPassReceiver());
  }
}

}  // namespace adblock
