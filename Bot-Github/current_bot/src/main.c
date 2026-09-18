#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
  #include <unistd.h>
  #include <fcntl.h>
#endif

#include "connection.h"
#include "log.h"
#include "protocol.h"
#include "net_rw.h"
#include "packet_enum.h"

#include "command.h"
#include "gathering.h"
#include "automation.h"
#include "activity_log.h"

// included mapping for debugging purpose 
#include "packet_map.h"


#include "map_point.h"
#include "items.h"

#include "config.h"
#include "bot_settings.h"
#include "gameassets_loader.h"

#include "version.h"

#define SERVER_ADDR "192.243.44.63"
#define SERVER_PORT 5999
#define BUFFER_SIZE 4096

#define GAME_MAJOR_VERSION 2
#define GAME_MINOR_VERSION 200
#define GAME_PATCH_VERSION 312
#define RUSSIAN_LANGUAGE_CODE 6

// debugging purpose 
void dump_data(const char *filename, const char *str, void *data, size_t size) {
	(void)str;
	char name[1024] = {0};

	snprintf(name, sizeof(name), "%s.bin", filename);

	FILE *file = fopen(name, "wb");
	if (file == NULL) {
		perror("open failed");
		return;
	}
	if (fwrite(data, 1, size, file) != size) {
		perror("write failed");
	}
	fclose(file);

	printf("file: %s saved\n", name);
}

void BotTick(Connection *c)
{
	if (c->server_time == 0) return;
	
	// Connection maintenance
	HeartbeatTick(c);
	
	// Central automation scheduler.
	AutomationTick(c);
	
}

uint8_t GetVIPLevel(uint32_t vipPoints)
{
	if (vipPoints >= 1500000) return 15;
	if (vipPoints >= 730000)  return 14;
	if (vipPoints >= 350000)  return 13;
	if (vipPoints >= 175000)  return 12;
	if (vipPoints >= 90000)   return 11;
	if (vipPoints >= 50000)   return 10;
	if (vipPoints >= 20000)   return 9;
	if (vipPoints >= 8000)	return 8;
	if (vipPoints >= 4000)	return 7;
	if (vipPoints >= 1600)	return 6;
	if (vipPoints >= 800)	 return 5;
	if (vipPoints >= 400)	 return 4;
	if (vipPoints >= 300)	 return 3;
	if (vipPoints >= 100)	 return 2;
	
	return 1;
}


void logger(Connection *c) {
	
	for (int i = 0; i < c->alliance_member.count; i++) {
		AllianceMember *m = &c->alliance_member.member[i];
		
		printf("id: %ld, ", m->user_id);
		// printf("head: %u\n",     m->head);
		printf("name: %s\n",     m->name);
		// printf("rank: %u\n",     m->rank);
		// printf("power: %lu\n",   m->power);
		// printf("troop_kill_num: %lu\n", m->troop_kill_num);
		// printf("logout_time: %ld\n", m->logout_time);
		// printf("white_list_flag: %u\n", m->white_list_flag);
		// printf("\n");
	}
	
}

