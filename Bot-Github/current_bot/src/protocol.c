#include "protocol.h"
#include "net_rw.h"
#include "connection.h"
#include "packet_enum.h"
#include "map_point.h"
#include "items.h"
#include "log.h"
#include <stdlib.h>
#include <time.h>

#include <stdarg.h>

#include "MAP_UPDATE_KIND.h"
#include "gathering.h"

static uint32_t now32(void) { return (uint32_t)time(NULL); }

// Bootstrap login
void RequestGuestLogIn(Connection *c)
{
	// reserve space for packet length
    c->size = 2;
    
    // write packet type 
    write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOL);
    c->size += 2;
    
    // write igg id
    write_u64(c->data + c->size, c->auth.igg_id);
    c->size += 8;
    
    // write game minor version 
    write_u8(c->data + c->size, c->app.version_minor);
    c->size += 1;
    
    // write game major version
    write_u8(c->data + c->size, c->app.version_major);
    c->size += 1;
    
    // write game patch version
    write_u16(c->data + c->size, c->app.version_patch);
    c->size += 2;

    write_u8(c->data + c->size, 1);
    c->size += 1;
    
    // write language code
    write_u8(c->data + c->size, c->app.language_code);
    c->size += 1;
    
    // write Device Universally Unique Identifier.
    write_raw(c->data + c->size, c->auth.device_uuid, 50);
    c->size += 50;
    
    // write session length
    write_u16(c->data + c->size, c->auth.session_len);
    c->size += 2;
    
    // write session token
    write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;
    
    // rewrite total packet length 
    write_u16(c->data, c->size);
    
    // call send packet
    send_packet(c, false);
}


// Game login
void RequestLogIn(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOP);
	c->size += 2;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	// write 50 byte zero 
	for (int i = 0; i < 25; i++) {
		write_u16(c->data + c->size, 0);
		c->size += 2;
	}
	
	// write version 
	write_u32(c->data + c->size, 0);
	c->size += 4;
	
	// battle_is_oul
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// b_recv_kingdom
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// session len
	write_u16(c->data + c->size, c->auth.session_len);
	c->size += 2;
	
	// session 
	write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;

    write_u16(c->data, c->size);
    
    send_packet(c, false);
}

// 
void RequestClientInitOver(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_CLIENTINITOVER);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}

void RequestHeartBeat(Connection *c) {
	// reserve space for packet length
	c->size = 2;
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_ACTIVE);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// update packet length 
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}

void RequestSimpleUseItem(Connection *c, uint32_t item_id, uint16_t quantity) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write item id
	write_u16(c->data + c->size, item_id);
	c->size += 2;
	
	// write item quantity
	write_u16(c->data + c->size, quantity);
	c->size += 2;
	
	// 10 byte zero
	write_zero(c->data + c->size, 10); c->size += 10;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true); // true is encryption flag
}

void RequestUseAdvancedRelocator(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write item id
	write_u16(c->data + c->size, ADVANCE_RELOCATOR);
	c->size += 2;
	
	// write item quantity
	write_u16(c->data + c->size, 0x0001);
	c->size += 2;
	
	// write kingdom id
	write_u16(c->data + c->size, kingdom_id);
	c->size += 2;
	
	// write zone id
	write_u16(c->data + c->size, zone_id);
	c->size += 2;
	
	// write point id
	write_u8 (c->data + c->size, point_id);
	c->size += 1;
	
	// nothing just write 5 byte zero 
	write_zero(c->data + c->size, 5); c->size += 5;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true);
}

void RequestMapData(Connection *c, uint8_t count, const uint16_t zone[], bool renew) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_MAPDATA); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    if (count > 4) count = 4;
    write_u8(c->data + c->size, count); c->size += 1;
    for (int i = 0; i < 4; ++i) {
        write_u16(c->data + c->size, zone ? zone[i] : 0); c->size += 2;
    }
    (void)renew;
    write_zero(c->data + c->size, 32); c->size += 32;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void GetBlackMarketData(Connection *c) {
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_BLACKMARKET_DATA); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u8 (c->data + c->size, 1); c->size += 1;
	write_u16(c->data,  c->size);
	
	send_packet(c, true);
}

void SendBlackMarketBuy(Connection *c, uint8_t mIdx) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_BLACKMARKET_BUY);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write slot
	write_u8(c->data + c->size, mIdx);
	c->size += 1;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true);
}

void RequestSmartUseBlackMarketBuy(Connection *c, SmartUseList smart_use, uint8_t mIdx) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_SMARTUSE_FOR_BLACKMARKET);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write slot
	write_u8(c->data + c->size, mIdx);
	c->size += 1;
	
	// number of items required 
	write_u16(c->data + c->size, c->smart_use.count);
	c->size += 2;
	
	for (int i = 0; i < c->smart_use.count; i++) {
		// item id
		write_u16(c->data + c->size, c->smart_use.items[i].id); c->size += 2;
		// quantity 
		write_u16(c->data + c->size, c->smart_use.items[i].qty); c->size += 2;
	}
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true);
}


/*
 * Hospital packet layout recovered from the client-side SendInstHealing path:
 *   protocol + seq + 16 x uint32 troop quantities
 *
 * The internal bot order is:
 *   T1..T4 Infantry, T1..T4 Ranged, T1..T4 Cavalry, T1..T4 Siege.
 *
 * The UI client indexes the same data in reverse tier order inside each
 * troop kind; writing the internal 16-slot order produces the corresponding
 * wire values.
 */
static void SendHealingPacket(Connection *c, uint16_t protocol_id)
{
    c->size = 2;
    write_u16(c->data + c->size, protocol_id);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    for (int kind = 0; kind < 4; ++kind) {
        for (int tier = 0; tier < 4; ++tier) {
            uint32_t amount = c->wounded.troop.loaded
                ? c->wounded.troop.infantry[tier] : 0;

            if (kind == 1) amount = c->wounded.troop.ranged[tier];
            if (kind == 2) amount = c->wounded.troop.cavalry[tier];
            if (kind == 3) amount = c->wounded.troop.siege[tier];

            write_u32(c->data + c->size, amount);
            c->size += 4;
        }
    }

    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestHealing(Connection *c, bool instant)
{
    if (!c->wounded.loaded) {
        LOGE("[HEAL] Hospital data not loaded.\n");
        return;
    }

    if (c->wounded.troop.total == 0) {
        LOGI("[HEAL] No wounded troops available.\n");
        return;
    }

    SendHealingPacket(
        c,
        instant ? _MSG_REQUEST_INSTANTHEALING
                : _MSG_REQUEST_HEALINGTROOP
    );

    LOGI("[HEAL] %s healing request sent for %u wounded troops.\n",
         instant ? "Instant" : "Normal",
         c->wounded.troop.total);
}



void RequestTroopTraining(Connection *c, uint8_t kind, uint8_t tier, uint32_t amount) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_TRAINING_); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);   c->size += 4;
	write_u8 (c->data + c->size, kind);                   c->size += 1;// write RD_Kind troop type (infantry, ranged, cavalry, siege)
	write_u8 (c->data + c->size, tier);                   c->size += 1;// write RD_Rank (server expects 0-based) (t1, t2, t3, t4, t5)
	write_u32(c->data + c->size, amount);                 c->size += 4;// write troop amount
	write_u16(c->data, c->size); // update packet size

	LOGI("[TRAIN TX] 2403 kind=%u tier=%u amount=%u payload[%u]=%02X %02X %02X %02X %02X %02X\n",
	     (unsigned)kind, (unsigned)tier, (unsigned)amount, (unsigned)(c->size - 8),
	     c->data[8], c->data[9], c->data[10], c->data[11],
	     c->data[12], c->data[13]);

	send_packet(c, true);
}

void CancelTroopTraining(Connection *c)
{
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_CANCELTRAINING); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);        c->size += 4;
	write_u16(c->data, c->size); // update packet size
	
	send_packet(c, true);
}

typedef struct {
	uint32_t food;
	uint32_t rock;
	uint32_t wood;
	uint32_t ore;
	uint32_t gold;
} Resources;


// MarchEventType enum values (from dump.cs EMarchEventType)
#define EMET_GatherMarching 7
#define EMET_AttackMarching 5

// PointCode structure for network serialization (zoneID + pointID)
static void write_point_code(uint8_t *buf, uint16_t zone_id, uint8_t point_id) {
    write_u16(buf, zone_id);
    write_u8(buf + 2, point_id);
}

/*
 * Two wire formats for gather marches, per-venue:
 *
 * 1) Kingdom (normal world) -> _MSG_REQUEST_TROOPMARCH (2415)
 *    Uses the MarchEventDataType struct layout (from dump.cs TypeDefIndex: 515):
 *      byte  Type (EMarchEventType)           -- EMET_GatherMarching = 7
 *      u16[5]  HeroID
 *      u32[16] TroopData T1..T4 (type-major)
 *      u16 zone + u8 point (PointCode)
 *      u32[5]  ResourceGetCount[5]            -- fill from tile count, zeros for other 4
 *      u32     Crystal                        -- 0
 *      u32     MaxOverLoad                    -- 0
 *      u8      PointKind (POINT_KIND)         -- from tile
 *      u16     DesPointLevel                  -- tile level
 *      string  DesPlayerName (empty)          -- empty (length prefix + bytes, we omit)
 *      u16     WonderEffectID                 -- 0
 *      u16     CentralWonderID                -- 0
 *      u8      bRallyHost                     -- 0
 *      u16     RallyWonderID                  -- 0
 *      u16     RallyWonderEffectID            -- 0
 *      u16     RallyCentralWonderID           -- 0
 *    Total payload ~141 bytes; frame = 8 + 141 = 149 bytes.
 *    This is what the native subcommand=0 handler expects (reads Type at byte[1]).
 *
 * 2) GBG / Chaos (event battlefield) -> _MSG_REQUEST_TROOPMARCH_NOTATK (6615)
 *    Uses the native 175-byte branch from UIExpedition_EX::SendExpedition
 *    (reconstructed in NATIVE_6615_EXACT_RECONSTRUCTION.md):
 *      u16[5]  HeroID
 *      u32[16] TroopData T1..T4
 *      u16 zone + u8 point
 *      u16[5]  PetID
 *      u32[16] TroopData T1..T4 (again)
 *      u32[4]  T5 (zero)
 *    Payload = 167 bytes; frame = 8 + 167 = 175 bytes.
 *    No EMET byte, no ResourceGetCount, no PointKind/level.
 *
 * 6615 for kingdom was returning subcommand=4 (modify-existing-march / standby).
 * 2415 with the 141-byte struct is the untested cell that should trigger subcommand=0
 * with EMET=7 (GatherMarching) -> real march created.
 */
bool RequestTroopMarchGather(Connection *c, const uint16_t hero_ids[5], const uint32_t troop_array[16],
                             uint16_t zone_id, uint8_t point_id, uint8_t point_kind, uint16_t tile_level,
                             uint32_t resource_count, uint32_t max_overload, bool no_attack,
                             uint32_t pin_node_id, uint32_t pin_dest_y, GatherVenue venue)
{
    /* Client ground truth (UIExpedition_EX::SendExpedition disassembly):
     *   venue KINGDOM (0) -> 2415  _MSG_REQUEST_TROOPMARCH  (full 141-byte MarchEventData)
     *   venue GBG      (1) -> 1809  (event/GBG uses different command per disassembly)
     *   venue CHAOS    (2) -> 6615  _MSG_REQUEST_TROOPMARCH_NOTATK
     * The bot's GATHER_VENUE_KINGDOM=0 maps exactly to client kind=0.
     * The client NEVER uses 6615 for kingdom tile gather; 6615 is strictly for
     * Expedition/Adventure venues (Chaos/GBG).  Sending 6615 for kingdom
     * produces subcommand=4 (standby/modify-existing) instead of a real gather. */
    uint16_t protocol;
    if (venue == GATHER_VENUE_KINGDOM) {
        protocol = _MSG_REQUEST_TROOPMARCH;           /* 2415 - kingdom gather */
    } else if (venue == GATHER_VENUE_GBG) {
        protocol = 1809;                             /* GBG event command */
    } else {
        protocol = _MSG_REQUEST_TROOPMARCH_NOTATK;   /* 6615 - Chaos/Expedition */
    }
    /* Reference-bot gather serializer: the full MarchEventDataType body (with
       its self-contained Zone+Point and PointKind/level) is the working form and
       is sent over 6615 when no_attack (gather), or over 2415 otherwise.  The
       OLD fat 175-byte / sparse 111-byte 6615 bodies are retained only behind
       explicit experiments; the DEFAULT now emits the 141-byte MarchEventData
       that the reference bot uses (it was previously reachable only on 2415,
       never on the 6615 gather path). */
    bool use_full   = !c->gathering.send_6615_fat && !c->gathering.send_6615_sparse;
    bool is_fat_6615 = (!use_full) && c->gathering.send_6615_fat;
    /* 2415 = MarchEventDataType body 133 bytes + 8 header = 141.
       6615 full = same 141-byte MarchEventData body over 6615 (reference bot).
       6615 fat = body 167 bytes + 8 header = 175.
       6615 sparse = body 103 bytes + 8 header = 111. */
    uint32_t expected_size = use_full ? 141 : (is_fat_6615 ? 175 : 111);

    c->size = 2;
    write_u16(c->data + c->size, protocol);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    if (use_full) {
        /* ========== 141-byte MarchEventDataType body (over 6615 or 2415) ========== */

        /* 1. Type: u8 EMarchEventType -- EMET_GatherMarching = 7 */
        write_u8(c->data + c->size, (uint8_t)EMET_GatherMarching);
        c->size += 1;

        /* 2. Hero_ID[5] u16 */
        for (int i = 0; i < 5; ++i) {
            write_u16(c->data + c->size, hero_ids ? hero_ids[i] : 0);
            c->size += 2;
        }

        /* 3. TroopData T1..T4 u32[16], type-major Infantry/Ranged/Cavalry/Siege. */
        for (int i = 0; i < 16; ++i) {
            write_u32(c->data + c->size, troop_array ? troop_array[i] : 0);
            c->size += 4;
        }

        /* 4. PointCode: zone u16 + point u8 */
        write_point_code(c->data + c->size, zone_id, point_id);
        c->size += 3;

        /* 5. ResourceGetCount[5] u32 -- keep ZERO per reference bot/dump.
           Do NOT guess/fill from tile; verified captures show zeroed field. */
        memset(c->data + c->size, 0, 5 * 4);
        c->size += 5 * 4;

        /* 6. Crystal u32 -- 0 */
        write_u32(c->data + c->size, 0);
        c->size += 4;

        /* 7. MaxOverLoad u32 -- 0 */
        write_u32(c->data + c->size, 0);
        c->size += 4;

        /* 8. PointKind u8 (POINT_KIND enum) */
        write_u8(c->data + c->size, point_kind);
        c->size += 1;

        /* 9. DesPointLevel u16 */
        write_u16(c->data + c->size, tile_level);
        c->size += 2;

        /* 10. DesPlayerName -- fixed 13-byte CString (empty for resource gathering) */
        memset(c->data + c->size, 0, 13);
        c->size += 13;

        /* 11..16. Trailing wonder/rally fields (11 bytes zero-filled):
           WonderEffectID(u16) + CentralWonderID(u16) + bRallyHost(u8) +
           RallyWonderID(u16) + RallyWonderEffectID(u16) + RallyCentralWonderID(u16) */
        memset(c->data + c->size, 0, 11);
        c->size += 11;

    } else if (is_fat_6615) {
        /* ========== 6615 FAT: 167-byte body / 175-byte frame ==========
           The OLD native UIExpedition_EX::SendExpedition layout (the "175-byte
           branch" that predates the lean capture).  Body:
             u16[5]  HeroID
             u32[16] TroopData T1..T4   (type-major, same 16 slots as sparse)
             u16 zone + u8 point        (PointCode)
             u16[5]  PetID              (all 0)
             u32[16] TroopData T1..T4   (again, same values)
             u32[4]  T5                 (all 0)
           Payload = 167; frame = 8 + 167 = 175.  No EMET byte, no
           ResourceGetCount, no PointKind/level. */
        c->size = 8;  /* already len+msg+seq; body starts at [8] */

        /* HeroID[5] u16 */
        for (int i = 0; i < 5; ++i) {
            write_u16(c->data + c->size, hero_ids ? hero_ids[i] : 0);
            c->size += 2;
        }
        /* TroopData T1..T4 u32[16], type-major. */
        for (int i = 0; i < 16; ++i) {
            write_u32(c->data + c->size, troop_array ? troop_array[i] : 0);
            c->size += 4;
        }
        /* PointCode: zone u16 + point u8 */
        write_point_code(c->data + c->size, zone_id, point_id);
        c->size += 3;
        /* PetID[5] u16, zeros. */
        for (int i = 0; i < 5; ++i) {
            write_u16(c->data + c->size, 0);
            c->size += 2;
        }
        /* TroopData T1..T4 u32[16] again. */
        for (int i = 0; i < 16; ++i) {
            write_u32(c->data + c->size, troop_array ? troop_array[i] : 0);
            c->size += 4;
        }
        /* T5 u32[4], zeros. */
        memset(c->data + c->size, 0, 16);
        c->size += 16;
        /* total from [8] = 10+64+3+10+64+16 = 167; frame 175. */
        c->size = expected_size;

        LOGI("[GATHER PACKET] FAT 6615 body serialized size=%u (EXPERIMENT send_6615_fat)",
             (unsigned)c->size);

    } else if (c->gathering.send_6615_sparse) {
        /* ========== 6615: sparse 111-byte request (matches real wire capture) ==========
           The fat 167-byte 6615 body (hero[5]+troop[16]+pt+pet[5]+troop[16]+T5[4])
           was mis-classified by the server as subcommand=4 / EMET_Standby.  The
           real client's 0x19d7 gather is LEAN: 104 encrypted body bytes that are
           mostly zero, with only the seq counter (u32 LE, already written above)
           and a few id/coordinate fields populated at offsets we are still
           confirming.  See GATHER_CAPTURE_FINDINGS.md + captured_6615_reference.py.

           send_packet() DES-encrypts [4:size] via EncryptData(), which encrypts
           floor(size/8) blocks and copies the remainder plaintext.  With size=111
           it encrypts bytes [4:108] (104 B = 13 blocks) and leaves [108:111]
           (3 B) as a plaintext zero tail — exactly the captured layout. */

        /* Zero the sparse body, then reproduce EVERY nonzero byte of the real
           captured 0x19d7 (BODY_104 in captured_6615_reference.py).  The capture
           is the ground truth for a successful kingdom gather; what remains below
           is the maximal-fidelity replica:
             body[0]    = 0xd6  per-session client message counter (d4/d5 on the
                                two preceding OPEN_UI frames, d6 on this 6615,
                                d7 on the following 0x0899 — monotonic across
                                message types, NOT a subcommand).  Fold the u32
                                seq_id (already advanced above) to one byte.
             body[46:48]= 0x51cc gather-request id / venue marker from the capture.
             body[78:80]= 0x0239 destination tile's map Y (569 >= 512 proves the
                                Y axis given CheckTileMapPos x<512); written from
                                the LIVE target so the request routes.  (frame abs
                                offset = 4 header + 78 body.)
             body[80]   = 0xcc  capture companion value.
           All other 100 body bytes are zero in the real gather.  */
        memset(c->data + c->size, 0, expected_size - c->size);

        /* body[0] counter byte (1 byte, [1:4] zeroed), mirroring the real client. */
        c->data[4 + 0] = (uint8_t)(c->protocol.seq_id & 0xFF);
        c->data[4 + 1] = 0;
        c->data[4 + 2] = 0;
        c->data[4 + 3] = 0;

        /* body[46:48] = destination tile's map X (u16 LE), matching body[78:80]
           which carries the tile's map Y.  The two open/create-session links
           141/569 as coordinate (X,Y); a REAL resource tile's own (X,Y) must be
           sent or the server parks the march as a Camp at wherever that object
           actually is.  Default: the live target's mp.x, plus the venue's world
           base (world_x_offset, e.g. 20562 for Kingdom-Battle) when configured.
           The EXPERIMENT pin is now a strict test override: when nonzero it is
           used verbatim and warns, but it no longer shadows a configured world
           base — the world conversion is derived from the scanned tile. */
        {
            uint32_t nid;
            map_pos_t mp = getTileMapPosbyPointCode(zone_id, point_id);
            if (pin_node_id) {
                nid = pin_node_id & 0xFFFFu;
                LOGW("[GATHER PACKET] pin_node_id=0x%04x overrides world-X of tile "
                     "mp.x=%u (test pin; not derived from the scanned tile)",
                     nid, (unsigned)mp.x);
            } else if (c->gathering.world_x_offset) {
                /* World-X from the live tile: home X + kingdom/world base. */
                nid = ((uint32_t)mp.x + c->gathering.world_x_offset) & 0xFFFFu;
            } else {
                nid = (uint32_t)(uint16_t)mp.x;   /* normal kingdom home-scale X */
            }
            c->data[4 + 46] = (uint8_t)(nid & 0xFF);
            c->data[4 + 47] = (uint8_t)(nid >> 8);
        }

        /* body[78:80] = destination tile map Y (u16 LE).  Applied in the same
           coordinate space as X: the world base (world_y_offset, e.g. 67 for
           this kingdom) is added to the live tile's mp.y when configured, so a
           venue that maps X to world space maps Y too.  pin_dest_y remains a
           warned test override.  Default (both zero) sends home-scale Y. */
        {
            map_pos_t mp = getTileMapPosbyPointCode(zone_id, point_id);
            uint32_t dy;
            if (pin_dest_y) {
                dy = pin_dest_y & 0xFFFFu;
                LOGW("[GATHER PACKET] pin_dest_y=%u overrides world-Y of tile "
                     "mp.y=%u (test pin; not derived from the scanned tile)",
                     (unsigned)dy, (unsigned)mp.y);
            } else if (c->gathering.world_y_offset) {
                /* World-Y from the live tile: home Y + kingdom/world base. */
                dy = (uint32_t)mp.y + c->gathering.world_y_offset;
            } else {
                dy = (uint32_t)(uint16_t)mp.y;   /* normal kingdom home-scale Y */
            }
            c->data[4 + 78] = (uint8_t)(dy & 0xFF);
            c->data[4 + 79] = (uint8_t)(dy >> 8);
        }

        /* body[80] = 0xcc — capture companion value. */
        c->data[4 + 80] = 0xCC;
        c->size = expected_size;
    }

    write_u16(c->data, c->size);

    if (c->size != expected_size) {
        LOGE("[GATHER PACKET] refusing to send malformed %s packet size=%u (expected %u)",
             !no_attack ? "2415" : "6615", c->size, expected_size);
        return false;
    }

    uint64_t troop_total = 0;
    for (int i = 0; i < 16; ++i) troop_total += troop_array ? troop_array[i] : 0;
    LOGI("[GATHER PACKET] type=%u size=%u zone=%u point=%u kind=%u level=%u troops=%llu",
         (unsigned)protocol, c->size, zone_id, point_id, point_kind, tile_level,
         (unsigned long long)troop_total);
    LOGI("[GATHER PACKET] troop slots: %u,%u,%u,%u | %u,%u,%u,%u | %u,%u,%u,%u | %u,%u,%u,%u",
         troop_array ? troop_array[0] : 0, troop_array ? troop_array[1] : 0, troop_array ? troop_array[2] : 0, troop_array ? troop_array[3] : 0,
         troop_array ? troop_array[4] : 0, troop_array ? troop_array[5] : 0, troop_array ? troop_array[6] : 0, troop_array ? troop_array[7] : 0,
         troop_array ? troop_array[8] : 0, troop_array ? troop_array[9] : 0, troop_array ? troop_array[10] : 0, troop_array ? troop_array[11] : 0,
         troop_array ? troop_array[12] : 0, troop_array ? troop_array[13] : 0, troop_array ? troop_array[14] : 0, troop_array ? troop_array[15] : 0);

    /* Keep a copy of the plaintext request header/body for diagnostics.
       send_packet() encrypts bytes 4..size-1 in-place, so logging after
       send_packet() cannot tell us what was actually serialized. */
    LOGI("[GATHER PACKET] plaintext header: len=%u msg=%u seq=%u",
         (unsigned)c->size,
         (unsigned)(c->data[2] | ((uint16_t)c->data[3] << 8)),
         (unsigned)(c->data[4] |
                    ((uint32_t)c->data[5] << 8) |
                    ((uint32_t)c->data[6] << 16) |
                    ((uint32_t)c->data[7] << 24)));

    /* Full plaintext dump of the serialized frame (bytes 0..size-1) before
       send_packet() encrypts 4..size-1 in place.  This is the exact wire
       request, byte-for-byte, for offline field diffing against the account's
       real march. */
    {
        char ph[8 * 175 + 8];
        char *pp = ph;
        char line[16];
        for (size_t i = 0; i < c->size; ++i) {
            if (i && (i % 8) == 0) *pp++ = '\n';
            snprintf(line, sizeof(line), "%02X ", c->data[i]);
            size_t l = strlen(line);
            memcpy(pp, line, l); pp += l;
        }
        *pp = '\0';
        LOGI("[GATHER PACKET] frame_%u:\n%s", (unsigned)c->size, ph);
    }

    bool sent_ok = send_packet(c, true);
    LOGI("[GATHER] socket transmission=%s", sent_ok ? "SUCCESS" : "FAILED");
    return sent_ok;
}

/*
 * Guild Expedition (GBG) / Alliance Battlefield lifecycle requests.
 *
 * These enter the expedition battlefield and request its running detail so the
 * gather venue can read the expedition's gatherable resource nodes.  The dump's
 * GBGExpeditionManager only exposes these as argument-less Send_* methods
 * (Send_REQUEST_ALLIANCE_BATTLEFIELD_EVENT_DETAIL / _ENTER), so the frames here
 * follow the established empty-send convention: protocol + seq only (same shape
 * as RequestRallyList / RequestAllianceMemberInfo).
 */
void RequestGuildBattlefieldEventDetail(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_BATTLEFIELD_EVENT_DETAIL);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
    LOGI("[GATHER GBG] REQUEST_ALLIANCE_BATTLEFIELD_EVENT_DETAIL (11403) sent");
}

void RequestGuildBattlefieldEnter(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_BATTLEFIELD_ENTER);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
    LOGI("[GATHER GBG] REQUEST_ALLIANCE_BATTLEFIELD_ENTER (11412) sent");
}

/*
 * RequestOpenUI (1144 / 0x0478): opens a client UI window.  The real gather
 * capture (game.pcap) shows the client sending TWO 0x0478 frames — seq d4,05,
 * both window=7 — immediately BEFORE the 0x19d7 (6615) dispatch, establishing
 * the march/open-ui context.  The bot previously sent a lone 6615 with no such
 * precursor and the server answered subcommand=2 (gathering-update) rather than
 * subcommand=0 (create), consistent with no live march-creation session.  Frame
 * = [msg 1144][seq u32][window u32][2-byte plaintext tail], len 14 (matches the
 * capture's 14-byte 0x0478).  send_packet() encrypts [4:14] via EncryptData():
 * 8 block bytes + 2 plaintext tail bytes, mirroring the captured layout. */
void RequestOpenUI(Connection *c, uint32_t window_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_OPEN_UI);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u32(c->data + c->size, window_id);
    c->size += 4;
    /* 2-byte plaintext tail (captured 0x0478 frames are len=14). */
    memset(c->data + c->size, 0, 2);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
    LOGI("[GATHER] REQUEST_OPEN_UI (1144) window=%u sent size=%u",
         (unsigned)window_id, (unsigned)c->size);
}

/*
 * Chaos Arena (Solo Battlefield) info request.  The CHAOS component derives its
 * map from the Solo Battlefield layer, so requesting its info (11992) yields the
 * arena coordinate frame / gatherable nodes on RESP_INFO (11993).
 */
void RequestSoloBattlefieldInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SOLO_BATTLEFIELD_INFO);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
    LOGI("[GATHER CHAOS] REQUEST_SOLO_BATTLEFIELD_INFO (11992) sent");
}

bool SendResource(Connection *c, Resources resource, uint16_t zoneId, uint8_t pointId) {
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_SEND_RESHELP);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);                 c->size += 4;
	
	write_u16(c->data + c->size, zoneId);  c->size += 2;
	write_u8 (c->data + c->size, pointId); c->size += 1;
	
	write_u32(c->data + c->size, resource.food); c->size += 4;
	write_u32(c->data + c->size, resource.rock); c->size += 4;
	write_u32(c->data + c->size, resource.wood); c->size += 4;
	write_u32(c->data + c->size, resource.ore);  c->size += 4;
	write_u32(c->data + c->size, resource.gold); c->size += 4;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
	return 1;
}

void RequestMissionInfo(Connection *c, uint8_t missionType) {
	if (missionType != 0 && missionType != 1) return;
	
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_MISSIONINFO);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);       c->size += 4;
	write_u8 (c->data + c->size, (missionType + 1)); c->size += 1;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
	return;
}

void RequestAllyPoint(Connection *c, const char *name) 
{
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_ALLYPOINT);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);     c->size += 4;
	write_raw(c->data + c->size, name, 13);                 c->size += 13;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}

void RequestSendChat(Connection *c, uint8_t channel, const char *message) {
	uint16_t message_len = (uint16_t)strlen(message);
	
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_SENDCHAT);
	c->size += 2;
	
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	write_u8 (c->data + c->size, channel);
	c->size += 1;
	
	write_u8 (c->data + c->size, 0);
	c->size += 1;
	
	write_u8 (c->data + c->size, 5);
	c->size += 1;
	
	write_u16(c->data + c->size, message_len);
	c->size += 2;
	
	write_raw(c->data + c->size, message, message_len);
	c->size += message_len;
	
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}


void RequestViewChat(Connection *c, uint8_t channel, uint8_t prev, int8_t kind, int64_t DataID, int64_t DataTime) {
	c->size = 2;
	
	// packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_VIEWCHAT);
	c->size += 2;
	
	// sequence id
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// channel type 
	write_u8 (c->data + c->size, channel);
	c->size += 1;
	
	// previous 
	write_u8 (c->data + c->size, prev);
	c->size += 1;
	
	if (c->app.version_major != 0) {
		write_u8(c->data + c->size, (kind == -1) ? 0xFF : (uint8_t)kind);
		c->size += 1;
		/*
		if (kind == -1) {
			write_u8 (c->data + c->size, 0xFF);
			c->size += 1;
		} else {
			write_u8 (c->data + c->size, (uint8_t)kind);
			c->size += 1;
		}
		*/
	}
	
	if (channel != 0) {
		write_u64(c->data + c->size, DataID);
		c->size += 8;
		
		write_u64(c->data + c->size, DataTime);
		c->size += 8;
	}
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}

void RequestSendMail(Connection *c, const char *player_name, const char *subject, const char *message) {
	uint8_t subject_len = (uint8_t)strlen(subject);
	uint16_t message_len = (uint16_t)strlen(message);
	
	c->size = 2;
	
	// packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_SENDREGMAIL);
	c->size += 2;
	
	// sequence id
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	
	write_u32(c->data + c->size, 0);
	c->size += 4;
	
	// player name
	write_raw(c->data + c->size, player_name, 13);
	c->size += 13;
	
	write_u8 (c->data + c->size, 1);
	c->size += 1;
	
	write_u8 (c->data + c->size, subject_len);
	c->size += 1;
	
	write_u16(c->data + c->size, message_len);
	c->size += 2;
	
	write_u8 (c->data + c->size, 0);
	c->size += 1;
	
	
	write_raw(c->data + c->size, subject, subject_len);
	c->size += subject_len;
	
	write_raw(c->data + c->size, message, message_len);
	c->size += message_len;
	
	
	write_zero(c->data + c->size, 10);
	c->size += 10;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}

void RequestSendMailFmt(Connection *c, const char *player_name, const char *subject, const char *fmt, ...) {
    char buf[4096];  // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    RequestSendMail(c, player_name, subject, buf);
}

void RequestAllianceMemberInfo(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_MEMBERINFO); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);   c->size += 4;
	write_u16(c->data, c->size); // update packet size
	
	send_packet(c, true);
}

void RequestHelpAllianceMember(Connection *c, uint16_t record_sn_count, const uint32_t *record_sn)
{
	c->size = 2;
	
	write_u16(c->data + c->size, 0x0b27);
	c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	write_u16(c->data + c->size, record_sn_count);
	c->size += 2;
	
	for (uint16_t i = 0; i < record_sn_count; i++) {
		write_u32(c->data + c->size, record_sn[i]);
		c->size += 4;
	}
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}

void SendStartBuilding(Connection *c,
                       uint8_t pos_x,
                       uint8_t pos_y,
                       uint16_t build_id,
                       uint8_t operation_type)
{
    c->size = 2;

    write_u16(c->data + c->size, _MSG_REQUEST_BUILDBEGIN);
    c->size += 2;

    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    /* v2.201.313 sendStartBuilding(TilePos, BuildID, opType) wire format,
       verified against libil2cpp.so (builder at file 0x32A3BA4 writes
       WriteU16(position), WriteU16(BuildID), WriteByte(opType); metadata:
       `sendStartBuilding(Vector2Int TilePos, ushort BuildID,
       BuildingOperationType operationType = 2)` with kUpgrade = 2).
       The position u16 is the same packed tile the 2001 record carries:
       low byte = x, high byte = y (Barracks reads as bytes E6 08).
       The old bot format `[x:u16][y:u16][build:u8]` placed BuildID in the
       middle field and put the building id into the operation byte, so the
       server rejected every request with RESP_BUILDINGERROR(2013). */
    uint16_t position_id = ((uint16_t)pos_y << 8) | pos_x;

    write_u16(c->data + c->size, position_id);
    c->size += 2;

    write_u16(c->data + c->size, build_id);
    c->size += 2;

    write_u8(c->data + c->size, operation_type);
    c->size += 1;

    write_u16(c->data, c->size);

    LOGI("[BUILD TX] 2003 pos=%u(%u,%u) build=%u op=%u payload[%u]=%02X %02X %02X %02X %02X\n",
         (unsigned)position_id, (unsigned)pos_x, (unsigned)pos_y, (unsigned)build_id,
         (unsigned)operation_type, (unsigned)(c->size - 8),
         c->data[8], c->data[9], c->data[10], c->data[11], c->data[12]);

    send_packet(c, true);
}

void ServerNewbieTeleport(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
    c->size += 2;
    
    // sequence id 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // item id
    write_u16(c->data + c->size, 0x03ed);
    c->size += 2;
    
    // item quantity 
    write_u16(c->data + c->size, 0x0001);
    c->size += 2;
    
    // Kingdom id
    write_u16(c->data + c->size, kingdom_id);
    c->size += 2;
    
    // zone id
    write_u16(c->data + c->size, zone_id);
    c->size += 2;
    
    // point id
    write_u8(c->data + c->size, point_id);
    c->size += 1;
    
    // zero
    write_zero(c->data + c->size, 5);
    c->size += 5;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}



void ServerRename(Connection *c, bool bought, uint16_t num, const char *name) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, 0x0450);
    c->size += 2;
    
    // sequence id
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // bought 
    write_u8(c->data + c->size, bought);
    c->size += 1;
    
    // num
    write_u16(c->data + c->size, num);
    c->size += 2;
    
    // item id
    write_u16(c->data + c->size, 0x03ee);
    c->size += 2;
    
    // name length 
    write_u8(c->data + c->size, strlen(name));
    c->size += 1;
    
    write_raw(c->data + c->size, name, 12); 
    c->size += 12;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
	
}

void RequestAllianceGiftInfo(Connection *c) {
	c->size = 2;

    write_u16(c->data + c->size, 0x0B2E);
    c->size += 2;

    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void RequestOpenAllianceGift(Connection *c, uint32_t SN) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, 0x0b30);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // SN
    write_u32(c->data + c->size, SN);
    c->size += 4;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void ServerMagicGateDoEvent(Connection *c, uint16_t n, uint8_t x) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, 0x2dd4);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // 
    write_u16(c->data + c->size, n);
    c->size += 2;
    
    write_u8(c->data + c->size, x);
    c->size += 1;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void RequestBuyItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_BUYITEM);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Type
    write_u8(c->data + c->size, Type);
    c->size += 1;
    
    // key
    write_u16(c->data + c->size, Key);
    c->size += 2;
    
    // item id
    write_u16(c->data + c->size, ItemID);
    c->size += 2;
    
    // quantity
    write_u16(c->data + c->size, Qty);
    c->size += 2;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
    // Debug printf("[INFO] RequestBuyItem(c, %u, %u, %u, %u)\n", Type, Key, ItemID, Qty);
}

void RequestBuyGiftItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty, const char Name[13]) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_GIFT);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Type
    write_u8(c->data + c->size, Type);
    c->size += 1;
    
    // key
    write_u16(c->data + c->size, Key);
    c->size += 2;
    
    // item id
    write_u16(c->data + c->size, ItemID);
    c->size += 2;
    
    // name
    write_raw(c->data + c->size, Name, 13);
    c->size += 13;
    
    // quantity
    write_u16(c->data + c->size, Qty);
    c->size += 2;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
    // Debug printf("[INFO] ServerBuyItem(c, %u, %u, %u, %s, %u)\n", Type, Key, ItemID, Name, Qty);
}

// It's for calculate how many migration scrolls require 
void RequsetWorldTeleportItemCount(Connection *c, uint64_t Power)
{
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_WORLD_TELEPORT_ITEM);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Power
    write_u64(c->data + c->size, Power);
    c->size += 8;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

void RequestDeleteAllianceGiftBox(Connection *c, uint32_t sn) {
	c->size = 2; // reserved 2 byte for packet length 
	
	// packet type 
	write_u16(c->data + c->size, 0x0b32);
	c->size += 2;
	
	// Sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// Serial Number 
	write_u32(c->data + c->size, sn);
	c->size += 4;
	
	/*
	// unknown! no information about this field 
	write_u8(c->data + c->size, 0);
	c->size += 1;
	*/
	
	// Update packet size
	write_u16(c->data, c->size);
	
	send_packet(c, true);
}

/*
 * Returns troops that have already reached their destination
 * (e.g. gathering, camping, or reinforcing) back to the castle.
 */
void RequestTroopTakeBack(Connection *c, uint8_t Index) {
	c->size = 2;
	
	// packet type 
    write_u16(c->data + c->size, _MSG_REQUEST_TROOPRETURN);
    c->size += 2;
    
    // Sequence 
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    
    // Index
    write_u32(c->data + c->size, Index);
    c->size += 4;
    
    write_u16(c->data, c->size);

    send_packet(c, true);
}

/*
 * Recalls a march before it reaches its destination.
 * Requires a Withdraw Squad item.
 */

/*
 * Hospital/healing packet helpers.
 *
 * The protocol IDs and the 16 uint32 payload slots are taken from the
 * client-side healing code in dump.cs.  The instant-heal client routine
 * explicitly writes 16 uint32 values after the sequence ID; the normal
 * healing command uses the same 16-slot troop selection.
 */
void RequestHealingTroops(Connection *c, const uint32_t troop_array[16])
{
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_HEALINGTROOP);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    for (int i = 0; i < 16; ++i) {
        write_u32(c->data + c->size, troop_array[i]);
        c->size += 4;
    }

    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestInstantHealing(Connection *c, const uint32_t troop_array[16])
{
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_INSTANTHEALING);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    for (int i = 0; i < 16; ++i) {
        write_u32(c->data + c->size, troop_array[i]);
        c->size += 4;
    }

    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestFinishHealing(Connection *c)
{
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_FINISHHEALING);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestCancelHealing(Connection *c)
{
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_CANCELHEALING);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestTroopRecall(Connection *c, uint8_t Index) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_USEITEM);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write item id
	write_u16(c->data + c->size, WITHDRAW_SQUAD);
	c->size += 2;
	
	// write item quantity
	write_u16(c->data + c->size, 1);
	c->size += 2;
	
	// write index
	write_u8(c->data + c->size, Index);
	c->size += 1;
	
	// 9 byte zero
	write_zero(c->data + c->size, 9); c->size += 9;
	
	// update packet size
	write_u16(c->data, c->size);
    
    // send packet
    send_packet(c, true); // true is encryption flag
}


void RequestJoinRally(Connection *c, const char *ally_name, const uint32_t troop_array[16])
{
    c->size = 2;

    // Packet type.
    write_u16(c->data + c->size, _MSG_REQUEST_JOIN_RALLY);
    c->size += 2;
	
    // Sequence ID.
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;

    // Rally leader name (13 bytes).
    write_raw(c->data + c->size, ally_name, 13);
    c->size += 13;

    // Build troop mask.
    uint32_t troop_mask = 0;

    for (int i = 0; i < 16; i++) {
        if (troop_array[i] != 0)
            troop_mask |= (1u << i);
    }

    // Write troop mask.
    write_u32(c->data + c->size, troop_mask);
    c->size += 4;

    // Write only selected troop counts.
    for (int i = 0; i < 16; i++) {
        if (troop_array[i] == 0)
            continue;

        write_u32(c->data + c->size, troop_array[i]);
        c->size += 4;
    }

    // Unknown flag (currently always 0).
    write_u8(c->data + c->size, 0);
    c->size += 1;

    // Packet length.
    write_u16(c->data, c->size);
    
    send_packet(c, true);
}

// Fetch all active rallies 
void RequestRallyList(Connection *c) {
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, _MSG_REQUEST_WARHALL_LIST);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
}

void RequestRallyDetail(Connection *c, uint8_t arg1, uint32_t arg2) {
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, _MSG_REQUEST_WARHALL_LIST_DETAIL);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// zero?
	write_u8(c->data + c->size, arg1);
	c->size += 1;
	
	// index
	write_u32(c->data + c->size, arg2);
	c->size += 4;
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
}

void Send_Mall_TestBuy(Connection *c, uint16_t type)
{
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, type);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	
	write_zero(c->data + c->size, 50);
	c->size += 50;
	
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
	
}

void RequestUnknown(Connection *c) {
	c->size = 2;
	
	// packet type
	write_u16(c->data + c->size, 0x252d);
	c->size += 2;
	
	// Sequence number 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// update packet size
	write_u16(c->data, c->size);
	
	// send request to server 
	send_packet(c, true);
	
	// 09 00 2d 25 0a 00 00 00 00 
}

void RecvBuyItem(Connection *c, const uint8_t *data, uint16_t size) {
	return; // exit 
	
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	printf("\nRecvBuyItem\n");
	printf("b: %u\n", b);
	printf("payload length: %u\n", size);
	
	// if (b != 0) return;
	
	uint8_t b2 = read_u8(data + offset); offset += 1;
	uint16_t type = read_u16(data + offset); offset+= 2;
	uint16_t item_id = read_u16(data + offset); offset+= 2;
	uint16_t quantity = read_u16(data + offset); offset+= 2;
	
	printf("b2: %u\n", b2);
	printf("type: %u\n", type);
	printf("item_id: %u\n", item_id);
	printf("quantity: %u\n", quantity);
	
	// if (b == 0) exit(0);
}

void HandleLoginValidate(Connection *c, const uint8_t *data, uint16_t size)
{
	uint16_t offset = 0;
	
	c->game_server.port = read_i32(data + offset); offset += 4;
	c->auth.igg_id      = read_i64(data + offset); offset += 8;
	read_bytes(c->game_server.addr, data + offset, 16); offset += 16;
	c->lobby_login = 1;
	
	// c->auth.igg_id = 2038130744;
	/*
	memset(c->game_server.addr, 0, sizeof(c->game_server.addr));
	
	memcpy(c->game_server.addr, "192.243.44.35", 13);
	c->game_server.port = 12024;
	*/
}


void RecvChatMessage(Connection *c, const uint8_t *data) {
	char player_name[13] = {0};
	char title_name[3] = {0};
	char str1[20] = {0};
	char str2[20] = {0};
	char message[4096] = {0};
	uint16_t offset = 0;
	
	memset(c->chat.player_name, 0,   13);
	memset(c->chat.message,     0, 4096);
	
	
	printf("RecvChatMessage\n");
	
	uint8_t b2 = read_u8(data + offset);
	offset += 1;
	
	printf("b2: %u\n", b2);
	
	if (c->app.version_major != 0) {
		uint8_t num3 = read_u8(data + offset);
		offset += 1;
		
		printf("num3: %u\n", num3);
	}
	
	if (b2 == 0 || b2 == 1) {
		uint16_t num4 = read_u16(data + offset);
		offset += 2;
		printf("num4: %u\n", num4);
		
		for (uint16_t i = 0; i < num4; i++) {
			// talkTime
			int64_t num5 = read_u64(data + offset);
			offset += 8;
			// playID
			int64_t num6 = read_u64(data + offset);
			offset += 8;
			// talkID
			int64_t num7 = read_u64(data + offset);
			offset += 8;
			
			uint8_t alli_or_king = read_u8(data + offset);
			offset += 1;
			uint8_t num8 = read_u8(data + offset);
			offset += 1;
			uint16_t pic_id = read_u16(data + offset);
			offset+= 2;
			read_bytes(c->chat.player_name, data + offset, 13);
			offset += 13;
			uint8_t vip_rank = read_u8(data + offset);
			offset+= 1;
			read_bytes(title_name, data + offset, 3);
			offset += 3;
			uint8_t special_block_id = read_u8(data + offset);
			offset += 1;
			uint8_t title_id = read_u8(data + offset);
			offset += 1;
			uint8_t b_have_arabic = read_u8(data + offset);
			offset += 1;
			uint16_t num9 = read_u16(data + offset);
			offset += 2;
					
			// printf("playID: %lu\n", num6);
			// printf("player_name: %s\n", player_name);
			// printf("vip_rank: %u\n", vip_rank);
			
			if (num8 == 108) {
				int message_talk_kind = 3;
				uint16_t message_kingdom_id = read_u16(data + offset);
				offset += 2;
				
				read_bytes(str1, data + offset, 3);
				offset += 3;
				read_bytes(str2, data + offset, 13);
				offset += 13;
				// printf("[%s] %s\n", str1, str2);
			} else if (num8 == 109) {
				int message_talk_kind = 0;
				uint16_t message_emoji_key = read_u16(data + offset);
				offset += 2;
				uint16_t message_num10 = read_u16(data + offset);
				offset += 2;
						
				printf("message_emoji_key: %u\n", message_emoji_key);
				printf("message_num10: %u\n", message_num10);
			} else if (num8 == 0) {
				read_bytes(c->chat.message, data + offset, num9);
				offset += num9;
				c->chat.message[num9] = '\0';
				// memcpy(res.player_name, player_name, 13);
				//p.read_bytes(message, num9);
			}
		}
	}
	
	return;
}


void RecvItemInfo(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t off = 0;
	
	eMsgState RecvItemState = (eMsgState)read_u8(data + off); off += 1;
	
	/*
	switch (RecvItemState) {
		case EMS_Begin:
			printf("Begin item list\n");
			break;
		case EMS_End:
			printf("End item list\n");
			break;
		case EMS_BeginAndEnd:
			printf("Complete item list\n");
			break;
		default:
			break;
	}*/
	
	
	uint16_t counts = read_u16(data + off); off += 2;
	
	printf("items count: %u\n", counts);
	
	for (int i = 0; i < counts; i++) {
		uint16_t item_id       = read_u16(data + off); off += 2;
		uint16_t item_quantity = read_u16(data + off); off += 2;
		
		c->items[item_id].quantity = item_quantity;
	}
	
	c->items_loaded = true;
}

const char *FormatTime(uint32_t totalSecs) {
    static char buffer[64];
    uint32_t secs = totalSecs;

    uint32_t days    = secs / 86400; secs %= 86400;
    uint32_t hours   = secs / 3600;  secs %= 3600;
    uint32_t minutes = secs / 60;    secs %= 60;

    if (days > 0 && hours > 0)
        snprintf(buffer, sizeof(buffer), "%ud %uh", days, hours);
    else if (days > 0)
        snprintf(buffer, sizeof(buffer), "%ud", days);
    else if (hours > 0 && minutes > 0)
        snprintf(buffer, sizeof(buffer), "%uh %um", hours, minutes);
    else if (hours > 0)
        snprintf(buffer, sizeof(buffer), "%uh", hours);
    else if (minutes > 0 && secs > 0)
        snprintf(buffer, sizeof(buffer), "%um %us", minutes, secs);
    else if (minutes > 0)
        snprintf(buffer, sizeof(buffer), "%um", minutes);
    else
        snprintf(buffer, sizeof(buffer), "%us", secs);

    return buffer;
}


void RecvIBuffInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->shield_info.active = false;
	
	uint8_t b = read_u8(data + offset); offset +=1;
	
	uint64_t end_time = 0;
	
	uint64_t remaining = 0;
	
	for (int i = 0; i < b; i++) {
		// quantity 
		uint16_t num    = read_u16(data + offset); offset += 2;
		// item id
		uint16_t itemID = read_u16(data + offset);  offset += 2;
		// use time
		uint64_t num2 = read_u64(data + offset); offset += 8;
		// duration 
		uint32_t num3 = read_u32(data + offset); offset += 4;
		
		switch (itemID) {
			case SHIELD_4H:
			case SHIELD_8H:
			case SHIELD_12H:
			case SHIELD_1D:
			case SHIELD_3D:
			case SHIELD_7D:
			case SHIELD_14D:
				end_time  = num2 + num3;
				remaining = (end_time > c->server_time) ? (end_time - c->server_time) : 0;
				
				c->shield_info.active     = true;
				c->shield_info.item_id    = itemID;
				c->shield_info.begin_time = num2;
				c->shield_info.duration   = num3;
				
				printf("[INFO] Shield expires in: %s\n", FormatTime(remaining));
				break;
		}
	}
	
	c->shield_info.loaded = true;
}

void RecvMarchData(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;
	c->player.max_marches     = read_u8(data + offset); offset += 1;
	c->player.current_marches = read_u8(data + offset); offset += 1;

	// There is more data include troops and location

	LOGI("RecvMarchData");
	LOGI("max_marches: %u", c->player.max_marches);
	LOGI("current_marches: %u", c->player.current_marches);

	/*
	 * Ground-truth capture of the authoritative MARCHEVENTDATA (2412) march
	 * snapshot.  This is the server's OWN serialization of the MarchData
	 * struct that RequestTroopMarchGather() must build back into a 6615, so
	 * the field layout here (type, PointCodes, troop counts, hero ids,
	 * timestamps) is exactly what a correctly-formed gather march looks like
	 * on the wire.  The bot currently reads only the leading two bytes
	 * (max/current marches) and discards the rest; capturing the full payload
	 * lets the 6615 request be corrected against the authoritative march
	 * shape instead of a guessed framing.
	 *
	 * If the account is mid-gather, the captured marches show the real
	 * GatherMarching wire layout; even a standby/camp march is useful as a
	 * negative reference (what a non-dispatched march looks like).
	 */
	{
		static unsigned seq = 0;
		if (size > 0 && seq < 64) {
			++seq;
			char bin_name[64];
			snprintf(bin_name, sizeof(bin_name), "march_2412_%04u.bin", seq);
			FILE *fb = fopen(bin_name, "wb");
			if (fb) { fwrite(data, 1, size, fb); fclose(fb); }

			FILE *ft = fopen("march_2412_dump.txt", "a");
			if (ft) {
				fprintf(ft, "=== MARCHEVENTDATA 2412 seq=%u size=%u max=%u current=%u ===\n",
				        seq, (unsigned)size,
				        (unsigned)c->player.max_marches,
				        (unsigned)c->player.current_marches);
				for (size_t off = 0; off < size; off += 16) {
					size_t n = size - off; if (n > 16) n = 16;
					fprintf(ft, "%04zx  ", off);
					for (size_t i = 0; i < 16; ++i) {
						if (i < n) fprintf(ft, "%02X ", data[off + i]);
						else       fprintf(ft, "   ");
					}
					fprintf(ft, " |");
					for (size_t i = 0; i < n; ++i) {
						unsigned char ch = data[off + i];
						fputc((ch >= 32 && ch <= 126) ? ch : '.', ft);
					}
					fprintf(ft, "|\n");
				}
				fclose(ft);
				LOGI("[MARCH 2412] captured seq=%u size=%u -> %s",
				     seq, (unsigned)size, bin_name);
			}
		}
	}

	// c->transfer.max_marches = c->player.max_marches;

	return;
}


uint16_t RoleAttrLevelUp(const uint8_t *data, int UpdateFlag) {
	uint16_t offset = 0;
	
	if ((UpdateFlag & 1) != 0) {
		read_u8(data + offset); offset += 1;
		
		// this.UpdateRoleAttrLevel(MP.ReadByte(-1));
	}
	
	if ((UpdateFlag & 2) != 0) {
		read_u32(data + offset); offset += 4;
		// this.UpdateRoleAttrExp(MP.ReadUInt(-1));
	}
	
	if ((UpdateFlag & 4) != 0) {
		read_u32(data + offset); offset += 4;
		// DataManager.Instance.Resource[4].Stock = MP.ReadUInt(-1);
	}
	
	if ((UpdateFlag & 8) != 0) {
		read_u16(data + offset); offset += 2;
		// this.UpdateRoleAttrMorale(MP.ReadUShort(-1));
	}
	
	if ((UpdateFlag & 16) != 0) {
		read_u64(data + offset); offset += 8;
		// DataManager.Instance.RoleAttr.LastMoraleRecoverTime = MP.ReadLong(-1);
	}
	
	if ((UpdateFlag & 32) != 0) {
		read_u16(data + offset); offset += 2;
		// this.UpdateRoleTalentPoint(MP.ReadUShort(-1));
	}
	
	return offset;
}

