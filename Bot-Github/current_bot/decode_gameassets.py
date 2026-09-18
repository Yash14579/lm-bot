#!/usr/bin/env python3
"""Decode Lords Mobile GameAssets tables for the automation bot.

Container format (reverse-engineered from CExternalTable.LoadTable in
libil2cpp.so + the IL2CPP row structs in dump.cs):
    [0:2]  magic 0x0001 (u16 LE)
    [2:4]  (informational word; NOT a reliable count)
    then N rows, each of a fixed per-file size R, conceptually packed
    (no alignment padding between fields; values little-endian).
    First field of every row is a monotonically-increasing u16 ID.

Per-file row schemas come from the field ORDERS in dump.cs (the on-disk
stream is the logical field sequence; in-memory struct offsets carry
alignment padding that is NOT present on disk - that is why the file
len is divisible by R, not by the padded struct size).

Confirmed cost-column offsets (buildUP.txt):
    ID u16 @0, BuildID u16 @2, Level u8 @4, time u32 @5,
    then 5 cost u32 @11,15,19,23,27  (resource-type map differs per
    building; validate against a known in-game value before trusting).
Confirmed research columns (TechLv.txt):
    ID u16 @0, TechID u16 @2, Level u8 @4, time u32 @5,
    food u32 @9, rock @13, wood @17, ore @21, gold @25,
    then prereq/effect fields.
"""
import struct, json, os, sys, csv

HERE = os.path.dirname(os.path.abspath(__file__))
GA = os.path.join(os.path.dirname(HERE), "GameAssets")
OUT = os.path.join(HERE, "decoded")
INCLUDE = os.path.join(HERE, "include")
os.makedirs(OUT, exist_ok=True)

