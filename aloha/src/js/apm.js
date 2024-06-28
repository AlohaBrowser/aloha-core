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



// Array of potential login input types.
// Sorted by parsing priority.
const CREDENTIALS_INPUT_TYPES = ['email', 'text', 'number', 'tel'];

let pageDocuments = [];
let pageForms = [];
let credentialsInputForms = [];

let focusedForm = null;

// Login inserted by a script.
// See [apm_fillFormData].
let insertedLogin = null;

// Password inserted by a script.
// See [apm_fillFormData].
let insertedPassword = null;

/**
 * Focus should be intercepted only once.
 */
let hasFocusBeenIntercepted = false;

/**
 * Flag should be set to `true` during
 * external interaction process: user focus
 * is obtained by Aloha credentials picker.
 *
 * When this flag is set to `true`,
 * any element interaction will blur
 * current active element to avoid
 * unnecessary keyboard open.
 */
let isAlohaCredentialsPickerShown = false;

/**
 * Flag should be set to `true` when
 * `onFormSubmitted` is called.
 *
 * After that input click events should not
 * be intercepted by the script to prevent
 * credentials picker from showing.
 * Such behavior is needed to avoid cases
 * when Aloha suggests a password that
 * have been saved seconds before.
 */
let isFormSubmissionEventEmitted = false;

const singleInputFieldAllowedOrigins = ['login.live.com', 'signup.live.com'];

/**
 * Extract login value from given [form].
 */
function getLoginFromForm(form) {
  // Getting login by attribute name because field.value may return ''.
  let login = form.loginField.getAttribute('value');
  if (!login || login === 'null') {
    login = form.loginField.value;
  }
  return login;
}

/**
 * Extract password value from given [form].
 */
function getPasswordFromForm(form) {
  if (form.passwordField) {
    return form.passwordField.value;
  }
  return '';
}

/**
 * Find all forms on a page and save them to [pageForms].
 */
function fillPageForms(document) {
  pageForms = pageForms.concat(Array.from(document.forms));
}

function isElementPositionFixed(element) {
  return window.getComputedStyle(element).getPropertyValue('position').toLowerCase() === 'fixed';
}

function isElementVisible(element) {
  if (isElementPositionFixed(element)) {
    return isElementInsideDocumentView(element);
  }
  return element.offsetParent !== null;
}

function isElementInsideDocumentView(element) {
  const rect = element.getBoundingClientRect();
  return !(rect.top > document.documentElement.scrollHeight
    || rect.bottom < 0
    || rect.right < 0
    || rect.left > document.documentElement.scrollWidth
  );
}

/**
 * Get currently active element within current document and its iframes.
 */
function getActiveElement() {
  var activeElement = document?.activeElement;
  if (!activeElement) {
    return null;
  }
  while (activeElement.tagName.toLowerCase() === 'iframe') {
    try {
      activeElement = activeElement.contentDocument.activeElement;
    } catch(e) {
      return null
    }
  }
  return activeElement;
}

/**
 * Find best login input candidate in given elements array.
 * @return Object of two fields:
 *         - input: Input element object.
 *         - isVerified: Is input element is a verified
 *           login input, not just a random text input.
 */