void RecvLoginRoleInfo(Connection *c, const uint8_t *data, uint16_t size) 
{
	uint16_t offset = 0;
	
	// printf("\n\n\n");
	
	uint32_t ReadPackNum = read_u32(data + offset); offset += 4;
	uint64_t UserId      = read_u64(data + offset); offset += 8;
	read_raw(c->player.name, data + offset, 13); offset += 13;
	uint16_t Head = read_u16(data + offset); offset += 2;
	
	offset += RoleAttrLevelUp(data + offset, 27);
	// p.read_bytes(d, 27);
		
	uint64_t ServerTime = read_u64(data + offset); offset += 8;
	uint64_t LogoutTime = read_u64(data + offset); offset += 8;
	uint64_t Guide = (unsigned long)read_u32(data + offset); offset += 4;
	uint32_t Diamond = read_u32(data + offset); offset += 4;
	
	c->player.gems = Diamond;
	
	uint8_t HeroSkillPoint = read_u8(data + offset); offset += 1;
	uint64_t LastHeroSPRecoverTime = read_u64(data + offset); offset += 8;
	uint16_t EnhanceEventHeroID = read_u16(data + offset); offset += 2;
	uint64_t HeroEnhanceEventTime_BeginTime = read_u64(data + offset); offset += 8;
	uint32_t HeroEnhanceEventTime_RequireTime = read_u32(data + offset); offset += 4;
	
	uint16_t StarUpEventHeroID = read_u16(data + offset); offset += 2;
	uint64_t HeroStarUpEventTime_BeginTime = read_u64(data + offset); offset += 8;
	uint32_t HeroStarUpEventTime_RequireTime = read_u32(data + offset); offset += 4;
	
	uint8_t temp[100];
	read_raw(temp, data + offset, 12); offset += 12;
	read_raw(temp, data + offset, 48); offset += 48;
	
	uint16_t abb = read_u16(data + offset); offset += 2;
	read_u16(data + offset); offset += 2;
	// printf("abb: %u\n", abb);
	
	// printf("Head: %u\n", Head);
	uint64_t BattleID = read_u64(data + offset); offset += 8;
	
	c->player.zone_id  = read_u16(data + offset); offset += 2;
	c->player.point_id = read_u8 (data + offset); offset += 1;
	/*
	uint16_t newZoneID = read_u16(data + offset); offset += 2;
	uint8_t newPointID = read_u8(data + offset); offset += 1;
	*/
	// bank_zoneId = newZoneID;
	// bank_pointId = newPointID;
	
	// map_pos_t pos = getTileMapPosbyPointCode(newZoneID, newPointID);
	// printf("Here: X:%u Y:%u\n", pos.x, pos.y);
	
	/*
	printf("name: %s\n", c->name);
	printf("userid: %lu\n", UserId);
	printf("ServerTime: %lu\n", ServerTime);
	
	printf("Diamond: %u\n", Diamond);
	
	printf("\n");
	*/
	// strcpy(login_name, name);
	// gems = Diamond;
	
	uint64_t LastChatterTime = read_u64(data + offset); offset += 8;
	uint32_t AllianceChatID  = read_u32(data + offset); offset += 4;
	c->player.power = read_u64(data + offset); offset += 8;
	c->player.kills = read_u64(data + offset); offset += 8;
	c->player.vip_point = read_u32(data + offset); offset += 4;
	uint64_t FirstTimer = read_u64(data + offset); offset += 8;
	uint32_t PrizeFlag = read_u32(data + offset); offset += 4;
	
	char PowerStr[30];
	char KillsStr[30];
				
	// format_number2(Power, PowerStr, 30);
	// format_number2(Kills, KillsStr, 30);
	
	/*
	printf("LastChatterTime: %lu\n", LastChatterTime);
	printf("AllianceChatID: %u\n", AllianceChatID);
	
	printf("Power: %s (%lu)\n", PowerStr, Power);
	printf("Kills: %s (%lu)\n", KillsStr, Kills);
	
	printf("VipPoint: %u\n", VipPoint);
	printf("FirstTimer: %lu\n", FirstTimer);
	printf("PrizeFlag: %u\n", PrizeFlag);
	*/
	
	uint64_t BookmarkTime = read_u64(data + offset); offset += 8;
	uint16_t BookmarkLimit = read_u16(data + offset); offset += 2;
	uint16_t BookmarkNum = read_u16(data + offset); offset += 2;
	
	/*
	printf("BookmarkTime: %ld\n", (int64_t)BookmarkTime);
	printf("BookmarkLimit: %u\n", BookmarkLimit);
	printf("BookmarkNum: %u\n", BookmarkNum);
	*/
	
	// read UpdateCorpsStageInfo data
	read_u8(data + offset); offset += 1;
	for (int i = 0; i < 10; i++) {
		/*
		NowCombatStageInfo[i].SoldierTableID = read_u8(data + offset); offset += 1;
		NowCombatStageInfo[i].Amount = read_u32(data + offset); offset += 4;
		
		printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, NowCombatStageInfo[i].SoldierTableID);
		printf("NowCombatStageInfo[%d].Amount: %u\n", i, NowCombatStageInfo[i].Amount);
		*/
		
		uint8_t SoldierTableID = read_u8(data + offset); offset += 1;
		uint32_t Amount = read_u32(data + offset); offset += 4;
		
		/*
		printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, SoldierTableID);
		printf("NowCombatStageInfo[%d].Amount: %u\n", i, Amount);
		*/
	}
	
	uint32_t CorpsStageWallDefence = read_u32(data + offset); offset += 4;
	// printf("CorpsStageWallDefence: %u\n", CorpsStageWallDefence);
	
	uint16_t SuccessiveLoginDays = read_u16(data + offset); offset += 2;
	// printf("SuccessiveLoginDays: %u\n", SuccessiveLoginDays);
	
	uint8_t TodayUseMoraleItemTimes = read_u8(data + offset); offset += 1;
	// printf("TodayUseMoraleItemTimes: %u\n", TodayUseMoraleItemTimes);
	
	uint8_t LordEquipBagSize = read_u8(data + offset); offset += 1;
	// printf("LordEquipBagSize: %u\n", LordEquipBagSize);
	
	uint64_t NextOnlineGiftOpenTime = read_u64(data + offset); offset += 8;
	// printf("NextOnlineGiftOpenTime: %ld\n", (int64_t)NextOnlineGiftOpenTime);
	
	uint8_t OnlineGiftOpenTimes = read_u8(data + offset); offset += 1;
	// printf("OnlineGiftOpenTimes: %u\n", OnlineGiftOpenTimes);
	
	uint16_t OnlineGiftItemID_ItemID = read_u16(data + offset); offset += 2;
	// printf("OnlineGiftItemID_ItemID: %u\n", OnlineGiftItemID_ItemID);
	
	uint16_t OnlineGiftItemID_Quantity = read_u16(data + offset); offset += 2;
	// printf("OnlineGiftItemID_Quantity: %u\n", OnlineGiftItemID_Quantity);
	
	int64_t LastLordEquipUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	int64_t LastItemMatUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	int64_t LastItemGemUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	uint16_t LordEquipEventData_ItemID = read_u16(data + offset); offset += 2;
	int8_t LordEquipEventData_Color = (int8_t)read_u8(data + offset); offset += 1;
	
	/*
	printf("LastLordEquipUpdateTime: %ld\n", LastLordEquipUpdateTime);
	printf("LastItemMatUpdateTime: %ld\n", LastItemMatUpdateTime);
	printf("LastItemGemUpdateTime: %ld\n", LastItemGemUpdateTime);
	printf("LordEquipEventData_ItemID: %u\n", LordEquipEventData_ItemID);
	printf("LordEquipEventData_Color: %d\n", LordEquipEventData_Color);
	*/
	
	int8_t LordEquipEventData_GemColor;
	
	for (int i = 0; i < 4; i++)
	{
		LordEquipEventData_GemColor = (int8_t)read_u8(data + offset); offset += 1;
		// printf("LordEquipEventData.GemColor[%d]: %d\n", i, LordEquipEventData_GemColor);
	}
	
	uint16_t LordEquipEventData_Gem;
	
	for (int j = 0; j < 4; j++) {
		LordEquipEventData_Gem = read_u16(data + offset); offset += 2;
		// printf("LordEquipEventData.Gem[%d]: %u\n", j, LordEquipEventData_Gem);
	}
	
	uint32_t LordEquipEventData_SerialNO = read_u32(data + offset); offset += 4;
	int64_t LordEquipEventTime_BeginTime = (int64_t)read_u64(data + offset); offset += 8;
	uint32_t LordEquipEventTime_RequireTime = read_u32(data + offset); offset += 4;
	int8_t VipLevelUp = (int8_t)read_u8(data + offset); offset += 1;
	
	/*
	printf("LordEquipEventData_SerialNO: %u\n", LordEquipEventData_SerialNO);
	printf("LordEquipEventTime_BeginTime: %ld\n", LordEquipEventTime_BeginTime);
	printf("LordEquipEventTime_RequireTime: %u\n", LordEquipEventTime_RequireTime);
	printf("VipLevelUp: %u\n", VipLevelUp);
	*/
	
	uint16_t nowKingdomID  = read_u16(data + offset); offset += 2;
	uint16_t homeKingdomID = read_u16(data + offset); offset += 2;
	
	c->player.current_kingdom_id = nowKingdomID;
	c->player.home_kingdom_id = homeKingdomID;
	// current_kingdom = nowKingdomID;
	
	// printf("nowKingdomID: %u\n", nowKingdomID);
	// printf("homeKingdomID: %u\n", homeKingdomID);
	
	/*
	DataManager.MapDataController.updateMyKingdom(MP.ReadUShort(-1), MP.ReadUShort(-1));
	DataManager.MapDataController.updateCapitalPoint(newZoneID, newPointID, DataManager.MapDataController.OtherKingdomData.kingdomID, false);
	*/
	
	// exit(1);
}

void format_number2(uint64_t num, char *out, size_t size) {
    if (num >= 1000000000ULL) {
    	snprintf(out, size, "%.*fB", 2, num / 1000000000.0);
        // snprintf(out, size, "%lluB", num / 1000000000ULL);
    } else if (num >= 1000000ULL) {
    	snprintf(out, size, "%.*fM", 2, num / 1000000.0);
        // snprintf(out, size, "%lluM", num / 1000000ULL);
    } else if (num >= 1000ULL) {
    	snprintf(out, size, "%.*fK", 2, num / 1000.0);
        // snprintf(out, size, "%lluK", num / 1000ULL);
    } else {
        snprintf(out, size, "%lu", num);
    }
}




#include <time.h>


void format_duration(time_t current_time, time_t expiry_time) {
    long diff = (long)(expiry_time - current_time);

    if (diff <= 0) {
        printf("Expired\n");
        return;
    }

    long days = diff / 86400;
    diff %= 86400;

    long hours = diff / 3600;
    diff %= 3600;

    long minutes = diff / 60;
    long seconds = diff % 60;
    
    if (days > 0) {
    printf("%ldd %ldh %ldm %lds\n",
           days, hours, minutes, seconds);
} else if (hours > 0) {
    printf("%ldh %ldm %lds\n",
           hours, minutes, seconds);
} else if (minutes > 0) {
    printf("%ldm %lds\n",
           minutes, seconds);
} else {
    printf("%lds\n", seconds);
}

}



uint64_t GetResourceAmount(
    const Connection *c,
    ResourceType type)
{
    switch (type) {
        case RESOURCE_FOOD:
            return c->resources.food;

        case RESOURCE_ROCK:
            return c->resources.rock;

        case RESOURCE_WOOD:
            return c->resources.wood;

        case RESOURCE_ORE:
            return c->resources.ore;

        case RESOURCE_GOLD:
            return c->resources.gold;

        default:
            return 0;
    }
}

const char *GetResourceName(ResourceType type)
{
    switch (type) {
        case RESOURCE_FOOD:
            return "food";

        case RESOURCE_ROCK:
            return "rock";

        case RESOURCE_WOOD:
            return "wood";

        case RESOURCE_ORE:
            return "ore";

        case RESOURCE_GOLD:
            return "gold";

        default:
            return "unknown";
    }
}


void BlackMarketTick(Connection *c) 
{
	if (!c->market.loaded)
		return;
	
	if (c->server_time >= c->market.refresh_time + 5) {
		printf("[MARKET] Refresh expired, requesting new data\n");
		RequestMissionInfo(c, 0);
		c->market.loaded = false;
	}
}

void HeartbeatTick(Connection *c)
{
    time_t now = time(NULL);

    if (now - c->last_heartbeat >= 15) {
        c->last_heartbeat = now;
        RequestHeartBeat(c);
    }
}

void LogMarketDecision(
    int slot,
    const char *action,
    ResourceType type,
    uint32_t need,
    uint64_t have)
{
    char need_str[32];
    char have_str[32];

    format_number2(need, need_str, sizeof(need_str));
    format_number2(have, have_str, sizeof(have_str));

    printf(
        "[MARKET] Slot %d -> %s (need %s %s, have %s)\n",
        slot,
        action,
        need_str,
        GetResourceName(type),
        have_str
    );
}

/*
bool ShouldBuyItem(Connection *c, const MarketItem *item)
{
	switch (item->item_id) {
		case BRIGHT_TALENT_ORB:
			return true;
		case SPEED_UP_30_MINUTE:
			return true;
		case SPEED_UP_60_MINUTE:
			return true;
		case SPEED_UP_3_HOUR:
			return true;
		case ANIMA:
			return true;
		case SPEED_UP_MERGING_3_HOUR: 
			return true;
		case SPEED_UP_RESEARCH_3_HOUR:
			return true;
		default:
			return false;
	}
}
*/

bool ShouldBuyItem(const MarketItem *item)
{
	// Return true; Buy everything 
	return true;
	
	switch (item->item_id) {
		case BRIGHT_TALENT_ORB:
		case SPEED_UP_30_MINUTE:
		case SPEED_UP_60_MINUTE:
		case SPEED_UP_3_HOUR:
		case SPEED_UP_MERGING_3_HOUR: 
		case SPEED_UP_RESEARCH_3_HOUR:
		case ANIMA:
			return true;
	}
	
	return false;
}

bool CanAffordItem(Connection *c, const MarketItem *item)
{
    switch (item->resource_kind) {
        case RESOURCE_FOOD:
            return c->resources.food >= item->resource_count;

        case RESOURCE_ROCK:
            return c->resources.rock >= item->resource_count;

        case RESOURCE_WOOD:
            return c->resources.wood >= item->resource_count;

        case RESOURCE_ORE:
            return c->resources.ore >= item->resource_count;

        case RESOURCE_GOLD:
            return c->resources.gold >= item->resource_count;
    }

    return false;
}

bool CanAffordMarketItem(Connection *c, const MarketItem *item)
{
    switch (item->resource_kind) {

        case RESOURCE_FOOD:
            return c->resources.food >=
                   c->market.reserve.food +
                   item->resource_count;

        case RESOURCE_ROCK:
            return c->resources.rock >=
                   c->market.reserve.rock +
                   item->resource_count;

        case RESOURCE_WOOD:
            return c->resources.wood >=
                   c->market.reserve.wood +
                   item->resource_count;

        case RESOURCE_ORE:
            return c->resources.ore >=
                   c->market.reserve.ore +
                   item->resource_count;

        case RESOURCE_GOLD:
            return c->resources.gold >=
                   c->market.reserve.gold +
                   item->resource_count;
    }

    return false;
}

bool CanSpendResource(Connection *c, ResourceType type)
{
	switch (type) {
		case RESOURCE_FOOD: return c->market.settings.spend_food;
		case RESOURCE_ROCK: return c->market.settings.spend_rock;
		case RESOURCE_WOOD: return c->market.settings.spend_wood;
		case RESOURCE_ORE:  return c->market.settings.spend_ore;
		case RESOURCE_GOLD: return c->market.settings.spend_gold;
	}
	
	return false;
}

void EvaluateBlackMarket(Connection *c)
{
	if (!c->market.settings.auto_trade)
		return;
	
	if (!c->market.loaded)
		return;
	
	if (c->market.buy_pending)
		return;
		
	for (int i = 0; i < 4; i++) 
	{
		// already purchased
		if (c->market.trade_status & (1 << i))
			continue;
		
		MarketItem *item = &c->market.items[i];
		
		if (!ShouldBuyItem(item)) {
			printf("[MARKET] Slot %d -> SKIP (unwanted item %u)\n", i, item->item_id);
			continue;
		}
		
		if (!CanSpendResource(c, item->resource_kind)) {
			printf("[MARKET] Slot %d -> SKIP (%s trading disabled)\n",
				i,
				GetResourceName(item->resource_kind)
			);
			
			continue;
		}
		
		if (!CanAffordMarketItem(c, item)) {
			uint64_t have = GetResourceAmount(c, (ResourceType)item->resource_kind);
			
			LogMarketDecision(
				i,
				"SKIP",
				item->resource_kind,
				item->resource_count,
				have
			);
			
			continue;
		}
		
		/*
		// skip unwanted items
		if (!ShouldBuyItem(c, item)) 
			continue;
		
		// not enough resources
		if (!CanAffordItem(c, item))
			continue;
		*/
		
		SendBlackMarketBuy(c, i);
		
		c->market.buy_pending = true;
		
		break;
    }
}

void TryEvaluateBlackMarket(Connection *c)
{
	if (!c->market.settings.auto_trade)
		return;
	
	if (!c->resource_loaded) 
		return;
		
	if (!c->market.loaded)
		return;
	
	EvaluateBlackMarket(c);
}

void RecvBlackMarket_Buy(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t result = read_u8(data + offset); offset++;
	
	if (result != 0) {
		c->market.buy_pending = false;
		printf("BlackMarket buy failed: %u\n", result);
		return;
	}
	
	uint8_t new_trade_status = read_u8(data + offset); offset++;
	uint8_t changed = new_trade_status ^ c->market.trade_status;
	
	for (int i = 0; i < 4; i++) 
	{
		if (((changed >> i) & 1) == 1) 
		{
			printf("Purchased slot %d\n", i);
			
			read_u16(data + offset); offset += 2;
			read_u16(data + offset); offset += 2;
			
			uint8_t resource_kind = read_u8(data + offset);
			offset += 1;
			
			uint32_t stock = read_u32(data + offset);
			offset += 4;
			
			printf("resource=%u stock=%u\n", resource_kind, stock);
		}
	}
	
	c->market.trade_status = new_trade_status;
	c->market.buy_pending = false;
	
	EvaluateBlackMarket(c);
}

void BlackMarketDataLog(Connection *c) {
	for (int i = 0; i < 4; i++) {
		printf("item_id:        %u\n", c->market.items[i].item_id);
		printf("item_count:     %u\n", c->market.items[i].item_count);
		printf("resource_kind:  %u\n", c->market.items[i].resource_kind);
		printf("resource_count: %u\n", c->market.items[i].resource_count);
		printf("rare:           %u\n", c->market.items[i].rare);
		printf("\n\n");
	}
}


void RecvBlackMarket_Data(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->market.refresh_time = read_u64(data + offset); offset += 8;
	c->market.trade_locks  = read_i8( data + offset); offset += 1;
	c->market.trade_status = read_u8( data + offset); offset += 1;
	
	for (int i = 0; i < 4; i++) {
		c->market.items[i].item_id        = read_u16(data + offset);  offset += 2;
		c->market.items[i].item_count     = read_u16(data + offset);  offset += 2;
		c->market.items[i].resource_kind  = read_u8( data + offset);  offset += 1;
		c->market.items[i].resource_count = read_u32(data + offset);  offset += 4;
		c->market.items[i].rare           = read_u8( data + offset);  offset += 1;
		
		/*
		printf("\n\n");
		printf("item_id: %u\n", c->market.items[i].item_id);
		printf("item_count: %u\n", c->market.items[i].item_count);
		printf("resource_kind: %u\n", c->market.items[i].resource_kind);
		printf("resource_count: %u\n", c->market.items[i].resource_count);
		printf("rare: %u\n", c->market.items[i].rare);
		*/
	}
	
	c->market.loaded = true;
	
	int8_t   extra_data = read_u8(data + offset); offset += 1;
	uint32_t extra_treasure_id = read_u32(data + offset); offset += 4;
	
	// printf("extra_data: %u\n", extra_data);
	if (extra_data != 1)
	{
		
	}
	
	// printf("[MARKET RESET] ");
	LOGI("Black Market resets in ");
	format_duration(c->server_time, c->market.refresh_time);
	
	TryEvaluateBlackMarket(c);
}

void ResourcesLog(Connection *c) {
	char food_str[20];
	char rock_str[20];
	char wood_str[20];
	char  ore_str[20];
	char gold_str[20];
	
	format_number2(c->resources.food, food_str, 20);
	format_number2(c->resources.rock, rock_str, 20);
	format_number2(c->resources.wood, wood_str, 20);
	format_number2(c->resources.ore,   ore_str, 20);
	format_number2(c->resources.gold, gold_str, 20);
	
	printf("[RESOURCE] FOOD = %s\n", food_str);
	printf("[RESOURCE] ROCK = %s\n", rock_str);
	printf("[RESOURCE] WOOD = %s\n", wood_str);
	printf("[RESOURCE] ORE  = %s\n", ore_str);
	printf("[RESOURCE] GOLD = %s\n", gold_str);
	return;
}

void RecvResources(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;

	c->resources.food  = read_u32(data + offset); offset += 4;
	c->production.food = read_i64(data + offset); offset += 8;
	
	c->resources.rock  = read_u32(data + offset); offset += 4;
	c->production.rock = read_i64(data + offset); offset += 8;
	
	c->resources.wood  = read_u32(data + offset); offset += 4;
	c->production.wood = read_i64(data + offset); offset += 8;
	
	c->resources.ore  = read_u32(data + offset); offset += 4;
	c->production.ore = read_i64(data + offset); offset += 8;
	
	c->resources.gold  = read_u32(data + offset); offset += 4;
	c->production.gold = read_i64(data + offset); offset += 8;
	
	// Server time when the resource values were last synchronized.
    c->resources_last_update = c->server_time;
    
	ResourcesLog(c);
	
	c->resource_loaded = true;
	
	if (c->market.loaded) {
        EvaluateBlackMarket(c);
    }
	return;
}


void RecvRefreshResources(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;

	c->resources.food  = read_u32(data + offset); offset += 4;
	c->resources.rock  = read_u32(data + offset); offset += 4;
	c->resources.wood  = read_u32(data + offset); offset += 4;
	c->resources.ore   = read_u32(data + offset); offset += 4;
	c->resources.gold  = read_u32(data + offset); offset += 4;
	
	return;
}

bool IsBuilding(uint16_t build_id)
{
    switch (build_id)
    {
        case 1:   // Timber
        case 2:   // Stone
        case 3:   // Ore
        case 4:   // Food
        case 5:   // Manor
        case 6:   // Barracks
        case 7:   // Infirmary
        case 8:   // Castle
        case 9:   // Vault
        case 10:  // Academy
        case 12:  // Wall
        case 13:  // Watchtower
        case 14:  // Embassy
        case 15:  // Workshop
        case 17:  // Trading Post
            return true;

        default:
            return false;
    }
}

const char *GetBuildingName(uint16_t build_id)
{
    switch (build_id)
    {
        case 1:  return "Timber";
        case 2:  return "Stone";
        case 3:  return "Ore";
        case 4:  return "Food";
        case 5:  return "Manor";
        case 6:  return "Barracks";
        case 7:  return "Infirmary";
        case 8:  return "Castle";
        case 9:  return "Vault";
        case 10: return "Academy";
        case 12: return "Wall";
        case 13: return "Watchtower";
        case 14: return "Embassy";
        case 15: return "Workshop";
        case 17: return "Trading Post";
        default: return "Unknown";
    }
}

static const uint32_t trading_post_supply_capacity[] = {
	0,      // Level 0 (unused)
	5000,   // Level 1
	15000,  // Level 2
	30000,  // Level 3
	50000,  // Level 4
	75000,  // Level 5
	105000,  // Level 6
	140000,  // Level 7
	180000,  // Level 8
	225000,  // Level 9
	275000,  // Level 10
	330000,  // Level 11
	400000,  // Level 12
	490000,  // Level 13
	600000,  // Level 14
	730000,  // Level 15
	880000,  // Level 16
	1050000,  // Level 17
	1250000,  // Level 18
	1450000,  // Level 19
	1650000,  // Level 20
	1850000,  // Level 21
	2050000,  // Level 22
	2250000,  // Level 23
	2500000,  // Level 24
	3000000,  // Level 25
};

uint32_t GetTradingPostSupplyCapacity(uint8_t level) {
	if (level > 25) return 0;
	
	return trading_post_supply_capacity[level];
}

void RecvAllBuildData(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	c->building_count = read_u8(data + offset); offset += 1;
	
	// printf("building:\n");
	uint8_t trading_post_lv = 0;
	
	for (int i = 0; i < c->building_count; i++) {
		/* Building record is [pos_x:u8][pos_y:u8][build:u16][level:u8].
		   The account is on the new-city layout, so position is a 2-D tile
		   (Vector2Int), not a single ushort. */
		c->building[i].pos_x       = read_u8(data + offset);  offset += 1;
		c->building[i].pos_y       = read_u8(data + offset);  offset += 1;
		c->building[i].build_id    = read_u16(data + offset); offset += 2;
		c->building[i].level       = read_u8(data + offset);  offset += 1;
		c->building[i].position_id = ((uint16_t)c->building[i].pos_y << 8) | c->building[i].pos_x;
		
		if (c->building[i].build_id == 17) {
			trading_post_lv = c->building[i].level;
		}
		
		// display only building 
		if (IsBuilding(c->building[i].build_id) == false) continue;
		
		// filter 
		// if (c->building[i].build_id != BUILD_MANOR) continue;
		
		/*
		printf("Building Name: %s\n", GetBuildingName(c->building[i].build_id));
		printf("Building Pos: %u\n", c->building[i].position_id);
		printf("Building Level: %u\n", c->building[i].level);
		
		printf("\n");
		*/
		
	}
	
	c->supply_capacity += GetTradingPostSupplyCapacity(trading_post_lv);
	printf(
    "[BANK] Trading Post Level=%u | Supply Capacity=%u\n",
    trading_post_lv,
    c->supply_capacity
);
}

void RecvMailInfo(Connection *c, const uint8_t *data)
{
    uint16_t offset = 0;

    MailInfo *mail = &c->mail;

    memset(mail, 0, sizeof(*mail));

    mail->serial_id = read_u32(data + offset);
    offset += 4;

    read_u8(data + offset); // b
    offset += 1;

    mail->send_time = read_u64(data + offset);
    offset += 8;

    mail->mail_type = read_u8(data + offset);
    offset += 1;

    mail->reply_id = read_u32(data + offset);
    offset += 4;

    mail->sender_head = read_u16(data + offset);
    offset += 2;

    mail->sender_kingdom = read_u16(data + offset);
    offset += 2;

    memcpy(mail->sender_tag, data + offset, 3);
    mail->sender_tag[3] = '\0';
    offset += 3;

    memcpy(mail->sender_name, data + offset, 13);
    mail->sender_name[13] = '\0';
    offset += 13;

    mail->extra_flag = read_u8(data + offset);
    offset += 1;

    uint8_t title_len = read_u8(data + offset);
    offset += 1;

    uint16_t content_len = read_u16(data + offset);
    offset += 2;

    mail->attachment_count = read_u8(data + offset);
    offset += 1;

    for (int i = 0; i < mail->attachment_count; i++) {
        offset += 5; // KingdomID + ZoneID + PointID
    }

    memcpy(mail->title, data + offset, title_len);
    offset += title_len;

    memcpy(mail->content, data + offset, content_len);

    printf("\n");
    printf("Sender: [%s] %s\n",
           mail->sender_tag,
           mail->sender_name);

    printf("Kingdom: %u\n",
           mail->sender_kingdom);

    printf("Title: %s\n",
           mail->title);

    printf("Content: %s\n",
           mail->content);
}


void RecvAllyPoint(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	uint8_t  status   = read_u8(data + offset);  offset += 1;
	uint16_t zone_id  = read_u16(data + offset); offset += 2;
	uint8_t  point_id = read_u8(data + offset);  offset += 1;
	
	map_pos_t pos;
	
	switch (status) {
		case 0: 
			pos = getTileMapPosbyPointCode(zone_id, point_id);
			
			printf(
				"Player found: Zone=%u Point=%u (%u,%u)\n",
				zone_id,
				point_id,
				pos.x,
				pos.y
			);
			
			if (c->transfer.state == TRANSFER_WAIT_TARGET) {
				c->transfer.zone_id  = zone_id;
				c->transfer.point_id = point_id;
				c->transfer.state = TRANSFER_SEND_MARCH;
			}
			
			break;
		case 1:
			printf("Target is in another kingdom\n");
			
			if (c->transfer.state == TRANSFER_WAIT_TARGET) {
				c->transfer.state = TRANSFER_FAILED;
			}
			
			break;
		default:
			printf("AllyPoint failed: %u\n", status);
			if (c->transfer.state == TRANSFER_WAIT_TARGET) {
				c->transfer.state = TRANSFER_FAILED;
			}
			break;
	}
}

void RecvAllianceHelp(Connection *c, const uint8_t *data) {
	printf("RecvAllianceHelp()\n");
}


void RecvAllianceMemberNeedsHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->help.record_sn = read_u32(data + offset); offset += 4;
	c->help.head      = read_u16(data + offset); offset += 2;
	c->help.rank      = read_u8(data + offset); offset += 1;
	read_raw(c->help.player_name, data + offset, 13); offset += 13;
	c->help.help_kind = (HelpKind)read_u8(data + offset); offset += 1;
	
	c->help.event_id = read_u16(data + offset); offset += 2;
	c->help.event_data_lv = read_u8(data + offset); offset += 1;
	c->help.already_helped = read_u8(data + offset); offset += 1;
	c->help.help_max = read_u8(data + offset); offset += 1;
	
	if (c->alliance.auto_help) {
		RequestHelpAllianceMember(c, 1, &c->help.record_sn);
		printf("[GUILD] Sent help to %s\n", c->help.player_name);
	}
}

void RecvPendingAllianceMembersNeedHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	bool do_not_update_ui = read_u8(data + offset); offset += 1;
	uint8_t count = read_u8(data + offset); offset += 1;
	
	for (int i = 0; i < count; i++) {
		c->help.record_sn = read_u32(data + offset); offset += 4;
		c->help.head      = read_u16(data + offset); offset += 2;
		c->help.rank      = read_u8(data + offset); offset += 1;
		read_raw(c->help.player_name, data + offset, 13); offset += 13;
		c->help.help_kind = (HelpKind)read_u8(data + offset); offset += 1;
		c->help.event_id = read_u16(data + offset); offset += 2;
		c->help.event_data_lv = read_u8(data + offset); offset += 1;
		c->help.already_helped = read_u8(data + offset); offset += 1;
		c->help.help_max = read_u8(data + offset); offset += 1;
		c->help.record_sn_arr[i] = c->help.record_sn;
	}
	
	if (c->alliance.auto_help) {
		RequestHelpAllianceMember(c, count, c->help.record_sn_arr);
		printf("[GUILD] Sent help to %u guild members\n", count);
	}
}

const char *GiftStatusToString(uint8_t status)
{
	switch (status) {
	case 0: return "NEW";
	case 1: return "OPEN";
	case 2: return "EXP";
	default: return "?";
	}
}

void RecvAllianceGiftInfo(Connection *c, const uint8_t *data) {
	if (!c->alliance.auto_open_gifts) return;
	
	uint16_t offset = 0;
	
	eMsgState AllianceGiftState = (eMsgState)read_u8(data + offset); offset += 1;
	
	/*
	switch (AllianceGiftState) {
		case EMS_Begin:
			printf("[Gift] Begin list\n");
			break;
		case EMS_Null:
			printf("[Gift] Middle list\n");
			break;
		case EMS_End:
			printf("[Gift] End list\n");
			break;
		case EMS_BeginAndEnd:
			printf("[Gift] Complete list\n");
			break;
	}
	*/
	
	uint8_t gift_count = read_u8(data + offset); offset += 1;
	
	if (gift_count == 0) return;
	
	if (AllianceGiftState == EMS_Begin || AllianceGiftState == EMS_BeginAndEnd) {
		c->alliance.gift_count = 0;
		c->alliance.gift_offset = 0;
		
		c->alliance.unopened_gift_count = 0;
	}
	
	// update 
	c->alliance.gift_count += gift_count;
	
	for (int i = 0; i < gift_count; i++) {
		c->alliance.gifts[c->alliance.gift_offset].sn          = read_u32(data + offset); offset += 4;
		c->alliance.gifts[c->alliance.gift_offset].status      = read_u8(data + offset);  offset += 1;
		c->alliance.gifts[c->alliance.gift_offset].rcv_time    = read_u64(data + offset); offset += 8;
		c->alliance.gifts[c->alliance.gift_offset].box_item_id = read_u16(data + offset); offset += 2;
		c->alliance.gifts[c->alliance.gift_offset].item_id     = read_u16(data + offset); offset += 2;
		c->alliance.gifts[c->alliance.gift_offset].num         = read_u16(data + offset); offset += 2;
		c->alliance.gifts[c->alliance.gift_offset].item_rank   = read_u8(data + offset);  offset += 1;
		read_raw(c->alliance.gifts[c->alliance.gift_offset].player, data + offset, 13); offset += 13;
		
		if (c->alliance.gifts[c->alliance.gift_offset].status == 0) {
			c->alliance.unopened_gift_count++;
			/*
			printf("[GIFT] ID %u FROM %s\n",
				c->alliance.gifts[c->alliance.gift_offset].sn,
				c->alliance.gifts[c->alliance.gift_offset].player
			);
			*/
			// RequestOpenAllianceGift(c, c->alliance.gifts[c->alliance.gift_offset].sn);
		}
		
		c->alliance.gift_offset++;
	}
	
	if (AllianceGiftState == EMS_End || AllianceGiftState == EMS_BeginAndEnd) {
		c->alliance.gift_offset = 0;
		printf("[GIFT] TOTAL COUNT: %u\n", c->alliance.gift_count);
		
		printf("[GIFT] UNOPENED COUNT: %u\n", c->alliance.unopened_gift_count);
	}
}

void RecvAllianceGiftOpen(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	if (b != 0 && b != 2)
		return;
	
	/*
	AllianceGift gift;
	
	gift.sn           = read_u32(data + offset); offset += 4;
	gift.status       = read_u8(data + offset);  offset += 1;
	gift.rcv_time     = read_u64(data + offset); offset += 8;
	gift.box_item_id  = read_u16(data + offset); offset += 2;
	gift.item_id      = read_u16(data + offset); offset += 2;
	gift.num          = read_u16(data + offset); offset += 2;
	gift.item_rank    = read_u8(data + offset);  offset += 1;
	*/
	
	uint32_t sn = read_u32(data + offset); offset += 4;
	
	printf("[OPENED] GIFT ID %u\n", sn);
	
	return;
	
	for (int i = 0; i < c->alliance.gift_count; i++) {
		AllianceGift *gift = &c->alliance.gifts[i];
		
		if (gift->sn != sn)
            continue;
            
        gift->status = 0xFF;

        RequestDeleteAllianceGiftBox(c, 0xFFFFFFFF);
        c->alliance.gift_state = GIFT_STATE_DELETING;
        break;
	}
	
}

void RecvDeleteAllianceGiftBox(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	if (b == 0) {
		uint16_t unknown = read_u16(data + offset); offset += 2;
		uint16_t num = read_u16(data + offset); offset += 2;
		
		// printf("[Gift] Delete %u gift(s)\n", num);
		
		for (int i = 0; i < (int)num; i++) {
			uint32_t mGift_UpdateSN = read_u32(data + offset); offset += 4;
			
			c->alliance.gift_state = GIFT_STATE_READY;
			
			printf("[DELETE] GIFT ID %u\n", mGift_UpdateSN);
			
			
			for (int i = 0; i < c->alliance.gift_count; i++) {
				AllianceGift *gift = &c->alliance.gifts[i];
				
				if (gift->sn != mGift_UpdateSN) 
					continue;
			
				gift->status = 0xFF;
				
				break;
			}
			
		}
	} else {
		printf("Delete error: b: %u\n", b);
	}
}



void RecvLoginError(Connection *c, const uint8_t *data) {
	uint8_t kind = read_u8(data);
	
	if (kind == 110) {
		printf("[ERROR] UPDATE CLIENT VERSION\n");
	} else if (kind == 9) {
		printf("[ERROR] LOGGING FROM ANOTHER DEVICE errorCode: %u\n", kind);
	} else {
		printf("Bootstrap Login failed: %u\n", kind);
	}
}


void RecvUseItem(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;

	uint8_t status = read_u8(data + offset); offset += 1;
	LOGI("[USEITEM] resp status=%u size=%u\n", (unsigned)status, (unsigned)size);

	if (status == 0) {
		uint16_t item_id       = read_u16(data + offset); offset += 2;
		uint16_t item_quantity = read_u16(data + offset); offset += 2;
		uint16_t num3          = read_u16(data + offset); offset += 2;

		LOGI("[USEITEM] accepted item=%u new_qty=%u num3=%u\n",
		     (unsigned)item_id, (unsigned)item_quantity, (unsigned)num3);
		// Server returns the updated inventory quantity after using the item.
		c->items[item_id].quantity = item_quantity;

		if (item_id == ADVANCE_RELOCATOR || item_id == RANDOM_RELOCATOR) {
			c->player.zone_id            = read_u16(data + offset); offset += 2;
			c->player.point_id           = read_u8(data + offset);  offset += 1;
			c->player.current_kingdom_id = read_u16(data + offset); offset += 2;
			return;
		} else if (item_id == SHIELD_4H || 
				item_id == SHIELD_8H || 
				item_id == SHIELD_12H || 
				item_id == SHIELD_1D || 
				item_id == SHIELD_3D || 
				item_id == SHIELD_7D) {
			
			c->shield_info.active     =   true;
			c->shield_info.pending    =   false;
			c->shield_info.quantity   =   read_u16(data + offset); offset += 2; // quantity 
			c->shield_info.item_id    =   read_u16(data + offset); offset += 2; // item id
			c->shield_info.begin_time =   read_u64(data + offset); offset += 8; // begin time
			c->shield_info.duration   =   read_u32(data + offset); offset += 4; // duration
			return;
		} else if (item_id == WITHDRAW_SQUAD) {
			// uint8_t march_index = read_u8(data + offset); offset += 1;
			// printf("[INFO] Withdraw Squad used. Recalled march #%u.\n", march_index + 1);
			printf("[INFO] Withdraw Squad used. Recalled march.\n");
			return;
		} else if (item_id == 0x03ed) {
			printf("Novice Relocator\n");

			return;
		} else {
			/* Consumable / bag resources (FOOD_*, etc.).  Live capture shows
			   the 1407 body is 149 bytes: the first 7 are the common header
			   (status+item_id+new_qty+num3); bytes 7-10 carry the amount of
			   bag resource granted by the item as a little-endian u32.
			   For FOOD_30K (1009=0x03F1) the capture consistently has
			     ... 00 00 | 30 75 00 00 | 00 00 00 ...
			          ^^^^^   ^^^^^^^^^
			          num3=0  0x7530 = 30,000  (remaining bytes are padding/zeros)
			   Without parsing this, resources.food stays 0 forever and
			   AutoTrainingTick starves even though the item was accepted
			   (new_qty decremented).  Apply it as a bonus on top of the
			   locally-tracked stock:
			     - We used food==0 as the "starved" signal and the upkeep is
			       negative (~-25M/hr), so the true value is 0.
			     - The server does not push a separate 2016 for item-used food
			       (confirmed: no 2016 ever appears after an accepted 1407).
			     - So we credit the grant directly.
			   Leave the existing 2014/2016 handlers as the authoritative sync;
			   this just makes bag-pack uses visible to the training resource
			   gate until the next FULL-RESOURCE sync arrives.
			   Supports both the single-resource-pack layout and future packs
			   that might carry different resources. */
			if (size >= 11) {
				uint32_t grant = read_u32(data + 7);
				if (grant != 0) {
					/* Identify which ResourceStock field to credit from the
					   item family.  Id bands are not dense, so check the known
					   tables (items.h).  Fall back to food for the packs we
					   know are food. */
					bool is_food =
					    (item_id == 0x0492 || item_id == 0x03F1 || item_id == 0x03F6 ||
					     item_id == 0x03FB || item_id == 0x0400 || item_id == 0x0445 ||
					     item_id == 0x044A || item_id == 0x044F);
					bool is_stone =
					    (item_id == 0x0493 || item_id == 0x03F2 || item_id == 0x03F7 ||
					     item_id == 0x03FC || item_id == 0x0401 || item_id == 0x0446 ||
					     item_id == 0x044B || item_id == 0x0450);
					bool is_timber =
					    (item_id == 0x0494 || item_id == 0x03F3 || item_id == 0x03F8 ||
					     item_id == 0x03FD || item_id == 0x0402 || item_id == 0x0447 ||
					     item_id == 0x044C || item_id == 0x0451);
					bool is_ore =
					    (item_id == 0x0495 || item_id == 0x03F4 || item_id == 0x03F9 ||
					     item_id == 0x03FE || item_id == 0x0403 || item_id == 0x0448 ||
					     item_id == 0x044D || item_id == 0x0452);
					bool is_gold =
					    (item_id == 0x03F5 || item_id == 0x03FA || item_id == 0x03FF ||
					     item_id == 0x0404 || item_id == 0x0449 || item_id == 0x044E ||
					     item_id == 0x0453);
					uint32_t *slot = NULL;
					const char *rname = "food";
					if (is_food)      { slot = &c->resources.food;  rname = "food";  }
					else if (is_stone) { slot = &c->resources.rock;  rname = "stone"; }
					else if (is_timber){ slot = &c->resources.wood;  rname = "timber"; }
					else if (is_ore)   { slot = &c->resources.ore;   rname = "ore";   }
					else if (is_gold)  { slot = &c->resources.gold;  rname = "gold";  }
					else {
						/* Unknown consumable: credit food so training is not
						   permanently stuck, but make it obvious. */
						slot = &c->resources.food;
						rname = "food(unknown-id)";
					}
					/* Clamp to avoid u32 wrap on pathological future captures. */
					uint64_t nv = (uint64_t)*slot + grant;
					if (nv > 0xFFFFFFFFULL) nv = 0xFFFFFFFFULL;
					*slot = (uint32_t)nv;
					/* Reset the resource sync timestamp so upkeep decay does
					   not immediately swallow the just-credited bonus before
					   training can spend it. */
					c->resources_last_update = c->server_time ? c->server_time : (uint64_t)time(NULL);
					LOGI("[USEITEM] bag %s +%u -> %u (item %u)\n",
					     rname, (unsigned)grant, (unsigned)*slot, (unsigned)item_id);
				}
			}
			return;
		}
	}
	return;
}


void RecvAllianceInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
    
    c->RoleAlliance.Channel = read_u32(data + offset); offset += 4;
	c->RoleAlliance.Rank = (AllianceRank)read_u8(data + offset); offset += 1;
	c->RoleAlliance.Apply = read_u8(data + offset); offset += 1;
	c->RoleAlliance.Money = read_u32(data + offset); offset += 4;
	
	return;
}

void RecvBuildingQueue(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	
	uint8_t queue_build_type = read_u8(data + offset); offset += 1;
	uint16_t position = read_u16(data + offset); offset += 2;
	uint16_t build_id = read_u16(data + offset); offset += 2;
	uint8_t level = read_u8(data + offset); offset += 1;
	uint64_t start_time = read_u64(data + offset); offset += 8;
	uint32_t total_time = read_u32(data + offset); offset += 4;

	/* The server confirmed a building queue entry.  Compute the finish time
	   and arm the completion path.  Previously this returned before setting
	   anything, so building_finish_time stayed 0 and the build automation
	   sent 2003 once and then stalled forever with queue_active set. */
	c->automation.building_queue_active = true;
	c->automation.building_finish_time = start_time + total_time;
	/* The client must echo this position back in the 2011/2008 finish-free
	   requests (sendBuildFinish writes a single position_id byte). */
	c->automation.building_position_id = position;
	LOGI("[AUTO][BUILD] queue event type=%u pos=%u build=%u level=%u finish=%llu",
	     (unsigned)queue_build_type, (unsigned)position, (unsigned)build_id,
	     (unsigned)level, (unsigned long long)c->automation.building_finish_time);
}

typedef enum
{
	EWATCHTOWER_LINE_TARGET_CAPITAL,
	EWATCHTOWER_LINE_TARGET_CAMP,
	EWATCHTOWER_LINE_TARGET_AMBUSH,
	EWATCHTOWER_ADDLINE_WONDER1,
	EWATCHTOWER_ADDLINE_WONDER2,
	EWATCHTOWER_ADDLINE_WONDER3,
	EWATCHTOWER_ADDLINE_WONDER4,
	EWATCHTOWER_ADDLINE_WONDER5,
	EWATCHTOWER_ADDLINE_WONDER6,
	EWATCHTOWER_ADDLINE_WONDER7
} EWATCHTOWER_LINE_TARGET;


typedef enum {
	GatherAttack,
	Attack,
	Wonder_GatherAttack,
	Wonder_Attack,
	None_Attack,
	PetAttack,
	Conflict,
	Detect,
	Wonder_Detect,
	None_Detect,
	Gather,
	Cantonment,
	Reinforce,
	Wonder_Reinforce,
	Supplies,
	_Max
} EAttackKind;


const char *GetShieldName(uint16_t item_id)
{
	switch (item_id) {
		case SHIELD_4H:  return "4 hour shield";
		case SHIELD_8H:  return "8 hour shield";
		case SHIELD_12H: return "12 hour shield";
		case SHIELD_1D:  return "1 day shield";
		case SHIELD_3D:  return "3 day shield";
		case SHIELD_7D:  return "7 day shield";
		case SHIELD_14D: return "14 day shield";
		default:         return "unknown shield";
	}
}


void DeployBestShield(Connection *c)
{
    if (c == NULL || c->shield_info.active || c->shield_info.pending)
        return;

    /* Prefer the configured priority list when one exists. */
    if (c->protection.shield_priority_count > 0) {
        UsePriorityShield(c);
        return;
    }

    /* Safe fallback: longest-duration shield first. */
    static const uint16_t fallback[] = {
        SHIELD_14D, SHIELD_7D, SHIELD_3D, SHIELD_1D,
        SHIELD_12H, SHIELD_8H, SHIELD_4H
    };

    for (size_t i = 0; i < sizeof(fallback) / sizeof(fallback[0]); ++i) {
        uint16_t item_id = fallback[i];
        if (c->items[item_id].quantity == 0)
            continue;

        RequestSimpleUseItem(c, item_id, 1);
        c->shield_info.pending = true;
        printf("[INFO] Deploying %s\n", GetShieldName(item_id));
        return;
    }

    printf("[WARN] No shield item available.\n");
}

void UsePriorityShield(Connection *c)
{
	// A shield activation request has already been sent.
	if (c->shield_info.pending)
		return;
	
	for (uint8_t i = 0; i < c->protection.shield_priority_count; i++) {
		uint16_t item_id = c->protection.shield_priority[i];
		
		switch (item_id) {
			case SHIELD_4H:
			case SHIELD_8H:
			case SHIELD_12H:
			case SHIELD_1D:
			case SHIELD_3D:
			case SHIELD_7D:
			case SHIELD_14D:
				break;
			default:
				continue;
		}
		
		if (c->items[item_id].quantity > 0) {
			RequestSimpleUseItem(c, item_id, 1);
			c->shield_info.pending = true;
			printf("[INFO] Using %s\n", GetShieldName(item_id));
			return;
		}
	}
	
	printf("[INFO] No shield items available.\n");
	return; // No shields available
}

/*
 * Automatically activate a shield when an enemy scout approaches the
 * turf, if the feature is enabled and no shield is currently active.
 */
static void WhenEnemyScoutApproachingTurf(Connection *c)
{
	// Protection system is disabled.
	if (!c->protection.enabled)
		return;
	
	// Shielding against incoming scouts is disabled.
	if (!c->protection.shield_on_incoming_scout)
		return;
	
	// A shield is already active; nothing to do.
	if (c->shield_info.active)
		return;
	
	// Activate the highest priority shield available.
	UsePriorityShield(c);
}

/*
 * Called when an enemy army is detected approaching the turf.
 * If protection is enabled for incoming attacks, activate the selected 
 * priority list shield available.
 */
static void WhenEnemyArmyApproachingTurf(Connection *c)
{
	// Protection system is disabled.
	if (!c->protection.enabled)
		return;
	
	// Shielding against incoming attacks is disabled.
	if (!c->protection.shield_on_incoming_attack) 
		return;
	
	// A shield is already active; nothing to do.
	if (c->shield_info.active)
		return;
	
	// Activate the highest priority shield available.
    UsePriorityShield(c);
}




static void WhenEnemyArmyApproachingCamp(Connection *c, uint8_t Index) {
	if (!c->protection.enabled) 
		return;
	
	if (!c->protection.recall_on_incoming_attack)
		return;
	
	RequestTroopTakeBack(c, Index);
}

static void WhenEnemyScoutApproachingCamp(Connection *c, uint8_t Index) {
	if (!c->protection.enabled) 
		return;
	
	if (!c->protection.recall_on_incoming_scout)
		return;
	
	RequestTroopTakeBack(c, Index);
}

// _MSG_RESP_WARHALL_INITLIST (0x9AD), size=12
// _MSG_RESP_WARHALL_INITLIST (0x9AD), size=12
// _MSG_RESP_UPDATEWATCHTOWER_UPDATELINE (0x98A), size=20
// [INFO ] PACKET TYPE: _MSG_RESP_TROOPHOME (0x973), size=105



// Prototype 
void RecvUpdateWatchTowerAddLineInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t LineID   = read_u32(data + offset); offset += 4;
	uint8_t  LineType = read_u8(data + offset);  offset += 1;
	uint8_t  Index    = read_u8(data + offset);  offset += 1;
	
	uint64_t MarchTimeData_BeginTime   = read_u64(data + offset); offset += 8;
	uint32_t MarchTimeData_RequireTime = read_u32(data + offset); offset += 4;
	
	EWATCHTOWER_LINE_TARGET ewatchtower_LINE_TARGET = (EWATCHTOWER_LINE_TARGET)read_u8(data + offset);  offset += 1;
	
	/*
	printf("\nRecvUpdateWatchTowerAddLineInfo\n");
	
	printf("LineID: %u\n", LineID);
	printf("LineType: %u\n", LineType);
	printf("Index: %u\n", Index);
	
	printf("MarchTimeData_BeginTime: %lu\n", MarchTimeData_BeginTime);
	printf("MarchTimeData_RequireTime: %u\n", MarchTimeData_RequireTime);
	
	printf("ewatchtower_LINE_TARGET: %u\n", ewatchtower_LINE_TARGET);
	*/
	
	switch (LineType) {
		case 5: 
		case 7:
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					WhenEnemyArmyApproachingTurf(c);
					printf("[WARNING] Enemy army is invading turf\n");
					break;
				case EWATCHTOWER_LINE_TARGET_CAMP:
					printf("[WARNING] Enemy army is invading camp\n");
					WhenEnemyArmyApproachingCamp(c, Index);
					break;
				case EWATCHTOWER_LINE_TARGET_AMBUSH:
					printf("Enemy army has set an ambush\n");
					// EAttackKind.None_Attack
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					printf("[WARNING] Enemy invading wonder\n");
					// EAttackKind.Wonder_Attack
					break;
			}
			break;
		case 6:
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					printf("[INFO] Garrison troops are approaching your Turf\n");
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					// EAttackKind.Cantonment
					printf("[INFO] Relieve troop are approaching to wonder\n");
					break;
				default:
					break;
			}
			break;
		case 8: 
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					WhenEnemyScoutApproachingTurf(c);
					printf("[WARNING] Enemy scout approaching to turf\n");
					break;
				case EWATCHTOWER_LINE_TARGET_CAMP:
					printf("[WARNING] Enemy scout approaching to camp\n");
					WhenEnemyScoutApproachingCamp(c, Index);
					break;
				case EWATCHTOWER_LINE_TARGET_AMBUSH:
					printf("Enemy target ambush\n");
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					printf("[WARNING] Enemy scout approaching to wonder\n");
					break;
			}
			break;
		case 10: 
			switch (ewatchtower_LINE_TARGET) {
				case EWATCHTOWER_LINE_TARGET_CAPITAL:
					printf("[INFO] Reinforcement troops are approaching your Turf\n");
					// EAttackKind.Reinforce
					break;
				case EWATCHTOWER_ADDLINE_WONDER1:
				case EWATCHTOWER_ADDLINE_WONDER2:
				case EWATCHTOWER_ADDLINE_WONDER3:
				case EWATCHTOWER_ADDLINE_WONDER4:
				case EWATCHTOWER_ADDLINE_WONDER5:
				case EWATCHTOWER_ADDLINE_WONDER6:
				case EWATCHTOWER_ADDLINE_WONDER7:
					printf("[INFO] Reinforcement troops are approaching wonder\n");
					break;
				default:
					break;
			}
			break;
		case 11:
			printf("EAttackKind.Gather\n");
			break;
		case 12: 
			printf("Someone approaching EAttackKind.Wonder_GatherAttack attacking\n");
			break;
		case 13: 
			printf("[INFO] Guild member is sending supplies\n");
			break;
		case 22: 
			printf("[WARNING] Familiar attack is approaching your Turf\n");
			break;
		default:
			break;
		
	}
	
	return;
}


void ShieldTick(Connection *c)
{
	if (!c->protection.enabled)
		return;
	
	if (!c->protection.shield_always_on)
		return;
		
	// Don't make any shield decisions until the server
	// has told us which buffs are currently active.
	if (!c->shield_info.loaded)
		return;
	
	// Already protected?
	if (c->shield_info.active) {
		// Still plenty of time remaining.
		uint64_t end_time = c->shield_info.begin_time + c->shield_info.duration;
		
		uint32_t remaining_time = (end_time > c->server_time) ? (uint32_t)(end_time - c->server_time) : 0;
		
		if (remaining_time > 300) // 5 minutes
			return;
	}
	
	// printf("[SHIELD] Shield expiring soon, renewing...\n");
	
	UsePriorityShield(c);
}

void AllianceGiftTick(Connection *c) {
	if (!c->alliance.auto_open_gifts) 
		return;
	
	if (c->alliance.gift_count == 0) 
		return;
	
	if (c->alliance.gift_state != GIFT_STATE_READY)
		return;
	
	for (int i = c->alliance.gift_offset; i < c->alliance.gift_count; i++) {
		AllianceGift *gift = &c->alliance.gifts[i];
		
		// Unopened gift → open it.
		if (gift->status == 0) {
			RequestOpenAllianceGift(c, gift->sn);
			c->alliance.gift_state = GIFT_STATE_OPENING;
			return;
		}
	}
	return;
}



// Not fully understand core mechanism yet
void RecvRoleUpdateInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	// return ;
	
	// printf("RecvRoleUpdateInfo\n");
	
	// printf("b: %u\n", b);
	
	
	// Guild gifts keys
	// 21 => PackPoint(packet.read_t()?),
	if (b == 21) {
		uint32_t v0 = read_u32(data + offset); offset += 4;
		// printf("PackPoint\n");
		// printf("v0: %u\n", v0);
		return;
	}
	
	// 41 => AllianceWarRegister(packet.read_t()?),
	if (b == 41) {
		uint8_t v0 = read_u8(data + offset); offset += 1;
		printf("AllianceWarRegister\n");
		printf("v0: %u\n", v0);
	}
	
	// 18 => GiftCount {
	if (b == 18) {
		uint16_t v0 = read_u16(data + offset); offset += 2;
		uint16_t v1 = read_u16(data + offset); offset += 2;
		
		printf("GiftCount\n");
		printf("v0: %u\n", v0);
		printf("v1: %u\n", v1);
		return;
	}
	
	// 19 => AllianceBox1 {
	// This will trigger when someone hunt monster in map and bot online 
	if (b == 19) {
		if (c->alliance.auto_open_gifts) {
			AllianceGift gift;
			
			gift.sn           = read_u32(data + offset); offset += 4;
			gift.status       = read_u8(data + offset);  offset += 1;
			gift.rcv_time     = read_u64(data + offset); offset += 8;
			gift.box_item_id  = read_u16(data + offset); offset += 2;
			gift.item_id      = read_u16(data + offset); offset += 2;
			gift.num          = read_u16(data + offset); offset += 2;
			gift.item_rank    = read_u8(data + offset);  offset += 1;
			read_raw(gift.player, data + offset, 13); offset += 13;
			
			uint32_t gift_update_sn = read_u32(data + offset); offset += 4;
			
			RequestOpenAllianceGift(c, gift.sn);
		}
		return;
	}
}


void RecvArmyGroupInfoLog(Connection *c) {
	printf("Total Troop: %u\n", c->troop.total);
	
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Infantry: %u\n", i + 1, c->troop.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Ranged: %u\n", i + 1, c->troop.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Cavalry: %u\n", i + 1, c->troop.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("T%d Siege: %u\n", i + 1, c->troop.siege[i]);
	}
	
}

void RecvArmyGroupInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	for (int index = 0; index < 4; index++) {
		c->troop.infantry[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.infantry[index];
	}
	
	for (int index = 0; index < 4; index++) {
		c->troop.ranged[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.ranged[index];
	}
	
	for (int index = 0; index < 4; index++) {
		c->troop.cavalry[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.cavalry[index];
	}
	
	for (int index = 0; index < 4; index++) {
		c->troop.siege[index] = read_u32(data + offset); offset += 4;
		c->troop.total += c->troop.siege[index];
	}
	
	c->troop.loaded = true;
	
	// RecvArmyGroupInfoLog(c);
	return;
}

static const uint32_t troop_might[4] = {
    2,   // T1
    8,   // T2
    24,  // T3
    36   // T4
};

uint64_t CalculateTroopMight(const TroopData *t)
{
    uint64_t might = 0;

    for (int i = 0; i < 4; i++) {
        might += (uint64_t)t->infantry[i] * troop_might[i];
        might += (uint64_t)t->ranged[i]   * troop_might[i];
        might += (uint64_t)t->cavalry[i]  * troop_might[i];
        might += (uint64_t)t->siege[i]    * troop_might[i];
    }

    return might;
}

void WoundedTroopDataLog(Connection *c) {
	// T1 might: 2
	// T2 might: 8
	// T3 might: 24
	// T4 might: 36
	
	printf("Total Wounded: %u\n", c->wounded.troop.total);
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Infantry: %u\n", i + 1, c->wounded.troop.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Ranged: %u\n", i + 1, c->wounded.troop.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Cavalry: %u\n", i + 1, c->wounded.troop.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Siege: %u\n", i + 1, c->wounded.troop.siege[i]);
	}
	
	
	printf("Healing Total: %u\n", c->wounded.healing.total);
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Infantry: %u\n", i + 1, c->wounded.healing.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Ranged: %u\n", i + 1, c->wounded.healing.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Cavalry: %u\n", i + 1, c->wounded.healing.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Siege: %u\n", i + 1, c->wounded.healing.siege[i]);
	}
	
	printf("num: %lu\n", c->wounded.num);
	printf("total time: %u\n", c->wounded.total_time);
	
	
	printf("Wounded Might: %llu\n",
       (unsigned long long)CalculateTroopMight(&c->wounded.troop));
       
   //printf("Healing Might: %llu\n",
     //  (unsigned long long)CalculateTroopMight(&c->wounded.healing));
}

void RecvWoundedTroopData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->wounded.troop.total = 0;
	c->wounded.healing.total = 0;
	
	// Infantry 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.infantry[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.infantry[index];
	}
	
	// Ranged 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.ranged[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.ranged[index];
	}
	
	// Cavalry 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.cavalry[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.cavalry[index];
	}
	
	// Siege
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.siege[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.siege[index];
	}
	
	
	// Infantry 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.infantry[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.infantry[index];
	}
	
	// Ranged 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.ranged[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.ranged[index];
	}
	
	// Cavalry 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.cavalry[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.cavalry[index];
	}
	
	// Siege
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.siege[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.siege[index];
	}
	
	c->wounded.num = read_u64(data + offset); offset += 8;
	c->wounded.total_time  = read_u32(data + offset); offset += 4;
	
	c->wounded.loaded = true;
	
	// WoundedTroopDataLog(c);
}


void RecvHealingResponse(Connection *c, const uint8_t *data)
{
    uint16_t offset = 0;
    uint8_t status = read_u8(data + offset);
    offset += 1;

    if (status != 0) {
        LOGE("[HEAL] Server rejected healing request: status=%u\n", status);
        return;
    }

    c->resources.food = read_u32(data + offset); offset += 4;
    c->resources.rock = read_u32(data + offset); offset += 4;
    c->resources.wood = read_u32(data + offset); offset += 4;
    c->resources.ore  = read_u32(data + offset); offset += 4;
    c->resources.gold = read_u32(data + offset); offset += 4;

    memset(&c->wounded.healing, 0, sizeof(c->wounded.healing));

    for (int i = 0; i < 16; ++i) {
        uint32_t v = read_u32(data + offset);
        offset += 4;

        int kind = i / 4;
        int tier = i % 4;

        if (kind == 0) c->wounded.healing.infantry[tier] = v;
        else if (kind == 1) c->wounded.healing.ranged[tier] = v;
        else if (kind == 2) c->wounded.healing.cavalry[tier] = v;
        else c->wounded.healing.siege[tier] = v;

        c->wounded.healing.total += v;
    }

    c->wounded.num = read_u64(data + offset);
    offset += 8;
    c->wounded.total_time = read_u32(data + offset);

    LOGI("[HEAL] Accepted: %u troops | total time=%us\n",
         c->wounded.healing.total, c->wounded.total_time);
}

void RecvDarknestBroadcast(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	char player_name[13];
	
	int64_t data_index = read_i64(data + offset); offset += 8;
	uint8_t level      = read_u8 (data + offset); offset += 1;
	read_raw(player_name, data + offset, 13); offset += 13;
	
	printf("[INFO] Lv.%u Darknest rally opened by %s\n", level, player_name);
	printf("data_index: %ld\n", data_index);
	
	// RequestRallyList(c);
}


typedef enum {
    T1_INFANTRY = 0,
    T1_RANGED   = 1,
    T1_CAVALRY  = 2,
    T1_SIEGE    = 3,
    
    T2_INFANTRY = 4,
    T2_RANGED   = 5,
    T2_CAVALRY  = 6,
    T2_SIEGE    = 7,
    
    T3_INFANTRY = 8,
    T3_RANGED   = 9,
    T3_CAVALRY  = 10,
    T3_SIEGE    = 11,
    
    T4_INFANTRY = 12,
    T4_RANGED   = 13,
    T4_CAVALRY  = 14,
    T4_SIEGE    = 15
} TroopSlot;

void DarknestRallyTick(Connection *c)
{
	if (!c->darknest.auto_join) 
		return;
	
    if (!c->rally.pending)
        return;

    if (c->server_time < c->rally.execute_time)
        return;
    printf("[DEBUG] RallyTick(Connection *c)\n");
    
    // RequestWarHallListDetail(c, 0, 0);
    
    /*
    uint32_t troop_array[16] = {0};
    troop_array[T1_CAVALRY] = 100;

    RequestJoinRally(c, c->rally.leader, troop_array);
    */
    c->rally.pending = false;
}


void RecvRallyCountData(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	// ally_rally_count: number of rallies opened by our alliance (e.g., darknest, fort, wonder, turf attacks).
	c->rally_status.active_rally_count = read_u32(data + offset);
	offset += 4;
	
	// enemy_rally_count: number of rallies opened against our alliance by enemy (targeting fort, wonder, turf, etc.).
	c->rally_status.being_rally_count = read_u32(data + offset);
	offset += 4;
	
	printf("ActiveRally: %u\n", c->rally_status.active_rally_count);
	printf("BeingRally: %u\n",  c->rally_status.being_rally_count);
	
	// No active or incoming rallies.
	if (c->rally_status.active_rally_count == 0 && c->rally_status.being_rally_count == 0) {
		return;
	}
	
	// RequestRallyDetail(c, 0, 0);
	// printf("RequestWarHallListDetail\n");
	
	// Fetch All rallies 
	// RequestRallyList(c);
	return;
}


void RecvWarBegin(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	char tmpS[13];
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	// War rally initiated by ally
	if (b == 0) {
		printf("[NOTICE] Rally Initiated!\n");
		return;
	}
	
	// war rally initiated by enemy
	if (b == 1) {
		read_raw(tmpS, data + offset, 13);
		offset += 13;
		
		printf("[WARNING] %s declared on your ally!\n", tmpS);
		return;
	}
}

/*
map_pos_t pos = getTileMapPosbyPointCode(enemy_zone_id, enemy_point_id);

printf(
    "[%s] %s -> %s | Target: K:%u X:%u Y:%u | Troops %u/%u | Time %us\n",
    (type == 0) ? "ALLY RALLY" : "ENEMY RALLY",
    ally_name,
    enemy_name,
    enemy_zone_id,
    pos.x,
    pos.y,
    ally_curr_troop,
    ally_max_troop,
    require_time
);*/

const char *GetNPCName(uint16_t npc_id)
{
	switch (npc_id) {
		case 1:  return "Darknest";
		// case 2: return "...";
		// case 3: return "...";
		default: return "Unknown NPC";
	}
}

void NPCRallyLog(const NPCRally *r)
{
	printf(
		"[NPC RALLY] %s -> %s Lv.%u (%u/%u) index: %u\n",
		r->ally_name,
		GetNPCName(r->enemy_npc_id),
		r->enemy_vip,
		r->ally_curr_troop,
		r->ally_max_troop,
		r->index
	);
}

void RecvNPCWallHallData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t index        = read_u32(data + offset); offset += 4;
	
	if (index >= 30) return;
	
	c->npc_rallies[index].index             = index;
	c->npc_rallies[index].kind              = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].begin_time        = read_i64(data + offset); offset += 8;
	c->npc_rallies[index].require_time      = read_u32(data + offset); offset += 4;
	c->npc_rallies[index].ally_zone_id      = read_u16(data + offset); offset += 2;
	c->npc_rallies[index].ally_point_id     = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].ally_head         = read_u16(data + offset); offset += 2;
	read_raw(c->npc_rallies[index].ally_name, data + offset,  13);     offset += 13;
	c->npc_rallies[index].ally_vip          = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].ally_rank         = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].ally_curr_troop   = read_u32(data + offset); offset += 4;
	c->npc_rallies[index].ally_max_troop    = read_u32(data + offset); offset += 4;
	c->npc_rallies[index].ally_home_kingdom = read_u16(data + offset); offset += 2;
	c->npc_rallies[index].enemy_head        = 255;
	c->npc_rallies[index].enemy_zone_id     = read_u16(data + offset); offset += 2;
	c->npc_rallies[index].enemy_point_id    = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].enemy_vip         = read_u8 (data + offset); offset += 1;
	c->npc_rallies[index].enemy_npc_id      = read_u16(data + offset); offset += 2;
	
	// RequestRallyDetail(c, 0, index);
	
	NPCRallyLog(&c->npc_rallies[index]);
	
	
	
	
	// if darknest auto join not enabled then leave 
	if (!c->darknest.auto_join) {
		return;
	}
	
	// must active rally
	if (c->npc_rallies[index].kind != 0) {
		return;
	}
	
	if (c->npc_rallies[index].enemy_vip < c->darknest.min_level || 
		c->npc_rallies[index].enemy_vip > c->darknest.max_level) {
		return;
	}
	
	if (c->darknest.formation_mode == DARKNEST_FORMATION_LEADER) {
		// RequestRallyDetail(c, 0, index);
		return;
	}
	
	
	
	// RequestRallyDetail(c, 0, index);
	return;
	
	
	
	map_pos_t pos = getTileMapPosbyPointCode(c->npc_rallies[index].ally_zone_id, c->npc_rallies[index].ally_point_id);
	
	printf("=== NPC Wall Hall Data ===\n");

	printf("index: %u\n", index);
	printf("kind: %u\n", c->npc_rallies[index].kind);
	printf("begin_time: %lld\n", (long long)c->npc_rallies[index].begin_time);
	printf("require_time: %u\n", c->npc_rallies[index].require_time);

	printf("ally_zone_id: %u\n", c->npc_rallies[index].ally_zone_id);
	printf("ally_point_id: %u\n", c->npc_rallies[index].ally_point_id);
	printf("ally_head: %u\n", c->npc_rallies[index].ally_head);
	printf("ally_name: %s\n", c->npc_rallies[index].ally_name);

	printf("ally_vip: %u\n", c->npc_rallies[index].ally_vip);
	printf("ally_rank: %u\n", c->npc_rallies[index].ally_rank);

	printf("ally_curr_troop: %u\n", c->npc_rallies[index].ally_curr_troop);
	printf("ally_max_troop: %u\n", c->npc_rallies[index].ally_max_troop);
	printf("ally_home_kingdom: %u\n", c->npc_rallies[index].ally_home_kingdom);

	printf("enemy_zone_id: %u\n", c->npc_rallies[index].enemy_zone_id);
	printf("enemy_point_id: %u\n", c->npc_rallies[index].enemy_point_id);
	printf("enemy_vip: %u\n", c->npc_rallies[index].enemy_vip);
	printf("enemy_npc_id: %u\n", c->npc_rallies[index].enemy_npc_id);

	printf("%s K:%u X:%u Y:%u\n", c->npc_rallies[index].ally_name, c->npc_rallies[index].ally_home_kingdom, pos.x, pos.y);
	
	printf("=== End NPC Wall Hall Data ===\n");
}

static const char *TroopNameByIndex(int index)
{
    static const char *names[20] = {
        "T1 Infantry",
        "T1 Ranged",
        "T1 Cavalry",
        "T1 Siege",

        "T2 Infantry",
        "T2 Ranged",
        "T2 Cavalry",
        "T2 Siege",

        "T3 Infantry",
        "T3 Ranged",
        "T3 Cavalry",
        "T3 Siege",

        "T4 Infantry",
        "T4 Ranged",
        "T4 Cavalry",
        "T4 Siege",
        
        "T5 Infantry",
        "T5 Ranged",
        "T5 Cavalry",
        "T5 Siege"
    };

    if (index < 0 || index >= 20)
        return "Unknown";

    return names[index];
}

// 0x1C94

/*
let kind = packet.read_u8()?;
let begin_time = packet.read_i64()?;
let require_time = packet.read_u32()?;
let ally_zone_id = packet.read_u16()?;
let ally_point_id = packet.read_u8()?;
let ally_head = packet.read_u16()?;
let ally_name = packet.read_string(13)?;
let ally_vip = packet.read_u8()?;
let ally_rank = packet.read_u8()?;
let ally_max_troop = packet.read_u32()?;
let enemy_head = u8::MAX as u16;
let enemy_zone_id = packet.read_u16()?;
let enemy_point_id = packet.read_u8()?;
let enemy_vip = packet.read_u8()?;
let enemy_npc_id = packet.read_u16()?;
let rec_num = packet.read_u8()?;
let self_participate = packet.read_u8()?;
let ally_home_kingdom = packet.read_u16()?;
                */
void RecvNPCWallHallDetail(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	printf("_MSG_RESP_NPC_WARHALL_INIT_LISTDETAIL\n");
	
	char ally_name[13];
	
	uint8_t  kind         = read_u8 (data + offset); offset += 1;
	int64_t  begin_time   = read_i64(data + offset); offset += 8;
	uint32_t require_time = read_u32(data + offset); offset += 4;
	uint16_t ally_zone_id = read_u16(data + offset); offset += 2;
	uint8_t ally_point_id = read_u8(data + offset);  offset += 1;
	uint16_t ally_head    = read_u16(data + offset); offset += 2;
	read_raw(ally_name, data + offset, 13); offset += 13;
	uint8_t ally_vip      = read_u8 (data + offset); offset += 1;
	uint8_t ally_rank     = read_u8 (data + offset); offset += 1;
	
	uint32_t ally_max_troop = read_u32(data + offset); offset += 4;
	
	uint16_t enemy_head     = 255;
	uint16_t enemy_zone_id  = read_u16(data + offset); offset += 2;
	uint8_t  enemy_point_id = read_u8 (data + offset); offset += 1;
	uint8_t  enemy_vip      = read_u8 (data + offset); offset += 1;
	uint16_t enemy_npc_id   = read_u16(data + offset); offset += 2;
	
	
	uint8_t rec_num = read_u8 (data + offset); offset += 1;
	uint8_t self_participate = read_u8 (data + offset); offset += 1;
	uint16_t ally_home_kingdom = read_u16(data + offset); offset += 2;
	
	
	
	
	
	map_pos_t pos = getTileMapPosbyPointCode(ally_zone_id, ally_point_id);
	
	
	printf("=== NPC Wall Hall Detail ===\n");

	printf("kind: %u\n", kind);
	printf("begin_time: %lld\n", (long long)begin_time);
	printf("require_time: %u\n", require_time);

	printf("ally_zone_id: %u\n", ally_zone_id);
	printf("ally_point_id: %u\n", ally_point_id);
	printf("ally_head: %u\n", ally_head);
	printf("ally_name: %s\n", ally_name);

	printf("ally_vip: %u\n", ally_vip);
	printf("ally_rank: %u\n", ally_rank);

	printf("ally_max_troop: %u\n", ally_max_troop);
	printf("ally_home_kingdom: %u\n", ally_home_kingdom);

	printf("enemy_zone_id: %u\n", enemy_zone_id);
	printf("enemy_point_id: %u\n", enemy_point_id);
	printf("enemy_vip: %u\n", enemy_vip);
	printf("enemy_npc_id: %u\n", enemy_npc_id);
	
	printf("rec_num: %u\n", rec_num);
	printf("self_participate: %u\n", self_participate);
	printf("ally_home_kingdom: %u\n", ally_home_kingdom);
	
	
	
	printf("%s K:%u X:%u Y:%u\n", ally_name, ally_home_kingdom, pos.x, pos.y);
	
	printf("=== End NPC Wall Hall Detail ===\n");
	
}

void RecvWallHallDel(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t  type  = read_u8(data + offset);  offset += 1;
	uint32_t index = read_u32(data + offset); offset += 4;
	
	if (type > 1) {
		return;
	}
	
	if (index >= 30) {
		return;
	}
	
	if (type == 0) {
		memset(&c->ally_rallies[index], 0, sizeof(Rally));
	} else {
		memset(&c->enemy_rallies[index], 0, sizeof(Rally));
	}
}


void RecvWallHallDetailClose(Connection *c, const uint8_t *data) {
	
}

void RecvWallHallDetail(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	printf("RecvWallHallDetail\n");
}

void RecvWallHallTroop(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t index = read_u32(data + offset); offset += 4;
	
	if (index >= 30) return;
	
	read_raw(c->rally_members[index].name, data + offset, 13); offset += 13;
	
	c->rally_members[index].vip      = read_u8 (data + offset); offset += 1;
	c->rally_members[index].rank     = read_u8 (data + offset); offset += 1;
	
	
	c->rally_members[index].begin_time   = read_i64(data + offset); offset += 8;
	c->rally_members[index].require_time = read_u32(data + offset); offset += 4;
	
	// unknown 
	// i don't know about this maybe mana troop or sigils flags
	uint8_t unk[6];
	read_raw(unk, data + offset, 6);
	offset += 6;
	
	printf("unknown 6 byte: ");
	for (int i = 0; i < 6; i++) {
		printf("%02x ", unk[i]);
	}
	printf("\n");
	
	uint32_t troop_flag   = read_u32(data + offset); offset += 4;
	
	c->rally_members[index].troop_total = 0;
	
	for (int i = 0; i < 20; i++) {
		if ((troop_flag >> i) & 1) {
			c->rally_members[index].troops[i] = read_u32(data + offset); offset += 4;
			c->rally_members[index].troop_total += c->rally_members[index].troops[i];
		} else {
			c->rally_members[index].troops[i] = 0;
		}
	}
	
	printf("[RALLY] Index  : %u\n",  index);
	printf("[RALLY] Name   : %s\n",  c->rally_members[index].name);
	printf("[RALLY] VIP    : %u\n",  c->rally_members[index].vip);
	printf("[RALLY] Rank   : %u\n",  c->rally_members[index].rank);
	printf("[RALLY] Troop  : %u\n",  c->rally_members[index].troop_total);
	printf("[RALLY] Btime  : %ld\n", c->rally_members[index].begin_time);
	printf("[RALLY] RTime  : %us\n", c->rally_members[index].require_time);
	
	for (int tier = 0; tier < 20; tier++) {
		if (c->rally_members[index].troops[tier] == 0)
			continue;
		
		printf("[RALLY] %-11s : %u\n",
			TroopNameByIndex(tier),
			c->rally_members[index].troops[tier]);
	}
	
	printf("\n");
}


/*
public void Init(MessagePacket MP)
	{
		this.Kind = MP.ReadByte(-1);
		this.EventTime.BeginTime = MP.ReadLong(-1);
		this.EventTime.RequireTime = MP.ReadUInt(-1);
		this.AllyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.AllyCapitalPoint.pointID = MP.ReadByte(-1);
		this.AllyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.AllyName, -1);
		this.AllyNameID = this.AllyName.GetHashCode(false);
		this.AllyVIP = MP.ReadByte(-1);
		this.AllyRank = MP.ReadByte(-1);
		if (this.PositionInfo != 1)
		{
			this.AllyCurrTroop = MP.ReadUInt(-1);
		}
		this.AllyMAXTroop = MP.ReadUInt(-1);
		this.EnemyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.EnemyCapitalPoint.pointID = MP.ReadByte(-1);
		this.EnemyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.EnemyName, -1);
		this.EnemyVIP = MP.ReadByte(-1);
		this.EnemyRank = MP.ReadByte(-1);
		MP.ReadStringPlus(3, this.EnemyAllianceTag, -1);
		this.EnemyHomeKingdom = MP.ReadUShort(-1);
		this.WonderID = byte.MaxValue;
		this.UIWonderID = byte.MaxValue;
	}
	
	*/

void WarRallyLog(Rally *r) {
	if (r->type == 0) {
		printf(
			"[ALLY RALLY] %s -> %s (%u/%u) index: %u\n",
			r->ally_name,
			r->enemy_name,
			r->ally_curr_troop,
			r->ally_max_troop,
			r->index
		);
	} else {
		printf(
			"[ENEMY RALLY] %s is rallying on %s reinforced (%u/%u)\n",
			r->enemy_name,
			r->ally_name,
			r->ally_curr_troop,
			r->ally_max_troop
		);
	}
}

// War rally information 
void RecvWallHallData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	Rally *rally;
	
	// type = 0 means our ally rallies; type = 1 is enemy rally
	uint8_t  type    =  read_u8 (data + offset); offset += 1;
	uint32_t index   =  read_u32(data + offset); offset += 4;
	
	if (type > 1) return;
	if (index >= 30) return;
	
	if (type == 0) {
		rally = &c->ally_rallies[index];
	} else if (type == 1) {
		rally = &c->enemy_rallies[index];
	} else {
		return;
	}
	
	rally->type         = type;
	rally->index        = index;
	
	rally->kind            = read_u8 (data + offset); offset += 1;
	rally->begin_time      = read_i64(data + offset); offset += 8;
	rally->require_time    = read_u32(data + offset); offset += 4;
	rally->ally_zone_id    = read_u16(data + offset); offset += 2;
	rally->ally_point_id   = read_u8(data + offset);  offset += 1;
	rally->ally_head       = read_u16(data + offset); offset += 2;
	read_raw(rally->ally_name, data + offset, 13); offset += 13;
	rally->ally_vip        = read_u8 (data + offset); offset += 1;
	rally->ally_rank       = read_u8 (data + offset); offset += 1;
	
	
	rally->ally_curr_troop = read_u32(data + offset); offset += 4;
	rally->ally_max_troop  = read_u32(data + offset); offset += 4;
	
	rally->enemy_zone_id   = read_u16(data + offset); offset += 2;
	rally->enemy_point_id  = read_u8 (data + offset); offset += 1;
	rally->enemy_head      = read_u16(data + offset); offset += 2;
	read_raw(rally->enemy_name, data + offset, 13); offset += 13;
	
	rally->enemy_vip       = read_u8(data + offset); offset += 1;
	rally->enemy_rank      = read_u8(data + offset); offset += 1;
	
	read_raw(rally->enemy_alliance_tag, data + offset, 3); offset += 3;
	
	rally->enemy_home_kingdom = read_u16(data + offset); offset += 2;
	
	/*
	WarRallyLog(rally);
	
	return;
	*/
	
	map_pos_t pos = getTileMapPosbyPointCode(rally->ally_zone_id, rally->ally_point_id);
	
	printf("\n\n\n");
	
	printf("=== Wall Hall Data ===\n");
	printf("type: %u\n", rally->type);
	printf("index: %u\n", rally->index);
	printf("kind: %u\n", rally->kind);
	printf("begin_time: %lld\n", (long long)rally->begin_time);
	printf("require_time: %u\n", rally->require_time);

	printf("ally_zone_id: %u\n", rally->ally_zone_id);
	printf("ally_point_id: %u\n", rally->ally_point_id);
	printf("ally_head: %u\n", rally->ally_head);
	printf("ally_name: %s\n", rally->ally_name);

	printf("ally_vip: %u\n", rally->ally_vip);
	printf("ally_rank: %u\n", rally->ally_rank);

	printf("ally_curr_troop: %u\n", rally->ally_curr_troop);
	printf("ally_max_troop: %u\n", rally->ally_max_troop);
	
	printf("enemy_zone_id: %u\n", rally->enemy_zone_id);
	printf("enemy_point_id: %u\n", rally->enemy_point_id);
	printf("enemy_head: %u\n", rally->enemy_head);
	printf("enemy_name: %s\n", rally->enemy_name);
	
	printf("enemy_vip:  %u\n", rally->enemy_vip);
	printf("enemy_rank: %u\n", rally->enemy_rank);
	
	printf("enemy_alliance_tag: %s\n", rally->enemy_alliance_tag);
	printf("enemy_home_kingdom: %u\n", rally->enemy_home_kingdom);
	
	printf("Ally : %s X:%u Y:%u\n", rally->ally_name, pos.x, pos.y);
	
	pos = getTileMapPosbyPointCode(rally->enemy_zone_id, rally->enemy_point_id);
	
	printf("Enemy: %s K:%u X:%u Y:%u\n", rally->enemy_name, rally->enemy_home_kingdom, pos.x, pos.y);
	// printf("Enemy: %s K:%u X:%u Y:%u\n", 
	
	printf("=== End Wall Hall Data ===\n");
	
	printf("\n\n\n");
	
	RequestRallyDetail(c, 0, index);
	
}


void RecvJoinedRallyData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	printf("RecvJoinedRallyData()\n");
	printf("b: %u\n", b);
	
	for (int i = 0; i < (int)b; i++)
	{
		// March Index
		uint8_t b2 = read_u8(data + offset); offset += 1;
		
		printf("b2: %u\n", b2);
		
		if (b2 >= 8)
		{
			return;
		}
		
		
		uint8_t state = read_u8(data + offset); offset += 1;
		uint64_t BeginTime   = read_u64(data + offset); offset += 8;
		uint32_t RequireTime = read_u32(data + offset); offset += 4;
		uint16_t zoneID = read_u16(data + offset); offset += 2;
		uint8_t pointID = read_u8(data + offset); offset += 1;
		
		printf("state: %u\n", state);
		printf("BeginTime: %lu\n", BeginTime);
		printf("RequireTime: %u\n", RequireTime);
		printf("zoneID: %u\n", zoneID);
		printf("pointID: %u\n", pointID);
		
		
		
		/*
		this.JoinedRallyDataType[(int)b2].MarchIndex = b2;
		byte state = MP.ReadByte(-1);
		this.JoinedRallyDataType[(int)b2].State = state;
		this.JoinedRallyDataType[(int)b2].MarchEventTime.BeginTime = MP.ReadLong(-1);
		this.JoinedRallyDataType[(int)b2].MarchEventTime.RequireTime = MP.ReadUInt(-1);
		this.JoinedRallyDataType[(int)b2].RallyPoint.zoneID = MP.ReadUShort(-1);
		this.JoinedRallyDataType[(int)b2].RallyPoint.pointID = MP.ReadByte(-1);
		this.SetQueueBarData(EQueueBarIndex.JoinedRallyBegin + (int)this.JoinedRallyDataType[(int)b2].MarchIndex, true, this.JoinedRallyDataType[(int)b2].MarchEventTime.BeginTime, this.JoinedRallyDataType[(int)b2].MarchEventTime.RequireTime);
		DataManager.Instance.SetRecvQueueBarData((int)(22 + this.JoinedRallyDataType[(int)b2].MarchIndex));
		*/
	}
	// this.CheckTroolCount();
}

