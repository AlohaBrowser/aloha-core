/**
 * Creates an animated glowing frame overlay for a <video>.
 *
 * Why this is an external overlay (attached to document.body):
 * - Many sites dynamically mutate the <video> element and its containers (styles, transforms, portals).
 * - Injecting UI inside the site's DOM subtree can easily break layout or be broken by the site.
 *
 * Positioning strategy:
 * - 'absolute' (document coordinates): avoids scroll "lag" on compositor-driven scrolling because the overlay
 *   scrolls with the document without requiring JS to run on every scroll tick.
 * - 'fixed' (viewport coordinates): required when the video (or its ancestors) becomes fixed/sticky (PiP-like).
 *
 * To cover as many sites as possible we also detect:
 * - detach/attach and re-parenting (PiP portals) via a lightweight DOM MutationObserver
 * - style/class changes on the video and nearby ancestors (position: fixed/sticky, transforms, etc.)
 */
class VideoFrameOverlay {
  /**
   * @param {HTMLVideoElement} videoElement - the video element to overlay
   * @param {string} color - glow color (e.g., '#FF0000' or 'red')
   * @param {number} intensity - glow intensity (1-3 recommended)
   * @param {number} inset - how many pixels the frame overlaps the video (default: 4)
   * @param {number} borderWidth - static border width in pixels (default: 2)
   * @param {number} borderOpacity - static border opacity 0-1 (default: 0.5)
   */
  constructor(videoElement, color, intensity = 2, inset = 4, borderWidth = 2, borderOpacity = 0.5) {
    /** @type {HTMLVideoElement} */
    this.videoElement = videoElement;
    /** @type {string} */
    this.color = color;
    /** @type {number} */
    this.intensity = intensity;
    /** @type {number} */
    this.inset = inset;
    /** @type {number} */
    this.borderWidth = borderWidth;
    /** @type {number} */
    this.borderOpacity = borderOpacity;
    /** @type {HTMLDivElement|null} */
    this.overlay = null;
    /** @type {ResizeObserver|null} */
    this.resizeObserver = null;
    /** @type {number|null} */
    this.animationFrameId = null;
    /** @type {(() => void)|null} */
    this.boundScheduleUpdate = null;
    /** @type {{modeCheck: boolean, rectUpdate: boolean}} */
    this.pendingUpdate = { modeCheck: false, rectUpdate: false };
    /** @type {MutationObserver|null} */
    this.domObserver = null;
    /** @type {number|null} */
    this.domObserverRafId = null;
    /** @type {boolean} */
    this.domObserverCheckScheduled = false;
    /** @type {MutationObserver|null} */
    this.anchorAttrObserver = null;
    /** @type {HTMLElement|null} */
    this.lastAnchorParent = null;
    /** @type {boolean|null} */
    this.wasConnected = null;
    /** @type {HTMLStyleElement|null} */
    this.styleElement = null;
    this.uniqueId = Date.now().toString(36) + Math.random().toString(36).slice(2, 7);
    /**
     * Cache of last applied geometry to avoid unnecessary style writes.
     * @type {{x: number|null, y: number|null, width: number|null, height: number|null}}
     */
    this.lastPosition = { x: null, y: null, width: null, height: null };
    /**
     * Overlay coordinate space.
     * - 'absolute': document coords (rect + scroll offsets)
     * - 'fixed': viewport coords (rect only)
     * @type {'absolute'|'fixed'}
     */
    this.positionMode = 'absolute';
  }

  /**
   * Shows the glow effect on top of the video
   * @returns {boolean} whether the operation was successful
   */
  show() {
    if (!this.videoElement) {
      console.warn('VideoFrameOverlay: videoElement is not provided');
      return false;
    }

    // Determine initial mode (some sites start in a PiP-like fixed container).
    this.positionMode = this._computePositionMode();
    this._injectStyles();
    this._createOverlay();
    this._scheduleUpdate({ modeCheck: true, rectUpdate: true });
    this._startTracking();

    return true;
  }

  /**
   * Removes the glow effect and cleans up resources
   */
  destroy() {
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
      this.resizeObserver = null;
    }