function findLoginInput(elements) {
  // 1. Short path for some websites.
  // We don't check for visibility here because some websites
  // add hidden inputs for password managers (like live.com).
  if (elements.username) {
    const usernameInputElement = elements.username;
    if (CREDENTIALS_INPUT_TYPES.includes(usernameInputElement.type) || usernameInputElement.type === 'hidden') {
      return {
        input: usernameInputElement,
        isVerified: true
      };
    }
  }
  if (elements.login) {
    const loginInputElement = elements.login;
    if (CREDENTIALS_INPUT_TYPES.includes(loginInputElement.type) || loginInputElement.type === 'hidden') {
      return {
        input: loginInputElement,
        isVerified: true
      };
    }
  }
  if (elements.email) {
    const emailInputElement = elements.email;
    if (CREDENTIALS_INPUT_TYPES.includes(emailInputElement.type) || emailInputElement.type === 'hidden') {
      return {
        input: emailInputElement,
        isVerified: true
      };
    }
  }

  // We're going to collect potential login inputs
  // alongside with their types to iterate later.
  // Items at the same indices of `potentialLoginInputs`
  // and `foundInputTypes` should describe the same inputs.
  const potentialLoginInputs = [];
  const foundInputTypes = [];

  // Some websites has no visible login inputs for password input forms.
  // We should collect such inputs to use them as a fall back if nothing else found.
  const hiddenFilledInputs = [];

  // 2. Collect potential inputs and their types.
  for (let i = 0; i < elements.length; i++) {
    const element = elements[i];
    const elementType = element.type;
    if (isElementVisible(element) && CREDENTIALS_INPUT_TYPES.includes(elementType)) {
      potentialLoginInputs.push(element);
      foundInputTypes.push(elementType);
    }
    if (elementType === 'hidden' && element.value.includes('@')) {
      // Input is hidden, but has a value with '@' in it.
      // We assume that this is a hidden email input
      // for password managers compatibility.
      hiddenFilledInputs.push(element);
    }
  }
  if (foundInputTypes.length < 1) {
    // No visible inputs found.
    // Look for password manager accessibility inputs.
    if (hiddenFilledInputs.length > 0) {
      return {
        input: hiddenFilledInputs[0],
        isVerified: false
      };
    }
    // No candidates found.
    return null;
  }
  // 3. Iterate the results by input type priority.
  for (let i = 0; i < CREDENTIALS_INPUT_TYPES.length; i++) {
    const inputIndex = foundInputTypes.indexOf(CREDENTIALS_INPUT_TYPES[i]);
    if (inputIndex >= 0) {
      return {
        input: potentialLoginInputs[inputIndex],
        isVerified: false
      };
    }
  }
  return null;
}

/**
 * Find a password input in given elements array.
 */
function findPasswordInput(elements) {
  // 1. Short path for some websites.
  // We don't check for input visibility to keep the consistency with
  // login input field search.
  // It prevents from adding login-only credential forms
  // to `credentialsInputForms` when hidden password input is present.
  if (elements.password) {
    const passwordInputElement = elements.password;
    if (passwordInputElement.type === 'password' && passwordInputElement.nodeName === 'INPUT') {
      return passwordInputElement;
    }
  }
  // 2. Look for the first password input.
  for (let i = 0; i < elements.length; i++) {
    let element = elements[i];
    let elementType = element.type;
    if (elementType === 'password' && element.nodeName === 'INPUT') {
      return element;
    }
  }
}

/**
 * Find credential inputs within given [elements].
 */
function findCredentialInputs(elements) {
  const loginInputResult = findLoginInput(elements);
  const foundLoginInput = (loginInputResult != null) ? loginInputResult.input : null;
  const isLoginInputVerified = (loginInputResult != null) ? loginInputResult.isVerified : false;
  const foundPasswordInput = findPasswordInput(elements);
  return {
    loginInput: foundLoginInput,
    isLoginInputVerified: isLoginInputVerified,
    passwordInput: foundPasswordInput
  };
}

/**
 * Check if forms without password field are allowed.
 */
function canProceedWithoutPasswordField() {
  const origin = window.location.origin;
  return singleInputFieldAllowedOrigins.some((allowedOrigin) => origin.includes(allowedOrigin));
}

/**
 * Find all credential input forms and save them to [credentialsInputForms].
 */
function fillAndSetupCredentialsInputForms(forms) {
  forms.forEach((form) => {
    const inputs = findCredentialInputs(form.elements);
    const formField_login = inputs.loginInput;
    const formField_password = inputs.passwordInput;
    if (formField_login && (formField_password || canProceedWithoutPasswordField() || inputs.isLoginInputVerified)) {
      const credentialInputForm = {
        loginField: formField_login,
        passwordField: formField_password
      };
      if (isFormRegistered(credentialInputForm)) {
        // Already added, skip it.
        return;
      }
      credentialsInputForms.push(credentialInputForm);

      form.addEventListener('submit', function() { onCredentialsInputFormSubmitted(credentialInputForm); });

      setupInputListeners(credentialInputForm);

      // Invoke focus callback immediately if input already has a focus.
      const activeElement = getActiveElement();
      if (activeElement === credentialInputForm.loginField || activeElement === credentialInputForm.passwordField) {
        onCredentialsInputGotFocus(null, credentialInputForm);
      }
    }
  });
}

/**
 * Find any new frames and register them for event handling.
 */
function findNewFrames() {
  document.querySelectorAll('frame,iframe').forEach((frame) => {
    if (frame.contentDocument) {
      registerDocument(frame.contentDocument);
    }
  });
}

/*
 * Register given document for event handling.
 */