void ProcessConnection(Connection *c)
{
	PacketStream *s = &c->stream;
	
	map_pos_t pos;
	
	time_t last_update = time(NULL);
	
	while (1) {
		// Tick 
		BotTick(c);
		
		/*
		time_t now = time(NULL);
		
		if (now - last_update >= 10) {
			last_update = now;
			
			logger(c);
		}
		*/
		
		int n = recv(c->sock, s->buffer + s->read_pos, BUFFER_SIZE - s->read_pos, 0);
		
		if (n > 0) {
			s->read_pos += n;
		}
		else if (n == 0) {
			LOGI("[NET] Remote peer closed TCP connection (recv=0)\n");
			LOGI("[NET] Last parsed packet type=%u size=%u\n",
			     (unsigned)s->packet_type, (unsigned)s->packet_size);
			break;
		}
		else {
			// non-blocking case
#ifdef _WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				Sleep(1);
				continue; // no more data right now
			}
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(1000);
				continue; // no more data right now
			}
#endif

			LOGE("Recv error\n");
			break;
		}
		
		while (s->read_pos - s->parse_pos >= 4) {
			s->packet_size = read_u16(s->buffer + s->parse_pos);
			s->packet_type = read_u16(s->buffer + s->parse_pos + 2);
			
			if (s->packet_size < 4 || s->packet_size > BUFFER_SIZE) {
				s->parse_pos += 4;
				continue;
			}
			
			if (s->read_pos - s->parse_pos < s->packet_size) {
				break;
			}

			/* DIAGNOSTIC: log every inbound packet type so the exact server
			   reply to each automation request is visible.  Also dump the full
			   body of building/research/training response packets so a
			   rejection / error code in them is readable.  Remove after the
			   build/research/train accept problem is resolved. */
			{
				int rel = (s->packet_type == 2001 || s->packet_type == 2002 ||
				           s->packet_type == 2004 || s->packet_type == 2005 ||
				           s->packet_type == 2007 || s->packet_type == 2008 ||
				           s->packet_type == 2010 || s->packet_type == 2012 ||
				           s->packet_type == 2013 ||
				           s->packet_type == 2402 || s->packet_type == 2408 ||
				           s->packet_type == 2410 || s->packet_type == 2413 ||
				           s->packet_type == 3203 || s->packet_type == 3205 ||
				           s->packet_type == 3207 || s->packet_type == 3208 ||
				           s->packet_type == 3210);
				if (rel) {
					char hex[4096] = {0};
					char *hp = hex;
					size_t n = s->packet_size - 4 < 1024 ? (size_t)s->packet_size - 4 : 1024;
					for (size_t di = 0; di < n; ++di)
						hp += sprintf(hp, "%02X%s", (unsigned char)s->buffer[s->parse_pos + 4 + di],
						              di + 1 == n ? "" : " ");
					LOGI("[DIAG] RX type=%u (%s) size=%u body=%s",
					     (unsigned)s->packet_type, get_packet_name(s->packet_type),
					     (unsigned)s->packet_size, hex);
				} else {
					LOGI("[DIAG] RX type=%u size=%u", (unsigned)s->packet_type,
					     (unsigned)s->packet_size);
				}
			}
			/* /DIAGNOSTIC */

            if (s->packet_type == 2217 || s->packet_type == 2219 || s->packet_type == 2220) {
                size_t payload_len = s->packet_size >= 4 ? (size_t)s->packet_size - 4 : 0;
                char first[128] = {0};
                char *hp = first;
                size_t nfirst = payload_len < 32 ? payload_len : 32;
                for (size_t di = 0; di < nfirst; ++di)
                    hp += sprintf(hp, "%02X%s", (unsigned char)s->buffer[s->parse_pos + 4 + di],
                                  di + 1 == nfirst ? "" : " ");
                LOGI("[PACKET DEBUG] MAP-RANGE type=%u size=%u first32=%s",
                     (unsigned)s->packet_type, (unsigned)s->packet_size, first);
            }

			c->sin.offset = 0;
			c->sin.size = s->packet_size - 4;
			memcpy(c->sin.data, s->buffer + s->parse_pos + 4, s->packet_size - 4);
			
			switch(s->packet_type) {
				case _MSG_RESP_LOGINVALIDATE:
					HandleLoginValidate(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// HandleClientGuestLogin(c, s->buffer + s->parse_pos + 4, packet_size - 4);
					LOGI("Gateway Login success!\n");
					LOGI("Gateway server disconnected\n");
					disconnect(c);
					return;
				case _MSG_GAMESERVER_LOGINLOG: 
					// kind = read_u16(s->buffer + s->parse_pos + 4);
					LOGI("Game login successful\n");
					// ServerInitOver(c);
					break;
				case _MSG_LOGIN_LOGINERRORRESP: 
					RecvLoginError(c, s->buffer + s->parse_pos + 4);
					disconnect(c);
					// printf("Login error\n");
					return;
				case _MSG_CLIENT_LOGINTOLRESP: 
					// kind = read_i32(s->buffer + s->parse_pos + 4);
					
					LOGE("Bootstrap Login failed session expired: %d\n", 0/*kind*/);
					disconnect(c);
					return;
				case _MSG_RESP_ACTIVE: 
					c->server_time = read_u64(s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_CHATMESSAGE: 
					RecvChatMessage(c, s->buffer + s->parse_pos + 4);
					
					command_handler(c, c->chat.player_name, c->chat.message);
					
					if (c->chat.message[0] != c->bot.command_prefix) {
						printf("[MSG] [%s]: %s\n", c->chat.player_name, c->chat.message);
					}
					break;
				case _MSG_RESP_SOCIAL_DATA: 
					RequestAllianceGiftInfo(c);
					
					RequestRallyList(c);
					
					RequestAllianceMemberInfo(c);
					
					/*
					RequestViewChat(
						c, // Connection *c
						0, // Channel 0 for world, 1 for guild 
						0, // Previous message 
						3, // kind
						0, // Data id
						c->server_time + 5
					);
					*/
					
					// RequestDeleteAllianceGiftBox(c, 0xFFFFFFFF);
					break;
				case _MSG_RESP_ITEMINFO: 
					RecvItemInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_BUFFINFO: 
					RecvIBuffInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_MARCH_MARCHEVENTDATA:
					RecvMarchData(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
                    GatheringOnMarchData(c, c->player.current_marches);
					break;
				case _MSG_LOGIN_ROLEINFO: 
					RecvLoginRoleInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					
					
					pos = getTileMapPosbyPointCode(c->player.zone_id, c->player.point_id);
					
					LOGI("Character: %s\n", c->player.name);
					LOGI("Location: K:%u X:%u Y:%u\n", c->player.current_kingdom_id, pos.x, pos.y);
					LOGI("VIP: %u\n", GetVIPLevel(c->player.vip_point ));
					LOGI("Might: %lu\n", c->player.power);
					LOGI("Kills: %lu\n", c->player.kills);
					LOGI("Gems: %u\n", c->player.gems);
					break;
				case _MSG_RESP_UPDATE_RESOURCEAMOUNT: 
					RecvRefreshResources(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_RESOURCEINFO: 
					RecvResources(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BLACKMARKET_DATA: 
					RecvBlackMarket_Data(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BLACKMARKET_BUY:
					RecvBlackMarket_Buy(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_LOGIN_VERIFY_SESSION: 
					// dump_data("verify_session", "", s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// printf("_MSG_RESP_LOGIN_VERIFY_SESSION\n");
					break;
				case _MSG_RESP_BUILDINGINFO: 
					RecvAllBuildData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_MAILINFO: 
					RecvMailInfo(c, s->buffer + s->parse_pos + 4);
					
					command_handler(c, c->mail.sender_name, c->mail.content);
					
					if (c->chat.message[0] != c->bot.command_prefix) {
						printf("[MAIL] [%s]: %s\n", c->mail.sender_name, c->mail.content);
					}
					break;
				case _MSG_RESP_ALLYPOINT: 
					RecvAllyPoint(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_ALLIANCE_HELP: 
					printf("_MSG_RESP_ALLIANCE_HELP\n");
					// RecvAllianceHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case 0xB26:
					RecvAllianceMemberNeedsHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case 0x0B23:
					RecvPendingAllianceMembersNeedHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case 0x0B2F: 
					RecvAllianceGiftInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case 0x0b33: 
					RecvDeleteAllianceGiftBox(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_USEITEM: 
					RecvUseItem(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// dump_data("_MSG_RESP_USEITEM", "", s->buffer + s->parse_pos, s->packet_size);
					break;
				case 0x0B31: 
					RecvAllianceGiftOpen(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BUYITEM: 
					RecvBuyItem(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					// dump_data("_MSG_RESP_BUYITEM", "", s->buffer + s->parse_pos, s->packet_size);
					break;
				case _MSG_RESP_WORLD_TELEPORT_ITEM:
					// dump_data("_MSG_RESP_WORLD_TELEPORT_ITEM", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_REQUEST_ALLIANCE_INFO:
					RecvAllianceInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BUILDINGEVENT: 
					RecvBuildingQueue(c, s->buffer + s->parse_pos + 4);
					break;
                case _MSG_RESP_BUILDBEGIN:
                    LOGI("[AUTO][BUILD] build request accepted\n");
                    c->automation.building_queue_active = true;
                    break;
                case _MSG_RESP_BUILDCOMPLETE:
                case _MSG_RESP_FINISHBUILD:
                    LOGI("[AUTO][BUILD] build queue completed\n");
                    c->automation.building_queue_active = false;
                    c->automation.building_finish_time = 0;
                    break;
                case _MSG_RESP_BUILDINGERROR:
                    /* The server rejected the upgrade request instead of
                       accepting it.  Clear the queue AND remember the tile
                       that was rejected: AutoBuildingTick skips that tile
                       until building_skip_until and picks another building,
                       so a persistent rejection cannot loop forever.
                       The 2013 body carries the server's failure reason and
                       the account's live resource stock is logged alongside
                       it so the next run shows which gate actually fired
                       (resource shortage vs castle-level cap) instead of the
                       bot treating every rejection as "skip the tile". */
                    {
                        char bhex[256] = {0};
                        char *bp = bhex;
                        size_t blen = s->packet_size >= 4
                                      ? (size_t)s->packet_size - 4 : 0;
                        if (blen > 96) blen = 96;
                        for (size_t bi = 0; bi < blen; ++bi)
                            bp += sprintf(bp, "%02X%s",
                                          (unsigned char)s->buffer[s->parse_pos + 4 + bi],
                                          bi + 1 == blen ? "" : " ");
                        LOGI("[AUTO][BUILD] server REJECTED building request "
                             "(BUILDINGERROR 2013) size=%u body=%s "
                             "stock[f=%.2fM r=%.2fM w=%.2fM o=%.2fM g=%.2fM] "
                             "skipping tile=(%u,%u) for 120s\n",
                             (unsigned)s->packet_size, bhex,
                             c->resources.food / 1000000.0,
                             c->resources.rock / 1000000.0,
                             c->resources.wood / 1000000.0,
                             c->resources.ore / 1000000.0,
                             c->resources.gold / 1000000.0,
                             (unsigned)c->automation.building_pos_x,
                             (unsigned)c->automation.building_pos_y);
                    }
                    c->automation.building_queue_active = false;
                    c->automation.building_finish_time = 0;
                    c->automation.building_skip_x = c->automation.building_pos_x;
                    c->automation.building_skip_y = c->automation.building_pos_y;
                    c->automation.building_skip_until =
                        (uint32_t)time(NULL) + 120;
                    break;
				case _MSG_RESP_UPDATEWATCHTOWER_ADDLINE:
					RecvUpdateWatchTowerAddLineInfo(c, s->buffer + s->parse_pos + 4);
					printf("RecvUpdateWatchTowerAddLineInfo()\n");
					// dump_data("_MSG_RESP_UPDATEWATCHTOWER_ADDLINE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case 0x0B2B: 
					RecvRoleUpdateInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_BROCAST_NPC_WAR_BEGIN: 
					RecvDarknestBroadcast(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_ARMYGROUPINFO_: 
					RecvArmyGroupInfo(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WATCHTOWER_LINEDETAIL: 
					// RecvWatchTowerLineDetail(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_INITLIST: 
					RecvRallyCountData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_NPC_WARHALL_UPDATE_LISTELE: 
					RecvNPCWallHallData(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_NPC_WARHALL_UPDATE_LISTELE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_NPC_WARHALL_INIT_LISTDETAIL: 
					RecvNPCWallHallDetail(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_NPC_WARHALL_INIT_LISTDETAIL", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_BROCAST_WAR_BEGIN: 
					RecvWarBegin(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_UPDATE_LISTDETAIL:
					RecvWallHallTroop(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_WARHALL_UPDATE_LISTDETAIL", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_WARHALL_UPDATE_LISTELE: 
					RecvWallHallData(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_WARHALL_UPDATE_LISTELE", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_WARHALL_INIT_LISTDETAIL:
					RecvWallHallDetail(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_DELETE_LISTELE:
					RecvWallHallDel(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_WARHALL_END_LISTDETAIL: 
					RecvWallHallDetailClose(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_JOINED_RALLYDATA: 
					RecvJoinedRallyData(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_JOINED_RALLYDATA", "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					break;
				case _MSG_RESP_RESEARCHINFO:
					RecvTechnologyInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					dump_data("_MSG_RESP_RESEARCHINFO", "", s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
                case _MSG_RESP_RESEARCH_EVENT_START:
                {
                    /* 3203 RESEARCH_EVENT_START — the response to our 3202
                     * REQUEST_RESEARCH.  The FIRST payload byte is a RESULT/error
                     * code, verified against the native RecvTechnologyResearch
                     * handler (libil2cpp file offset 0x4F25314):
                     *     0 = success (server started the research)
                     *     1 = reject (client toast 0x1399 / 5017)
                     *     2 = reject (client toast 0x1caf / 7343)
                     *     5 = reject (client toast 0xf66  / 3942)
                     *     6 = reject (client toast 0x1d60 / 7520)
                     *
                     * All live 3203 captures from the OLD (broken) 3202 format
                     * started with result=0x01 => the server was rejecting every
                     * research request, so research never applied and the gate
                     * never armed, causing AutoResearchTick to re-fire 3202.
                     *
                     * On SUCCESS the authoritative research state (research_tech
                     * + finish_time) is delivered by the 3201 RESEARCHINFO push
                     * that follows, parsed by RecvTechnologyInfo.  Here we only
                     * lift the tech_id out of the first field as a fast signal
                     * and clear any rejection backoff; finish_time is armed by
                     * the 3201, and the gate waits while finish_time == 0. */
                    if (s->packet_size >= 4 + 1) {
                        const uint8_t *body = s->buffer + s->parse_pos + 4;
                        uint8_t result = body[0];
                        if (result == 0) {
                            uint16_t tech_id = (s->packet_size >= 4 + 3)
                                               ? read_u16(body + 1) : 0;
                            if (tech_id) c->technology.research_tech = tech_id;
                            c->automation.research_rejects = 0;
                            c->automation.research_retry_time = 0;
                            LOGI("[AUTO][RESEARCH] 3203 ACCEPTED tech_id=%u; waiting for 3201 finish_time state",
                                 (unsigned)tech_id);
                        } else {
                            c->automation.research_rejects++;
                            c->automation.research_retry_time =
                                (uint32_t)time(NULL) + 30; /* back off before re-firing */
                            LOGI("[AUTO][RESEARCH] 3203 REJECTED result=%u (toast %s) rejects=%u; "
                                 "stock[f=%.2fM r=%.2fM w=%.2fM o=%.2fM g=%.2fM]; backing off 30s",
                                 (unsigned)result,
                                 result == 1 ? "5017" :
                                 result == 2 ? "7343" :
                                 result == 5 ? "3942" :
                                 result == 6 ? "7520" : "unknown",
                                 (unsigned)c->automation.research_rejects,
                                 c->resources.food / 1000000.0,
                                 c->resources.rock / 1000000.0,
                                 c->resources.wood / 1000000.0,
                                 c->resources.ore / 1000000.0,
                                 c->resources.gold / 1000000.0);
                        }
                    } else {
                        LOGI("[AUTO][RESEARCH] 3203 body too short size=%u", (unsigned)s->packet_size);
                    }
                    break;
                }
                case _MSG_RESP_RESEARCH_EVENT_FREE:
                    /* 3205 — the response to our 3204 complete-free.  The
                       first payload byte is a RESULT code like 3203.  Only a
                       result==0 means the server actually released the
                       research slot.  On rejection the slot stays occupied
                       ("research already in progress"), so DO NOT clear
                       research_tech here; arm research_cancel_pending and let
                       AutoResearchTick send 3206 CANCEL once the timer is up. */
                    if (s->packet_size >= 4 + 1) {
                        uint8_t result = s->buffer[s->parse_pos + 4];
                        if (result == 0) {
                            c->technology.research_tech = 0;
                            c->technology.finish_time = 0;
                            c->automation.research_cancel_pending = 0;
                            LOGI("[AUTO][RESEARCH] 3204 complete-free ACCEPTED; slot released\n");
                        } else {
                            uint16_t active = c->technology.research_tech;
                            if (active != 0) {
                                /* A rejected complete-free (result != 0) means the
                                   research is still genuinely in progress on the
                                   server — the free-finish was simply not allowed
                                   yet.  Do NOT arm a 3206 cancel here: cancelling
                                   would abort real progress.  Clear any stale cancel
                                   and back off so we don't re-spam 3204. */
                                c->automation.research_cancel_pending = 0;
                                c->automation.research_retry_time =
                                    (uint32_t)time(NULL) + 60;
                                LOGI("[AUTO][RESEARCH] 3204 complete-free REJECTED "
                                     "result=%u (research %u still in progress); "
                                     "NOT cancelling, backing off 60s\n",
                                     (unsigned)result, (unsigned)active);
                            } else {
                                LOGI("[AUTO][RESEARCH] 3204 REJECTED result=%u (no active tech)\n",
                                     (unsigned)result);
                            }
                        }
                    }
                    break;
                case _MSG_RESP_RESEARCH_EVENT_CANCEL:
                    /* 3207 — the response to our 3206 cancel.  result==0 means
                       the server cancelled the research and freed the slot. */
                    c->automation.research_cancel_pending = 0;
                    if (s->packet_size >= 4 + 1) {
                        uint8_t result = s->buffer[s->parse_pos + 4];
                        if (result == 0) {
                            c->technology.research_tech = 0;
                            c->technology.finish_time = 0;
                            LOGI("[AUTO][RESEARCH] 3206 cancel ACCEPTED; slot released\n");
                        } else {
                            LOGI("[AUTO][RESEARCH] 3206 cancel REJECTED result=%u; giving up on cancel, "
                                 "will retry complete-free\n", (unsigned)result);
                        }
                    }
                    break;
                case _MSG_RESP_RESEARCH_EVENT_COMPLETE:
                case _MSG_RESP_RESEARCH_EVENT_INSTANT:
                    c->technology.research_tech = 0;
                    c->technology.finish_time = 0;
                    c->automation.research_cancel_pending = 0;
                    break;
				case _MSG_RESP_ADDCONFLICT_LINE: 
					RecvAddConflictLine(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_HOSPITAL_HOSPITALINFO: 
					RecvWoundedTroopData(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_HEALINGTROOP:
					RecvHealingResponse(c, s->buffer + s->parse_pos + 4);
                    c->automation.hospital_queue_active = false;
					break;
                case _MSG_RESP_TRAININGINFO_:
                    RecvTrainingInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
                    break;
                case _MSG_RESP_TRAINING_:
                    /* 2408 — response to our 2403 request.  Live capture (run_live.log
                       2026-09-15) distinguishes the two cases by body LENGTH AND first
                       byte:
                         * ACCEPT  = large body (43 bytes) whose first byte is 0.
                           The first 2403 at T4 got this — the tile/tier train fine.
                         * REJECT  = tiny body (1 byte) whose value is the result code.
                           Every later reject was `body=01` / `body=02` while the just-
                           accepted batch was STILL TRAINING => that is "queue busy",
                           NOT a tier lock.
                       The OLD handler cleared queue_active even on ACCEPT, so
                       AutoTrainingTick re-fired a duplicate 2403 ~5s later, got
                       result=1, and the training_highest_tier step-down then walked
                       T4->T0 on a queue-busy reject.  All of that re-fire + step-down
                       is wrong.  Fix: on ACCEPT KEEP the queue armed until the server
                       sends FINISHTRAINING (2409) to clear it; on REJECT never step
                       the tier down (we have no evidence of an actual tier lock). */
                    if (s->packet_size >= 4 + 1 && s->buffer[s->parse_pos + 4] != 0) {
                        LOGI("[AUTO][TRAIN] 2403 REJECTED result=%u (queue busy / "
                             "cannot train right now); backing off 60s, keeping "
                             "tier=%u\n",
                             (unsigned)s->buffer[s->parse_pos + 4],
                             (unsigned)c->automation.training_tier);
                        /* Do NOT step the tier down: the live log proved result=1/2
                           fire while an accepted batch is still training.  A genuine
                           tier-lock has its own result code we have not observed yet.
                           Drop the queue flag so we may retry the SAME tier once the
                           backoff expires, and let the next FINISHTRAINING re-arm. */
                        c->automation.training_queue_active = false;
                        c->automation.training_finish_time = 0;
                        c->automation.training_last_action =
                            (uint32_t)time(NULL) + 60;
                    } else {
                        LOGI("[AUTO][TRAIN] 2403 ACCEPTED; queue armed (waiting for "
                             "FINISHTRAINING 2409)\n");
                        /* A batch is genuinely queued now: keep queue_active=true so
                           AutoTrainingTick does not re-fire a duplicate 2403.  The
                           2409/FINISHTRAINING handler below clears it when the batch
                           completes. finish_time stays 0 because the exact accept
                           timestamp is not decoded — we rely on 2409, not on a guess. */
                        c->automation.training_queue_active = true;
                        c->automation.training_finish_time = 0;
                        c->automation.training_last_action = (uint32_t)time(NULL);
                    }
                    break;
                case _MSG_RESP_FINISHTRAINING:
                    c->automation.training_queue_active = false;
                    c->automation.training_finish_time = 0;
                    break;
				case _MSG_RESP_TROOPMARCH:
                    /* CORRECTED 2416 handler (2026-09-10), faithful to the real
                     * DataManager::RecvTroopMarch (libil2cpp.so file offset
                     * 0x4F03ABC, ARM64 disassembly).
                     *
                     * The OLD handler fabricated a flat record
                     *   [march_index][type][...][status@14]
                     * and logged every live reply as "march_index=4 type=0
                     * (EMET_Standby) status=0".  That is WRONG: the native
                     * function reads byte[0] as a MESSAGE SUB-COMMAND in
                     * {0..0xC} and dispatches to 13 different paths.
                     *
                     * Byte[0] semantics (from the dispatch tree):
                     *   sub 0   -> the ONLY path that creates a real march;
                     *             the march's EMarchEventType (0..7) is carried
                     *             at payload[2] -- see the note at the parse
                     *             site.  (The 2026-09-10 disassembly note said
                     *             byte[1], but the captured wire shows the type
                     *             at byte[2]; byte[1] is always 0.)
                     *   sub 4   -> a modify/standby path (reads toast ids
                     *             0x266/0x285), does NOT create a gather march.
                     *   sub 0xB,0xC,>0xC -> error/dialog handling.
                     *
                     * So "04 00 00 00..." is SUBCOMMAND 4 (non-gather), NOT an
                     * accepted Standby march.  Only subcommand 0 with a
                     * march_type of 2/7 is a real gathering march.
                     */
                    LOGI("[GATHER ACK] _MSG_RESP_TROOPMARCH received size=%u",
                         (unsigned)s->packet_size);
                    if (s->packet_size > 4) {
                        size_t payload_size = s->packet_size - 4;
                        const unsigned char *payload =
                            (const unsigned char *)(s->buffer + s->parse_pos + 4);
                        LOGI("[GATHER ACK] response payload size=%zu", payload_size);

                        if (payload_size < 1) {
                            LOGI("[GATHER ACK] empty payload; no march subcommand");
                            GatheringOnMarchReject(c, 0xFE);
                            break;
                        }

                        uint8_t sub = payload[0];
                        /* The native dispatch handles sub 0..0xC; anything
                           outside falls into the error/dialog path.  Match
                           that so an unexpected byte[0] is not misread. */
                        if (sub > 0x0C) {
                            LOGI("[GATHER ACK] subcommand=%u (>0xC) -> native error/dialog path; no march created",
                                 (unsigned)sub);
                            GatheringOnMarchReject(c, sub);
                        } else if (sub == 0) {
                            /* Subcommand 0 = real march created.  Parse the
                               EMarchEventType (0..7); >7 means the client
                               returns without a march.
                               NOTE the type lives at payload[2], not payload[1].
                               Ground truth = the single real gather exchange in
                               game.pcap (server->client 2416 is PLAINTEXT, not
                               DES): its 2416 body is "00 00 07 00 ..." where the
                               real gather is 07 = EMET_GatherMarching.  A live
                               reply to the bot's own 6615 is "00 00 06 00 ..."
                               = 06 = EMET_CampMarching.  payload[1] is always 0
                               and was previously misread as "type 0 Standby",
                               hiding that the server actually created a Camp
                               march. */
                            if (payload_size < 3) {
                                LOGI("[GATHER ACK] subcommand=0 but payload too short for march type");
                                GatheringOnMarchReject(c, 0xFE);
                                break;
                            }
                            uint8_t march_type = payload[2];
                            if (march_type > 7) {
                                LOGI("[GATHER ACK] subcommand=0 march_type=%u >7 -> native ignores; no march created",
                                     (unsigned)march_type);
                                GatheringOnMarchReject(c, march_type);
                                break;
                            }
                            const char *type_name = "UNKNOWN";
                            switch (march_type) {
                                case 0: type_name = "EMET_Standby"; break;
                                case 1: type_name = "EMET_Camp"; break;
                                case 2: type_name = "EMET_Gathering"; break;
                                case 3: type_name = "EMET_InforceStanby"; break;
                                case 4: type_name = "EMET_RallyStanby"; break;
                                case 5: type_name = "EMET_AttackMarching"; break;
                                case 6: type_name = "EMET_CampMarching"; break;
                                case 7: type_name = "EMET_GatherMarching"; break;
                                default: break;
                            }
                            LOGI("[GATHER ACK] subcommand=0 march created type=%u (%s)",
                                 (unsigned)march_type, type_name);
                            /* GatheringOnMarchAck counts ONLY 2/7 as a real
                               gather march; anything else keeps the pending
                               window open for an authoritative snapshot. */
                            GatheringOnMarchAck(c, (uint32_t)march_type);
                        } else {
                            /* Subcommands 1..0xC are non-creation update paths
                               (4 = modify/standby).  None creates a gather
                               march.  Log it clearly and keep the reservation. */
                            const char *sub_name = "UNKNOWN";
                            switch (sub) {
                                case 1: sub_name = "camp"; break;
                                case 2: sub_name = "gathering-update"; break;
                                case 3: sub_name = "inForceUpdate"; break;
                                case 4: sub_name = "modify/standby (NON-GATHER)"; break;
                                case 5: sub_name = "attack-update"; break;
                                case 6: sub_name = "camp-marching"; break;
                                case 7: sub_name = "gather-marching-detail"; break;
                                case 8: sub_name = "scout-marching"; break;
                                case 9: sub_name = "hit-monster"; break;
                                case 10: sub_name = "inForce-marching"; break;
                                case 11: sub_name = "rally-marching"; break;
                                case 12: sub_name = "rally-attack"; break;
                                default: break;
                            }
                            LOGI("[GATHER ACK] subcommand=%u (%s) -> NOT a march-created event; no gather march",
                                 (unsigned)sub, sub_name);
                            /* Do not release the reservation here: if the
                               server later publishes a real march the
                               authoritative snapshot clears it.  Keep the
                               pending window open (as the non-gather path
                               already does). */
                            if (sub == 4) {
                                /* sub 4 is the fixed non-gather reply to every
                                   6615 we send; count it as a standby-reject so
                                   the per-venue backoff (limit 4 -> 300s) still
                                   protects the session. */
                                GatheringOnMarchAck(c, 0);  /* 0 != 2/7 => not counted */
                            }
                        }

                        if (payload_size >= 4) {
                            uint32_t first_u32 =
                                ((uint32_t)payload[0]) |
                                ((uint32_t)payload[1] << 8) |
                                ((uint32_t)payload[2] << 16) |
                                ((uint32_t)payload[3] << 24);
                            LOGI("[GATHER ACK] first_u32=0x%08X (diagnostic only)",
                                 (unsigned)first_u32);
                        }

                        for (size_t off = 0; off < payload_size; off += 16) {
                            char hex[64] = {0};
                            char *hp = hex;
                            size_t row_end = off + 16;
                            if (row_end > payload_size) row_end = payload_size;
                            for (size_t i = off; i < row_end; ++i) {
                                hp += sprintf(hp, "%02X%s", payload[i],
                                              (i + 1 == row_end) ? "" : " ");
                            }
                            LOGI("[GATHER ACK] %04zu: %s", off, hex);
                        }
                    }
                    break;
                case _MSG_RESP_TROOPRETURN:
				case _MSG_RESP_TROOPHOME:
				case _MSG_RESP_TROOPCAMPING:
					if (c->player.current_marches > 0) c->player.current_marches--;
                    GatheringOnMarchEnd(c);
					break;
				case _MSG_RESP_GATHERINGEVENT:
					break;
				case _MSG_RESP_UPDATE_MAPINFO_PLUS: 
					RecvMapInfoPlus(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					RecvGatherMapInfoPlus(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_SEND_RESHELP: 
					RecvSHelp(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_RESHELP_HOME: 
					RecvHelp_Home(c, s->buffer + s->parse_pos + 4);
					break;
				case _MSG_RESP_ALLIANCE_MEMBERINFO:
					RecvAllianceMemberInfo(c, s->buffer + s->parse_pos + 4);
					// dump_data("_MSG_RESP_ALLIANCE_MEMBERINFO", "", s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Daily Mission / Battle Pass */
				case _MSG_RESP_DAILY_MISSION:
					RecvDailyMissionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_DAILY_MISSION_REWARD:
					RecvDailyMissionReward(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* VIP Mission */
				case _MSG_RESP_MISSION_VIP:
					RecvVipMissionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_MISSION_VIP_SPEEDUP:
					RecvVipMissionCollect(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Daily Sign-in */
				case _MSG_RESP_DAILYSIGNIN_SIGNIN:
					RecvDailySignin(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_UPDATE_DAILYSIGNIN_TABLEREFRESH:
					RecvDailySigninInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Pet Training */
				case _MSG_RESP_PET_TRAINING_EVENT:
				case _MSG_RESP_PET_TRAINING_EVENT_EXFLAG:
					RecvPetTrainingInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_PET_TRAINING_BEGIN:
				case _MSG_RESP_PET_TRAINING_BEGIN_EXFLAG:
					RecvPetTrainingBegin(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_PET_TRAINING_COMPLETE:
					RecvPetTrainingFinish(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Hero System */
				case _MSG_RESP_HEROSAVE:
					RecvHeroInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_HEROENHANCE_COMPLETE:
					RecvHeroEnhanceFinish(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Item Crafting */
				case _MSG_ITEMCRAFT_INFO:
					RecvItemCraftInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_ITEMCRAFT_DONE:
					RecvItemCraftStart(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ITEMCRAFT:
					RecvItemCraftFinish(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Achievement */
				case _MSG_RESP_ACHIEVEMENT_ACTIVITY:
					RecvAchievementInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ACHIEVEMENT_PRIZE:
					RecvAchievementPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
					/* Quest Chapter */
				case _MSG_RESP_QUEST_CHAPTER:
					RecvQuestChapterInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Expedition */
				case _MSG_RESP_EXPEDITION_INFO:
					RecvExpeditionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_EXPEDITION_PRIZE:
					RecvExpeditionPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Valhalla */
				case _MSG_RESP_VALHALLA_INFO:
					RecvValhallaInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_VALHALLA_PRIZE:
					RecvValhallaPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_VALHALLA_INSTANT_REVIVE:
					RecvValhallaInstantRevive(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_VALHALLA_DIVINE_REVIVE:
					RecvValhallaDivineRevive(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Adventure */
				case _MSG_RESP_ADVENTURE_MISSIONINFO:
					RecvAdventureMissionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ADVENTURE_MISSIONPRIZE:
					RecvAdventureMissionPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ADVENTURE_HUNT:
					RecvAdventureHunt(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Relics */
				case _MSG_RESP_RELICS_INFO:
					RecvRelicsInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_RELICS_GACHA_DATA:
					RecvRelicsGacha(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_RELICS_GACHA_CONTENT:
					RecvRelicsGacha(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_SYNTHESIS_RELIC:
					RecvRelicsSynthesize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_RELICS_ENHANCE:
					RecvRelicsEnhance(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Serial Gift */
				case _MSG_RESP_SERIALGIFT_EVENTLIST:
					RecvSerialGiftList(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_SERIALGIFT_GIFTINFO:
					RecvSerialGiftList(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_SERIALGIFT_GETGIFT:
					RecvSerialGiftGet(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Old Player Back (claim-only) */
				case _MSG_RESP_OLDPLAYERBACK_INFO:
					RecvOldPlayerBackInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_OLDPLAYERBACK_GETGIFT:
					RecvOldPlayerBackGetGift(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Gift Activity (claim-only) */
				case _MSG_RESP_GIFT_ACTIVITY_LIST:
					RecvGiftActivityList(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_GIFT_ACTIVITY_OPEN_GIFT_BOX:
					RecvGiftActivityOpenBox(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Week Challenge */
				case _MSG_RESP_WEEKCHALLENGE_INFO:
					RecvWeekChallengeInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_WEEK_CHALLENGE_ITEM_SHOP_BUY_INFO:
					RecvWeekChallengeBuy(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_WEEK_CHALLENGE_ITEM_SHOP_BUY:
					RecvWeekChallengeBuy(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_WEEKCHALLENGE_PRIZE:
					RecvWeekChallengePrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Lucky Card */
				case _MSG_RESP_LUCKYCARD_INFO:
					RecvLuckyCardInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_LUCKYCARD_EXCHANGE:
					RecvLuckyCardExchange(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Dark Nest */
				case _MSG_RESP_SET_AUTO_DARK_NEST:
					RecvDarkNestRallyList(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Fantasy Realm */
				case _MSG_RESP_FANTASY_REALM_TROOP_MARCH:
					RecvFantasyRealmInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_FANTASY_REALM_HUNT:
					RecvFantasyRealmHunt(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Growth Fund */
				case _MSG_RESP_INIT_GROWTHFUND:
					RecvGrowthFundInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_GROWTHFUND_GETPRIZE:
					RecvGrowthFundPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Treasure Back Event */
				case _MSG_RESP_TREASUREBACKEVENT_INFO:
					RecvTreasureBackEventInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_TREASUREBACKEVENT_PRIZEINFO:
					RecvTreasureBackEventInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_TREASUREBACKEVENT_GETPRIZE:
					RecvTreasureBackEventPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Newbie Challenge */
				case _MSG_RESP_NEWBIECHALLENGE_INFO:
					RecvNewbieChallengeInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_NEWBIECHALLENGE_PRIZE:
					RecvNewbieChallengePrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_NEWBIECHALLENGE_VIPPRIZE:
					RecvNewbieChallengePrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_NEWBIECHALLENGE_ALLPRIZE:
					RecvNewbieChallengePrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Cycle Mission */
				case _MSG_ACTIVITY_INIT_CYCLE_MISSION_COMBO_INFO:
					RecvCycleMissionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_ACTIVITY_UPDATE_CYCLE_MISSION_COMBO_INFO:
					RecvCycleMissionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_MISSION_CUSTOMPRIZE:
					RecvCycleMissionPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Custom Mission */
				case _MSG_RESP_ACTIVITY_CUSTOMMISSION:
					RecvCustomMissionInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ACTIVITY_CUSTOMPRIZE:
					RecvCustomMissionPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_MISSION_CUSTOMREWARD:
					RecvCustomMissionPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Mobilization (Alliance Mobilization) */
				case _MSG_RESP_ALLIANCEMOBLIZATION_MISSION_DATA:
					RecvMobilizationMissionData(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBLIZATION_MISSION_REFLASH:
					RecvMobilizationMissionRefresh(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBLIZATION_MISSION_BUY:
					RecvMobilizationMissionBuy(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBLIZATION_MISSION_GET:
					RecvMobilizationMissionGet(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBLIZATION_MISSION_DEL:
					RecvMobilizationMissionDel(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBLIZATION_MISSION_FINISH:
					RecvMobilizationMissionFinish(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBILIZATION_MISSION_UPDATE:
					RecvMobilizationMissionUpdate(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBILIZATION_MISSION_DONE:
					RecvMobilizationMissionDone(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ACTIVITY_AM_DEGREEPRIZE:
				case _MSG_RESP_ACTIVITY_AM_DEGREEPRIZE_NEW:
					RecvActivityAmDegeePrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ACTIVITY_AM_GET_DEGREEPRIZE:
					RecvActivityAmGetDegreePrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ACTIVITY_AM_GET_PERSONAL_PRIZE:
					RecvActivityAmGetPersonalPrize(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_UPDATE_ALLIANCE_MOBILIZATION_LEGEND_MISSION_DATA:
					RecvActivityAmUpdateInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCEMOBILIZATION_LEGENDRANK:
					RecvMobilizationLegendMissionGetScore(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Alliance Gather Point */
				case _MSG_UPDATE_ALLIANCE_GATHERING_POINT:
					RecvAllianceGatherPointUpdate(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCE_GATHERING_POINT_SET:
					RecvAllianceGatherPointSet(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCE_GATHERING_POINT_FREE_TELEPORT:
					RecvAllianceGatherPointFreeTeleport(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Alliance WhiteList (Apply Auto Accept List) */
				case _MSG_RESP_ALLIANCE_APPLY_AUTO_ACCEPT_LIST:
					RecvAllianceWhiteListInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCE_ADD_APPLY_AUTO_ACCEPT:
					RecvAllianceWhiteListAdd(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCE_REMOVE_APPLY_AUTO_ACCEPT:
					RecvAllianceWhiteListRemove(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				/* Gather: Guild Expedition (GBG) + Chaos Arena (Solo Battlefield) + instance map */
				case _MSG_RESP_UPDATE_INSTANCE_MAPINFO:
					RecvGatherMapInfoPlus(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_ALLIANCE_BATTLEFIELD_RUNNING_DETAIL:
					RecvGbgEventDetail(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				case _MSG_RESP_SOLO_BATTLEFIELD_INFO:
				case _MSG_SOLO_BATTLEFIELD_UPDATE_INFO:
					RecvChaosInfo(c, s->buffer + s->parse_pos + 4, s->packet_size - 4);
					break;
				default:
					
					/*
					// Debugging purpose only 
					LOGI("PACKET TYPE: %s (0x%X), size=%u",
						get_packet_name(s->packet_type),
						s->packet_type,
						s->packet_size
					);
					*/
					
					
					/*
					dump_data(get_packet_name(s->packet_type), "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
					*/
					
					break;
			}
			
			s->parse_pos += s->packet_size;
		}
		
		if (s->parse_pos > 0) {
			memmove(s->buffer, s->buffer + s->parse_pos, s->read_pos - s->parse_pos);
			s->read_pos -= s->parse_pos;
			s->parse_pos = 0;
		}
	}
}
 

// Configuration settings 
void Configuration(Connection *client)
{
	
	// Default settings
	// Automation defaults. War remains disabled.
	client->automation.enabled = true;
	client->automation.building = true;
	client->automation.research = true;
	client->automation.training = true;
	client->automation.traps = true;
	client->automation.hospital = true;
	client->automation.hunting = true;
	client->automation.quests = true;
	client->automation.activities = true;
	client->automation.alliance = true;
	client->automation.mail = true;
	client->automation.economy = true;
	client->automation.map = true;
	client->automation.war = false;
	/* New automation subsystems from dump */
	client->automation.daily_mission = true;
	client->automation.vip_mission = true;
	client->automation.daily_signin = true;
	client->automation.pet_training = true;
	client->automation.hero_system = true;
	client->automation.item_craft = true;
	client->automation.achievement = true;
	client->automation.quest_chapter = true;

	/* Additional automation subsystems from dump */
	client->automation.expedition = true;
	client->automation.valhalla = true;
	client->automation.adventure = true;
	client->automation.relics = true;
	client->automation.serial_gift = true;
	client->automation.week_challenge = true;
	client->automation.lucky_card = true;
	client->automation.dark_nest = true;
	client->automation.fantasy_realm = true;
	client->automation.growth_fund = true;
	client->automation.treasure_back_event = true;
	client->automation.newbie_challenge = true;
	client->automation.cycle_mission = true;
	client->automation.custom_mission = true;

	/* Bot activity log default: ON (local text journal, no gameplay impact) */
	client->automation.activity_log = true;
	client->automation.activity_log_path[0] = 0;

	/* Daily Mission defaults */
	client->automation.daily_mission_auto_claim = true;
	client->automation.daily_mission_auto_speedup = false;
	client->automation.daily_mission_last_action = 0;

	/* VIP Mission defaults */
	client->automation.vip_mission_auto_collect = true;
	client->automation.vip_mission_auto_speedup = false;
	client->automation.vip_mission_last_action = 0;

	/* Daily Sign-in defaults */
	client->automation.daily_signin_auto_signin = true;
	client->automation.daily_signin_auto_choose_hero = false;
	client->automation.daily_signin_hero_choice = 0;
	client->automation.daily_signin_last_action = 0;

	/* Pet Training defaults */
	client->automation.pet_training_auto_start = true;
	client->automation.pet_training_auto_complete_free = true;
	client->automation.pet_training_auto_instant = false;
	client->automation.pet_training_use_speedup = false;
	client->automation.pet_training_slot = 0;
	client->automation.pet_training_last_action = 0;
	for (int i = 0; i < 5; ++i) {
	    client->automation.pet_training_pet_id[i] = 0;
	    client->automation.pet_training_type[i] = 0;
	}

	/* Hero System defaults */
	client->automation.hero_auto_enhance = false;
	client->automation.hero_auto_starup = false;
	client->automation.hero_auto_skill = false;
	client->automation.hero_last_action = 0;

	/* Item Crafting defaults */
	client->automation.item_craft_auto_start = false;
	for (int i = 0; i < 8; ++i) {
	    client->automation.item_craft_recipe_id[i] = 0;
	    client->automation.item_craft_count[i] = 0;
	}
	client->automation.item_craft_last_action = 0;

	/* Achievement defaults */
	client->automation.achievement_auto_claim = true;
	client->automation.achievement_last_action = 0;

	/* Quest Chapter defaults */
	client->automation.quest_chapter_auto_progress = true;
	client->automation.quest_chapter_last_action = 0;

	/* Expedition defaults */
	client->automation.expedition_auto_prize = true;
	client->automation.expedition_last_action = 0;

	/* Valhalla defaults */
	client->automation.valhalla_auto_revive = false;
	client->automation.valhalla_last_action = 0;

	/* Adventure defaults */
	client->automation.adventure_auto_hunt = false;
	client->automation.adventure_auto_quest = false;
	client->automation.adventure_last_action = 0;

	/* Relics defaults */
	client->automation.relics_auto_gacha = false;
	client->automation.relics_auto_enhance = false;
	client->automation.relics_auto_synthesize = false;
	client->automation.relics_last_action = 0;

	/* Serial Gift defaults */
	client->automation.serial_gift_auto_claim = true;
	client->automation.serial_gift_last_action = 0;

	/* Week Challenge defaults */
	client->automation.week_challenge_auto_buy = false;
	client->automation.week_challenge_auto_prize = true;
	client->automation.week_challenge_last_action = 0;

	/* Lucky Card defaults */
	client->automation.lucky_card_auto_exchange = false;
	client->automation.lucky_card_last_action = 0;

	/* Dark Nest defaults */
	client->automation.dark_nest_auto_rally = false;
	client->automation.dark_nest_last_action = 0;

	/* Fantasy Realm defaults */
	client->automation.fantasy_realm_auto_hunt = false;
	client->automation.fantasy_realm_auto_summon = false;
	client->automation.fantasy_realm_last_action = 0;
	client->automation.fantasy_realm_target_monster_id = 0;

	/* Growth Fund defaults */
	client->automation.growth_fund_auto_claim = true;
	client->automation.growth_fund_last_action = 0;

	/* Treasure Back Event defaults */
	client->automation.treasure_back_auto_claim = true;
	client->automation.treasure_back_last_action = 0;

	/* Newbie Challenge defaults */
	client->automation.newbie_challenge_auto_claim = true;
	client->automation.newbie_challenge_last_action = 0;

	/* Cycle Mission defaults */
	client->automation.cycle_mission_auto_claim = true;
	client->automation.cycle_mission_last_action = 0;

	/* Custom Mission defaults */
	client->automation.custom_mission_auto_claim = true;
	client->automation.custom_mission_last_action = 0;

	/* Mobilization defaults */
	client->automation.mobilization_auto_claim = true;
	client->automation.mobilization_auto_refresh = false;
	client->automation.mobilization_auto_buy = false;
	client->automation.mobilization_last_action = 0;

	/* Alliance Gather Point defaults */
	client->automation.alliance_gather_point_auto_teleport = false;
	client->automation.alliance_gather_point_last_action = 0;

	/* Alliance WhiteList defaults */
	client->automation.alliance_whitelist_auto_accept = false;
	client->automation.alliance_whitelist_last_action = 0;

	// Command prefix
	client->bot.command_prefix = '$';
	// Data folder 
	strcpy(client->bot.data_path, "./lmbot/");
	// default admin
	strcpy(client->bot.admin_name, "yash1459");
	
	/*
	client->lobby_server_addr = 0;
	client->lobby_server_port = 0;
	
	client->game_server_addr = 0;
	client->game_server_port = 0;
	*/
	
	// Game Version And Language
	client->app.version_major = 2;
	client->app.version_minor = 200;
	client->app.version_patch = 312;
	client->app.language_code = 1;   // g_config.language_code;
	
	
	
	
	
	// client->cargo_ship.settings.auto_trade = true;
	
	
	// Automatically purchase desired Black Market (Cargo ship) items.
	client->market.settings.auto_trade = true;
	
	// Minimum resources to keep after market purchases.
	client->market.reserve.food = 0;
	client->market.reserve.rock = 0;
	client->market.reserve.wood = 0;
	client->market.reserve.ore  = 0;
	client->market.reserve.gold = 0;
	
	// Allow these resources to be spent on Black Market trades.
	client->market.settings.spend_food = true;
	client->market.settings.spend_rock = true;
	client->market.settings.spend_wood = true;
	client->market.settings.spend_ore  = true;
	client->market.settings.spend_gold = true;
	
	// Guild Auto help
	client->alliance.auto_help = true;
	
	// Open alliance gifts
	client->alliance.auto_open_gifts = true; // Guild Gift will not open auto if set false 
	
	
	// Master switch for the protection system.
	client->protection.enabled = true;
	
	/* Shield */
	client->protection.shield_always_on = false; // shield 24/7
	client->protection.shield_on_incoming_attack = true; // shield when army invading 
	client->protection.shield_on_incoming_scout = true; // shield when scout approach
	
	/* Shield priority order.
	 * The bot tries shields from highest priority to lowest priority.
	 * If the first shield is unavailable, it falls back to the next available shield.
	 */
	client->protection.shield_priority_count = 4;
	client->protection.shield_priority[0] = SHIELD_4H;  // Priority 1
	client->protection.shield_priority[1] = SHIELD_8H;  // Priority 2
	client->protection.shield_priority[2] = SHIELD_12H; // Priority 3
	client->protection.shield_priority[3] = SHIELD_1D;  // Priority 4
	
	// Additional shields available for fallback.
	// Currently not included in the priority list above.
	client->protection.shield_priority[4] = SHIELD_3D;
	client->protection.shield_priority[5] = SHIELD_7D;
	client->protection.shield_priority[6] = SHIELD_14D;
	
	/* Troop recall */
	// Recalls targeted camped or gathering troops.
	client->protection.recall_on_incoming_attack = true;
	client->protection.recall_on_incoming_scout = true;
	
	// Automatically recalls a gathering or camp march before it reaches
	// its destination when an incoming conflict is detected.
	// Requires Withdraw Squad items; otherwise no action is taken.
	client->protection.recall_on_incoming_conflict = true;
	
	// client->protection.shelter_always = false;
	// client->protection.shelter_leader = true;
	// client->protection.shelter_troops = false;
	// client->protection.shelter_on_incoming_attack = true;
	// client->protection.shelter_on_incoming_scout = true;
	
	/*
	 * Darknest Configuration 
	 * Automatic Darknest settings
	 * Currently core logic not implemented yet
	 */ 
	client->darknest.auto_join = true;
	client->darknest.min_level = 4;
	client->darknest.max_level = 6;
	
	// Maximum number of marches the bot can use for Darknest rallies at the same time (if available)
	client->darknest.max_march = 2;
	
	// Automatically set Darknest essence in Transmutation Lab
	client->darknest.auto_transmute = true;
	
	// Desired essence level
	client->darknest.essence_level = 18;
	
	// Do not join if the rally host is more than 200 miles away.
	client->darknest.max_distance = 200; 
	
	// Total troops to send when joining Darknest rally.
	client->darknest.troop_count = 200000;
	
	// Minimum troops required; if available troops cannot reach this, do not join.
	client->darknest.min_join_troops = 150000;
	
	client->darknest.formation_mode = DARKNEST_FORMATION_LEADER;
	
	// Used only when formation_mode == DARKNEST_FORMATION_FIXED
	// Troop ratio (8480 = 80% Inf, 40% Ranged, 80% Cavalry, 0% Siege).
	client->darknest.formation = 8480;
	
	// Join random delay between.
	client->darknest.min_join_delay = 3;
	client->darknest.max_join_delay = 180;
	
	// Darknest troop priority order: bot tries higher tier troops first (T5 → T1) when selecting troops for rally join.
	client->darknest.tier_priority_count = 3;
	client->darknest.tier_order[0] = TIER_T5; // T5
	client->darknest.tier_order[1] = TIER_T4;
	client->darknest.tier_order[2] = TIER_T3;
	client->darknest.tier_order[3] = TIER_T2;
	client->darknest.tier_order[4] = TIER_T1;
	
	
	// currently no banking system implemented 
	client->bank.enabled   = true;
	client->bank.send_food = true;
	client->bank.send_rock = true;
	client->bank.send_wood = true;
	client->bank.send_ore  = true;
	client->bank.send_gold = true;
	
	client->bank.reserve.food = 0;
	client->bank.reserve.rock = 0;
	client->bank.reserve.wood = 0;
	client->bank.reserve.ore  = 0;
	client->bank.reserve.gold = 0;
	
	client->bank.max_delivery_distance = 100;
	
	client->bank.use_bag_rss  = false;
	client->bank.use_bag_food = false;
	client->bank.use_bag_rock = false;
	client->bank.use_bag_wood = false;
	client->bank.use_bag_ore  = false;
	client->bank.use_bag_gold = false;
	
}

void PrintUsage(void)
{
	printf(
		"Lords Mobile Bot\n"
		"\n"
		"Usage:\n"
		"  client <config_file>         Load and start the bot using the specified configuration file.\n"
		"  client --create-config, -c   Create a default configuration file.\n"
		"  client --help, -h            Display this help message.\n"
		"  client --version, -v         Display version information.\n"
		"\n"
		"Project:\n"
		"  https://github.com/halloweeks/lords-mobile-bot\n"
	);
}

void PrintVersion(void)
{
    printf(
        "Lords Mobile Bot v%s\n"
        "Build: %s %s\n"
        "Project: https://github.com/halloweeks/lords-mobile-bot\n",
        VERSION,
        __DATE__,
        __TIME__);
}

bool CreateDefaultConfig(const char *filename)
{
	FILE *fp = fopen(filename, "w");
	
	if (!fp)
		return false;
	
	fprintf(fp,
		"# Lords Mobile Bot Configuration\n"
		"# Generated automatically. Edit this file to configure the bot.\n"
		"# Project: https://github.com/halloweeks/lords-mobile-bot\n"
		"# Docs: https://github.com/halloweeks/lords-mobile-bot/blob/main/docs/configuration.md\n\n"
		
		"# Gateway server\n"
		"server.addr = 192.243.44.63\n"
		"server.port = 5999\n\n"
		
		"# Client version\n"
		"client.version_major = 2\n"
		"client.version_minor = 200\n"
		"client.version_patch = 312\n"
		"client.language_code = 1\n\n"
		
		"# Directory used to store bot data (logs, databases, cache, etc.).\n"
		"data.path = /sdcard/lmbot/\n\n"
		
		"# Privileged player.\n"
		"# This player can execute administrator commands and bypass normal restrictions.\n"
		"admin.name = halloweeks\n\n"
		
		"# Replace the example values below with your own account information.\n"
		"account.igg_id = 1234567890\n"
		"account.device_uuid = 12345678-1234-1234-1234-123456789abc\n"
		"account.access_key = YOUR_ACCESS_KEY_HERE\n\n"
		
		"# Prefix used to identify bot commands.\n"
		"command.prefix = $\n\n"
		
		"# Command channels: WORLD, GUILD, MAIL\n"
		"command.input = GUILD\n"
		"command.output = MAIL\n\n"
		
		"# Bank\n"
		"# Master switch for the banking system.\n"
		"# When enabled, the bot accepts and processes banking commands.\n"
		"# When disabled, all banking commands are ignored.\n"
		"bank.enabled = false\n\n"
		
		"# Resource types allowed for delivery.\n"
		"bank.send_food = false\n"
		"bank.send_rock = false\n"
		"bank.send_wood = false\n"
		"bank.send_ore  = false\n"
		"bank.send_gold = false\n\n"
		
		"# Resource reserve.\n"
		"# These values are reserved for the bot's own use. The bot will not send\n"
		"# resources that would reduce the balance below these amounts.\n"
		"bank.reserve_food = 20M\n"
		"bank.reserve_rock = 50M\n"
		"bank.reserve_wood = 50M\n"
		"bank.reserve_ore  = 30M\n"
		"bank.reserve_gold = 0\n\n"
		
		"# Maximum map distance (tiles) for resource delivery.\n"
		"bank.max_delivery_distance = 100\n\n"
		
		"# Automatically use resource items from the bag if the available\n"
		"# resources are insufficient to fulfill a banking command.\n"
		"bank.use_bag_rss  = false\n"
		"bank.use_bag_food = false\n"
		"bank.use_bag_rock = false\n"
		"bank.use_bag_wood = false\n"
		"bank.use_bag_ore  = false\n"
		"bank.use_bag_gold = false\n\n"
		
		/*
		"# Alliance\n"
		"alliance.auto_help = false\n"
		"alliance.auto_open_gifts = false\n\n"
		*/
		
		
		"# Enable or disable all automatic protection features.\n"
		"protection.enabled = false\n\n"
		
		"# Keep a shield active at all times.\n"
		"protection.shield_always_on = false\n\n"
		
		"# Automatically use a shield when an incoming attack is detected.\n"
		"protection.shield_on_incoming_attack = false\n\n"
		
		"# Automatically use a shield when an incoming scout is detected.\n"
		"protection.shield_on_incoming_scout = false\n\n"
		
		"# Shield priority list.\n"
		"# The bot will use the first available shield in this order.\n"
		"# Available shields:\n"
		"# SHIELD_4H, SHIELD_8H, SHIELD_12H, SHIELD_1D, SHIELD_3D, SHIELD_7D, SHIELD_14D\n"
		"protection.shield_priority = SHIELD_4H, SHIELD_8H, SHIELD_12H, SHIELD_1D\n"
		
		"\n\n# Automatically recall marches when an incoming attack is detected.\n"
		"protection.recall_on_incoming_attack = false\n\n"
		
		"# Automatically recall marches when an incoming scout is detected.\n"
		"protection.recall_on_incoming_scout = false\n\n"
		
		"# Automatically recalls a gathering or camp march before it reaches\n"
		"# its destination when an incoming conflict is detected.\n"
		"# Requires Withdraw Squad items; otherwise no action is taken.\n"
		"protection.recall_on_incoming_conflict = false\n\n"
		
		"# Currently this feature not available\n"
		"# protection.shelter_always = false\n"
		"# protection.shelter_leader = true\n"
		"# protection.shelter_troops = false\n"
		"# protection.shelter_on_incoming_attack = true\n"
		"# protection.shelter_on_incoming_scout = true\n\n"


		"# Gathering\n"
		"# Master switch for automatic gathering.\n"
		"gathering = false\n\n"
		"# Leave one army at home for emergencies.\n"
		"gathering.spare_army = true\n\n"
		"# Prioritize highest level tiles over closest.\n"
		"gathering.highest_level_first = true\n\n"
		"# Prefer lower-tier troops for gathering before higher tiers.\n"
		"gathering.low_tier_first = true\n\n"
		"# Prioritize tiles of the resource type you have the least of.\n"
		"gathering.lowest_resource = false\n\n"
		"# Only target tiles that can be fully cleared with available troops.\n"
		"gathering.clearable_only = false\n\n"
		"# Ignore level settings for gem lodes (always gather any level).\n"
		"gathering.gems_ignore_level = true\n\n"
		"# Auto-recall camps that appear when a tile disappears during march.\n"
		"gathering.recall_camps = false\n\n"
		"# Equip gathering gear before sending marches.\n"
		"gathering.gathering_gear = false\n\n"
		"# Maximum number of armies to use for gathering (0 = use all).\n"
		"gathering.max_armies = 0\n\n"
		"# Delay in seconds between sending each march.\n"
		"gathering.delay_seconds = 5\n\n"
		"# Maximum travel time in seconds for gathering marches.\n"
		"gathering.max_travel_seconds = 900\n\n"
		"# Search range multiplier (1 = 1 screen, 2 = 2 screens, etc.).\n"
		"gathering.search_multiplier = 1\n\n"
		"# Min distance from the city (world units) for a gather destination.\n"
		"# Near-castle tiles are answered Standby, never dispatched as a march;\n"
		"# only nodes outside the city's own protected ring become a real gather.\n"
		"gathering.min_gather_distance = 40\n\n"
		"# Southward floor (world units below the city's latitude): the euclidean\n"
		"# min_gather_distance gate alone lets at-latitude EAST nodes pass on the\n"
		"# protected latitude.  Only nodes this far south clear the ~64-unit ring\n"
		"# (the proven type-7 gather was ~57 units south).  0 disables the floor.\n"
		"gathering.min_south_units = 50\n\n"
		"# How many zones outward the map scan sweeps to discover far real nodes.\n"
		"gathering.scan_reach = 4\n\n"
		"# Minimum resource count on a tile to consider it.\n"
		"gathering.minimum_tile_count = 1\n\n"
		"# Resource types to gather (food/stone/wood/ore/gold/gems).\n"
		"gathering.resource_food = true\n"
		"gathering.resource_stone = true\n"
		"gathering.resource_ore = true\n"
		"gathering.resource_wood = true\n"
		"gathering.resource_gold = true\n"
		"gathering.resource_gems = true\n\n"
		"# Tile levels to gather (1-5).\n"
		"gathering.level_1 = true\n"
		"gathering.level_2 = true\n"
		"gathering.level_3 = true\n"
		"gathering.level_4 = true\n"
		"gathering.level_5 = true\n\n"
		"# Hero IDs for gathering marches (0 = no hero, gathering works without).\n"
		"gathering.hero_1 = 0\n"
		"gathering.hero_2 = 0\n"
		"gathering.hero_3 = 0\n"
		"gathering.hero_4 = 0\n"
		"gathering.hero_5 = 0\n\n"
		"# Gathering schedule (24h format, same day only).\n"
		"gathering.schedule_enabled = false\n"
		"gathering.schedule_start = 00:00\n"
		"gathering.schedule_end = 23:59\n\n"
		"# Venue switches: independent gather loops sharing the march slot.\n"
		"# Guild Expedition (GBG / Alliance Battlefield, 11403+11412)\n"
		"gathering.enable_gbg = false\n"
		"# Chaos Arena (Solo Battlefield, 11992)\n"
		"gathering.enable_chaos = false\n\n"


		"# Cargo Ship Trading\n"
		"# Automatically completes Cargo Ship trades.\n"
		"# The options below specify which resources the bot is allowed to spend.\n"
		"cargo_ship.auto_trade  = false\n"
		"cargo_ship.spend_food  = false\n"
		"cargo_ship.spend_rock  = false\n"
		"cargo_ship.spend_wood  = false\n"
		"cargo_ship.spend_ore   = false\n"
		"cargo_ship.spend_gold  = false\n\n"
		
		"# Use resource items from the bag when required to complete a trade.\n"
		"# If disabled, the bot will never consume bag resource items.\n"
		"cargo_ship.use_bag_rss = false\n\n"
		
		"# Resource reserve limits.\n"
		"# The bot always keeps at least this amount and only spends the excess.\n"
		"# Set to 0 to disable the reserve.\n"
		"cargo_ship.reserve_food = 10M\n"
		"cargo_ship.reserve_rock = 10M\n"
		"cargo_ship.reserve_wood = 10M\n"
		"cargo_ship.reserve_ore  = 10M\n"
		"cargo_ship.reserve_gold = 10M\n\n"

	);

    fclose(fp);
    return true;
}

/* Portable blocking sleep, used by the auto-restart loop between retries. */
static void sleep_sec(int seconds)
{
	if (seconds < 1) seconds = 1;
#ifdef _WIN32
	Sleep((unsigned)(seconds * 1000));
#else
	usleep((useconds_t)seconds * 1000000u);
#endif
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "--create-config") == 0 || strcmp(argv[1], "-c") == 0) {
		if (CreateDefaultConfig("config.cfg")) {
			printf("[INFO ] Default configuration generated: config.cfg\n");
			return 0;
		}
		
		printf("[ERROR] Failed to create configuration file: config.cfg\n");
		return 1;
	}
	
	if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		PrintVersion();
		return 0;
	}

#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		LOGE("WSAStartup failed");
		return 1;
	}
#endif
	
	if (argc != 2) {
        printf("Usage: %s <config.cfg>\n", argv[0]);
        return EXIT_FAILURE;
    }
	
	Connection client = {0};

	// Initialize gathering defaults before loading config so config values are preserved.
	GatheringInit(&client);

	if (!LoadConfig(&client, argv[1])) {
		LOGE("Failed to load config\n");
		return EXIT_FAILURE;
	}

	LOGI("Configuration loaded\n");

	// Load bot settings from settings.json
	if (!LoadBotSettings("settings.json")) {
		LOGI("Settings file not found, using defaults\n");
	}

	// Load GameAssets tables
	if (!LoadAllGameAssets()) {
		LOGI("Some GameAssets failed to load, continuing anyway\n");
	}
	LOGI("[CFG] use_bag_rss=%d rss_floor=%u training_highest_tier=%d training_tier=%u\n",
	     client.automation.use_bag_rss, client.automation.rss_floor,
	     client.automation.training_highest_tier, client.automation.training_tier);

	// Open the bot activity log (local file journal; no gameplay impact).
	ActivityLogOpen(&client);

	// Establish TCP connection to server and store socket descriptor in client
	client.sock = connect_server(client.gateway_server.addr, client.gateway_server.port);

	// Validate socket creation; -1 indicates connection failure
	if (client.sock == -1) {
		LOGE("Failed to connect to %s:%d\n", client.gateway_server.addr, client.gateway_server.port);
		return EXIT_FAILURE;
	}

	LOGI("Connected gateway server: %s:%u\n", client.gateway_server.addr, client.gateway_server.port);
	LOGI("Game client: v%u.%u.%u\n", client.app.version_major, client.app.version_minor, client.app.version_patch);

	RequestGuestLogIn(&client);

	ProcessConnection(&client);

	if (!client.lobby_login) {
		LOGE("Login failed!\n");
		return EXIT_FAILURE;
	}

	// Establish TCP connection to game server
	client.sock = connect_server(client.game_server.addr, client.game_server.port);

	if (client.sock == -1) {
		// LOGE("Failed to connect game server!");
		LOGE("Failed to connect game server %s:%d\n", client.game_server.addr, client.game_server.port);
		return EXIT_FAILURE;
	}

	if (set_nonblocking(&client) != 0) {
		LOGE("set_nonblocking\n");
	}

	LOGI("IGG ID: %lu\n", client.auth.igg_id);

	LOGI("Connected game server: %s:%u\n", client.game_server.addr, client.game_server.port);

	// clear old buffer
	memset(&client.stream, 0, sizeof(client.stream));

	// Game login
	LOGI("Logging game server\n");
	RequestLogIn(&client);

	RequestClientInitOver(&client);

	// Handle
	ProcessConnection(&client);

	// Close the bot activity log before tearing down.
	ActivityLogClose(&client);

	disconnect(&client);

	return 0;
}
