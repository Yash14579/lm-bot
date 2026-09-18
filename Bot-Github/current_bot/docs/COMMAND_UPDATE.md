# Command Update

This update was built against the supplied `dump.cs` and `il2cpp.h` reference.

## Verified commands added/fixed

- `!pos` / `!position`
- `!shield`
- `!shield deploy`
- `!hospital`
- `!wounded`
- `!heal`
- `!heal instant`
- `!heal finish`
- `!heal cancel`
- `!adminbag`
- `!help`

The configured command prefix still works, and `!` is also accepted as a command prefix.

## Healing

The client dump identifies these protocol IDs:

- `_MSG_REQUEST_HEALINGTROOP` = 2426
- `_MSG_REQUEST_INSTANTHEALING` = 2431
- `_MSG_REQUEST_FINISHHEALING` = 2433
- `_MSG_REQUEST_CANCELHEALING` = 2429

The 16-slot healing payload is written as 16 `uint32` values. The command implementation builds those slots from the bot's loaded T1-T4 wounded-troop state.

`!heal` and `!heal instant` operate on all loaded T1-T4 wounded slots. T5 is not silently fabricated into the 16-slot packet.

## Shield

Shield status uses the existing `ShieldInfo` state populated by `RecvIBuffInfo`.

`!shield deploy` uses the configured shield priority when present. If no priority is configured, it falls back to the longest available shield first.

## Position

`!pos` reports kingdom, X/Y and raw zone/point data from the loaded player state.

## Important

The supplied bot source does not currently contain verified packet builders for every command in the external Guild Bank command list. Those commands have therefore **not** been given guessed packet layouts. The next phase should map each remaining command from `dump.cs` to the corresponding packet fields in `il2cpp.h` before sending anything to the game server.