function registerDocument(newDocument) {
  if (pageDocuments.includes(newDocument)) {
    // Already added.
    return;
  }

  pageDocuments.push(newDocument);
  fillPageForms(newDocument);
  fillAndSetupCredentialsInputForms(pageForms);
  setupUnownedForms();
  observeDocumentMutations(newDocument);
  newDocument.addEventListener('click', function(event) { onElementInteraction(event); });
  newDocument.addEventListener('focus', function(event) { onElementInteraction(event); });
  newDocument.addEventListener('focusin', function(event) { onElementInteraction(event); });
}

function startNewDocumentsObserver() {
  setInterval(findNewFrames, 500);
}

/**
 * Invalidate forms on DOM change.
 * For example, google.com adds login form in runtime.
 */
function observeDocumentMutations(document) {
  const config = {
    childList: true,
    subtree: true
  };
  const callback = function() {
    Array.from(document.forms).forEach((form) => {
      if (!pageForms.includes(form)) {
        pageForms.push(form);
      }
    });
    // We should always re-init every form because
    // they are mutable and new inputs can be added.
    fillAndSetupCredentialsInputForms(pageForms);
    setupUnownedForms();
  };
  const observer = new MutationObserver(callback);
  observer.observe(document, config);
}

/**
 * Function to be called when the input form field is focused.
 */
function onCredentialsInputGotFocus(event, credentialInputForm) {
  if (isFormSubmissionEventEmitted) {
    // Ignore focus events if password has been saved for current page.
    // See `isFormSubmissionEventEmitted` for details.
    return;
  }
  if (event && event.isAlohaEvent) {
    // Focus has been obtained due to custom Aloha event handling.
    // Such events should be skipped.
    return;
  }
  focusedForm = credentialInputForm;
  const origin = window.location.origin;
  const login = getLoginFromForm(credentialInputForm);
  if (hasFocusBeenIntercepted) {
    // Workaround for a multiple-step websites like Twitter.
    // If some form has been filled, but current password input value is ''
    // we should type user password into corresponding field.
    if (insertedPassword && credentialInputForm.passwordField && credentialInputForm.passwordField.value === '') {
      setInputElementValue(credentialInputForm.passwordField, insertedPassword);
    }
    return;
  }
  const shouldInterceptFocus = alohaPasswordManager.onInputFieldClicked(origin, login);
  if (shouldInterceptFocus) {
    hasFocusBeenIntercepted = true;
    isAlohaCredentialsPickerShown = true;
    try {
      // Reset focus to prevent keyboard from showing up.
      getActiveElement().blur();
    } catch(e) {}
  }
}

/**
 * Function to be called on form data submission.
 */
function onCredentialsInputFormSubmitted(form) {
  const origin = window.location.origin;
  const login = getLoginFromForm(form);
  const password = getPasswordFromForm(form);
  if (login === insertedLogin && password === insertedPassword) {
    // User submitted auto-filled data, no action needed.
    return;
  }
  alohaPasswordManager.onFormSubmitted(origin, login, password);
  isFormSubmissionEventEmitted = true;
}

/**
 * Function to be called when form data has been changed.
 */
function onCredentialsInputFormDataChanged(form) {
  const origin = window.location.origin;
  const login = getLoginFromForm(form);
  const password = getPasswordFromForm(form);
  if (login === insertedLogin && password === insertedPassword) {
    return;
  }
  alohaPasswordManager.onFormDataChanged(origin, login, password);
}

/**
 * Sets the value of a data-bound input using AngularJS.
 *
 * The method first set the value using the val() method. Then, if input is
 * bound to a model value, it sets the model value.
 * Documentation of relevant modules of AngularJS can be found at
 * https://docs.angularjs.org/guide/databinding
 * https://docs.angularjs.org/api/auto/service/$injector
 * https://docs.angularjs.org/api/ng/service/$parse
 *
 * @param {Element} input The input element of which the value is set.
 * @param {string} value The value the input element will be set.
 */
function setInputElementAngularValue_(input, value) {
  if (!input || !window['angular']) {
    return;
  }
  const angularElement =
      window['angular'].element && window['angular'].element(input);
  if (!angularElement) {
    return;
  }
  angularElement.val(value);
  const angularModel = angularElement.data && angularElement.data('ngModel');
  const angularScope = angularElement.scope();
  if (!angularModel || !angularScope) {
    return;
  }
  angularElement.injector().invoke([
    '$parse',
    function(parse) {
      const setter = parse(angularModel);
      setter.assign(angularScope, value);
    }
  ]);
}