    if (this.animationFrameId) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }

    if (this.domObserver) {
      this.domObserver.disconnect();
      this.domObserver = null;
    }

    if (this.domObserverRafId) {
      cancelAnimationFrame(this.domObserverRafId);
      this.domObserverRafId = null;
    }
    this.domObserverCheckScheduled = false;

    if (this.anchorAttrObserver) {
      this.anchorAttrObserver.disconnect();
      this.anchorAttrObserver = null;
    }

    if (this.boundScheduleUpdate) {
      // Note: removeEventListener must match the options object with capture: true that was used when adding.
      window.removeEventListener('scroll', this.boundScheduleUpdate, true);
      window.removeEventListener('resize', this.boundScheduleUpdate, true);
      if (window.visualViewport) {
        window.visualViewport.removeEventListener('scroll', this.boundScheduleUpdate, true);
        window.visualViewport.removeEventListener('resize', this.boundScheduleUpdate, true);
      }
      this.boundScheduleUpdate = null;
    }

    if (this.overlay && this.overlay.parentNode) {
      this.overlay.parentNode.removeChild(this.overlay);
    }

    if (this.styleElement && this.styleElement.parentNode) {
      this.styleElement.parentNode.removeChild(this.styleElement);
    }

    this.overlay = null;
    this.styleElement = null;
    this.lastPosition = { x: null, y: null, width: null, height: null };
    this.pendingUpdate = { modeCheck: false, rectUpdate: false };
    this.lastAnchorParent = null;
    this.wasConnected = null;
  }

  _hexToRgb(hex) {
    if (!hex.startsWith('#')) {
      const temp = document.createElement('div');
      temp.style.color = hex;
      document.body.appendChild(temp);
      const computed = getComputedStyle(temp).color;
      document.body.removeChild(temp);
      const match = computed.match(/(\d+),\s*(\d+),\s*(\d+)/);
      if (match) {
        return { r: parseInt(match[1]), g: parseInt(match[2]), b: parseInt(match[3]) };
      }
      return { r: 255, g: 0, b: 0 };
    }

    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
      r: parseInt(result[1], 16),
      g: parseInt(result[2], 16),
      b: parseInt(result[3], 16)
    } : { r: 255, g: 0, b: 0 };
  }

  _injectStyles() {
    // Clean up existing style element to prevent orphaned elements in DOM
    if (this.styleElement && this.styleElement.parentNode) {
      this.styleElement.parentNode.removeChild(this.styleElement);
    }

    const rgb = this._hexToRgb(this.color);
    const baseSpread = 8 * this.intensity;
    const midSpread = 16 * this.intensity;
    const largeSpread = 28 * this.intensity;
    const hugeSpread = 45 * this.intensity;

    const animationId = `aloha-glow-${this.uniqueId}`;

    this.styleElement = document.createElement('style');
    this.styleElement.textContent = `
      @keyframes ${animationId} {
        0%, 100% {
          box-shadow:
            0 0 ${baseSpread}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.8),
            0 0 ${midSpread}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.6),
            0 0 ${largeSpread}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.4),
            0 0 ${hugeSpread}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.2);
        }
        25% {
          box-shadow:
            0 0 ${baseSpread * 1.3}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.9),
            0 0 ${midSpread * 1.2}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.7),
            0 0 ${largeSpread * 1.1}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.5),
            0 0 ${hugeSpread * 1.2}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.25);
        }
        50% {
          box-shadow:
            0 0 ${baseSpread * 0.9}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.7),
            0 0 ${midSpread * 0.85}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.5),
            0 0 ${largeSpread * 0.9}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.35),
            0 0 ${hugeSpread * 0.8}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.15);
        }
        75% {
          box-shadow:
            0 0 ${baseSpread * 1.15}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.85),
            0 0 ${midSpread * 1.1}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.65),
            0 0 ${largeSpread * 1.05}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.45),
            0 0 ${hugeSpread * 1.1}px rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, 0.22);
        }
      }
    `;

    document.head.appendChild(this.styleElement);
    this.animationId = animationId;
  }

  _createOverlay() {
    if (this.overlay) {
      this.destroy();
      this._injectStyles();
    }

    const rgb = this._hexToRgb(this.color);

    this.overlay = document.createElement('div');
    this.overlay.dataset.alohaVideoOverlay = this.uniqueId;

    Object.assign(this.overlay.style, {
      // The overlay itself is always rooted in document.body to avoid site DOM conflicts.
      position: this.positionMode,
      pointerEvents: 'none',
      boxSizing: 'border-box',
      zIndex: '2147483647',
      margin: '0',
      padding: '0',
      background: 'transparent',
      border: `${this.borderWidth}px solid rgba(${rgb.r}, ${rgb.g}, ${rgb.b}, ${this.borderOpacity})`,
      borderRadius: '4px',
      animation: `${this.animationId} 3s ease-in-out infinite`,
      top: '0',
      left: '0',
      // Use top/left as a fixed origin (0,0); all movement is done via transform to avoid layout thrash and reduce jank.
      transform: 'translate3d(-99999px, -99999px, 0)',
      willChange: 'transform',
    });

    // Prefer document.body, but don't assume it's always present (very early injections).
    // Fallback to documentElement to avoid throwing if body isn't available yet.
    const root = document.body || document.documentElement;
    if (!root) return;
    root.appendChild(this.overlay);
  }

  _computePositionMode() {
    // If the video (or any ancestor) becomes fixed/sticky, we must use a fixed overlay
    // to stay glued to the viewport (PiP-like implementations).
    //
    // Otherwise prefer an absolute overlay in document coordinates to avoid scroll lag on
    // compositor-driven scrolling (the overlay will scroll with the document).
    let el = this.videoElement;
    let depth = 0;
    while (el && el.nodeType === Node.ELEMENT_NODE) {
      try {
        const pos = window.getComputedStyle(el).position;
        // Treat sticky as fixed-like: it can become effectively fixed during scroll.
        if (pos === 'fixed' || pos === 'sticky') return 'fixed';
      } catch (e) {
        // getComputedStyle can throw for some edge cases; fall back to 'absolute'
      }
      el = el.parentElement;
      depth++;
      // Safety guard for extremely deep DOM trees.
      if (depth > 32) break;
    }
    return 'absolute';
  }

  _hideOverlay() {
    if (!this.overlay) return;
    // Avoid leaving the overlay at the last known position if the anchor temporarily disappears.
    this.overlay.style.transform = 'translate3d(-99999px, -99999px, 0)';
    this.lastPosition = { x: null, y: null, width: null, height: null };
  }

  _updatePositionWithRect() {
    if (!this.videoElement || !this.overlay) return;
    if (!this.videoElement.isConnected) {
      this._hideOverlay();
      return;
    }

    // One layout read per update. Writes are minimized and go through transform (compositor-friendly).
    const rect = this.videoElement.getBoundingClientRect();
    const x = (this.positionMode === 'fixed' ? rect.left : rect.left + window.scrollX) + this.inset;
    const y = (this.positionMode === 'fixed' ? rect.top : rect.top + window.scrollY) + this.inset;
    const width = Math.max(0, rect.width - this.inset * 2);
    const height = Math.max(0, rect.height - this.inset * 2);

    if (x !== this.lastPosition.x || y !== this.lastPosition.y) {
      this.overlay.style.transform = `translate3d(${x}px, ${y}px, 0)`;
      this.lastPosition.x = x;
      this.lastPosition.y = y;
    }

    if (width !== this.lastPosition.width) {
      this.overlay.style.width = `${width}px`;
      this.lastPosition.width = width;
    }

    if (height !== this.lastPosition.height) {
      this.overlay.style.height = `${height}px`;
      this.lastPosition.height = height;
    }
  }

  _applyPositionMode(newMode) {
    if (newMode === this.positionMode) return false;
    this.positionMode = newMode;
    if (this.overlay) this.overlay.style.position = this.positionMode;
    // Coordinate space changes between 'absolute' and 'fixed' – reset caches.
    this.lastPosition = { x: null, y: null, width: null, height: null };
    return true;
  }

  _refreshAnchorAttributeObserver() {
    if (this.anchorAttrObserver) {
      this.anchorAttrObserver.disconnect();
      this.anchorAttrObserver = null;
    }
    if (!this.videoElement || !this.videoElement.isConnected) return;

    // Watch for CSS changes that can affect positioning (PiP/sticky, transforms, etc.).
    // We observe only 'style' and 'class' to keep this cheap.
    // Limit depth to avoid attaching too many observers in huge DOM trees.
    const nodesToWatch = [];
    let el = this.videoElement;
    let depth = 0;
    while (el && el.nodeType === Node.ELEMENT_NODE) {
      nodesToWatch.push(el);
      el = el.parentElement;
      depth++;
      if (depth > 16) break;
    }

    this.anchorAttrObserver = new MutationObserver(() => {
      // Attribute changes often imply position mode changes and/or geometry updates.
      this._scheduleUpdate({ modeCheck: true, rectUpdate: true });
    });

    nodesToWatch.forEach(node => {
      try {
        this.anchorAttrObserver.observe(node, {
          attributes: true,
          attributeFilter: ['style', 'class'],
        });
      } catch (e) {
        // Some nodes may reject observation in edge cases; ignore.
      }
    });
  }

  _startDomTracking() {
    if (!this.videoElement) return;
    this.lastAnchorParent = this.videoElement.parentElement;
    this.wasConnected = this.videoElement.isConnected;

    // Detect re-parenting / detach-attach cycles used by many PiP implementations.
    // We keep this observer lightweight: only childList changes, no global attribute tracking.
    this.domObserver = new MutationObserver(() => {
      // Coalesce potentially frequent DOM mutations to at most once per frame.
      if (this.domObserverCheckScheduled) return;
      this.domObserverCheckScheduled = true;
      this.domObserverRafId = requestAnimationFrame(() => {
        this.domObserverRafId = null;
        this.domObserverCheckScheduled = false;

        if (!this.videoElement) return;
        const isConnected = this.videoElement.isConnected;
        const parent = this.videoElement.parentElement;

        const connectionChanged = isConnected !== this.wasConnected;
        const parentChanged = parent !== this.lastAnchorParent;

        if (connectionChanged || parentChanged) {
          this.wasConnected = isConnected;
          this.lastAnchorParent = parent;
          // The anchor's ancestry likely changed; rebind attribute observers to the new chain.
          this._refreshAnchorAttributeObserver();
          // Force a full update in the new coordinate space.
          this._scheduleUpdate({ modeCheck: true, rectUpdate: true });
        }
      });
    });

    const root = document.documentElement || document.body;
    if (root) {
      this.domObserver.observe(root, { childList: true, subtree: true });
    }
  }

  _scheduleUpdate({ modeCheck = false, rectUpdate = false } = {}) {
    // Coalesce multiple triggers (scroll/resize/mutations) into a single rAF update.
    // This prevents back-to-back layout reads during bursty DOM mutations.
    this.pendingUpdate.modeCheck = this.pendingUpdate.modeCheck || modeCheck;
    this.pendingUpdate.rectUpdate = this.pendingUpdate.rectUpdate || rectUpdate;
    if (this.animationFrameId) return;
    this.animationFrameId = requestAnimationFrame(() => {
      this.animationFrameId = null;

      const shouldModeCheck = this.pendingUpdate.modeCheck;
      let shouldRectUpdate = this.pendingUpdate.rectUpdate;
      this.pendingUpdate.modeCheck = false;
      this.pendingUpdate.rectUpdate = false;

      let modeChanged = false;
      if (shouldModeCheck) {
        const newMode = this._computePositionMode();
        modeChanged = this._applyPositionMode(newMode);
      }

      // If the element is fixed/sticky, we must update on scroll to stay attached.
      if (this.positionMode === 'fixed') shouldRectUpdate = true;
      if (modeChanged) shouldRectUpdate = true;

      if (shouldRectUpdate) {
        this._updatePositionWithRect();
      }
    });
  }

  _startTracking() {
    // Track video resizes (layout changes, responsive player UI, etc.).
    this.resizeObserver = new ResizeObserver(() => this._scheduleUpdate({ modeCheck: true, rectUpdate: true }));
    this.resizeObserver.observe(this.videoElement);

    /**
     * Scroll handler.
     *
     * - For the main document scroll we prefer rectUpdate: false in 'absolute' mode to avoid scroll lag
     *   on compositor-driven scrolling (the overlay scrolls naturally with the document).
     * - For nested scroll containers, the document doesn't move, but the element's viewport rect changes,
     *   so we must update the overlay geometry to keep it attached.
     *
     * @param {Event} event
     */
    this.boundScheduleUpdate = (event) => {
      const type = event && event.type;
      const target = event && event.target;
      const isScroll = type === 'scroll';
      const isDocumentScrollTarget =
        target === document ||
        target === document.documentElement ||
        target === document.body;
      const isNestedScroll = isScroll && target && !isDocumentScrollTarget;

      const shouldRectUpdate =
        // Nested scroll containers move the element without moving the document.
        isNestedScroll ||
        // visualViewport events should be treated as geometry changes (mobile URL bar/zoom/etc.).
        (target === window.visualViewport);

      this._scheduleUpdate({ modeCheck: true, rectUpdate: shouldRectUpdate });
    };

    // Capture is important because "scroll" doesn't bubble.
    // We listen to scroll even in 'absolute' mode to detect transitions to 'fixed' (PiP-like behavior).
    window.addEventListener('scroll', this.boundScheduleUpdate, { passive: true, capture: true });
    window.addEventListener('resize', this.boundScheduleUpdate, { passive: true, capture: true });
    if (window.visualViewport) {
      // visualViewport events help on mobile when the viewport changes due to URL bar, zoom, etc.
      window.visualViewport.addEventListener('scroll', this.boundScheduleUpdate, { passive: true, capture: true });
      window.visualViewport.addEventListener('resize', this.boundScheduleUpdate, { passive: true, capture: true });
    }

    // Cover PiP-like portals and CSS-driven mode changes.
    this._startDomTracking();
    this._refreshAnchorAttributeObserver();
    this._scheduleUpdate({ modeCheck: true, rectUpdate: true });
  }
}

//export default VideoFrameOverlay;
