## 2024-10-13 - WebSerial Web Component Fallbacks
**Learning:** `esp-web-install-button` hides silently in browsers without WebSerial (Safari/Firefox/mobile) if fallback slots are omitted, leaving users with prompt text but no action element.
**Action:** Always provide `slot="unsupported"` and `slot="not-allowed"` fallback content when embedding WebSerial tools.