/**
 * Creates and dispatches an HTML event.
 *
 * @param {Element} element The element for which an event is created.
 * @param {string} type The type of the event.
 * @param {boolean} bubbles A boolean indicating whether the event should
 *     bubble up through the event chain or not.
 * @param {boolean} cancelable A boolean indicating whether the event can be
 *     canceled.
 */
function createAndDispatchHTMLEvent(element, value, type, bubbles, cancelable) {
  const event = new Event(type, {bubbles: bubbles, cancelable: cancelable, data: value});
  if (type === 'input') {
    event.inputType = 'insertText';
  }
  // Add custom payload to distinguish between
  // user and script events.
  event.isAlohaEvent = true;
  if (element) {
    element.dispatchEvent(event);
  }
}

/**
 * Creates and sends notification that element has changed.
 *
 * Send events that 'mimic' the user typing in a field.
 * 'input' event is often use in case of a text field, and 'change' event is
 * more often used in case of selects.
 *
 * @param {Element} element The element that changed.
 */
function notifyElementValueChanged(element, value) {
  createAndDispatchHTMLEvent(element, value, 'keydown', true, false);
  createAndDispatchHTMLEvent(element, value, 'keypress', true, false);
  createAndDispatchHTMLEvent(element, value, 'input', true, false);
  createAndDispatchHTMLEvent(element, value, 'keyup', true, false);
  createAndDispatchHTMLEvent(element, value, 'change', true, false);
}

/**
 * Internal function to set the element value.
 */
function setInputElementValue_(input, value) {
  const propertyName = 'value';

  // Return early if the value hasn't changed.
  if (input[propertyName] === value) {
    return false;
  }

  // When the user inputs a value in an HTMLInput field, the property setter is
  // not called. The different frameworks often call it explicitly when
  // receiving the input event.
  // This is probably due to the sync between the HTML object and the DOM
  // object.
  // The sequence of event is: User input -> input event -> setter.
  // When the property is set programmatically (input.value = 'foo'), the setter
  // is called immediately (then probably called again on the input event)
  // JS input -> setter.
  // The only way to emulate the user behavior is to override the property
  // The getter will return the new value to emulate the fact the the HTML
  // value was updated without calling the setter.
  // The setter simply forwards the set to the older property descriptor.
  // Once the setter has been called, just forward get and set calls.

  const oldPropertyDescriptor = /** @type {!Object} */ (Object.getOwnPropertyDescriptor(input, propertyName));
  const overrideProperty = oldPropertyDescriptor && oldPropertyDescriptor.configurable;
  let setterCalled = false;

  if (overrideProperty) {
    const newProperty = {
      get() {
        if (setterCalled && oldPropertyDescriptor.get) {
          return oldPropertyDescriptor.get.call(input);
        }
        // Simulate the fact that the HTML value has been set but not yet the
        // property.
        return value + '';
      },
      configurable: true
    };
    if (oldPropertyDescriptor.set) {
      newProperty.set = function() {
        setterCalled = true;
        oldPropertyDescriptor.set.call(input, value);
      };
    }
    Object.defineProperty(input, propertyName, newProperty);
  } else {
    setterCalled = true;
    input[propertyName] = value;
  }

  if (window['angular']) {
    // The page uses the AngularJS framework. Update the angular value before
    // sending events.
    setInputElementAngularValue_(value, input);
  }
  notifyElementValueChanged(input, value);

  if (overrideProperty) {
    Object.defineProperty(input, propertyName, oldPropertyDescriptor);
    if (!setterCalled && input[propertyName] !== value) {
      // The setter was never called. This may be intentional (the framework
      // ignored the input event) or not (the event did not conform to what
      // framework expected). The whole function will likely fail, but try to
      // set the value directly as a last try.
      input[propertyName] = value;
    }
  }
  return true;
}

/**
 * Sets the value of an input and dispatches the events on the changed element.
 */
function setInputElementValue(input, value) {
  const activeElement = getActiveElement();
  if (input !== activeElement) {
    createAndDispatchHTMLEvent(activeElement, value, 'blur', true, false);
    createAndDispatchHTMLEvent(input, value, 'focus', true, false);
  }
  setInputElementValue_(input, value);
  if (input !== activeElement) {
    createAndDispatchHTMLEvent(input, value, 'blur', true, false);
    createAndDispatchHTMLEvent(activeElement, value, 'focus', true, false);
  }
}

