# Requirements: EIDAuthentication v1.7

**Defined:** 2026-02-24
**Core Value:** A clean, maintainable, and secure codebase with zero static analysis issues, leveraging modern C++23 features while preserving all existing authentication functionality.

## v1.7 Requirements

Requirements for v1.7 UI/UX Enhancement milestone. Each maps to roadmap phases.

### Smart Card Configuration UI

- [x] **UIUX-01**: User can Configure Smart Card without P12 import option displayed
- [ ] **UIUX-02**: User sees progress popup during card flashing operation
- [ ] **UIUX-03**: User views enhanced certificate authority information in Selected Authority info box

## Out of Scope

Explicitly excluded from v1.7:

| Feature | Reason |
|---------|--------|
| Cancel button on progress popup | Requires thread cancellation signaling, defer to future milestone |
| Progress stages text | Requires refactoring card operations into discrete steps |
| Changes to Credential Provider UI | v1.7 focuses on Configuration Wizard only |
| Certificate enrollment/creation UI | Out of scope for this milestone |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| UIUX-01 | Phase 51 | Complete |
| UIUX-02 | Phase 53 | Pending |
| UIUX-03 | Phase 52 | Pending |

**Coverage:**
- v1.7 requirements: 3 total
- Mapped to phases: 3
- Unmapped: 0

---

*Requirements defined: 2026-02-24*
*Last updated: 2026-02-24 after Phase 51 completion*