/*
public void Init(MessagePacket MP)
	{
		this.Kind = MP.ReadByte(-1);
		this.EventTime.BeginTime = MP.ReadLong(-1);
		this.EventTime.RequireTime = MP.ReadUInt(-1);
		this.AllyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.AllyCapitalPoint.pointID = MP.ReadByte(-1);
		this.AllyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.AllyName, -1);
		this.AllyNameID = this.AllyName.GetHashCode(false);
		this.AllyVIP = MP.ReadByte(-1);
		this.AllyRank = MP.ReadByte(-1);
		if (this.PositionInfo != 1)
		{
			this.AllyCurrTroop = MP.ReadUInt(-1);
		}
		this.AllyMAXTroop = MP.ReadUInt(-1);
		this.EnemyCapitalPoint.zoneID = MP.ReadUShort(-1);
		this.EnemyCapitalPoint.pointID = MP.ReadByte(-1);
		this.EnemyHead = MP.ReadUShort(-1);
		MP.ReadStringPlus(13, this.EnemyName, -1);
		this.EnemyVIP = MP.ReadByte(-1);
		this.EnemyRank = MP.ReadByte(-1);
		MP.ReadStringPlus(3, this.EnemyAllianceTag, -1);
		this.EnemyHomeKingdom = MP.ReadUShort(-1);
		this.WonderID = byte.MaxValue;
		this.UIWonderID = byte.MaxValue;
	}
*/



/*
void RecvNPCWallHallData(Connection *c, const uint8_t *data)
{
		this.WarhallProtocol = 2476;
		byte b = 0;
		uint num = MP.ReadUInt(-1);
		if ((int)b >= this.WarHall.Length)
		{
			return;
		}
		WarlobbyData warlobbyData;
		bool warHallInstance = this.GetWarHallInstance(b, num, out warlobbyData);
		warlobbyData.PositionInfo = 0;
		warlobbyData.InitNpc(MP);
		if (warlobbyData.AllyNameID == this.RoleAttr.Name.GetHashCode(false))
		{
			this.Sponsor = (ushort)(num + 1U);
		}
		if (warHallInstance)
		{
			GUIManager.Instance.UpdateUI(EGUIWindow.UI_Alliance_Info, 4, 0);
		}
		else
		{
			GUIManager.Instance.UpdateUI(EGUIWindow.UI_Alliance_Info, 4, (int)num);
		}
		GUIManager.Instance.UpdateUI(EGUIWindow.UI_WarLobby, 0, 0);
		GameManager.OnRefresh(NetworkNews.Refresh_QBarTime, null);
	}*/

#include "tech_research.h"

void RecvTechnologyInfo(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;

	/* Store the research state into c->technology and mark research loaded.
	   Previously this function read everything into locals and returned
	   without storing anything, so research_loaded stayed false and the
	   research automation never ran at all.  The header layout mirrors the
	   TechnologyInfo struct (research_tech u16, unk u8, finish_time i64,
	   total_time u32, then 200 level bytes). */
	if (size < 15) {
		LOGI("RecvTechnologyInfo too short size=%u; ignoring", (unsigned)size);
		return;
	}

	c->technology.research_tech = read_u16(data + offset); offset += 2;

	c->technology.unk = read_u8(data + offset); offset += 1;

	c->technology.finish_time = read_u64(data + offset); offset += 8;

	c->technology.total_time = read_u32(data + offset); offset += 4;

	LOGI("RecvTechnologyInfo research_tech=%u finish_time=%lld total_time=%u",
	     (unsigned)c->technology.research_tech,
	     (long long)c->technology.finish_time,
	     (unsigned)c->technology.total_time);

	{
		size_t avail = size >= offset + 200 ? 200 : (size > offset ? size - offset : 0);
		memset(c->technology.tech_data, 0, sizeof(c->technology.tech_data));
		read_raw(c->technology.tech_data, data + offset, avail);
	}

	c->automation.research_loaded = true;
	
	
	const TechInfo *bag1 = GetTechInfo(TECH_BIGGER_BAGS_I);
	const TechInfo *bag2 = GetTechInfo(TECH_BIGGER_BAGS_II);
	const TechInfo *bag3 = GetTechInfo(TECH_BIGGER_BAGS_III);
	
	uint8_t lv1 = GetTechLevel(c->technology.tech_data, TECH_BIGGER_BAGS_I);
	uint8_t lv2 = GetTechLevel(c->technology.tech_data, TECH_BIGGER_BAGS_II);
	uint8_t lv3 = GetTechLevel(c->technology.tech_data, TECH_BIGGER_BAGS_III);
	
	/*
	if (bag1) {
		printf("%s Lv.%u: +%.1f%%\n", bag1->name, lv1, bag1->value[lv1]);
		c->supply_capacity += bag1->value[lv1];
	}
	
	if (bag2) {
		printf("%s Lv.%u: +%.1f%%\n", bag3->name, lv2, bag2->value[lv2]);
		c->supply_capacity += bag2->value[lv2];
	}
	
	if (bag3) {
		printf("%s Lv.%u: +%.1f%%\n", bag3->name, lv3, bag3->value[lv3]);
		c->supply_capacity += bag3->value[lv3];
	}
	
	
	printf("Supply Capacity: %u\n", c->supply_capacity);
	*/
	
	/*
	
	uint16_t tech_id = 1;
	
	for (size_t i = 0; i < 200; i++) {
		uint8_t b = AllTechData[i];
		
		uint8_t level1 = b & 0x0F;         // Odd TechID
		uint8_t level2 = (b >> 4) & 0x0F;  // Even TechID
		
		const TechInfo *bag1 = GetTechInfo(tech_id);
	
		
		printf("%s Lv.%u\n", 
			bag1->name,
			level1
		);
		
		tech_id++;
		
		const TechInfo *bag3 = GetTechInfo(tech_id);
		
		printf("%s Lv.%u\n",
			bag3->name,
			level2
		);
		
		tech_id++;
    
	}
	
	*/
	
	
	/*
	printf("%u\n", GetTechLevel(AllTechData, 1));
	printf("%u\n", GetTechLevel(AllTechData, 2));
	printf("%u\n", GetTechLevel(AllTechData, 280));
	*/
	//printf("\n\n");
}

/*
Debug log
Buy Item by gems
Withdraw Squad
seq_id: 24
Type: 1
Key: 2
ItemId: 1001
Qty: 1
*/

void RecvAddConflictLine(Connection *c, const uint8_t *data) {
	// Protection disabled.
	if (!c->protection.enabled) return;
	
	// Automatic conflict recall disabled.
	if (!c->protection.recall_on_incoming_conflict) return;
	
	uint8_t march_index = read_u8(data);
	
	// Army march index maximum 8 (0-7)
	// Valid march indices: 0-7.
	if (march_index >= 8) return;
	
	// Recall the affected march if a Withdraw Squad item is available.
	if (c->items[WITHDRAW_SQUAD].quantity == 0) return;
	
	RequestTroopRecall(c, march_index);
}

typedef enum {
    PK_NONE = 0,
    PK_FOOD,
    PK_STONE,
    PK_IRON,
    PK_WOOD,
    PK_GOLD,
    PK_CRYSTAL,
    PK_SP_MINE,
    PK_CITY,
    PK_CAMP,
    PK_NPC,
    PK_YOLK,
    PK_DYNAMIC_OBSTACLE,
    PK_UNDEFINED,
    PK_MAX
} POINT_KIND;

bool IsResources(uint layoutMapInfoID) {
	return layoutMapInfoID > PK_NONE && layoutMapInfoID < PK_CITY;
}

bool IsCityOrCamp(uint layoutMapInfoID) {
	return layoutMapInfoID == PK_CAMP || layoutMapInfoID == PK_CITY;
}

void point_kind_str(int pk, char *buf) {
    const char *str;

    switch (pk) {
        case PK_NONE: str = "PK_NONE"; break;
        case PK_FOOD: str = "food"; break;
        case PK_STONE: str = "stone"; break;
        case PK_IRON: str = "ore"; break;
        case PK_WOOD: str = "wood"; break;
        case PK_GOLD: str = "gold"; break;
        case PK_CRYSTAL: str = "gem"; break;
        case PK_SP_MINE: str = "PK_SP_MINE"; break;
        case PK_CITY: str = "PK_CITY"; break;
        case PK_CAMP: str = "PK_CAMP"; break;
        case PK_NPC: str = "PK_NPC"; break;
        case PK_YOLK: str = "PK_YOLK"; break;
        case PK_DYNAMIC_OBSTACLE: str = "PK_DYNAMIC_OBSTACLE"; break;
        case PK_UNDEFINED: str = "PK_UNDEFINED"; break;
        case PK_MAX: str = "PK_MAX"; break;
        default: str = "UNKNOWN"; break;
    }
    
    strcpy(buf, str);
}


// Not implemented core logic for parse map information
void RecvMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size) {
	uint16_t offset = 0;
	
}


uint32_t CalculateTransferAmount(Connection *c)
{
	uint32_t current = 0;
	uint32_t reserve = 0;
	
	switch (c->transfer.resource_type) {
		case RESOURCE_FOOD:
			current = c->resources.food;
			reserve = c->bank.reserve.food;
			break;
		case RESOURCE_ROCK:
			current = c->resources.rock;
			reserve = c->bank.reserve.rock;
			break;
		case RESOURCE_WOOD:
			current = c->resources.wood;
			reserve = c->bank.reserve.wood;
			break;
		case RESOURCE_ORE:
			current = c->resources.ore;
			reserve = c->bank.reserve.ore;
			break;
		case RESOURCE_GOLD:
			current = c->resources.gold;
			reserve = c->bank.reserve.gold;
			break;
		default:
			return 0;
	}
	
	/* Keep reserved resources. */
	if (current <= reserve)
		return 0;
	
	uint32_t available = current - reserve;
	
	/* Don't send more than requested. */
	if (available > c->transfer.remaining)
		available = c->transfer.remaining;
	
	/* Don't exceed Trading Post capacity. */
	if (available > c->supply_capacity)
		available = c->supply_capacity;
	
	return available;
}

// currently food sending available for testing purpose 
void SendResourceMarch(Connection *c) {
	printf(
    "[TRANSFER] SendResourceMarch reached | state=%d | remaining=%u | capacity=%u | marches=%u/%u\n",
    c->transfer.state,
    c->transfer.remaining,
    c->supply_capacity,
    c->player.current_marches,
    c->player.max_marches
);
	
	if (c->player.max_marches == 0)
{
    printf("[TRANSFER] ERROR: march data unavailable (max_marches=0)\n");
    c->transfer.state = TRANSFER_FAILED;
    return;
}

if (c->player.current_marches >= c->player.max_marches)
{
    printf(
        "[TRANSFER] BLOCKED: all marches busy (%u/%u)\n",
        c->player.current_marches,
        c->player.max_marches
    );

    c->transfer.state = TRANSFER_FAILED;
    return;
}
	if (c->transfer.remaining == 0) {
		c->transfer.state = TRANSFER_COMPLETE;
		return;
	}
	
	uint32_t amount = CalculateTransferAmount(c);
	
	if (amount == 0) {
		c->transfer.state = TRANSFER_COMPLETE;
		return;
	}
	
	Resources resource = {0};
	
	switch (c->transfer.resource_type) {
		case RESOURCE_FOOD: 
			resource.food = amount;
			break;
		case RESOURCE_ROCK: 
			resource.rock = amount;
			break;
		case RESOURCE_WOOD: 
			resource.wood = amount;
			break;
		case RESOURCE_ORE: 
			resource.ore = amount;
			break;
		case RESOURCE_GOLD: 
			resource.gold = amount;
			break;
	} 
	printf(
    "[TRANSFER] SENDING RSS | amount=%u | zone=%u | point=%u | type=%d\n",
    amount,
    c->transfer.zone_id,
    c->transfer.point_id,
    c->transfer.resource_type
);
	SendResource(c, resource, c->transfer.zone_id, c->transfer.point_id);
	
	c->transfer.remaining -= amount;
	c->transfer.state = TRANSFER_WAIT_MARCH;
	
	return;
}

void ResourceTransferTick(Connection *c)
{
	if (c->transfer.state == TRANSFER_IDLE) return;
	
	// Bot doesn't have trading post yet
	if (c->supply_capacity == 0)
{
    static int warned = 0;

    if (!warned)
    {
        printf(
            "[TRANSFER] BLOCKED: supply_capacity is 0. "
            "Trading Post/capacity data was not loaded correctly.\n"
        );
        warned = 1;
    }

    return;
}
	
	switch (c->transfer.state) {
		case TRANSFER_FIND_TARGET:
			/* Find player's location */
			RequestAllyPoint(c, c->transfer.target_name);
			c->transfer.timeout = time(NULL) + 10;   // wait up to 10 seconds
			c->transfer.state = TRANSFER_WAIT_TARGET;
			// printf("TRANSFER_FIND_TARGET\n");
			break;
		case TRANSFER_WAIT_TARGET: 
			if (time(NULL) >= c->transfer.timeout) {
				// printf("[TRANSFER] Target lookup timed out.\n");
				c->transfer.state = TRANSFER_FAILED;
			}
			break;
		case TRANSFER_SEND_MARCH:
			SendResourceMarch(c);
			// printf("TRANSFER_SEND_MARCH\n");
			break;
		case TRANSFER_WAIT_MARCH:
			/* Wait until march returns */
			
			break;
		case TRANSFER_COMPLETE:
			c->transfer.state = TRANSFER_IDLE;
			break;
		case TRANSFER_FAILED:
			c->transfer.state = TRANSFER_IDLE;
			break;
		default:
			break;
	}
}

void RecvSHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	// b == 1 means max march reached 
	if (b != 0) {
		c->transfer.state = TRANSFER_FAILED;
		return;
	}
	
	// marches counts
	uint8_t b2 = read_u8(data + offset); offset += 1;
	
	if (b2 >= 8)
	{
		c->transfer.state = TRANSFER_FAILED;
		return;
	}
	
	uint16_t zoneID      = read_u16(data + offset); offset += 2;
	uint8_t pointID      = read_u8(data + offset);  offset += 1;
	uint64_t BeginTime   = read_u64(data + offset); offset += 8;
	uint32_t RequireTime = read_u32(data + offset); offset += 4;
	
	uint32_t food_stock = read_u32(data + offset); offset += 4;
	uint32_t rock_stock = read_u32(data + offset); offset += 4;
	uint32_t wood_stock = read_u32(data + offset); offset += 4;
	uint32_t ore_stock  = read_u32(data + offset); offset += 4;
	uint32_t gold_stock = read_u32(data + offset); offset += 4;
	
	
	uint32_t food_send = read_u32(data + offset); offset += 4;
	uint32_t rock_send = read_u32(data + offset); offset += 4;
	uint32_t wood_send = read_u32(data + offset); offset += 4;
	uint32_t ore_send  = read_u32(data + offset); offset += 4;
	uint32_t gold_send = read_u32(data + offset); offset += 4;
	
	
	uint8_t PointKind = (POINT_KIND)read_u8(data + offset);  offset += 1;
	uint8_t DesPointLevel = read_u8(data + offset);  offset += 1;
	
	char DesPlayerName[13];
	read_raw(DesPlayerName, data + offset, 13); offset += 13;
	
	// c->transfer.cur_marches++;
	c->player.current_marches++;
	c->transfer.state = TRANSFER_SEND_MARCH;
}

void RecvHelp_Home(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;	
	
	if (b >= 8) {
		c->transfer.state = TRANSFER_FAILED;
		return;
	}
	
	if (b < 8) {
		c->resources.food  = read_u32(data + offset); offset += 4;
		c->resources.rock  = read_u32(data + offset); offset += 4;
		c->resources.wood  = read_u32(data + offset); offset += 4;
		c->resources.ore  = read_u32(data + offset);  offset += 4;
		c->resources.gold  = read_u32(data + offset); offset += 4;
		
		c->resources_last_update = c->server_time;
		
		if (c->player.current_marches > 0) {
			c->player.current_marches--;
		}
		
		if (c->transfer.remaining > 0) {
			c->transfer.state = TRANSFER_SEND_MARCH;
		} else {
			c->transfer.state = TRANSFER_COMPLETE;
		}
		
		return;
	}		
}

void RecvAllianceMemberInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	// printf("RecvAllianceMember()\n");
	
	uint8_t b  = read_u8(data + offset);  offset += 1;
	uint8_t b2 = read_u8(data + offset);  offset += 1;
	uint8_t b3 = read_u8(data + offset);  offset += 1;
	
	// printf("RecvAllianceMember(type=%u, finished=%u, count=%u)\n", b, b2, b3);
	
	if (b != 0 && b != 2)
	{
		// printf("RecvAllianceMember: unknown type %u\n", b);
		return;
	}
	
	if (b == 0) 
	{
		/* Start of a new full member list */
		if (c->alliance_member.data_finished) {
			c->alliance_member.recv_index = 0;
			c->alliance_member.count = 0;
		}
		
		c->alliance_member.count += b3;
		
		for (uint8_t i = 0; i < b3 && c->alliance_member.recv_index < MAX_ALLIANCE_MEMBER; i++)
		{
			
			AllianceMember *m = &c->alliance_member.member[c->alliance_member.recv_index];
			
			m->user_id         = read_i64(data + offset);  offset += 8;
			m->head            = read_u16(data + offset);  offset += 2;
			
			read_raw(m->name, data + offset, 13); offset += 13;
			m->rank            = read_u8(data + offset);   offset += 1;
			m->power           = read_u64(data + offset);  offset += 8;
			m->troop_kill_num  = read_u64(data + offset);  offset += 8;
			m->logout_time     = read_i64(data + offset);  offset += 8;
			m->white_list_flag = read_u8(data + offset);   offset += 1;
			c->alliance_member.recv_index++;
			
			// if (m->white_list_flag == 0) continue;
			
			/*
			printf("user_id: %ld\n", m->user_id);
			// printf("head: %u\n",     m->head);
			printf("name: %s\n",     m->name);
			// printf("rank: %u\n",     m->rank);
			// printf("power: %lu\n",   m->power);
			// printf("troop_kill_num: %lu\n", m->troop_kill_num);
			// printf("logout_time: %ld\n", m->logout_time);
			printf("white_list_flag: %u\n", m->white_list_flag);
			printf("\n");
			*/
		}
	} else if (b == 2) {
		for (int i = 0; i < (int)b3; i++) 
		{
			AllianceMember tmp;
			
			tmp.user_id         = read_i64(data + offset); offset += 8;
			tmp.head            = read_u16(data + offset); offset += 2;
			read_raw(tmp.name, data + offset, 13);         offset += 13;
			tmp.rank            = read_u8(data + offset);  offset += 1;
			tmp.power           = read_u64(data + offset); offset += 8;
			tmp.troop_kill_num  = read_u64(data + offset); offset += 8;
			tmp.logout_time     = read_i64(data + offset); offset += 8;
			tmp.white_list_flag = read_u8(data + offset);  offset += 1;
			
			for (int j = 0; j < MAX_ALLIANCE_MEMBER; j++)
			{
				if (c->alliance_member.member[j].user_id == tmp.user_id)
				{
					c->alliance_member.member[j] = tmp;
					
					printf("Updated member: %s (%lld)\n", tmp.name, (long long)tmp.user_id);
					
					break;
				}
			}
		}
	}
	
	
	c->alliance_member.data_finished = b2;
	
	if (b2 == 1)
	{
		c->alliance_member.recv_index = 0;
	}

}

/* =========================================================================
 * NEW SUBSYSTEM PROTOCOL IMPLEMENTATIONS
 * These functions are declared in protocol.h but were missing from protocol.c
 * Implemented based on packet_enum.h message IDs and connection.h state structs
 * ========================================================================= */

/* ---- Valhalla ---- */
void RequestValhallaInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_VALHALLA_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestValhallaInstantRevive(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_VALHALLA_INSTANT_REVIVE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestValhallaDivineRevive(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_VALHALLA_DIVINE_REVIVE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvValhallaInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->valhalla.loaded = true;
    c->valhalla.valhalla_level = read_u16(data + offset); offset += 2;
    c->valhalla.resources[0] = read_u64(data + offset); offset += 8;  // food
    c->valhalla.resources[1] = read_u64(data + offset); offset += 8;  // wood
    c->valhalla.resources[2] = read_u64(data + offset); offset += 8;  // stone
    c->valhalla.resources[3] = read_u64(data + offset); offset += 8;  // ore
    c->valhalla.troop_count = read_u32(data + offset); offset += 4;
    c->valhalla.instant_revive_available = read_u8(data + offset); offset += 1;
    c->valhalla.divine_revive_available = read_u8(data + offset); offset += 1;
    c->valhalla.next_refresh_time = read_u64(data + offset); offset += 8;

    c->valhalla.last_update = now32();
}

void RecvValhallaPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t prize_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < prize_count && i < 32; i++) {
        uint16_t mission_id = read_u16(data + offset); offset += 2;
        uint8_t claimed = read_u8(data + offset); offset += 1;
        // Store prize info if needed
    }

    c->valhalla.prize_loaded = true;
    c->valhalla.last_update = now32();
}

void RecvValhallaInstantRevive(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->valhalla.instant_revive_available = false;
        uint64_t revived_count = read_u64(data + offset); offset += 8;
        // Could store revived troop count
    }
    c->valhalla.last_update = now32();
}

void RecvValhallaDivineRevive(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->valhalla.divine_revive_available = false;
        uint64_t revived_count = read_u64(data + offset); offset += 8;
        // Could store revived troop count
    }
    c->valhalla.last_update = now32();
}

/* ---- Serial Gift ---- */
void RequestSerialGiftList(Connection *c) {
    LOGI("[CLAIM][TX] RequestSerialGiftList\n");
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SERIALGIFT_GIFTINFO);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestSerialGiftGet(Connection *c, uint8_t stage) {
    /* v2.201.313 native SerialGiftManager.SEND_REQUEST_SERIALGIFT_GETGIFT
       (file 0x3ae38e8) takes uint8_t stage and writes WriteByte(stage), NOT the
       u16 gift_id this bot sent before.  A 2-byte body shifted the parse ->
       Serial Gift never actually claimed.  stage is the gift's index within the
       serial-gift event, not its item id. */
    LOGI("[CLAIM][TX] RequestSerialGiftGet stage=%u\n", stage);
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SERIALGIFT_GETGIFT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8(c->data + c->size, stage);
    c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvSerialGiftList(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->serial_gift.loaded = true;
    c->serial_gift.event_count = read_u16(data + offset); offset += 2;

    for (int i = 0; i < c->serial_gift.event_count && i < 32; i++) {
        c->serial_gift.gifts[i].event_id = read_u16(data + offset); offset += 2;
        c->serial_gift.gifts[i].gift_id = read_u16(data + offset); offset += 2;
        c->serial_gift.gifts[i].status = read_u8(data + offset); offset += 1;
        c->serial_gift.gifts[i].expire_time = read_u64(data + offset); offset += 8;
    }
    c->serial_gift.next_refresh_time = read_u64(data + offset); offset += 8;

    c->serial_gift.last_update = now32();
}

void RecvSerialGiftGet(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t event_id = read_u16(data + offset); offset += 2;
    uint8_t result = read_u8(data + offset); offset += 1;

    if (result == 0) {
        // Find and update the gift status
        for (int i = 0; i < c->serial_gift.event_count && i < 32; i++) {
            if (c->serial_gift.gifts[i].event_id == event_id) {
                c->serial_gift.gifts[i].status = 1; // claimed
                break;
            }
        }
    }
    c->serial_gift.last_update = now32();
}

/* ---- Old Player Back (claim-only) ---- */
/* Note: no _MSG_REQUEST_OLDPLAYERBACK_INFO command exists; the server pushes
   RESP_OLDPLAYERBACK_INFO=3152 to the client, so we never send a request for
   that cmd id (sending a response-id as a request would be rejected/incorrect). */

void RequestOldPlayerBackGetGift(Connection *c) {
    LOGI("[CLAIM][TX] RequestOldPlayerBackGetGift\n");
    /* Claim the veteran-return gift. Header-only request. */
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_OLDPLAYERBACK_GETGIFT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvOldPlayerBackInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 2) return;

    c->oldplayerback.loaded = true;
    /* Layout unverified (stub dump). First u8 = event active flag. */
    uint8_t active = read_u8(data + offset); offset += 1;
    c->oldplayerback.event_loaded = (active != 0);
    if (offset < size) {
        uint8_t gift_flag = read_u8(data + offset); offset += 1;
        c->oldplayerback.gift_available = (gift_flag != 0) && !c->oldplayerback.gift_claimed;
    }
    c->oldplayerback.last_update = now32();
}

void RecvOldPlayerBackGetGift(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 1) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->oldplayerback.gift_claimed = true;
        c->oldplayerback.gift_available = false;
    }
    c->oldplayerback.last_update = now32();
}

/* ---- Gift Activity (claim-only) ---- */
void RequestGiftActivityList(Connection *c) {
    LOGI("[CLAIM][TX] RequestGiftActivityList\n");
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_GIFT_ACTIVITY_LIST);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestGiftActivityOpenBox(Connection *c, uint8_t box_id) {
    /* v2.201.313 native GiftActivityMgr.Send_MSG_REQUEST_GIFT_ACTIVITY_OPEN_GIFT_BOX
       (file 0x34a54f0) writes WriteByte(boxIdx): box_id is a single u8 index, NOT
       the u16 this bot sent before.  Writing 2 bytes made the server read the 2nd
       byte as the start of the next packet -> framing lost, open never applied. */
    LOGI("[CLAIM][TX] RequestGiftActivityOpenBox box=%u\n", box_id);
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_GIFT_ACTIVITY_OPEN_GIFT_BOX);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8 (c->data + c->size, box_id);
    c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvGiftActivityList(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 2) return;

    c->gift_activity.loaded = true;
    c->gift_activity.box_count = read_u16(data + offset); offset += 2;
    if (c->gift_activity.box_count > 32) c->gift_activity.box_count = 32;

    for (uint16_t i = 0; i < c->gift_activity.box_count; ++i) {
        if (offset + 3 > size) break;
        c->gift_activity.boxes[i].box_id = read_u16(data + offset); offset += 2;
        c->gift_activity.boxes[i].state = read_u8(data + offset); offset += 1;
        c->gift_activity.boxes[i].available = (c->gift_activity.boxes[i].state == 1);
        c->gift_activity.boxes[i].claimed = (c->gift_activity.boxes[i].state == 2);
    }
    c->gift_activity.last_update = now32();
}

void RecvGiftActivityOpenBox(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 3) return;

    uint16_t box_id = read_u16(data + offset); offset += 1; /* box idx echo (best-effort) */
    uint8_t result = read_u8(data + offset); offset += 1;

    for (uint16_t i = 0; i < c->gift_activity.box_count; ++i) {
        if (c->gift_activity.boxes[i].box_id == box_id || i == box_id) {
            c->gift_activity.boxes[i].state = result == 0 ? 2 : 1;
            c->gift_activity.boxes[i].available = (result != 0);
            c->gift_activity.boxes[i].claimed = (result == 0);
            break;
        }
    }
    c->gift_activity.last_update = now32();
}

/* ---- Week Challenge ---- */
void RequestWeekChallengeInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_WEEKCHALLENGE_RANK);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestWeekChallengeBuy(Connection *c, uint16_t item_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_WEEK_CHALLENGE_ITEM_SHOP_BUY);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, item_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestWeekChallengePrize(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_WEEKCHALLENGE_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvWeekChallengeInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->week_challenge.loaded = true;
    c->week_challenge.challenge_id = read_u16(data + offset); offset += 2;
    c->week_challenge.week = read_u8(data + offset); offset += 1;
    c->week_challenge.score = read_u64(data + offset); offset += 8;
    c->week_challenge.rank = read_u16(data + offset); offset += 2;

    c->week_challenge.shop_items_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->week_challenge.shop_items_count && i < 32; i++) {
        c->week_challenge.shop_items[i].item_id = read_u16(data + offset); offset += 2;
        c->week_challenge.shop_items[i].price = read_u16(data + offset); offset += 2;
        c->week_challenge.shop_items[i].currency_type = read_u16(data + offset); offset += 2;
        c->week_challenge.shop_items[i].limit = read_u8(data + offset); offset += 1;
        c->week_challenge.shop_items[i].bought = read_u8(data + offset); offset += 1;
    }

    c->week_challenge.prize_claimed = read_u8(data + offset); offset += 1;
    c->week_challenge.next_refresh_time = read_u64(data + offset); offset += 8;

    c->week_challenge.last_update = now32();
}