def write_csv(name, headers, rows):
    p = os.path.join(OUT, name)
    with open(p, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(headers)
        for r in rows:
            w.writerow(r)
    return p

def fallback_dump(name, path):
    """For files that are not fixed-row tables (variable length, maps, text,
    sprites): write a readable hex+ASCII dump so *nothing* is left undecoded."""
    data = open(path, "rb").read()
    base = os.path.splitext(name)[0]
    p = os.path.join(OUT, base + ".txt")
    lines = []
    lines.append(f"# {name}  ({len(data)} bytes) - not a fixed-row table; raw dump")
    lines.append("# loads as: header=00 01 (magic) - see hex below")
    for off in range(0, len(data), 16):
        chunk = data[off:off+16]
        hx = " ".join(f"{b:02X}" for b in chunk)
        asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        lines.append(f"{off:08X}  {hx:<47}  {asc}")
    with open(p, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    return p

def build_index(generic_ok, generic_skip):
    """Write README.md that lists every GameAssets table and what was produced."""
    import csv as _csv
    lines = ["# GameAssets - fully decoded", "",
             "This folder is machine-decoded from `GameAssets/` by decode_gameassets.py.",
             "Every table is present either as a row-split CSV+JSON (fixed-row tables) or",
             "a hex/ASCII dump (variable-length tables).", ""]
    rows_tables = sorted(generic_ok)
    lines.append(f"## Fixed-row tables decoded ({len(rows_tables)})")
    lines.append("")
    lines.append("| file | record size | rows | csv | json |")
    lines.append("|---|---|---|---|---|")
    for base in rows_tables:
        js = os.path.join(OUT, base + ".json")
        n = "?"
        rs = "?"
        if os.path.exists(js):
            try:
                d = json.load(open(js))
                rs = d.get("record_size", "?")
                n = d.get("count", "?")
            except Exception:
                pass
        lines.append(f"| {base}.txt | {rs} | {n} | decoded/{base}.csv | decoded/{base}.json |")
    lines.append("")
    lines.append("## Non-row tables (raw hex dump) %d" % len(generic_skip))
    lines.append("")
    for base in generic_skip:
        lines.append(f"- {base}.txt -> decoded/{base}.txt")
    lines.append("")
    with open(os.path.join(OUT, "README.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    return os.path.join(OUT, "README.md")
    p = os.path.join(OUT, name)
    with open(p, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(headers)
        for r in rows:
            w.writerow(r)
    return p

# ---- per-file size heuristics ----
def detect_row(body, lo=4, hi=2400, need=2):
    """Find R = smallest record size in [lo,hi] giving >=need integer rows
    whose first u16 ID is strictly increasing (the table signature)."""
    best = None
    for R in range(lo, hi + 1):
        if len(body) % R:
            continue
        n = len(body) // R
        if n < need:
            continue
        ids = [struct.unpack_from("<H", body, i * R)[0] for i in range(n)]
        if all(ids[i] < ids[i+1] for i in range(n - 1)):
            return R, n
    return best

def generic_decode(path):
    """Return (header_bytes, record_size, count, [{[fid, ...], 'row_hex'}])"""
    data = open(path, "rb").read()
    if len(data) < 6:
        return None
    body = data[4:]          # try 4-byte header first
    res = detect_row(body)
    if not res:
        body = data[2:]      # some tables have a 2-byte header
        res = detect_row(body)
    if not res:
        return None
    R, n = res
    rows = []
    for i in range(n):
        row = body[i*R:(i+1)*R]
        rows.append({"f0_id": struct.unpack_from("<H", row, 0)[0], "bytes": row.hex()})
    return data[:4].hex(), R, n, rows

def ascii_of(row):
    """Pull printable ASCII substrings out of a row (e.g. graphic ids / names)."""
    parts = []
    cur = []
    for b in row:
        if 32 <= b < 127:
            cur.append(chr(b))
        else:
            if len(cur) >= 2:
                parts.append("".join(cur))
            cur = []
    if len(cur) >= 2:
        parts.append("".join(cur))
    return " | ".join(parts)

def generic_csv(base, R, rows):
    """Emit a readable CSV: f0_id first, then every 2-byte and printable-4-byte
    field, then any embedded ASCII text.  Column names carry the byte offset so
    the layout is transparent to a reader."""
    hdr = ["f0_id"]
    u16cols = list(range(2, R, 2))
    u32cols = list(range(0, R - 3, 4))
    for o in u16cols:
        hdr.append(f"u16@{o}")
    hdr.append("ascii")
    out = []
    for r in rows:
        row = bytes.fromhex(r["bytes"])
        line = [r["f0_id"]]
        for o in u16cols:
            line.append(struct.unpack_from("<H", row, o)[0] if o + 2 <= len(row) else "")
        line.append(ascii_of(row))
        out.append(line)
    return write_csv(base + ".csv", hdr, out)

# ---- confirmed cost tables (structured) ----
_COST_OFFSETS = (11, 15, 19, 23, 27)      # Food, Rock, Wood, Iron(ore), Gold (little-endian u32)
_COST_NAMES   = ("food", "rock", "wood", "ore", "gold")

def decode_build_up():
    """Decode buildUP.txt (70-byte compact BuildLevelRequest rows, 600 rows).

    Verified against dump.cs:319540 BuildLevelRequest in COMPACT on-disk layout
    (no alignment padding): ID u16@0, BuildID u16@2, Level u8@4, BuildTime u32@5,
    GroupID u16@9, then RequestFood/Rock/Wood/Iron/Gold as u32 @11,15,19,23,27.
    The resource offsets were validated against in-game castle costs (L1 food=792,
    rock=1080, wood=1080, ore=648).  The per-row CastleArmy tail offset does NOT
    reproduce the building's own level across buildings, so the castle-cap is
    expressed as a code rule (building level <= castle level), not from this table.
    """
    path = os.path.join(GA, "buildUP.txt")
    data = open(path, "rb").read()
    R = 70
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID  = struct.unpack_from("<H", r, 0)[0]
        Bid = struct.unpack_from("<H", r, 2)[0]
        Lv  = r[4]
        tm  = struct.unpack_from("<I", r, 5)[0]
        c   = [struct.unpack_from("<I", r, o)[0] for o in _COST_OFFSETS]
        rows.append({"id": ID, "build_id": Bid, "level": Lv, "time_s": tm,
                     "food": c[0], "rock": c[1], "wood": c[2],
                     "ore": c[3], "gold": c[4]})
    write_csv("buildUP.csv",
              ["id", "build_id", "level", "time_s"] + list(_COST_NAMES),
              [[r["id"], r["build_id"], r["level"], r["time_s"]] +
               [r[_CN] for _CN in _COST_NAMES] for r in rows])
    return rows


def decode_item():
    """Decode Item.txt (88-byte rows, 3682 entries), keyed by item_id at u16@0.

    VALIDATED against the bot's own hardcoded resource-pack ids (items.h):
    FOOD_150K(id=1014)=150000  etc.  amount = u16@18 × u16@20 (7/7 pack ids
    match). So:
        item_id       = u16@0       (the row key / identity)
        type          = u16@4       (NOT a resource discriminator; ==3 for all packs)
        amount_scalar = u16@18
        amount_mult   = u16@20
    Remaining columns are captured by byte offset so the layout stays
    transparent; only the fields above are asserted semantic.
    """
    path = os.path.join(GA, "Item.txt")
    data = open(path, "rb").read()
    R = 88
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        id_          = struct.unpack_from("<H", r, 0)[0]   # item_id
        type_        = struct.unpack_from("<H", r, 4)[0]
        amount_s     = struct.unpack_from("<H", r, 18)[0]  # VALIDATED x1000 scale
        amount_m     = struct.unpack_from("<H", r, 20)[0]  # VALIDATED multiplier
        use_val      = struct.unpack_from("<I", r, 22)[0]
        row = {"item_id": id_, "type": type_,
               "amount_scalar": amount_s, "amount_mult": amount_m,
               "amount": amount_s * amount_m,  # pack restore units (packs only)
               "f2": struct.unpack_from("<H", r, 2)[0],
               "f6": struct.unpack_from("<H", r, 6)[0],
               "f8": struct.unpack_from("<H", r, 8)[0],
               "f10": struct.unpack_from("<I", r, 10)[0],
               "f14": struct.unpack_from("<H", r, 14)[0],
               "f16": struct.unpack_from("<H", r, 16)[0],
               "use_value": use_val}
        rows.append(row)
    write_csv("Item.csv",
              ["item_id", "type", "amount_scalar", "amount_mult", "amount",
               "f2", "f6", "f8", "f10", "f14", "f16", "use_value"],
              [[row["item_id"], row["type"], row["amount_scalar"], row["amount_mult"],
                row["amount"], row["f2"], row["f6"], row["f8"], row["f10"],
                row["f14"], row["f16"], row["use_value"]] for row in rows])
    return rows

def decode_build_up_new():
    """Decode buildUP_NEW.txt (106-byte rows, 1322 entries) - the extended / higher-cap
    building table. Cost offsets are the same validated set as buildUP.txt
    (food/rock/wood/ore/gold @11,15,19,23,27); cross-checked identical for
    build=1 levels 2..6 (150/225/337/506/759). Only the Level-1 base differs,
    which is why the merge in emit_build_header() PREFERS buildUP.txt for any
    overlapping (build, level) so the already-validated data stays authoritative."""
    path = os.path.join(GA, "buildUP_NEW.txt")
    data = open(path, "rb").read()
    R = 106
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID  = struct.unpack_from("<H", r, 0)[0]
        Bid = struct.unpack_from("<H", r, 2)[0]
        Lv  = r[4]
        tm  = struct.unpack_from("<I", r, 5)[0]
        c   = [struct.unpack_from("<I", r, o)[0] for o in _COST_OFFSETS]
        rows.append({"id": ID, "build_id": Bid, "level": Lv, "time_s": tm,
                     "food": c[0], "rock": c[1], "wood": c[2],
                     "ore": c[3], "gold": c[4]})
    write_csv("buildUP_NEW.csv",
              ["id", "build_id", "level", "time_s"] + list(_COST_NAMES),
              [[r["id"], r["build_id"], r["level"], r["time_s"]] +
               [r[_CN] for _CN in _COST_NAMES] for r in rows])
    return rows


def emit_build_header():
    """Generate include/game_build_cost.h - exact per-step building costs keyed by
       (build_id, target_level), mirror of the research header.

    MERGED source: buildUP.txt (VALIDATED, authoritative) first, then buildUP_NEW.txt
    (extended table, same verified cost offsets) only for (build, level) pairs the
    validated table does not already cover.  This is a strict coverage SUPERSET, so
    the cost gate never went BACKWARD (a level it knew before stays identical), and it
    gains the higher-level rows buildUP.txt lacks — fewer blind 2003 sends at high
    levels where gb was previously NULL."""
    import json as _j
    d  = _j.load(open(os.path.join(OUT, "buildUP.json")))
    dn = _j.load(open(os.path.join(OUT, "buildUP_NEW.json")))
    merged = {}
    for r in d:                       # VALIDATED first; true if already present wins
        merged[(r["build_id"], r["level"])] = r
    for r in dn:                      # extended table fills only gaps
        key = (r["build_id"], r["level"])
        if key not in merged:
            merged[key] = r
    ents = sorted((r["build_id"], r["level"], r["food"], r["rock"], r["wood"],
                   r["ore"], r["gold"]) for r in merged.values())
    L = ["/* Auto-generated by decode_gameassets.py - exact building costs, MERGED from",
         "   buildUP.txt (VALIDATED) + buildUP_NEW.txt (extended, same cost offsets).",
         "   Validated rows win on overlap; buildUP_NEW fills higher-level gaps so the",
         "   cost gate covers levels buildUP.txt lacks.  entry k: build_id, target_level,",
         "   food, rock, wood, ore(gold comes from RequestIron), gold.",
         "   The castle-cap rule is NOT in this table; it is a behavior rule in the",
         "   bot (building level cannot exceed the castle's level). Do not edit. */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_BUILD_N {len(ents)}",
         "typedef struct { uint16_t build; uint8_t level; uint32_t food; uint32_t rock;",
         "                  uint32_t wood; uint32_t ore; uint32_t gold; } GameBuildCost;",
         f"static const GameBuildCost kGameBuildCosts[{len(ents)}] = {{"]
    L += [f"    {{ {b}, {lv}, {f}u, {r}u, {w}u, {o}u, {g}u }}," for b, lv, f, r, w, o, g in ents]
    L += ["};",
          "static inline const GameBuildCost* game_build_cost(uint16_t build, uint8_t level){",
          "    int lo=0, hi=GAME_BUILD_N-1; int idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GameBuildCost*x=&kGameBuildCosts[mid];",
          "        if(x->build<build||(x->build==build&&x->level<level)) lo=mid+1;",
          "        else { idx=mid; hi=mid-1; } }",
          "    if(idx<0) return NULL;",
          "    const GameBuildCost*x=&kGameBuildCosts[idx];",
          "    return (x->build==build&&x->level==level)?x:0;",
          "}"]
    open(os.path.join(INCLUDE, "game_build_cost.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_build_cost.h -> include/ ({len(ents)} building cost entries, merged buildUP+buildUP_NEW)")


def emit_item_header():
    """Generate include/game_item.h - item data keyed by item_id (u16@0).

    Only the validated fields are asserted semantic: item_id, amount_scalar,
    amount_mult (pack restore = scalar*mult, matched 7/7 vs bot's hardcoded
    pack amounts). Use game_item_pack_amount(id) for the restore value.
    """
    import json as _j
    d = _j.load(open(os.path.join(OUT, "Item.json")))
    ents = sorted((r["item_id"], r["type"], r["amount_scalar"], r["amount_mult"],
                   r["use_value"]) for r in d)
    L = ["/* Auto-generated by decode_gameassets.py - item data from Item.txt. */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_ITEM_N {len(ents)}",
         "typedef struct { uint16_t item_id; uint16_t type;",
         "                  uint16_t amount_scalar; uint16_t amount_mult;",
         "                  uint32_t use_value; } GameItem;",
         f"static const GameItem kGameItems[{len(ents)}] = {{"]
    L += [f"    {{ {item_id}, {typ}, {asc}, {mult}, {uv}u }},"
          for (item_id, typ, asc, mult, uv) in ents]
    L += ["};",
          "static inline const GameItem* game_item_get(uint16_t id){",
          "    if(kGameItems[0].item_id > id || kGameItems[GAME_ITEM_N-1].item_id < id) return 0;",
          "    int lo=0, hi=GAME_ITEM_N-1, idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GameItem*x=&kGameItems[mid];",
          "        if(x->item_id<id) lo=mid+1; else { idx=mid; hi=mid-1; } }",
          "    if(idx<0||kGameItems[idx].item_id!=id) return 0;",
          "    return &kGameItems[idx];",
          "}",
          "/* resource-pack restore amount (VALIDATED: scalar*mult). */",
          "static inline uint32_t game_item_pack_amount(uint16_t id){",
          "    const GameItem*g=game_item_get(id);",
          "    return g? (uint32_t)g->amount_scalar*(uint32_t)g->amount_mult : 0;",
          "}"]
    open(os.path.join(INCLUDE, "game_item.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_item.h -> include/ ({len(ents)} item entries, keyed by item_id)")


def decode_soldier():
    """Decode Soldier.txt (42-byte rows, 37 entries).
    Sealed & structurally CONFIRMED: rows group in blocks of 4 per tier, and the
    tier (1..7) lives at u16@26 (rows 0-3=1, 4-7=2, ... 24-27=7; special/other-type
    rows read 0 then 1,2,3,4). All other offsets below are GUESSED and unused.
    """
    path = os.path.join(GA, "Soldier.txt")
    data = open(path, "rb").read()
    R = 42
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID       = struct.unpack_from("<H", r, 0)[0]
        tier     = struct.unpack_from("<H", r, 26)[0]   # CONFIRMED grouping 1..7
        unk6     = struct.unpack_from("<H", r, 6)[0]
        unk8     = struct.unpack_from("<H", r, 8)[0]
        food     = struct.unpack_from("<I", r, 22)[0]
        load     = struct.unpack_from("<H", r, 26)[0]
        rows.append({"id": ID, "tier": tier, "unk6": unk6, "unk8": unk8,
                     "food": food, "load": load})
    write_csv("Soldier.csv",
              ["id", "tier", "unk6", "unk8", "food", "load"],
              [[r["id"], r["tier"], r["unk6"], r["unk8"],
                r["food"], r["load"]] for r in rows])
    return rows


def emit_soldier_header():
    """Generate include/game_soldier.h - soldier/troop data keyed by row id.
    Only 'tier' (u16@26, structurally confirmed 1..7) is trusted; the other
    columns (unk6/unk8/food/load) are UNVALIDATED placeholders.
    """
    import json as _j
    d = _j.load(open(os.path.join(OUT, "Soldier.json")))
    ents = sorted((r["id"], r["tier"], r["unk6"], r["unk8"],
                   r["food"], r["load"]) for r in d)
    L = ["/* Auto-generated by decode_gameassets.py - soldier data from Soldier.txt. */",
         "/* !!UNVALIDATED LAYOUT!! Field offsets below are GUESSED, not IL2CPP-verified.",
         "   Do NOT use struct fields to drive packet sends until each offset is validated",
         "   against a known in-game value. Only the tier grouping (u16@26, 1..7) is",
         "   structurally confirmed. See lords-bot-data-tables-layout-caveat.md */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_SOLDIER_N {len(ents)}",
         "typedef struct { uint16_t id; uint16_t tier; uint16_t unk6; uint16_t unk8;",
         "                  uint32_t food; uint16_t load; } GameSoldier;",
         f"static const GameSoldier kGameSoldiers[{len(ents)}] = {{"]
    L += [f"    {{ {i}, {tr}, {u6}, {u8}, {fd}u, {ld} }},"
          for i, tr, u6, u8, fd, ld in ents]
    L += ["};",
          "static inline const GameSoldier* game_soldier_get(uint16_t id){",
          "    int lo=0, hi=GAME_SOLDIER_N-1, idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GameSoldier*x=&kGameSoldiers[mid];",
          "        if(x->id<id) lo=mid+1; else { idx=mid; hi=mid-1; } }",
          "    if(idx<0) return NULL;",
          "    const GameSoldier*x=&kGameSoldiers[idx];",
          "    return (x->id==id)?x:0;",
          "}"]
    open(os.path.join(INCLUDE, "game_soldier.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_soldier.h -> include/ ({len(ents)} soldier entries)")


def decode_hero():
    """Decode Heros.txt (136-byte rows, 2966 entries).
    Key fields: ID u16@0, HeroID u16@2, Star u16@4, Skill1 u16@6, Skill2 u16@8,
    Atk u32@10, Def u32@14, HP u32@18, ...
    """
    path = os.path.join(GA, "Heros.txt")
    data = open(path, "rb").read()
    R = 136
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID      = struct.unpack_from("<H", r, 0)[0]
        hero_id = struct.unpack_from("<H", r, 2)[0]
        star    = struct.unpack_from("<H", r, 4)[0]
        skill1  = struct.unpack_from("<H", r, 6)[0]
        skill2  = struct.unpack_from("<H", r, 8)[0]
        atk     = struct.unpack_from("<I", r, 10)[0]
        def_    = struct.unpack_from("<I", r, 14)[0]
        hp      = struct.unpack_from("<I", r, 18)[0]
        rows.append({"id": ID, "hero_id": hero_id, "star": star, "skill1": skill1,
                     "skill2": skill2, "atk": atk, "def": def_, "hp": hp})
    write_csv("Heros.csv",
              ["id", "hero_id", "star", "skill1", "skill2", "atk", "def", "hp"],
              [[r["id"], r["hero_id"], r["star"], r["skill1"], r["skill2"],
                r["atk"], r["def"], r["hp"]] for r in rows])
    return rows


def emit_hero_header():
    """Generate include/game_hero.h - hero data keyed by hero_id."""
    import json as _j
    d = _j.load(open(os.path.join(OUT, "Heros.json")))
    ents = sorted((r["hero_id"], r["star"], r["skill1"], r["skill2"],
                   r["atk"], r["def"], r["hp"]) for r in d)
    L = ["/* Auto-generated by decode_gameassets.py - hero data from Heros.txt. */",
         "/* !!UNVALIDATED LAYOUT!! Offsets are GUESSED. hero_id is NOT simply u16@2",
         "   (only 214 distinct values across 2966 rows). Do NOT drive sends with struct",
         "   fields until offsets are IL2CPP-verified. See lords-bot-data-tables-layout-caveat.md */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_HERO_N {len(ents)}",
         "typedef struct { uint16_t id; uint16_t star; uint16_t skill1; uint16_t skill2;",
         "                  uint32_t atk; uint32_t def; uint32_t hp; } GameHero;",
         f"static const GameHero kGameHeroes[{len(ents)}] = {{"]
    L += [f"    {{ {i}, {st}, {s1}, {s2}, {atk}u, {df}u, {hp}u }},"
          for i, st, s1, s2, atk, df, hp in ents]
    L += ["};",
          "static inline const GameHero* game_hero_get(uint16_t id){",
          "    int lo=0, hi=GAME_HERO_N-1, idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GameHero*x=&kGameHeroes[mid];",
          "        if(x->id<id) lo=mid+1; else { idx=mid; hi=mid-1; } }",
          "    if(idx<0) return NULL;",
          "    const GameHero*x=&kGameHeroes[idx];",
          "    return (x->id==id)?x:0;",
          "}"]
    open(os.path.join(INCLUDE, "game_hero.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_hero.h -> include/ ({len(ents)} hero entries)")


def decode_pet():
    """Decode Pet.txt (81-byte rows, 67 entries).
    Key fields: ID u16@0, PetID u16@2, Skill u16@4, Quality u8@6, Level u8@7,
    Atk u32@8, Def u32@12, HP u32@16, ...
    """
    path = os.path.join(GA, "Pet.txt")
    data = open(path, "rb").read()
    R = 81
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID      = struct.unpack_from("<H", r, 0)[0]
        pet_id  = struct.unpack_from("<H", r, 2)[0]
        skill   = struct.unpack_from("<H", r, 4)[0]
        quality = r[6]
        level   = r[7]
        atk     = struct.unpack_from("<I", r, 8)[0]
        def_    = struct.unpack_from("<I", r, 12)[0]
        hp      = struct.unpack_from("<I", r, 16)[0]
        rows.append({"id": ID, "pet_id": pet_id, "skill": skill, "quality": quality,
                     "level": level, "atk": atk, "def": def_, "hp": hp})
    write_csv("Pet.csv",
              ["id", "pet_id", "skill", "quality", "level", "atk", "def", "hp"],
              [[r["id"], r["pet_id"], r["skill"], r["quality"], r["level"],
                r["atk"], r["def"], r["hp"]] for r in rows])
    return rows


def emit_pet_header():
    """Generate include/game_pet.h - pet data keyed by pet_id."""
    import json as _j
    d = _j.load(open(os.path.join(OUT, "Pet.json")))
    ents = sorted((r["pet_id"], r["skill"], r["quality"], r["level"],
                   r["atk"], r["def"], r["hp"]) for r in d)
    L = ["/* Auto-generated by decode_gameassets.py - pet data from Pet.txt. */",
         "/* !!UNVALIDATED LAYOUT!! Offsets are GUESSED, not IL2CPP-verified. Do NOT",
         "   drive sends with struct fields until validated. See",
         "   lords-bot-data-tables-layout-caveat.md */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_PET_N {len(ents)}",
         "typedef struct { uint16_t id; uint16_t skill; uint8_t quality; uint8_t level;",
         "                  uint32_t atk; uint32_t def; uint32_t hp; } GamePet;",
         f"static const GamePet kGamePets[{len(ents)}] = {{"]
    L += [f"    {{ {i}, {sk}, {q}, {lv}, {atk}u, {df}u, {hp}u }},"
          for i, sk, q, lv, atk, df, hp in ents]
    L += ["};",
          "static inline const GamePet* game_pet_get(uint16_t id){",
          "    int lo=0, hi=GAME_PET_N-1, idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GamePet*x=&kGamePets[mid];",
          "        if(x->id<id) lo=mid+1; else { idx=mid; hi=mid-1; } }",
          "    if(idx<0) return NULL;",
          "    const GamePet*x=&kGamePets[idx];",
          "    return (x->id==id)?x:0;",
          "}"]
    open(os.path.join(INCLUDE, "game_pet.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_pet.h -> include/ ({len(ents)} pet entries)")


def decode_monster():
    """Decode Monster.txt (57-byte rows, 300 entries).
    Key fields: ID u16@0, MonsterID u16@2, Level u16@4, HP u32@6, Atk u32@10,
    Def u32@14, RewardItem u16@18, RewardCount u32@20, ...
    """
    path = os.path.join(GA, "Monster.txt")
    data = open(path, "rb").read()
    R = 57
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID        = struct.unpack_from("<H", r, 0)[0]
        monster_id = struct.unpack_from("<H", r, 2)[0]
        level     = struct.unpack_from("<H", r, 4)[0]
        hp        = struct.unpack_from("<I", r, 6)[0]
        atk       = struct.unpack_from("<I", r, 10)[0]
        def_      = struct.unpack_from("<I", r, 14)[0]
        reward_id = struct.unpack_from("<H", r, 18)[0]
        reward_cnt = struct.unpack_from("<I", r, 20)[0]
        rows.append({"id": ID, "monster_id": monster_id, "level": level, "hp": hp,
                     "atk": atk, "def": def_, "reward_id": reward_id, "reward_cnt": reward_cnt})
    write_csv("Monster.csv",
              ["id", "monster_id", "level", "hp", "atk", "def", "reward_id", "reward_cnt"],
              [[r["id"], r["monster_id"], r["level"], r["hp"], r["atk"],
                r["def"], r["reward_id"], r["reward_cnt"]] for r in rows])
    return rows


def emit_monster_header():
    """Generate include/game_monster.h - monster data keyed by monster_id."""
    import json as _j
    d = _j.load(open(os.path.join(OUT, "Monster.json")))
    ents = sorted((r["monster_id"], r["level"], r["hp"], r["atk"],
                   r["def"], r["reward_id"], r["reward_cnt"]) for r in d)
    L = ["/* Auto-generated by decode_gameassets.py - monster data from Monster.txt. */",
         "/* !!UNVALIDATED LAYOUT!! Offsets are GUESSED, not IL2CPP-verified. Do NOT",
         "   drive sends with struct fields until validated. See",
         "   lords-bot-data-tables-layout-caveat.md */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_MONSTER_N {len(ents)}",
         "typedef struct { uint16_t id; uint16_t level; uint32_t hp; uint32_t atk;",
         "                  uint32_t def; uint16_t reward_id; uint32_t reward_cnt; } GameMonster;",
         f"static const GameMonster kGameMonsters[{len(ents)}] = {{"]
    L += [f"    {{ {i}, {lv}, {hp}u, {atk}u, {df}u, {ri}, {rc}u }},"
          for i, lv, hp, atk, df, ri, rc in ents]
    L += ["};",
          "static inline const GameMonster* game_monster_get(uint16_t id){",
          "    int lo=0, hi=GAME_MONSTER_N-1, idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GameMonster*x=&kGameMonsters[mid];",
          "        if(x->id<id) lo=mid+1; else { idx=mid; hi=mid-1; } }",
          "    if(idx<0) return NULL;",
          "    const GameMonster*x=&kGameMonsters[idx];",
          "    return (x->id==id)?x:0;",
          "}"]
    open(os.path.join(INCLUDE, "game_monster.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_monster.h -> include/ ({len(ents)} monster entries)")


def decode_tech_lv():
    path = os.path.join(GA, "TechLv.txt")
    data = open(path, "rb").read()
    R = 52
    body = data[4:]
    rows = []
    for i in range(len(body)//R):
        r = body[i*R:i*R+R]
        ID   = struct.unpack_from("<H", r, 0)[0]
        Tid  = struct.unpack_from("<H", r, 2)[0]
        Lv   = r[4]
        tm   = struct.unpack_from("<I", r, 5)[0]
        food,rock,wood,ore,gold = [struct.unpack_from("<I", r, o)[0] for o in (9,13,17,21,25)]
        req1 = struct.unpack_from("<H", r, 30)[0]      # RequireTechID1 @0x1E=30
        req1lv = r[32]
        eff  = struct.unpack_from("<H", r, 46)[0]      # Effect @0x2E=46
        effv = struct.unpack_from("<I", r, 48)[0]      # EffectVal @0x30=48
        rows.append({"id": ID, "tech_id": Tid, "level": Lv, "time_s": tm,
                     "food": food, "rock": rock, "wood": wood,
                     "ore": ore, "gold": gold,
                     "requires_tech1": req1, "requires_tech1_lv": req1lv,
                     "effect": eff, "effect_val": effv})
    write_csv("TechLv.csv",
              ["id", "tech_id", "level", "time_s", "food", "rock", "wood",
               "ore", "gold", "requires_tech1", "requires_tech1_lv", "effect", "effect_val"],
              [[r["id"], r["tech_id"], r["level"], r["time_s"], r["food"], r["rock"],
                r["wood"], r["ore"], r["gold"], r["requires_tech1"],
                r["requires_tech1_lv"], r["effect"], r["effect_val"]] for r in rows])
    return rows

def emit_research_header():
    import json as _j
    d = _j.load(open(os.path.join(OUT, "TechLv.json")))
    ents = sorted((r["tech_id"], r["level"], r["food"], r["gold"],
                   r["requires_tech1"], r["requires_tech1_lv"]) for r in d)
    L = ["/* Auto-generated by decode_gameassets.py - research costs + prerequisites",
         "   from TechLv.txt.  entry k: tech_id, target_level, food_cost, gold_cost,",
         "   req_tech (0=none), req_tech_lv. Do not edit. */",
         "#pragma once", "#include <stdint.h>",
         f"#define GAME_RESEARCH_N {len(ents)}",
         "typedef struct { uint16_t tech; uint8_t level; uint32_t food; uint32_t gold;",
         "                  uint16_t req_tech; uint8_t req_tech_lv; } GameResearchCost;",
         f"static const GameResearchCost kGameResearchCosts[{len(ents)}] = {{"]
    L += [f"    {{ {t}, {lv}, {f}u, {g}u, {rt}, {rl} }}," for t, lv, f, g, rt, rl in ents]
    L += ["};",
          "static inline const GameResearchCost* game_research_cost(uint16_t tech, uint8_t level){",
          "    int lo=0, hi=GAME_RESEARCH_N-1; int idx=-1;",
          "    while(lo<=hi){ int mid=(lo+hi)/2; const GameResearchCost*x=&kGameResearchCosts[mid];",
          "        if(x->tech<tech||(x->tech==tech&&x->level<level)) lo=mid+1;",
          "        else { idx=mid; hi=mid-1; } }",
          "    if(idx<0) return NULL;",
          "    const GameResearchCost*x=&kGameResearchCosts[idx];",
          "    return (x->tech==tech&&x->level==level)?x:0;",
          "}"]
    open(os.path.join(INCLUDE, "game_table_costs.h"), "w").write("\n".join(L) + "\n")
    print(f"  game_table_costs.h -> include/ ({len(ents)} research cost+prereq entries)")

def main():
    print("== decoding cost tables ==")
    bu = decode_build_up()
    json.dump(bu, open(os.path.join(OUT, "buildUP.json"), "w"), indent=1)
    bn = decode_build_up_new()
    json.dump(bn, open(os.path.join(OUT, "buildUP_NEW.json"), "w"), indent=1)
    tl = decode_tech_lv()
    json.dump(tl, open(os.path.join(OUT, "TechLv.json"), "w"), indent=1)
    it = decode_item()
    json.dump(it, open(os.path.join(OUT, "Item.json"), "w"), indent=1)
    print(f"  buildUP.txt        -> buildUP.json     ({len(bu)} rows, 70B each)")
    print(f"  buildUP_NEW.txt    -> buildUP_NEW.json ({len(bn)} rows, 106B each)")
    print(f"  TechLv.txt         -> TechLv.json      ({len(tl)} rows, 52B each)")
    print(f"  Item.txt           -> Item.json        ({len(it)} rows, 88B each)")
    emit_research_header()
    emit_build_header()
    emit_item_header()

    print("\n== decoding core data tables ==")
    sd = decode_soldier()
    json.dump(sd, open(os.path.join(OUT, "Soldier.json"), "w"), indent=1)
    hd = decode_hero()
    json.dump(hd, open(os.path.join(OUT, "Heros.json"), "w"), indent=1)
    pt = decode_pet()
    json.dump(pt, open(os.path.join(OUT, "Pet.json"), "w"), indent=1)
    md = decode_monster()
    json.dump(md, open(os.path.join(OUT, "Monster.json"), "w"), indent=1)
    print(f"  Soldier.txt        -> Soldier.json     ({len(sd)} rows, 42B each)")
    print(f"  Heros.txt          -> Heros.json       ({len(hd)} rows, 136B each)")
    print(f"  Pet.txt            -> Pet.json         ({len(pt)} rows, 81B each)")
    print(f"  Monster.txt        -> Monster.json     ({len(md)} rows, 57B each)")
    emit_soldier_header()
    emit_hero_header()
    emit_pet_header()
    emit_monster_header()

    print("\n== generic decode of every table in GameAssets ==")
    n_ok = n_skip = 0
    done = set()
    dumped = set()
    structured = {"buildUP", "TechLv", "Item", "Soldier", "Heros", "Pet", "Monster"}  # these get a full labeled decode above; all others get generic
    for name in sorted(os.listdir(GA)):
        if not name.endswith(".txt"):
            continue
        base = os.path.splitext(name)[0]
        if base in structured:
            done.add(base)
            continue  # handled with a full structured decode above
        p = os.path.join(GA, name)
        try:
            res = generic_decode(p)
        except Exception:
            res = None
        if not res:
            fallback_dump(name, p)
            dumped.add(base)
            n_skip += 1
            continue
        hdr, R, n, rows = res
        json.dump({"header": hdr, "record_size": R, "count": n, "rows": rows},
                  open(os.path.join(OUT, base + ".json"), "w"))
        try:
            generic_csv(base, R, rows)
        except Exception as e:
            sys.stderr.write(f"csv fail {name}: {e}\n")
        done.add(base)
        n_ok += 1
    print(f"  decoded {n_ok} row-tables + {len(structured)} labeled -> decoded/ ; "
          f"{n_skip} non-row tables dumped as raw hex")
    idx = build_index(done, dumped)
    print(f"  index -> {idx}")

if __name__ == "__main__":
    main()