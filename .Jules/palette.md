# Palette's Journal

## 2025-05-10 - ESP Web Tools WebSerial Fallback & Keyboard Accessibility
**Learning:** ESP Web Tools `<esp-web-install-button>` defaults to unstyled/hidden fallback elements when WebSerial is unsupported (Firefox, Safari, mobile) or in non-HTTPS contexts, leaving users without clear next steps.
**Action:** Always provide explicit `slot="activate"`, `slot="unsupported"`, and `slot="not-allowed"` custom elements with accessible ARIA labels, visual feedback, and `:focus-visible` states.