void RecvWeekChallengeBuy(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t item_id = read_u16(data + offset); offset += 2;
    uint8_t result = read_u8(data + offset); offset += 1;

    if (result == 0) {
        for (int i = 0; i < c->week_challenge.shop_items_count && i < 32; i++) {
            if (c->week_challenge.shop_items[i].item_id == item_id) {
                c->week_challenge.shop_items[i].bought++;
                break;
            }
        }
    }
    c->week_challenge.last_update = now32();
}

void RecvWeekChallengePrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->week_challenge.prize_claimed = 1;
    }
    c->week_challenge.last_update = now32();
}

/* ---- Adventure ---- */
void RequestAdventureMissionInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ADVENTURE_STARTQUEST);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAdventureStartQuest(Connection *c, uint16_t mission_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ADVENTURE_STARTQUEST);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, mission_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAdventureHunt(Connection *c, uint16_t monster_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ADVENTURE_HUNT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, monster_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvAdventureMissionInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->adventure.loaded = true;
    c->adventure.mission_id = read_u16(data + offset); offset += 2;
    c->adventure.mission_type = read_u8(data + offset); offset += 1;
    c->adventure.progress = read_u64(data + offset); offset += 8;
    c->adventure.target = read_u64(data + offset); offset += 8;
    c->adventure.completed = read_u8(data + offset); offset += 1;
    c->adventure.prize_claimed = read_u8(data + offset); offset += 1;

    c->adventure.hunt_monsters_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->adventure.hunt_monsters_count && i < 16; i++) {
        c->adventure.monsters[i].monster_id = read_u16(data + offset); offset += 2;
        c->adventure.monsters[i].level = read_u8(data + offset); offset += 1;
        c->adventure.monsters[i].zone_id = read_u16(data + offset); offset += 2;
        c->adventure.monsters[i].point_id = read_u8(data + offset); offset += 1;
    }

    c->adventure.next_refresh_time = read_u64(data + offset); offset += 8;

    c->adventure.last_update = now32();
}

void RecvAdventureMissionPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->adventure.prize_claimed = 1;
    }
    c->adventure.last_update = now32();
}

void RecvAdventureHunt(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t monster_id = read_u16(data + offset); offset += 2;
        // Mark monster as hunted
        for (int i = 0; i < c->adventure.hunt_monsters_count && i < 16; i++) {
            if (c->adventure.monsters[i].monster_id == monster_id) {
                c->adventure.monsters[i].point_id = 0; // mark as hunted
                break;
            }
        }
    }
    c->adventure.last_update = now32();
}

/* ---- Relics ---- */
void RequestRelicsInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RELICS_GACHA_DATA);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestRelicsGacha(Connection *c, uint16_t gacha_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RELICS_GACHA_CONTENT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, gacha_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestRelicsSynthesize(Connection *c, uint16_t relic_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SYNTHESIS_RELIC);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, relic_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestRelicsEnhance(Connection *c, uint16_t relic_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RELICS_ENHANCE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, relic_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvRelicsInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->relics.loaded = true;
    c->relics.gacha_list_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->relics.gacha_list_count && i < 16; i++) {
        c->relics.gacha_list[i].gacha_id = read_u16(data + offset); offset += 2;
        c->relics.gacha_list[i].gacha_type = read_u8(data + offset); offset += 1;
        c->relics.gacha_list[i].price = read_u16(data + offset); offset += 2;
        c->relics.gacha_list[i].currency_type = read_u16(data + offset); offset += 2;
    }

    c->relics.relics_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->relics.relics_count && i < 32; i++) {
        c->relics.relics[i].relic_id = read_u16(data + offset); offset += 2;
        c->relics.relics[i].level = read_u8(data + offset); offset += 1;
        c->relics.relics[i].quality = read_u8(data + offset); offset += 1;
        c->relics.relics[i].exp = read_u64(data + offset); offset += 8;
    }

    c->relics.coin_points = read_u64(data + offset); offset += 8;
    c->relics.next_refresh_time = read_u64(data + offset); offset += 8;

    c->relics.last_update = now32();
}

void RecvRelicsGacha(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t gacha_id = read_u16(data + offset); offset += 2;
        uint16_t relic_id = read_u16(data + offset); offset += 2;
        uint8_t quality = read_u8(data + offset); offset += 1;
        // Add to relics list if not exists
        for (int i = 0; i < c->relics.relics_count && i < 32; i++) {
            if (c->relics.relics[i].relic_id == relic_id) {
                c->relics.relics[i].level = 1;
                c->relics.relics[i].quality = quality;
                c->relics.relics[i].exp = 0;
                break;
            }
        }
    }
    c->relics.last_update = now32();
}

void RecvRelicsSynthesize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t relic_id = read_u16(data + offset); offset += 2;
        uint8_t new_level = read_u8(data + offset); offset += 1;
        // Update relic level
        for (int i = 0; i < c->relics.relics_count && i < 32; i++) {
            if (c->relics.relics[i].relic_id == relic_id) {
                c->relics.relics[i].level = new_level;
                break;
            }
        }
    }
    c->relics.last_update = now32();
}

void RecvRelicsEnhance(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t relic_id = read_u16(data + offset); offset += 2;
        uint64_t exp_gained = read_u64(data + offset); offset += 8;
        // Update relic exp
        for (int i = 0; i < c->relics.relics_count && i < 32; i++) {
            if (c->relics.relics[i].relic_id == relic_id) {
                c->relics.relics[i].exp += exp_gained;
                break;
            }
        }
    }
    c->relics.last_update = now32();
}

/* ---- Expedition ---- */
void RequestExpeditionInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_EXPEDITION_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestExpeditionPrize(Connection *c, uint16_t expedition_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_EXPEDITION_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, expedition_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvExpeditionInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->expedition.loaded = true;
    c->expedition.expedition_id = read_u16(data + offset); offset += 2;
    c->expedition.stage = read_u8(data + offset); offset += 1;
    c->expedition.max_stage = read_u8(data + offset); offset += 1;
    c->expedition.progress = read_u64(data + offset); offset += 8;
    c->expedition.target = read_u64(data + offset); offset += 8;
    c->expedition.unlocked = read_u8(data + offset); offset += 1;
    c->expedition.prize_claimed = read_u8(data + offset); offset += 1;
    c->expedition.next_refresh_time = read_u64(data + offset); offset += 8;

    c->expedition.last_update = now32();
}

void RecvExpeditionPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->expedition.prize_claimed = 1;
    }
    c->expedition.last_update = now32();
}

/* ---- Growth Fund ---- */
void RequestGrowthFundInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_GROWTHFUND_GETPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestGrowthFundPrize(Connection *c, uint16_t level) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_GROWTHFUND_GETPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, level);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvGrowthFundInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->growth_fund.loaded = true;
    c->growth_fund.fund_level = read_u16(data + offset); offset += 2;
    c->growth_fund.total_invested = read_u64(data + offset); offset += 8;
    c->growth_fund.rewards_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->growth_fund.rewards_count && i < 32; i++) {
        c->growth_fund.rewards[i].level = read_u16(data + offset); offset += 2;
        c->growth_fund.rewards[i].reward_id = read_u16(data + offset); offset += 2;
        c->growth_fund.rewards[i].claimed = read_u8(data + offset); offset += 1;
    }
    c->growth_fund.next_refresh_time = read_u64(data + offset); offset += 8;

    c->growth_fund.last_update = now32();
}

void RecvGrowthFundPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t level = read_u16(data + offset); offset += 2;
        for (int i = 0; i < c->growth_fund.rewards_count && i < 32; i++) {
            if (c->growth_fund.rewards[i].level == level) {
                c->growth_fund.rewards[i].claimed = 1;
                break;
            }
        }
    }
    c->growth_fund.last_update = now32();
}

/* ---- Treasure Back Event ---- */
void RequestTreasureBackEventInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_TREASUREBACKEVENT_PRIZEINFO);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestTreasureBackEventPrize(Connection *c, uint16_t prize_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_TREASUREBACKEVENT_GETPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, prize_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvTreasureBackEventInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->treasure_back_event.loaded = true;
    c->treasure_back_event.event_id = read_u16(data + offset); offset += 2;
    c->treasure_back_event.points = read_u64(data + offset); offset += 8;
    c->treasure_back_event.prizes_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->treasure_back_event.prizes_count && i < 32; i++) {
        c->treasure_back_event.prizes[i].prize_id = read_u16(data + offset); offset += 2;
        c->treasure_back_event.prizes[i].tier = read_u8(data + offset); offset += 1;
        c->treasure_back_event.prizes[i].claimed = read_u8(data + offset); offset += 1;
    }
    c->treasure_back_event.next_refresh_time = read_u64(data + offset); offset += 8;

    c->treasure_back_event.last_update = now32();
}

void RecvTreasureBackEventPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t prize_id = read_u16(data + offset); offset += 2;
        for (int i = 0; i < c->treasure_back_event.prizes_count && i < 32; i++) {
            if (c->treasure_back_event.prizes[i].prize_id == prize_id) {
                c->treasure_back_event.prizes[i].claimed = 1;
                break;
            }
        }
    }
    c->treasure_back_event.last_update = now32();
}

/* ---- Newbie Challenge ---- */
void RequestNewbieChallengeInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_NEWBIECHALLENGE_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestNewbieChallengePrize(Connection *c, uint16_t prize_id, bool is_vip) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_NEWBIECHALLENGE_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, prize_id);
    c->size += 2;
    write_u8(c->data + c->size, is_vip ? 1 : 0);
    c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvNewbieChallengeInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->newbie_challenge.loaded = true;
    c->newbie_challenge.challenge_id = read_u16(data + offset); offset += 2;
    c->newbie_challenge.score = read_u64(data + offset); offset += 8;
    c->newbie_challenge.vip_prizes_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->newbie_challenge.vip_prizes_count && i < 16; i++) {
        c->newbie_challenge.vip_prizes[i].prize_id = read_u16(data + offset); offset += 2;
        c->newbie_challenge.vip_prizes[i].claimed = read_u8(data + offset); offset += 1;
    }
    c->newbie_challenge.all_prizes_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->newbie_challenge.all_prizes_count && i < 16; i++) {
        c->newbie_challenge.all_prizes[i].prize_id = read_u16(data + offset); offset += 2;
        c->newbie_challenge.all_prizes[i].claimed = read_u8(data + offset); offset += 1;
    }
    c->newbie_challenge.next_refresh_time = read_u64(data + offset); offset += 8;

    c->newbie_challenge.last_update = now32();
}

void RecvNewbieChallengePrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t prize_id = read_u16(data + offset); offset += 2;
        uint8_t is_vip = read_u8(data + offset); offset += 1;
        if (is_vip) {
            for (int i = 0; i < c->newbie_challenge.vip_prizes_count && i < 16; i++) {
                if (c->newbie_challenge.vip_prizes[i].prize_id == prize_id) {
                    c->newbie_challenge.vip_prizes[i].claimed = 1;
                    break;
                }
            }
        } else {
            for (int i = 0; i < c->newbie_challenge.all_prizes_count && i < 16; i++) {
                if (c->newbie_challenge.all_prizes[i].prize_id == prize_id) {
                    c->newbie_challenge.all_prizes[i].claimed = 1;
                    break;
                }
            }
        }
    }
    c->newbie_challenge.last_update = now32();
}

/* ---- Cycle Mission ---- */
void RequestCycleMissionInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_MISSION_CUSTOMPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestCycleMissionPrize(Connection *c, uint16_t combo_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_MISSION_CUSTOMPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, combo_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvCycleMissionInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->cycle_mission.loaded = true;
    c->cycle_mission.combo_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->cycle_mission.combo_count && i < 16; i++) {
        c->cycle_mission.combos[i].combo_id = read_u16(data + offset); offset += 2;
        c->cycle_mission.combos[i].stage = read_u8(data + offset); offset += 1;
        c->cycle_mission.combos[i].progress = read_u64(data + offset); offset += 8;
        c->cycle_mission.combos[i].target = read_u64(data + offset); offset += 8;
        c->cycle_mission.combos[i].completed = read_u8(data + offset); offset += 1;
        c->cycle_mission.combos[i].prize_claimed = read_u8(data + offset); offset += 1;
    }
    c->cycle_mission.next_refresh_time = read_u64(data + offset); offset += 8;

    c->cycle_mission.last_update = now32();
}

void RecvCycleMissionPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t combo_id = read_u16(data + offset); offset += 2;
        for (int i = 0; i < c->cycle_mission.combo_count && i < 16; i++) {
            if (c->cycle_mission.combos[i].combo_id == combo_id) {
                c->cycle_mission.combos[i].prize_claimed = 1;
                break;
            }
        }
    }
    c->cycle_mission.last_update = now32();
}

/* ---- Custom Mission ---- */
void RequestCustomMissionInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ACTIVITY_CUSTOMPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestCustomMissionPrize(Connection *c, uint16_t mission_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_MISSION_CUSTOMPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, mission_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvCustomMissionInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->custom_mission.loaded = true;
    c->custom_mission.mission_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->custom_mission.mission_count && i < 32; i++) {
        c->custom_mission.missions[i].mission_id = read_u16(data + offset); offset += 2;
        c->custom_mission.missions[i].mission_type = read_u8(data + offset); offset += 1;
        c->custom_mission.missions[i].progress = read_u64(data + offset); offset += 8;
        c->custom_mission.missions[i].target = read_u64(data + offset); offset += 8;
        c->custom_mission.missions[i].completed = read_u8(data + offset); offset += 1;
        c->custom_mission.missions[i].prize_claimed = read_u8(data + offset); offset += 1;
    }
    c->custom_mission.next_refresh_time = read_u64(data + offset); offset += 8;

    c->custom_mission.last_update = now32();
}

void RecvCustomMissionPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t mission_id = read_u16(data + offset); offset += 2;
        for (int i = 0; i < c->custom_mission.mission_count && i < 32; i++) {
            if (c->custom_mission.missions[i].mission_id == mission_id) {
                c->custom_mission.missions[i].prize_claimed = 1;
                break;
            }
        }
    }
    c->custom_mission.last_update = now32();
}

/* ---- Lucky Card ---- */
void RequestLuckyCardInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_LUCKYCARD_EXCHANGE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestLuckyCardExchange(Connection *c, uint8_t card_index) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_LUCKYCARD_EXCHANGE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8(c->data + c->size, card_index);
    c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvLuckyCardInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->lucky_card.loaded = true;
    c->lucky_card.board_id = read_u16(data + offset); offset += 2;
    c->lucky_card.unlocked_count = read_u8(data + offset); offset += 1;
    c->lucky_card.total_cards = read_u8(data + offset); offset += 1;
    c->lucky_card.prizes_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->lucky_card.prizes_count && i < 16; i++) {
        c->lucky_card.prizes[i].prize_id = read_u16(data + offset); offset += 2;
        c->lucky_card.prizes[i].card_index = read_u8(data + offset); offset += 1;
        c->lucky_card.prizes[i].claimed = read_u8(data + offset); offset += 1;
    }
    c->lucky_card.next_refresh_time = read_u64(data + offset); offset += 8;

    c->lucky_card.last_update = now32();
}

void RecvLuckyCardExchange(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint8_t card_index = read_u8(data + offset); offset += 1;
        uint16_t prize_id = read_u16(data + offset); offset += 2;
        for (int i = 0; i < c->lucky_card.prizes_count && i < 16; i++) {
            if (c->lucky_card.prizes[i].card_index == card_index) {
                c->lucky_card.prizes[i].claimed = 1;
                c->lucky_card.unlocked_count++;
                break;
            }
        }
    }
    c->lucky_card.last_update = now32();
}

/* ---- Dark Nest ---- */
void RequestDarkNestRallyList(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SET_AUTO_DARK_NEST);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestDarkNestJoinRally(Connection *c, uint32_t rally_id, const uint32_t troop_array[16]) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SET_AUTO_DARK_NEST);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u32(c->data + c->size, rally_id);
    c->size += 4;
    for (int i = 0; i < 16; ++i) {
        write_u32(c->data + c->size, troop_array[i]);
        c->size += 4;
    }
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvDarkNestRallyList(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->dark_nest.loaded = true;
    c->dark_nest.rally_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->dark_nest.rally_count && i < 30; i++) {
        c->dark_nest.rallies[i].rally_id = read_u32(data + offset); offset += 4;
        c->dark_nest.rallies[i].monster_id = read_u16(data + offset); offset += 2;
        c->dark_nest.rallies[i].monster_level = read_u8(data + offset); offset += 1;
        c->dark_nest.rallies[i].zone_id = read_u16(data + offset); offset += 2;
        c->dark_nest.rallies[i].point_id = read_u8(data + offset); offset += 1;
        c->dark_nest.rallies[i].start_time = read_u64(data + offset); offset += 8;
        c->dark_nest.rallies[i].end_time = read_u64(data + offset); offset += 8;
        c->dark_nest.rallies[i].member_count = read_u8(data + offset); offset += 1;
    }
    c->dark_nest.next_refresh_time = read_u64(data + offset); offset += 8;

    c->dark_nest.last_update = now32();
}

/* ---- Fantasy Realm ---- */
void RequestFantasyRealmInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_FANTASY_REALM_TROOP_MARCH);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestFantasyRealmHunt(Connection *c, uint16_t monster_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_FANTASY_REALM_HUNT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, monster_id);
    c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestFantasyRealmSummon(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_FANTASY_REALM_SUMMON);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvFantasyRealmInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->fantasy_realm.loaded = true;
    c->fantasy_realm.realm_id = read_u16(data + offset); offset += 2;
    c->fantasy_realm.auto_hunt_enabled = read_u8(data + offset); offset += 1;
    c->fantasy_realm.monsters_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->fantasy_realm.monsters_count && i < 32; i++) {
        c->fantasy_realm.monsters[i].monster_id = read_u16(data + offset); offset += 2;
        c->fantasy_realm.monsters[i].level = read_u8(data + offset); offset += 1;
        c->fantasy_realm.monsters[i].zone_id = read_u16(data + offset); offset += 2;
        c->fantasy_realm.monsters[i].point_id = read_u8(data + offset); offset += 1;
        c->fantasy_realm.monsters[i].hunted = read_u8(data + offset); offset += 1;
    }
    c->fantasy_realm.summon_available = read_u8(data + offset); offset += 1;
    c->fantasy_realm.next_refresh_time = read_u64(data + offset); offset += 8;

    c->fantasy_realm.last_update = now32();
}

void RecvFantasyRealmHunt(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint16_t monster_id = read_u16(data + offset); offset += 2;
        // Mark monster as hunted
        for (int i = 0; i < c->fantasy_realm.monsters_count && i < 32; i++) {
            if (c->fantasy_realm.monsters[i].monster_id == monster_id) {
                c->fantasy_realm.monsters[i].hunted = 1;
                break;
            }
        }
    }
    c->fantasy_realm.last_update = now32();
}

/* ---- Mobilization (Alliance Mobilization) ---- */
void RequestMobilizationMissionData(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCEMOBLIZATION_MISSION_DATA);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestMobilizationMissionRefresh(Connection *c, uint8_t mission_pos, uint8_t mission_kind, uint8_t reset) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCEMOBLIZATION_MISSION_REFLASH);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8(c->data + c->size, mission_pos); c->size += 1;
    write_u8(c->data + c->size, mission_kind); c->size += 1;
    write_u8(c->data + c->size, reset); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestMobilizationMissionBuy(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCEMOBLIZATION_MISSION_BUY);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestMobilizationMissionGet(Connection *c, uint8_t mission_pos, uint8_t mission_kind) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCEMOBLIZATION_MISSION_GET);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8(c->data + c->size, mission_pos); c->size += 1;
    write_u8(c->data + c->size, mission_kind); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestMobilizationMissionDel(Connection *c, uint8_t mission_pos, uint8_t mission_kind) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCEMOBLIZATION_MISSION_DEL);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8(c->data + c->size, mission_pos); c->size += 1;
    write_u8(c->data + c->size, mission_kind); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestMobilizationMissionFinish(Connection *c, uint8_t mission_pos, uint8_t mission_kind) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCEMOBLIZATION_MISSION_FINISH);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u8(c->data + c->size, mission_pos); c->size += 1;
    write_u8(c->data + c->size, mission_kind); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestMobilizationLegendMissionGetScore(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_MOBILIZATION_LEGEND_MISSION_GET_SCORE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestActivityAmDegeePrize(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ACTIVITY_AM_DEGREEPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestActivityAmGetDegreePrize(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ACTIVITY_AM_GET_DEGREEPRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestActivityAmGetPersonalPrize(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ACTIVITY_AM_GET_PERSONAL_PRIZE);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvMobilizationMissionData(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->mobilization.mission_loaded = true;
    c->mobilization.activity_lv = read_u8(data + offset); offset += 1;
    c->mobilization.mission_status = read_u8(data + offset); offset += 1;
    c->mobilization.mission_id = read_u16(data + offset); offset += 2;
    c->mobilization.mission_pos = read_u8(data + offset); offset += 1;
    c->mobilization.mission_kind = read_u8(data + offset); offset += 1;
    c->mobilization.mission_difficulty = read_u8(data + offset); offset += 1;
    c->mobilization.mission_time = read_i64(data + offset); offset += 8;
    c->mobilization.mission_target = read_u32(data + offset); offset += 4;
    c->mobilization.complete_score = read_u32(data + offset); offset += 4;
    c->mobilization.am_score = read_u32(data + offset); offset += 4;
    c->mobilization.am_complete_degree = read_u8(data + offset); offset += 1;
    c->mobilization.personal_score = read_u32(data + offset); offset += 4;
    c->mobilization.personal_prize_earned_step = read_u8(data + offset); offset += 1;
    c->mobilization.personal_extra_prize_earned_times = read_u16(data + offset); offset += 2;
    c->mobilization.available_mission = read_u8(data + offset); offset += 1;
    c->mobilization.extra_mission = read_u8(data + offset); offset += 1;
    c->mobilization.involved_member = read_u8(data + offset); offset += 1;
    c->mobilization.available_mission_cd_time = read_i64(data + offset); offset += 8;
    c->mobilization.next_refresh_time = read_u64(data + offset); offset += 8;

    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionRefresh(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.mission_status = read_u8(data + offset); offset += 1;
        c->mobilization.mission_id = read_u16(data + offset); offset += 2;
        c->mobilization.mission_pos = read_u8(data + offset); offset += 1;
        c->mobilization.mission_kind = read_u8(data + offset); offset += 1;
        c->mobilization.mission_difficulty = read_u8(data + offset); offset += 1;
        c->mobilization.mission_time = read_i64(data + offset); offset += 8;
        c->mobilization.mission_target = read_u32(data + offset); offset += 4;
    }
    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionBuy(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.extra_mission = read_u8(data + offset); offset += 1;
    }
    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionGet(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.mission_status = read_u8(data + offset); offset += 1;
    }
    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionDel(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.mission_status = 0;
        c->mobilization.mission_id = 0;
    }
    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionFinish(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.mission_status = 3; // completed
    }
    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionUpdate(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->mobilization.mission_status = read_u8(data + offset); offset += 1;
    c->mobilization.last_update = now32();
}

void RecvMobilizationMissionDone(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->mobilization.mission_status = 4; // done
    c->mobilization.last_update = now32();
}

void RecvMobilizationLegendMissionGetScore(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->mobilization.personal_score = read_u32(data + offset); offset += 4;
    c->mobilization.personal_last_stage_point = read_u32(data + offset); offset += 4;
    c->mobilization.next_rank_degree = read_u8(data + offset); offset += 1;
    c->mobilization.more_rewards = read_u8(data + offset); offset += 1;
    c->mobilization.last_update = now32();
}

void RecvActivityAmDegeePrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.am_complete_degree = read_u8(data + offset); offset += 1;
        c->mobilization.am_score = read_u32(data + offset); offset += 4;
    }
    c->mobilization.last_update = now32();
}

void RecvActivityAmGetDegreePrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.personal_prize_earned_step++;
    }
    c->mobilization.last_update = now32();
}

void RecvActivityAmGetPersonalPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->mobilization.personal_extra_prize_earned_times++;
    }
    c->mobilization.last_update = now32();
}

void RecvActivityAmUpdateInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t update_type = read_u8(data + offset); offset += 1;
    if (update_type == 0) { // kLoginInfo
        c->mobilization.activity_lv = read_u8(data + offset); offset += 1;
        c->mobilization.mission_status = read_u8(data + offset); offset += 1;
        c->mobilization.mission_id = read_u16(data + offset); offset += 2;
        c->mobilization.mission_pos = read_u8(data + offset); offset += 1;
        c->mobilization.mission_kind = read_u8(data + offset); offset += 1;
        c->mobilization.mission_difficulty = read_u8(data + offset); offset += 1;
        c->mobilization.mission_time = read_i64(data + offset); offset += 8;
        c->mobilization.mission_target = read_u32(data + offset); offset += 4;
        c->mobilization.complete_score = read_u32(data + offset); offset += 4;
        c->mobilization.am_score = read_u32(data + offset); offset += 4;
        c->mobilization.am_complete_degree = read_u8(data + offset); offset += 1;
        c->mobilization.personal_score = read_u32(data + offset); offset += 4;
        c->mobilization.personal_prize_earned_step = read_u8(data + offset); offset += 1;
        c->mobilization.personal_extra_prize_earned_times = read_u16(data + offset); offset += 2;
        c->mobilization.available_mission = read_u8(data + offset); offset += 1;
        c->mobilization.extra_mission = read_u8(data + offset); offset += 1;
        c->mobilization.involved_member = read_u8(data + offset); offset += 1;
        c->mobilization.available_mission_cd_time = read_i64(data + offset); offset += 8;
    } else if (update_type == 1) { // kPersonalScore
        c->mobilization.personal_score = read_u32(data + offset); offset += 4;
        c->mobilization.personal_last_stage_point = read_u32(data + offset); offset += 4;
        c->mobilization.next_rank_degree = read_u8(data + offset); offset += 1;
        c->mobilization.more_rewards = read_u8(data + offset); offset += 1;
    }
    c->mobilization.last_update = now32();
}

/* ---- Alliance Gather Point ---- */
void RequestAllianceGatherPointInfo(Connection *c) {
    // This info is typically received via _MSG_RESP_ALLIANCE_INFO or push
    // No explicit request packet in the dump, but we can trigger a refresh
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_INFO);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAllianceGatherPointSet(Connection *c, uint16_t zone_id, uint8_t point_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_GATHERING_POINT_SET);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data + c->size, zone_id); c->size += 2;
    write_u8(c->data + c->size, point_id); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAllianceGatherPointFreeTeleport(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_GATHERING_POINT_FREE_TELEPORT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}


void RecvAllianceGatherPointUpdate(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->alliance_gather_point.loaded = true;
    c->alliance_gather_point.point_set = read_u8(data + offset); offset += 1;
    c->alliance_gather_point.zone_id = read_u16(data + offset); offset += 2;
    c->alliance_gather_point.point_id = read_u8(data + offset); offset += 1;
    c->alliance_gather_point.point_changed_time = read_u32(data + offset); offset += 4;
    c->alliance_gather_point.next_refresh_time = read_u64(data + offset); offset += 8;

    c->alliance_gather_point.last_update = now32();
}

