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

#include "attribute_style_observer.h"

namespace blink
{
    AttributeStyleObserver::AttributeStyleObserver(blink::Element& video_element,  StyleCallback callback)
        : video_element_(video_element), callback_(std::move(callback))
    {}

    AttributeStyleObserver::~AttributeStyleObserver() = default;

    blink::ExecutionContext* AttributeStyleObserver::GetExecutionContext() const 
    {
        return video_element_->GetExecutionContext();
    }

    void AttributeStyleObserver::Trace(Visitor* visitor) const 
    {
        visitor->Trace(video_element_);
        blink::MutationObserver::Delegate::Trace(visitor);
    }

    void AttributeStyleObserver::Deliver(const blink::MutationRecordVector&, blink::MutationObserver&) 
    {
        callback_.Run(*video_element_);
    }
    
} // namespace blink

