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


var alohaIdLibrary;(()=>{"use strict";var o={};(o=>{"undefined"!=typeof Symbol&&Symbol.toStringTag&&Object.defineProperty(o,Symbol.toStringTag,{value:"Module"}),Object.defineProperty(o,"__esModule",{value:!0})})(o);class e{constructor(o){this.success=o}}class r{constructor(o){this.success=o}}class s{constructor(o){this.success=o}}globalThis.alohabrowser=new class{get alohaIdProxy(){return alohaIdProxy}requestTwoFactor(o){return new Promise((r=>{const s=this.alohaIdProxy.requestTwoFactor(o.code,o.token,o.profile_id);r(new e(s))}))}login(){return new Promise((o=>{const e=this.alohaIdProxy.login();o(new r(e))}))}logout(){return new Promise((o=>{const e=this.alohaIdProxy.logout();o(new s(e))}))}},alohaIdLibrary=o})();