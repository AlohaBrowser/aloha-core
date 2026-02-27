// *** ALOHA: Chrome compat ***

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


function chrome() {}
function Notification() {}
Notification.permission = "denied"

function findShadowElementByAlohaId(alohaId, root = document) {
  // At first, try to find element directly in the current root
  const directMatch = root.querySelector(`[aloha_id="${alohaId}"]`);
  if (directMatch) return directMatch;

  // Check all elements in the current root
  const allElements = root.querySelectorAll('*');
  for (const el of allElements) {
    // If element has shadowRoot - recursively go inside
    if (el.shadowRoot) {
      const found = findShadowElementByAlohaId(alohaId, el.shadowRoot);
      if (found) return found;
    }
    // Also check inside iframe elements within shadow DOM
    if (el.tagName === 'IFRAME' && el.contentDocument) {
      try {
        const found = findShadowElementByAlohaId(alohaId, el.contentDocument);
        if (found) return found;
      } catch (e) {
        // Cross-origin iframe, skip
      }
    }
  }
  console.log('Could not find element with aloha_id in shadow DOM:', alohaId);
  // Not found
  return null;
}

function findIframeInShadowDOM(iframeId, root = document) {
  // Search for iframe with aloha_id in current root (including shadow DOM)
  const directMatch = root.querySelector(`iframe[aloha_id="${iframeId}"]`);
  if (directMatch && directMatch.contentDocument) {
    try {
      return directMatch.contentDocument;
    } catch (e) {
      // Cross-origin, skip
      return null;
    }
  }

  // Check all elements including those in shadow DOM
  const allElements = root.querySelectorAll('*');
  for (const el of allElements) {
    if (el.tagName === 'IFRAME' && el.getAttribute('aloha_id') === iframeId) {
      try {
        return el.contentDocument;
      } catch (e) {
        // Cross-origin iframe
        continue;
      }
    }
    // Recursively check shadow DOM
    if (el.shadowRoot) {
      const found = findIframeInShadowDOM(iframeId, el.shadowRoot);
      if (found) return found;
    }
  }
  return null;
}

function findMediaElement(mediaId, iframeIds) {
  // getElementById not work for elements in iframes, so need to iterate inside iframes:
  // document.getElementById("<iframe 0>").contentDocument
  //         .getElementById("<iframe 1>").contentDocument
  //         ....
  //         .getElementById("<id of media player>")
  let doc = document

  for (const iframeId of iframeIds) {
    // Try to find iframe first in normal DOM, then in shadow DOM
    const iframeEl = doc.querySelector("[aloha_id=\"" + iframeId + "\"]");
    if (iframeEl && iframeEl.contentDocument) {
      doc = iframeEl.contentDocument;
    } else {
      // Not found in normal DOM, try shadow DOM
      const shadowDoc = findIframeInShadowDOM(iframeId, doc);
      if (shadowDoc) {
        doc = shadowDoc;
      } else {
        return findShadowElementByAlohaId(mediaId);
      }
    }
  }

  // Some sites duplicate attributes to parent element and
  // querySelectorAll can return several elements, we have to choose VIDEO
  elems = doc.querySelectorAll("[aloha_id=\"" + mediaId + "\"]")
  for (const elem of elems) {
    if (elem.tagName == "VIDEO") {
      return elem;
    }
  }

  return findShadowElementByAlohaId(mediaId, doc);
}
