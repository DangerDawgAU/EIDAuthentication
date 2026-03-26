# EIDMigrate - Design Decisions Summary

**Date:** 2026-03-23

---

## All Decisions Made

| Question | Decision | Rationale/Notes |
|----------|----------|-----------------|
| **Project Location** | Top-level `EIDMigrate/` | New folder at root, alongside existing components |
| **JSON Library** | jsoncpp | Mature, widely used, header-only option available |
| **Build Architecture** | x64 only | Simplifies build, matches modern deployment |
| **DPAPI Handling** | Silent skip + log | DPAPI credentials can't migrate, log for review |
| **Account Creation** | Preserve source structure | Export groups, recreate on import if missing |
| **Password Settings** | Set "Password never expires" | Appropriate for smart card-only auth |
| **Passphrase Input** | Prompt + `-password` flag | Scriptable but secure by default |
| **File Permissions** | Default inherited | Simpler, relies on folder security |
| **Deployment** | Installed with main package | Part of EIDAuthentication installer |
| **Logging Level** | Multiple verbosity levels | Default, -v, -vv for increasing detail |
| **Import Failures** | Rollback all on error | Atomic import - partial state is dangerous |
| **Cert Validation** | Warning only | Don't fail migration, warn about trust issues |
| **Group Export** | Include groups | Export groups and memberships, create on import |

---

## Updated Implementation Scope

### New Requirements Added

1. **Group Membership Export/Import**
   - Export local groups with their memberships
   - Create groups on target if they don't exist
   - Map users to their original groups
   - Handle built-in groups (Administrators, Users, etc.)

2. **Expanded JSON Schema**
   - Add `groups` array to export format
   - Each group has: name, description, members (array of usernames)
   - Built-in groups are referenced, not created

### Updated File Format

```json
{
  "version": 1,
  "exportDate": "2026-03-23T10:30:00Z",
  "sourceMachine": "OLDPC-01",
  "credentials": [...],
  "groups": [
    {
      "name": "CustomAdmins",
      "description": "Custom administrator group",
      "members": ["jsmith", "jdoe"],
      "isBuiltin": false
    }
  ],
  "statistics": {...}
}
```

---

## Updated Effort Estimate

| Phase | Original | New | Notes |
|-------|----------|-----|-------|
| 1. Project Setup | 4h | 4h | No change |
| 2. LSA IPC | 12h | 12h | No change |
| 3. Crypto Library | 16h | 16h | No change |
| 4. File Format | 12h | 16h | +4h for group schema |
| 5. Export | 16h | 20h | +4h for group enumeration |
| 6. Import | 20h | 26h | +6h for group creation/membership |
| 7. User Management | 8h | 12h | +4h for group management functions |
| 8. CLI & Logging | 8h | 8h | No change |
| 9. Testing | 24h | 28h | +4h for group testing |
| **Total** | **120h** | **142h** | +22h for group support |

---

## Next Steps

Ready to begin implementation with these decisions. Start with:
1. Phase 1: Project Setup (create EIDMigrate/ folder, VS project)
2. Update JSON schema to include groups
3. Implement group enumeration/export/import

Proceed?
