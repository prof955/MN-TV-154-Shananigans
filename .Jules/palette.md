## 2026-10-12 - Web Serial Custom Component Fallbacks & Keyboard Focus

**Learning:** Web components like `<esp-web-install-button>` wrap browser Web Serial / Web USB APIs that are only available in Chromium browsers over HTTPS/localhost. Providing explicit `slot="unsupported"` and `slot="not-allowed"` elements ensures users on unsupported browsers (Firefox/Safari/mobile) or non-HTTPS connections receive clear actionable guidance rather than a missing/silent button UI.

**Action:** When working with web components wrapping browser hardware capabilities (WebSerial, WebUSB, WebBluetooth), always inspect slot API definitions and provide themed fallback notices and explicit `:focus-visible` styles for link/button accessibility.