void RecvAllianceGatherPointSet(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->alliance_gather_point.point_set = 1;
        c->alliance_gather_point.zone_id = read_u16(data + offset); offset += 2;
        c->alliance_gather_point.point_id = read_u8(data + offset); offset += 1;
    }
    c->alliance_gather_point.last_update = now32();
}

void RecvAllianceGatherPointFreeTeleport(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        // Free teleport successful
    }
    c->alliance_gather_point.last_update = now32();
}

/* ---- Alliance WhiteList ---- */
void RequestAllianceWhiteListInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_APPLY_AUTO_ACCEPT_LIST);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAllianceWhiteListAdd(Connection *c, int64_t user_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_ADD_APPLY_AUTO_ACCEPT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_i64(c->data + c->size, user_id); c->size += 8;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAllianceWhiteListRemove(Connection *c, int64_t user_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_REMOVE_APPLY_AUTO_ACCEPT);
    c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id);
    c->size += 4;
    write_i64(c->data + c->size, user_id); c->size += 8;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Research ---- */
void RequestResearchStart(Connection *c, uint16_t tech_id, uint8_t level) {
    /* v2.201.313 sendTechnologyResearchStart(ushort TechID) wire format,
       verified against libil2cpp.so: WriteU16(TechID) then
       WriteByte(GetTechLevel(TechID) + 1).  level is the CURRENT level of the
       tech (0-based); the wire carries level+1.  Sending only the tech_id
       (the old bot format) left the server reading the next packet's bytes as
       the level, so the request was misparsed and never applied. */
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RESEARCH); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, tech_id); c->size += 2;
    write_u8 (c->data + c->size, (uint8_t)(level + 1)); c->size += 1;
    write_u16(c->data, c->size);
    LOGI("[RESEARCH TX] 3202 tech_id=%u level=%u level+1=%u\n",
         (unsigned)tech_id, (unsigned)level, (unsigned)(level + 1));
    send_packet(c, true);
}

void RequestResearchCompleteFree(Connection *c, uint16_t tech_id, uint8_t level) {
    /* v2.201.313 native 3204 builder (file 0x4F26FFC) is NOT empty: it writes
       WriteU16(DataManager.ResearchTech) then WriteByte(GetTechLevel+1) — the
       same [tech_id][level+1] payload shape as 3202.  The old bot sent an
       empty body, so the server never completed the stuck research (research_tech
       stayed set), which in turn made every new 3202 get rejected with toast
       5017 ("research already in progress"). */
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RESEARCH_FINISH_FREE); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, tech_id); c->size += 2;
    write_u8 (c->data + c->size, (uint8_t)(level + 1)); c->size += 1;
    write_u16(c->data, c->size);
    LOGI("[RESEARCH TX] 3204 complete-free tech_id=%u level=%u level+1=%u\n",
         (unsigned)tech_id, (unsigned)level, (unsigned)(level + 1));
    send_packet(c, true);
}

void RequestResearchCompleteImmediate(Connection *c, uint16_t tech_id, uint8_t level) {
    /* v2.201.313 native 3209 builder (file 0x4F27220): WriteU16(tech_id) then
       WriteByte(GetTechLevel+1) — same [tech_id][level+1] shape as 3202/3204. */
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RESEARCH_FINISH_IMMEDIATE); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, tech_id); c->size += 2;
    write_u8 (c->data + c->size, (uint8_t)(level + 1)); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestResearchCancel(Connection *c, uint16_t tech_id, uint8_t level) {
    /* v2.201.313 native sendTechnologyResearchCancel (file 0x4F27068): msg
       0xc86 (3206) with WriteU16(DataManager.ResearchTech@0x15e8) then
       WriteByte(GetTechLevel+1) — the same [tech_id][level+1] shape as
       3202/3204/3209.  Used to un-stick a research whose timer finished but
       whose server slot was never released (every 3202 start then returns
       "research already in progress"). */
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_RESEARCH_EVENT_CANCEL); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, tech_id); c->size += 2;
    write_u8 (c->data + c->size, (uint8_t)(level + 1)); c->size += 1;
    write_u16(c->data, c->size);
    LOGI("[RESEARCH TX] 3206 cancel tech_id=%u level=%u level+1=%u\n",
         (unsigned)tech_id, (unsigned)level, (unsigned)(level + 1));
    send_packet(c, true);
}

/* ---- Building ---- */
/* v2.201.313 sendBuildFinish(ushort position_id) and
   sendBuildCompleteFree(ushort position_id, freeSource) both write a single
   position_id byte (msg 2011 / 2008).  position_id is the building queue
   position the server reported in the 2002 BUILDINGEVENT (the client echoes
   it back), NOT the tile x/y. */
void RequestBuildFinish(Connection *c, uint8_t position_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_BUILD_FINISH); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u8(c->data + c->size, position_id); c->size += 1;
    write_u16(c->data, c->size);
    LOGI("[BUILD TX] 2011 finish position_id=%u\n", (unsigned)position_id);
    send_packet(c, true);
}

void RequestBuildCompleteFree(Connection *c, uint8_t position_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_BUILD_FINISH_FREE); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u8(c->data + c->size, position_id); c->size += 1;
    write_u16(c->data, c->size);
    LOGI("[BUILD TX] 2008 free-finish position_id=%u\n", (unsigned)position_id);
    send_packet(c, true);
}

/* ---- Training Finish ---- */
void RequestTrainingFinish(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_TRAINING_FINISH); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Watch Tower ---- */
void RequestWatchTowerLineDetail(Connection *c, uint32_t line_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_WATCHTOWER_LINE_DETAIL); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u32(c->data + c->size, line_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Send Help ---- */
void RequestSendHelp(Connection *c, uint16_t record_sn_count, const uint32_t *record_sn) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_SENDHELP); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, record_sn_count); c->size += 2;
    for (int i = 0; i < record_sn_count; i++) {
        write_u32(c->data + c->size, record_sn[i]); c->size += 4;
    }
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Server Relocate ---- */
void ServerRelocate(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_USEITEM); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, ADVANCE_RELOCATOR); c->size += 2;
    write_u16(c->data + c->size, 1); c->size += 2;
    write_u16(c->data + c->size, kingdom_id); c->size += 2;
    write_u16(c->data + c->size, zone_id); c->size += 2;
    write_u8(c->data + c->size, point_id); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Server Rename ---- */
/* Already implemented above */

/* ---- Daily Mission / Battle Pass ---- */
void RequestDailyMissionInfo(Connection *c) {
    LOGI("[CLAIM][TX] RequestDailyMissionInfo\n");
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_DAILY_MISSION_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestDailyMissionReward(Connection *c, uint16_t reward_id) {
    /* BPDailyMission variant of the polymorphic 3179: WriteByte(0x0f) +
       WriteUInt16(missionKind) + WriteUInt16(activityGroupId).  The real client
       has no sender named for 11872; daily rewards ride 3179.  --daily_mission_via_3179
       selects empirical testing; otherwise keep the legacy flat 11872. */
    if (c->automation.daily_mission_via_3179) {
        RequestAchievementPrize(c, 0x0f, reward_id, c->daily_mission.activity_group_id);
        return;
    }
    LOGI("[CLAIM][TX] RequestDailyMissionReward reward=%u\n", reward_id);
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_DAILY_MISSION_REWARD); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, reward_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- VIP Mission ---- */
void RequestVipMissionInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_VIP_MISSION_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestVipMissionCollect(Connection *c, uint16_t mission_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_VIP_MISSION_COLLECT); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, mission_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Daily Sign-in ---- */
void RequestDailySigninInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_DAILY_SIGNIN_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestDailySignin(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_DAILY_SIGNIN); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Pet Training ---- */
void RequestPetTrainingInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_PET_TRAINING_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestPetTrainingBegin(Connection *c, uint16_t pet_id, uint8_t training_type, bool use_speedup) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_PET_TRAINING_BEGIN); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, pet_id); c->size += 2;
    write_u8(c->data + c->size, training_type); c->size += 1;
    write_u8(c->data + c->size, use_speedup); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestPetTrainingFinish(Connection *c, uint8_t slot_index) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_PET_TRAINING_FINISH); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u8(c->data + c->size, slot_index); c->size += 1;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Hero System ---- */
void RequestHeroInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_HERO_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestHeroEnhanceFinish(Connection *c, uint16_t hero_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_HERO_ENHANCE_FINISH); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, hero_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Item Crafting ---- */
void RequestItemCraftInfo(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ITEM_CRAFT_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestItemCraftStart(Connection *c, uint16_t recipe_id, uint16_t count) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ITEM_CRAFT_START); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, recipe_id); c->size += 2;
    write_u16(c->data + c->size, count); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestItemCraftFinish(Connection *c, uint16_t recipe_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ITEM_CRAFT_FINISH); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, recipe_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Achievement ---- */
void RequestAchievementInfo(Connection *c) {
    LOGI("[CLAIM][TX] RequestAchievementInfo\n");
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ACHIEVEMENT_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestAchievementPrize(Connection *c, uint8_t kind, uint16_t activity_id, uint16_t reward_id) {
    /* 3179 is polymorphic: the payload always begins with a u8 ACHIEVEMENT-KIND
       discriminator, then per-kind ids.  kind is passed in -- normally the
       per-activity achievement_kind the server reported in RecvAchievementInfo
       (SoloBattlefield 0x12 = index+groupID), or a config override for A/B
       testing.  Before this the bot sent NO kind byte, so the server routed the
       packet wrong and the prize never applied. */
    LOGI("[CLAIM][TX] RequestAchievementPrize kind=0x%02x activity=%u reward=%u\n", kind, activity_id, reward_id);
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ACHIEVEMENT_PRIZE); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u8 (c->data + c->size, kind); c->size += 1;
    write_u16(c->data + c->size, activity_id); c->size += 2;
    write_u16(c->data + c->size, reward_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Quest Chapter ---- */
void RequestQuestChapterInfo(Connection *c) {
    LOGI("[CLAIM][TX] RequestQuestChapterInfo\n");
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_QUEST_CHAPTER_INFO); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RequestQuestChapterReward(Connection *c, uint16_t quest_id) {
    /* Hypothesized to ride the polymorphic 3179 (real client has no sender named
       for 2059).  --quest_chapter_via_3179 routes through 3179 with the BPDailyMission
       kind 0x0f for empirical comparison; otherwise keep legacy flat 2059. */
    if (c->automation.quest_chapter_via_3179) {
        /* quest_chapter has no group id; pass 0 (pure test hypothesis). */
        RequestAchievementPrize(c, 0x0f, quest_id, 0);
        return;
    }
    LOGI("[CLAIM][TX] RequestQuestChapterReward quest=%u\n", quest_id);
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_QUEST_CHAPTER_REWARD); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, quest_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Valhalla ---- */
void RequestValhallaPrize(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_VALHALLA_PRIZE); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Adventure ---- */
void RequestAdventureMissionPrize(Connection *c, uint16_t mission_id) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_ADVENTURE_MISSION_PRIZE); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data + c->size, mission_id); c->size += 2;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

/* ---- Black Market ---- */
void RequestBlackMarketData(Connection *c) {
    c->size = 2;
    write_u16(c->data + c->size, _MSG_REQUEST_BLACK_MARKET_DATA); c->size += 2;
    write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
    write_u16(c->data, c->size);
    send_packet(c, true);
}

void RecvAllianceWhiteListInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->alliance_whitelist.loaded = true;
    c->alliance_whitelist.data_count = read_u8(data + offset); offset += 1;
    for (int i = 0; i < c->alliance_whitelist.data_count && i < 50; i++) {
        c->alliance_whitelist.entries[i].user_id = read_i64(data + offset); offset += 8;
        uint16_t name_len = read_u16(data + offset); offset += 2;
        if (name_len < 14) {
            memcpy(c->alliance_whitelist.entries[i].name, data + offset, name_len);
            c->alliance_whitelist.entries[i].name[name_len] = '\0';
        }
        offset += name_len;
        uint16_t nickname_len = read_u16(data + offset); offset += 2;
        if (nickname_len < 14) {
            memcpy(c->alliance_whitelist.entries[i].nickname, data + offset, nickname_len);
            c->alliance_whitelist.entries[i].nickname[nickname_len] = '\0';
        }
        offset += nickname_len;
        c->alliance_whitelist.entries[i].leave_time = read_i64(data + offset); offset += 8;
        c->alliance_whitelist.entries[i].rank = read_u8(data + offset); offset += 1;
        c->alliance_whitelist.entries[i].active = read_u8(data + offset); offset += 1;
    }
    c->alliance_whitelist.next_refresh_time = read_u64(data + offset); offset += 8;

    c->alliance_whitelist.last_update = now32();
}

void RecvAllianceWhiteListAdd(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        // Refresh list
        RequestAllianceWhiteListInfo(c);
    }
    c->alliance_whitelist.last_update = now32();
}

void RecvAllianceWhiteListRemove(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        // Refresh list
        RequestAllianceWhiteListInfo(c);
    }
    c->alliance_whitelist.last_update = now32();
}

/* ---- Training Info ---- */
void RecvTrainingInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    /* TRAININGINFO (2402) - the server's training queue snapshot.  Previously
       this clobbered c->size (the OUTBOUND packet-size buffer) with a value
       read off the wire, corrupting the next outgoing frame.  The real
       queue/finish_time layout is not yet known (needs a Diag capture), so
       only log it for now and never write to c->size here. */
    uint16_t head = read_u16(data + offset); offset += 2;
    LOGI("[AUTO][TRAIN] RecvTrainingInfo size=%u head=%u", (unsigned)size, (unsigned)head);
}

/* ---- Daily Mission / Battle Pass ---- */
void RecvDailyMissionInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    LOGI("[CLAIM][DIAG] DailyMission size=%u raw:", (unsigned)size);
    for (uint16_t i = 0; i < size && i < 96; i++) LOGI(" %02X", data[i]);
    LOGI("\n");

    c->daily_mission.loaded = true;
    c->daily_mission.activity_group_id = read_u16(data + offset); offset += 2;
    c->daily_mission.total_points = read_u64(data + offset); offset += 8;
    c->daily_mission.claimed_points = read_u64(data + offset); offset += 8;
    c->daily_mission.reward_stages_count = read_u8(data + offset); offset += 1;
    for (int i = 0; i < c->daily_mission.reward_stages_count && i < 16; i++) {
        c->daily_mission.reward_stage_ids[i] = read_u16(data + offset); offset += 2;
        c->daily_mission.reward_stage_claimed[i] = read_u8(data + offset); offset += 1;
    }
    c->daily_mission.missions_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->daily_mission.missions_count && i < 32; i++) {
        c->daily_mission.missions[i].mission_id = read_u16(data + offset); offset += 2;
        c->daily_mission.missions[i].mission_kind = read_u8(data + offset); offset += 1;
        c->daily_mission.missions[i].current_progress = read_u64(data + offset); offset += 8;
        c->daily_mission.missions[i].target_progress = read_u64(data + offset); offset += 8;
        c->daily_mission.missions[i].completed = read_u8(data + offset); offset += 1;
        c->daily_mission.missions[i].claimed = read_u8(data + offset); offset += 1;
    }
}

void RecvDailyMissionReward(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t reward_id = read_u16(data + offset); offset += 2;
    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        // Mark reward as claimed
        for (int i = 0; i < c->daily_mission.reward_stages_count && i < 16; i++) {
            if (c->daily_mission.reward_stage_ids[i] == reward_id) {
                c->daily_mission.reward_stage_claimed[i] = 1;
                break;
            }
        }
    }
}

/* ---- VIP Mission ---- */
void RecvVipMissionInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->vip_mission.loaded = true;
    c->vip_mission.vip_level = read_u8(data + offset); offset += 1;
    c->vip_mission.vip_points = read_u64(data + offset); offset += 8;
    c->vip_mission.missions_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->vip_mission.missions_count && i < 16; i++) {
        c->vip_mission.missions[i].mission_id = read_u16(data + offset); offset += 2;
        c->vip_mission.missions[i].mission_kind = read_u8(data + offset); offset += 1;
        c->vip_mission.missions[i].current_progress = read_u64(data + offset); offset += 8;
        c->vip_mission.missions[i].target_progress = read_u64(data + offset); offset += 8;
        c->vip_mission.missions[i].completed = read_u8(data + offset); offset += 1;
        c->vip_mission.missions[i].claimed = read_u8(data + offset); offset += 1;
    }
}

void RecvVipMissionCollect(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t mission_id = read_u16(data + offset); offset += 2;
    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        for (int i = 0; i < c->vip_mission.missions_count && i < 16; i++) {
            if (c->vip_mission.missions[i].mission_id == mission_id) {
                c->vip_mission.missions[i].claimed = 1;
                break;
            }
        }
    }
}

/* ---- Daily Sign-in ---- */
void RecvDailySigninInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->daily_signin.loaded = true;
    c->daily_signin.current_day = read_u8(data + offset); offset += 1;
    c->daily_signin.total_days = read_u8(data + offset); offset += 1;
    c->daily_signin.signed_today = read_u8(data + offset); offset += 1;
    c->daily_signin.can_choose_hero = read_u8(data + offset); offset += 1;
    c->daily_signin.hero_choices_count = read_u8(data + offset); offset += 1;
    for (int i = 0; i < c->daily_signin.hero_choices_count && i < 5; i++) {
        c->daily_signin.hero_choices[i] = read_u16(data + offset); offset += 2;
    }
    c->daily_signin.next_reset_time = read_u64(data + offset); offset += 8;
}

void RecvDailySignin(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        c->daily_signin.signed_today = 1;
        c->daily_signin.current_day++;
    }
}

/* ---- Pet Training ---- */
void RecvPetTrainingInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->pet_training.loaded = true;
    c->pet_training.training_slots_count = read_u8(data + offset); offset += 1;
    for (int i = 0; i < c->pet_training.training_slots_count && i < 5; i++) {
        c->pet_training.slots[i].active = read_u8(data + offset); offset += 1;
        c->pet_training.slots[i].slot_index = read_u8(data + offset); offset += 1;
        c->pet_training.slots[i].pet_id = read_u16(data + offset); offset += 2;
        c->pet_training.slots[i].training_type = read_u8(data + offset); offset += 1;
        c->pet_training.slots[i].start_time = read_u64(data + offset); offset += 8;
        c->pet_training.slots[i].duration = read_u32(data + offset); offset += 4;
        c->pet_training.slots[i].pet_level = read_u8(data + offset); offset += 1;
        c->pet_training.slots[i].pet_star = read_u8(data + offset); offset += 1;
    }
}

void RecvPetTrainingBegin(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        uint8_t slot_index = read_u8(data + offset); offset += 1;
        if (slot_index < 5) {
            c->pet_training.slots[slot_index].active = 1;
            c->pet_training.slots[slot_index].slot_index = slot_index;
            c->pet_training.slots[slot_index].pet_id = read_u16(data + offset); offset += 2;
            c->pet_training.slots[slot_index].training_type = read_u8(data + offset); offset += 1;
            c->pet_training.slots[slot_index].start_time = read_u64(data + offset); offset += 8;
            c->pet_training.slots[slot_index].duration = read_u32(data + offset); offset += 4;
            c->pet_training.slots[slot_index].pet_level = read_u8(data + offset); offset += 1;
            c->pet_training.slots[slot_index].pet_star = read_u8(data + offset); offset += 1;
        }
    }
}

void RecvPetTrainingFinish(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    uint8_t slot_index = read_u8(data + offset); offset += 1;
    if (result == 0 && slot_index < 5) {
        c->pet_training.slots[slot_index].active = 0;
    }
}

/* ---- Hero System ---- */
void RecvHeroInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->hero.loaded = true;
    c->hero.heroes_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->hero.heroes_count && i < 32; i++) {
        c->hero.heroes[i].hero_id = read_u16(data + offset); offset += 2;
        c->hero.heroes[i].level = read_u8(data + offset); offset += 1;
        c->hero.heroes[i].star = read_u8(data + offset); offset += 1;
        c->hero.heroes[i].quality = read_u8(data + offset); offset += 1;
        c->hero.heroes[i].exp = read_u32(data + offset); offset += 4;
        for (int j = 0; j < 4; j++) {
            c->hero.heroes[i].skill_levels[j] = read_u8(data + offset); offset += 1;
        }
        c->hero.heroes[i].enhancement_active = read_u8(data + offset); offset += 1;
        c->hero.heroes[i].enhancement_finish_time = read_u64(data + offset); offset += 8;
    }
}

void RecvHeroEnhanceFinish(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    uint16_t hero_id = read_u16(data + offset); offset += 2;
    if (result == 0) {
        for (int i = 0; i < c->hero.heroes_count && i < 32; i++) {
            if (c->hero.heroes[i].hero_id == hero_id) {
                c->hero.heroes[i].enhancement_active = 0;
                c->hero.heroes[i].level++;
                break;
            }
        }
    }
}

/* ---- Item Crafting ---- */
void RecvItemCraftInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->item_craft.loaded = true;
    c->item_craft.recipes_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->item_craft.recipes_count && i < 32; i++) {
        c->item_craft.recipes[i].recipe_id = read_u16(data + offset); offset += 2;
        c->item_craft.recipes[i].craft_type = read_u8(data + offset); offset += 1;
        c->item_craft.recipes[i].result_item_id = read_u16(data + offset); offset += 2;
        c->item_craft.recipes[i].result_count = read_u16(data + offset); offset += 2;
        c->item_craft.recipes[i].materials_count = read_u8(data + offset); offset += 1;
        for (int j = 0; j < c->item_craft.recipes[i].materials_count && j < 8; j++) {
            c->item_craft.recipes[i].materials[j].item_id = read_u16(data + offset); offset += 2;
            c->item_craft.recipes[i].materials[j].count = read_u32(data + offset); offset += 4;
        }
        c->item_craft.recipes[i].crafting_active = read_u8(data + offset); offset += 1;
        c->item_craft.recipes[i].finish_time = read_u64(data + offset); offset += 8;
    }
}

void RecvItemCraftStart(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    uint16_t recipe_id = read_u16(data + offset); offset += 2;
    if (result == 0) {
        for (int i = 0; i < c->item_craft.recipes_count && i < 32; i++) {
            if (c->item_craft.recipes[i].recipe_id == recipe_id) {
                c->item_craft.recipes[i].crafting_active = 1;
                c->item_craft.recipes[i].finish_time = read_u64(data + offset); offset += 8;
                break;
            }
        }
    }
}

void RecvItemCraftFinish(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint8_t result = read_u8(data + offset); offset += 1;
    uint16_t recipe_id = read_u16(data + offset); offset += 2;
    if (result == 0) {
        for (int i = 0; i < c->item_craft.recipes_count && i < 32; i++) {
            if (c->item_craft.recipes[i].recipe_id == recipe_id) {
                c->item_craft.recipes[i].crafting_active = 0;
                break;
            }
        }
    }
}

/* ---- Achievement ---- */
void RecvAchievementInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    LOGI("[CLAIM][DIAG] AchievementInfo size=%u raw:", (unsigned)size);
    for (uint16_t i = 0; i < size && i < 96; i++) LOGI(" %02X", data[i]);
    LOGI("\n");

    /* The private server broadcasts a packed login snapshot that is NOT the real
     * client's activity-array layout (observed payloads of 10/23/27/35 bytes).
     * A real activity entry needs >= 2+2+1+8+8+1 = 22 bytes after the count.
     * If the payload cannot hold even one, reject it instead of reading past the
     * buffer and fabricating garbage state (which caused activity=0 reward=255). */
    uint16_t cnt = read_u16(data + offset); offset += 2;
    if (size < offset + 22 || cnt == 0 || cnt > 16) {
        c->achievement.loaded = false;
        return;
    }
    c->achievement.activities_count = cnt;
    if (c->achievement.activities_count > 16) c->achievement.activities_count = 16;
    c->achievement.loaded = true;
    for (int i = 0; i < c->achievement.activities_count; i++) {
        c->achievement.activities[i].activity_id = read_u16(data + offset); offset += 2;
        c->achievement.activities[i].activity_group_id = read_u16(data + offset); offset += 2;
        c->achievement.activities[i].achievement_kind = read_u8(data + offset); offset += 1;
        c->achievement.activities[i].current_points = read_u64(data + offset); offset += 8;
        c->achievement.activities[i].target_points = read_u64(data + offset); offset += 8;
        c->achievement.activities[i].rewards_count = read_u8(data + offset); offset += 1;
        if (c->achievement.activities[i].rewards_count > 8)
            c->achievement.activities[i].rewards_count = 8;
        for (int j = 0; j < c->achievement.activities[i].rewards_count && j < 8; j++) {
            if (offset + 3 > size) break;
            c->achievement.activities[i].reward_ids[j] = read_u16(data + offset); offset += 2;
            c->achievement.activities[i].reward_claimed[j] = read_u8(data + offset); offset += 1;
        }
    }
}

void RecvAchievementPrize(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t activity_id = read_u16(data + offset); offset += 2;
    uint16_t reward_id = read_u16(data + offset); offset += 2;
    uint8_t result = read_u8(data + offset); offset += 1;
    /* Log each prize ack so an empirical test of the 3179 kind-byte routing can
       be read from S2C: result==0 => server accepted and marked claimed. */
    if (result == 0) {
        LOGI("[CLAIM][RX] AchievementPrize ACCEPTED activity=%u reward=%u\n",
             activity_id, reward_id);
        for (int i = 0; i < c->achievement.activities_count && i < 16; i++) {
            if (c->achievement.activities[i].activity_id == activity_id) {
                for (int j = 0; j < c->achievement.activities[i].rewards_count && j < 8; j++) {
                    if (c->achievement.activities[i].reward_ids[j] == reward_id) {
                        c->achievement.activities[i].reward_claimed[j] = 1;
                        break;
                    }
                }
                break;
            }
        }
    } else {
        LOGI("[CLAIM][RX] AchievementPrize REJECTED result=%u activity=%u reward=%u\n",
             result, activity_id, reward_id);
    }
}

/* ---- Quest Chapter ---- */
void RecvQuestChapterInfo(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    c->quest_chapter.loaded = true;
    c->quest_chapter.current_chapter = read_u16(data + offset); offset += 2;
    c->quest_chapter.completed_chapters = read_u16(data + offset); offset += 2;
    c->quest_chapter.quests_count = read_u16(data + offset); offset += 2;
    for (int i = 0; i < c->quest_chapter.quests_count && i < 64; i++) {
        c->quest_chapter.quests[i].quest_id = read_u16(data + offset); offset += 2;
        c->quest_chapter.quests[i].quest_type = read_u8(data + offset); offset += 1;
        c->quest_chapter.quests[i].progress = read_u64(data + offset); offset += 8;
        c->quest_chapter.quests[i].target = read_u64(data + offset); offset += 8;
        c->quest_chapter.quests[i].completed = read_u8(data + offset); offset += 1;
        c->quest_chapter.quests[i].claimed = read_u8(data + offset); offset += 1;
    }
}

void RecvQuestChapterReward(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;

    uint16_t quest_id = read_u16(data + offset); offset += 2;
    uint8_t result = read_u8(data + offset); offset += 1;
    if (result == 0) {
        for (int i = 0; i < c->quest_chapter.quests_count && i < 64; i++) {
            if (c->quest_chapter.quests[i].quest_id == quest_id) {
                c->quest_chapter.quests[i].claimed = 1;
                break;
            }
        }
    }
}

/* ---- Gather Map Info Plus ---- */
/* ---- Magic Gate ---- */
void RecvMagicGateDoEvent(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;
    // Magic gate event response
    (void)c; (void)data; (void)size;
}

/* ---- Watch Tower Line Detail ---- */
void RecvWatchTowerLineDetail(Connection *c, const uint8_t *data, uint16_t size) {
    uint16_t offset = 0;
    if (size < 4) return;
    // Watch tower line detail response
    (void)c; (void)data; (void)size;
}