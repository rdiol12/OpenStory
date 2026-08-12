# Monster Life / Monster Battle — wire protocol

Authored by the **client** side (OpenStory). The client is the reference implementation:
when the two disagree, the client is correct.

These are **custom** features. Monster Life and Monster Battle are post-Big-Bang KMS content
with no v83 protocol. Nothing here is canonical Nexon layout — do not substitute opcodes or
payloads found elsewhere.

## Conventions

- Little-endian, matching every other v83 packet.
- `byte` = int8, `short` = int16, `int` = int32, `long` = int64.
- `string` = `short length` followed by `length` raw bytes (the standard Maple string).
- Every packet begins with its opcode as `short`, then a `byte mode`.
- All coordinates are farm-local pixels, origin top-left.
- Card ids are `FamiliarCard.img` node ids (item ids, 9960000+).

## Opcode allocation

Chosen to clear every existing range (Cosmic send peaks at 0x166, Cosmic recv at 0xFD).

**Server-to-client opcodes must stay below 500.** The client dispatches inbound packets
through a fixed array, `PacketSwitch::NUM_HANDLERS = 500`, and a larger opcode fails a
static assert at compile time. Client-to-server has no such ceiling: Cosmic sizes its
handler table to `maxRecvOp + 1` at runtime.

| direction | name | value |
|---|---|---|
| client → server | `MLIFE_OP` | `0x400` (1024) |
| client → server | `MBATTLE_OP` | `0x401` (1025) |
| server → client | `MLIFE_UPDATE` | `0x190` (400) |
| server → client | `MBATTLE_UPDATE` | `0x191` (401) |

Later phases add modes to these opcodes. They do **not** add new opcodes.

---

## Monster Life — phase 1

Scope: a farm exists, persists, can be entered, and monsters can be placed/moved/removed.
No gacha, breeding, harvest, buffs or visiting yet.

### `MLIFE_OP` (client → server) `0x400`

| mode | name | payload |
|---|---|---|
| `0x00` | ENTER | `int ownerCharId` (0 = own farm) |
| `0x01` | LEAVE | — |
| `0x02` | PLACE | `int cardId; short x; short y` |
| `0x03` | MOVE | `int slotId; short x; short y` |
| `0x04` | REMOVE | `int slotId` |

### `MLIFE_UPDATE` (server → client) `0x190`

**`0x00` FARM_STATE** — full snapshot, sent in reply to ENTER.

```
int   ownerCharId
string ownerName
byte  farmLevel
int   farmExp
byte  editable        1 = viewer may place/move/remove
short monsterCount
  repeated monsterCount times:
    int   slotId      server-assigned, stable for the session
    int   cardId
    short x
    short y
    byte  level
    int   exp
```

**`0x01` PLACED** — `int slotId; int cardId; short x; short y`
**`0x02` MOVED** — `int slotId; short x; short y`
**`0x03` REMOVED** — `int slotId`
**`0x04` ERROR** — `byte code`

Error codes: `0` unknown, `1` farm full, `2` card not owned, `3` not editable,
`4` invalid position, `5` no such slot.

Rules the server owns: slot id assignment, farm capacity, ownership of the card, and whether
`editable` is set. The client never assigns a slot id and never assumes a placement succeeded —
it waits for `PLACED`.

---

## Monster Battle — phase 1

Scope: browse the collection and set a team. No capture, no battles, no rewards.

### `MBATTLE_OP` (client → server) `0x401`

| mode | name | payload |
|---|---|---|
| `0x00` | OPEN_COLLECTION | — |
| `0x01` | SET_TEAM | `byte count; repeated count: int cardId` |

`count` is 0..6.

### `MBATTLE_UPDATE` (server → client) `0x191`

**`0x00` COLLECTION** — full snapshot, sent in reply to OPEN_COLLECTION.

```
short unitCount
  repeated unitCount times:
    int   cardId
    byte  level
    int   exp
    short owned        how many copies
short teamCount
  repeated teamCount times:
    int   cardId
```

**`0x01` TEAM** — `byte count; repeated count: int cardId` (confirmation after SET_TEAM)
**`0x02` ERROR** — `byte code`

Error codes: `0` unknown, `1` unit not owned, `2` team too large, `3` duplicate unit.

The server validates every team member against the player's collection and replies with the
team it actually stored, which may differ from what was requested. The client renders the
server's version, never its own optimistic copy.

---

## Phase 2+ (reserved, not yet specified)

Monster Life: gacha (`BoxBeforeUI`/`BoxResultUI`), `lockerUI`, harvest, character buffs,
`Fusion`/`FusionResult`, visiting + `farmChat` + `visitorReward`.
Monster Battle: `MonsterBattleCapture`, server-authoritative battle resolution, rewards.

Battle outcomes are computed server-side only. Any client-supplied result is to be treated as
hostile input.
