# Audit v2 Runtime Findings

Generated: 2026-03-22T20:10:51+00:00

Triage these into `docs/behavioral_worklist.md` with appropriate severity.

## error_codes

- **MISSING** `error_codes.eCannotCreateChatSocket`: original=`30042`
- **MISSING** `error_codes.eCannotLookupDomainName`: original=`30043`
- **MISSING** `error_codes.eChatAlreadyConnected`: original=`30049`
- **MISSING** `error_codes.eChatAlreadyListening`: original=`30047`
- **MISSING** `error_codes.eChatIDNotFound`: original=`30048`
- **MISSING** `error_codes.eChatPersonNotFound`: original=`30045`
- **EXTRA** `error_codes.eFileNotOpened`: mushkin=`30076`
- **EXTRA** `error_codes.eInvalidColourName`: mushkin=`30077`
- **MISSING** `error_codes.eNoChatConnections`: original=`30044`
- **EXTRA** `error_codes.eNoSuchNotepad`: mushkin=`30075`
- **MISMATCH** `error_codes.ePluginCouldNotSaveState`: original=`30037` mushkin=`30038`

## getinfo

- **MISMATCH** `getinfo.21`: original=`#` mushkin=``
- **MISMATCH** `getinfo.271`: original=`-1` mushkin=`4278190080`
- **MISMATCH** `getinfo.37`: original=`` mushkin=`say `
- **MISMATCH** `getinfo.53`: original=`` mushkin=`Ready`

## lua_environment

- **MISMATCH** `lua_environment.jit_version`: original=`LuaJIT 2.1.0-beta3` mushkin=`LuaJIT 2.1.1765228720`
- **MISMATCH** `lua_environment.package_loaders_count`: original=`4` mushkin=`5`

