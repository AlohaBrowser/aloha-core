// Copyright 2024 Aloha Mobile Ltd.

// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "third_party/blink/renderer/core/dom/mutation_observer.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_mutation_observer_init.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_element.h"

namespace blink {

using StyleCallback = base::RepeatingCallback<void(blink::Element& video_element)>;

class AttributeStyleObserver : public blink::MutationObserver::Delegate {
public:
  explicit AttributeStyleObserver(blink::Element& video_element,  StyleCallback callback);
  ~AttributeStyleObserver() override; 
    
  void Deliver(const blink::MutationRecordVector&, blink::MutationObserver&) override;

  blink::ExecutionContext* GetExecutionContext() const override;

  void Trace(Visitor* visitor) const override;

private:
  blink::Member<blink::Element> video_element_;
  StyleCallback callback_;   
};
}