/**
 * Function to be called to fill the focused form with given data.
 */
function apm_fillFormData(login, password) {
  isAlohaCredentialsPickerShown = false;
  if (focusedForm === null) {
    return;
  }
  // Save credentials to avoid redundant [onCredentialsInputFormSubmitted] call.
  insertedLogin = login;
  insertedPassword = password;

  // Type credentials into corresponding fields.
  if (focusedForm.loginField) {
    setInputElementValue(focusedForm.loginField, login);
  }
  if (focusedForm.passwordField) {
    // Set password with a delay to imitate real user flow.
    // Delay is 50ms – taken from Chromium sources.
    setTimeout(function() {
      setInputElementValue(focusedForm.passwordField, password);
    }, 50);
  }
}

/**
 * Method to be called by Aloha when credentials picker
 * is closed.
 */
function apm_onCredentialsPickerClosed() {
  isAlohaCredentialsPickerShown = false;
}

/**
 * Add input event listeners to given form inputs.
 */
function setupInputListeners(credentialInputForm) {
  const loginField = credentialInputForm.loginField;
  const passwordField = credentialInputForm.passwordField;
  if (loginField) {
    loginField.addEventListener('change', function() { onCredentialsInputFormDataChanged(credentialInputForm); });
  }
  if (passwordField) {
    passwordField.addEventListener('change', function() { onCredentialsInputFormDataChanged(credentialInputForm); });
  }
}

/**
 * Function to be called on any click or focus event.
 * We should check if user focused or clicked on an input field known as an input field
 * to trigger [onCredentialsInputGotFocus] if needed.
 */
function onElementInteraction(event) {
  const target = event.target;
  if (isAlohaCredentialsPickerShown) {
    // Take focus from an active element to avoid keyboard show.
    // See [isAlohaCredentialsPickerShown] doc.
    try {
      getActiveElement().blur();
    } catch(e) {}
    return;
  }
  credentialsInputForms.forEach((form) => {
    if (form.loginField === target || form.passwordField === target) {
      onCredentialsInputGotFocus(event, form);
      return;
    }
  });
}

/**
 * Check if given [credentialsInputForm] is already added to [credentialsInputForms] and registered.
 */
function isFormRegistered(credentialsInputForm) {
  for (let i = 0; i < credentialsInputForms.length; i++) {
    const form = credentialsInputForms[i];
    if (form.loginField === credentialsInputForm.loginField && form.passwordField === credentialsInputForm.passwordField) {
      return true;
    }
  }
  return false;
}

/**
 * Inputs on some websites may be not attached to any forms.
 * We should still handle such cases.
 * Examples: twitter.com, quora.com
 */
function setupUnownedForms() {
  const documentInputs = Array.from(document.getElementsByTagName('input')).filter(input => input.form === null);
  if (documentInputs.length === 0) {
    // No inputs found
    return;
  }
  const isPasswordInput = (input) => input.type === 'password';
  const isTwitter = window.origin.includes('twitter.com');
  if (!documentInputs.some(isPasswordInput) && !isTwitter) {
    // Only twitter login inputs are allowed to have no password field.
    return;
  }
  const credentialInputs = findCredentialInputs(documentInputs);
  formField_login = credentialInputs.loginInput;
  formField_password = credentialInputs.passwordInput;

  if (formField_login || formField_password) {
    const credentialInputForm = {
      loginField: formField_login,
      passwordField: formField_password
    };
    if (isFormRegistered(credentialInputForm)) {
      return;
    }
    credentialsInputForms.push(credentialInputForm);

    if (formField_login) {
      formField_login.addEventListener('click', function(event) { onCredentialsInputGotFocus(event, credentialInputForm); }, { once: true });
    }
    if (formField_password) {
      formField_password.addEventListener('click', function(event) { onCredentialsInputGotFocus(event, credentialInputForm); }, { once: true });
    }

    setupInputListeners(credentialInputForm);

    // Invoke focus callback immediately if input already has a focus.
    const activeElement = getActiveElement();
    if (activeElement === credentialInputForm.loginField || activeElement === credentialInputForm.passwordField) {
      onCredentialsInputGotFocus(null, credentialInputForm);
    }
  }
}

function apm_init() {
  registerDocument(document);
  startNewDocumentsObserver();
}
