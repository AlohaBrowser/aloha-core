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

#include "find_video_url.h"

#include <vector>

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/html/html_html_element.h"

namespace aloha {
namespace {

using namespace blink;

// See https://app.clickup.com/2558578/v/dc/2e2kj-3039/2e2kj-17104
struct TraverseSettings {
  // Max traversed elements of one start or parent tree.
  unsigned max_traversed_elements;
  // Max number of ascents to parents of the start element.
  unsigned max_parent_level;
};

// See https://app.clickup.com/2558578/v/dc/2e2kj-3039/2e2kj-17104
constexpr TraverseSettings find_url_settings{200, 10};
// On bbc.com fullscreen element is an IFrame and the queue size is ~1750 in order to find video element.
constexpr TraverseSettings foreach_settings{2000, 2};

// Add children of |elem| to list for traverse. Element |except| will not be added.
// Adding will be finished if |elements_for_traverse| size reaches |max_traversed_elements|.
template <class ElementType>
void AddChildren(
    HeapVector<Member<ElementType>>& elements_for_traverse,
    unsigned max_traversed_elements,
    ElementType& elem,
    ElementType* except = nullptr) {
  for (auto* it = Traversal<Element>::FirstChild(elem);
      it != nullptr && elements_for_traverse.size() < max_traversed_elements;
      it = Traversal<Element>::NextSibling(*it)) {
    if (it != except) {
      elements_for_traverse.push_back(it);
    }
  }
}

// Traverse elements in depth from |elements_for_traverse| and
// call |processor| for all elements with type |ElementTypeForProcess|.
// Traverse will be finished if count of traversed elements reaches |max_traversed_elements|.
// |processor| can return true for break traverse.
template <class ElementTypeForProcess, class ElementType, class ElementProcessor>
bool TraverseInDepth(
    HeapVector<Member<ElementType>>& elements_for_traverse,
    unsigned max_traversed_elements,
    ElementProcessor&& processor) {

  for (size_t i = 0; i < elements_for_traverse.size(); ++i) {
    ElementType* elem = elements_for_traverse[i];
    DCHECK(elem != nullptr);

    if (auto* elem_as_searched = DynamicTo<ElementTypeForProcess>(elem)) {
      if (processor(*elem_as_searched)) {
        return true;
      }
    }
    if (auto* iframe_elem = DynamicTo<HTMLIFrameElement>(elem)) {
      if (auto* doc = iframe_elem->contentDocument()) {
        if (auto* doc_element = doc->documentElement()) {
          // Copy-paste from Document::FirstBodyElement()
          if (IsA<HTMLHtmlElement>(doc_element)) {
            for (auto* doc_child = Traversal<HTMLElement>::FirstChild(*doc_element);
                doc_child != nullptr;
                doc_child = Traversal<HTMLElement>::NextSibling(*doc_child)) {
              if (auto* body = DynamicTo<HTMLBodyElement>(*doc_child)) {
                AddChildren<ElementType>(elements_for_traverse, max_traversed_elements, *body);
              }
            }
          }
        }
      }
    } else {
      AddChildren(elements_for_traverse, max_traversed_elements, *elem);
    }
  }
  return false;
}

// Traverse to up and down in html-tree starts from |start_element| and
// call |processor| for all elements with type |ElementTypeForProcess|.
// See https://app.clickup.com/2558578/v/dc/2e2kj-3039/2e2kj-17104
// |processor| can return true for break traverse.
template <class ElementTypeForProcess, class ElementType, class ElementProcessor>
void Traverse(
    const TraverseSettings& settings,
    ElementType* start_element,
    ElementProcessor&& processor) {

  if (start_element == nullptr) {
    return;
  }
  HeapVector<Member<ElementType>> elements_for_traverse;
  elements_for_traverse.reserve(settings.max_traversed_elements);
  elements_for_traverse.push_back(start_element);
  if (TraverseInDepth<ElementTypeForProcess>(
      elements_for_traverse, settings.max_traversed_elements, processor)) {
    return;
  }

  auto* traversed_element = start_element;
  auto* parent = start_element->parentElement();
  unsigned level = 1;
  while (parent != nullptr) {
    elements_for_traverse.clear();
    AddChildren<ElementType>(
       elements_for_traverse, settings.max_traversed_elements,
       *parent, traversed_element);
    if (TraverseInDepth<ElementTypeForProcess>(
        elements_for_traverse, settings.max_traversed_elements, processor)) {
      return;
    }
    if (level >= settings.max_parent_level ||
        elements_for_traverse.size() >= settings.max_traversed_elements ||
        parent->HasTagName(html_names::kArticleTag) ||
        parent->HasTagName(html_names::kBodyTag)) {
      break;
    }
    ++level;
    traversed_element = parent;
    parent = parent->parentElement();
  }
}

// See https://app.clickup.com/2558578/v/dc/2e2kj-3039/2e2kj-17104
template <class ElementTypeForProcess, class UrlGetter>
KURL FindURL(const Node* node, const gfx::Point& point_in_viewport, UrlGetter&& url_getter) {
  KURL result;
  Traverse<ElementTypeForProcess>(
    find_url_settings,
    DynamicTo<Element>(node),
    [&] (const auto& elem) {
      auto elem_rect = elem.VisibleBoundsInLocalRoot();
      if (elem_rect.Contains(point_in_viewport)) {
        result = url_getter(elem);
        return !result.IsEmpty();
      }
      return false;
    });
  return result;
}

} // namespace

KURL GetMediaSourceURL(const Element& elem) {
  if (const auto* media_elem = DynamicTo<HTMLMediaElement>(elem)) {
    const auto & src_url = media_elem->currentSrc();
    if (!src_url.IsEmpty()) {
      return src_url;
    }
  }
  const auto & src_attr = elem.getAttribute(html_names::kSrcAttr);
  if (!src_attr.empty()) {
    KURL src_url(src_attr);
    if (!src_url.IsEmpty()) {
      return src_url;
    }
  }

  const Element* child = Traversal<Element>::FirstChild(elem);
  while (child != nullptr) {
    KURL traverse_result = GetMediaSourceURL(*child);
    if (!traverse_result.IsEmpty()) {
      return traverse_result;
    }
    child = Traversal<Element>::NextSibling(*child);
  }
  return {};
}

KURL FindVideoURL(const Node* node, const gfx::Point& point_in_viewport) {
  return FindURL<HTMLVideoElement>(
    node, point_in_viewport,
    [] (const auto& elem) { return GetMediaSourceURL(elem); });
}

KURL FindImageURL(const Node* node, const gfx::Point& point_in_viewport) {
  return FindURL<HTMLImageElement>(
    node, point_in_viewport,
    [] (const auto& elem) { return KURL(elem.ImageSourceURL()); });
}

void ForeachByVideoElements(Element& elem,
  const std::function<bool (HTMLVideoElement&)>& processor) {

  Traverse<HTMLVideoElement>(
    foreach_settings,
    &elem,
    [&processor] (auto& video_elem) {
      return processor(video_elem);
    });
}

}  // namespace aloha
