## 2025-05-18 - Web Serial Browser Compatibility & A11y in Web Flasher Interfaces
**Learning:** Web Serial API feature support (`'serial' in navigator`) is absent in WebKit/Safari and Firefox. Users landing on Web Serial flashers using unsupported browsers often encounter dead interactive buttons or silent failures without feedback.
**Action:** Always include inline Web Serial feature detection with `role="alert"` / `aria-live="polite"` fallback notice banners, and pair custom web components with clear focus-visible outline indicators for keyboard accessibility.
