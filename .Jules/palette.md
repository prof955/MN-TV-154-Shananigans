## 2025-05-18 - WebSerial Hardware Flasher Accessibility & Compatibility
**Learning:** WebSerial components (like `esp-web-install-button`) silently fail or hide without explanation on unsupported browsers (Firefox, Safari, mobile).
**Action:** Always provide runtime feature detection (`'serial' in navigator`) with an accessible `role="alert"` fallback notification and keyboard focus states (`:focus-visible`).
