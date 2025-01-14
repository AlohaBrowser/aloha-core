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


function findMediaElement(mediaId, iframeIds) {
  // getElementById not work for elements in iframes, so need to iterate inside iframes:
  // document.getElementById("<iframe 0>").contentDocument
  //         .getElementById("<iframe 1>").contentDocument
  //         ....
  //         .getElementById("<id of media player>")
  let doc = document
  for (const iframeId of iframeIds) {
    doc = doc.querySelector("[aloha_id=\"" + iframeId + "\"]").contentDocument
    if (doc == null) {
      return null
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

  return null;
}
