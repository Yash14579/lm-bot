#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "connection.h"

void RequestGuestLogIn(Connection *conn);
void RequestLogIn(Connection *conn);
void RequestClientInitOver(Connection *conn);
void RequestHeartBeat(Connection *conn);

void RequestTroopTraining(Connection *c, uint8_t kind, uint8_t tier, uint32_t amount);
void RequestResearchStart(Connection *c, uint16_t tech_id, uint8_t level);
void RequestResearchCompleteFree(Connection *c, uint16_t tech_id, uint8_t level);
void RequestResearchCompleteImmediate(Connection *c, uint16_t tech_id, uint8_t level);
void RequestResearchCancel(Connection *c, uint16_t tech_id, uint8_t level);
void RequestBuildFinish(Connection *c, uint8_t position_id);
void RequestBuildCompleteFree(Connection *c, uint8_t position_id);
void RequestTrainingFinish(Connection *c);

void RequestTroopRecall(Connection *c, uint8_t Index);
void RequestHealingTroops(Connection *c, const uint32_t troop_array[16]);
void RequestInstantHealing(Connection *c, const uint32_t troop_array[16]);
void RequestFinishHealing(Connection *c);
void RequestCancelHealing(Connection *c);
const char *GetShieldName(uint16_t item_id);
void UsePriorityShield(Connection *c);
void DeployBestShield(Connection *c);
void RequestViewChat(Connection *c, uint8_t channel, uint8_t prev, int8_t kind, int64_t DataID, int64_t DataTime);

void RequestSendChat(Connection *c, uint8_t channel, const char *message);

void RequestRallyList(Connection *c);
void RequestRallyDetail(Connection *c, uint8_t arg1, uint32_t arg2);

void RequestJoinRally(Connection *c, const char *ally_name, const uint32_t troop_array[16]);


void Send_Mall_TestBuy(Connection *c, uint16_t type);

void RequestMapData(Connection *c, uint8_t count, const uint16_t zone[], bool renew);
bool RequestTroopMarchGather(Connection *c, const uint16_t hero_ids[5], const uint32_t troop_array[16],
                             uint16_t zone_id, uint8_t point_id, uint8_t point_kind, uint16_t tile_level,
                             uint32_t resource_count, uint32_t max_overload, bool no_attack,
                             uint32_t pin_node_id, uint32_t pin_dest_y, GatherVenue venue);
void RecvGatherMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size);
void GatheringTick(Connection *c);
void RequestGuildBattlefieldEventDetail(Connection *c);
void RequestGuildBattlefieldEnter(Connection *c);
void RequestSoloBattlefieldInfo(Connection *c);
void RequestOpenUI(Connection *c, uint32_t window_id);

void RequestBlackMarketData(Connection *c);
void RequestBlackMarketBuy(Connection *c, uint8_t mIdx);
void RequestSmartUseBlackMarketBuy(Connection *c, SmartUseList smart_use, uint8_t mIdx);
void SendBlackMarketBuy(Connection *c, uint8_t mIdx);

void RequestMissionInfo(Connection *c, uint8_t missionType);
void RequestAllyPoint(Connection *c, const char *name);

void RequestWatchTowerLineDetail(Connection *c, uint32_t);
void RequestTroopTakeBack(Connection*, uint8_t);

void RequestSendHelp(Connection *c, uint16_t record_sn_count, const uint32_t *record_sn);
void SendStartBuilding(Connection *c, uint8_t pos_x, uint8_t pos_y, uint16_t build_id, uint8_t operation_type);
void ServerNewbieTeleport(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id);
void ServerRelocate(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id);
void RequestUseAdvancedRelocator(Connection *c, uint16_t kingdom_id, uint16_t zone_id, uint8_t point_id);
void RequestAllianceGiftInfo(Connection*);
void RequestOpenAllianceGift(Connection*, uint32_t);

void RequestDeleteAllianceGiftBox(Connection*, uint32_t);

void ServerRename(Connection *c, bool bought, uint16_t num, const char *name);
void RequestBuyItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty);
void RequestBuyGiftItem(Connection *c, uint8_t Type, uint16_t Key, uint16_t ItemID, uint16_t Qty, const char Name[13]);


void RequestSimpleUseItem(Connection *c, uint32_t item_id, uint16_t quantity);
void RecvUseItem(Connection *c, const uint8_t *data, uint16_t);

void ServerMagicGateDoEvent(Connection *c, uint16_t n, uint8_t x);

void RequsetWorldTeleportItemCount(Connection *c, uint64_t Power);


void RecvLoginError(Connection *c, const uint8_t *data);
void HandleLoginValidate(Connection *c, const uint8_t *data, uint16_t size);
void RecvChatMessage(Connection *c, const uint8_t *data);

void RecvAllBuildData(Connection *c, const uint8_t *data);
bool IsBuilding(uint16_t build_id);
const char *GetBuildingName(uint16_t build_id);
void RequestHealing(Connection *c, bool instant);

void RecvItemInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvIBuffInfo(Connection *c, const uint8_t *data);
void RecvMarchData(Connection*, const uint8_t*, uint16_t size);
void RecvLoginRoleInfo(Connection *c, const uint8_t *data, uint16_t size);

void RecvResources(Connection *c, const uint8_t *data);

void RecvBlackMarket_Data(Connection *c, const uint8_t *data);
void RecvBlackMarket_Buy(Connection *c, const uint8_t *data);
void RecvMailInfo(Connection *c, const uint8_t *data);


void RecvAllyPoint(Connection *c, const uint8_t *data);

void RecvAllianceHelp(Connection *c, const uint8_t *data);
void RecvAllianceMemberNeedsHelp(Connection *c, const uint8_t *data);
void RecvPendingAllianceMembersNeedHelp(Connection *c, const uint8_t *data);
void RecvAllianceGiftInfo(Connection *c, const uint8_t *data);
void RecvRoleUpdateInfo(Connection *, const uint8_t*);

void RecvAllianceGiftOpen(Connection *c, const uint8_t *data);
void RecvDeleteAllianceGiftBox(Connection*, const uint8_t*);

void RecvBuyItem(Connection *c, const uint8_t *data, uint16_t size);
void RecvArmyGroupInfo(Connection *c, const uint8_t *data);
void RecvTrainingInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvWoundedTroopData(Connection *c, const uint8_t *data);
void RecvHealingResponse(Connection *c, const uint8_t *data);

void RecvRefreshResources(Connection *c, const uint8_t *data);

void HeartbeatTick(Connection *c);
void BlackMarketTick(Connection *c);
void ShieldTick(Connection *c);
void AllianceGiftTick(Connection*);
void RecvAllianceInfo(Connection*, const uint8_t*);

void RecvBuildingQueue(Connection*, const uint8_t*);

void RecvUpdateWatchTowerAddLineInfo(Connection*, const uint8_t*);
void RecvWatchTowerLineDetail(Connection *c, const uint8_t *data, uint16_t size);
const char *FormatTime(uint32_t totalSecs);


void RecvDarknestBroadcast(Connection *c, const uint8_t *data);

void RecvRallyCountData(Connection*, const uint8_t*);

void DarknestRallyTick(Connection *);

void RecvWarBegin(Connection *c, const uint8_t *data);

void RecvNPCWallHallData(Connection *c, const uint8_t *data);
void RecvWallHallTroop(Connection*, const uint8_t*);
void RecvNPCWallHallDetail(Connection*, const uint8_t*);
void RecvWallHallDel(Connection *c, const uint8_t *data);
void RecvWallHallData(Connection *c, const uint8_t *data);
void RecvWallHallDetail(Connection *c, const uint8_t *data);
void RecvTechnologyInfo(Connection*, const uint8_t*, uint16_t);


void RecvAddConflictLine(Connection *c, const uint8_t *data);
void RecvMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size);

void RecvMagicGateDoEvent(Connection *c, const uint8_t *data, uint16_t size);

void RequestUnknown(Connection *c);
void RecvWallHallDetailClose(Connection *c, const uint8_t *data);


void RecvJoinedRallyData(Connection *c, const uint8_t *data);

void ResourceTransferTick(Connection *c);


void RequestSendMail(Connection *c, const char *player_name, const char *subject, const char *message);
void RequestSendMailFmt(Connection *c, const char *player_name, const char *subject, const char *fmt, ...);

void RecvSHelp(Connection *c, const uint8_t *data);
void RecvHelp_Home(Connection *c, const uint8_t *data);


void format_number2(uint64_t num, char *out, size_t size);

void RecvAllianceMemberInfo(Connection *c, const uint8_t *data);
void RequestAllianceMemberInfo(Connection *c);

/* Daily Mission / Battle Pass */
void RequestDailyMissionInfo(Connection *c);
void RequestDailyMissionReward(Connection *c, uint16_t reward_id);
void RecvDailyMissionInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvDailyMissionReward(Connection *c, const uint8_t *data, uint16_t size);

/* VIP Mission */
void RequestVipMissionInfo(Connection *c);
void RequestVipMissionCollect(Connection *c, uint16_t mission_id);
void RecvVipMissionInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvVipMissionCollect(Connection *c, const uint8_t *data, uint16_t size);

/* Daily Sign-in */
void RequestDailySigninInfo(Connection *c);
void RequestDailySignin(Connection *c);
void RecvDailySigninInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvDailySignin(Connection *c, const uint8_t *data, uint16_t size);

/* Pet Training */
void RequestPetTrainingInfo(Connection *c);
void RequestPetTrainingBegin(Connection *c, uint16_t pet_id, uint8_t training_type, bool use_speedup);
void RequestPetTrainingFinish(Connection *c, uint8_t slot_index);
void RecvPetTrainingInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvPetTrainingBegin(Connection *c, const uint8_t *data, uint16_t size);
void RecvPetTrainingFinish(Connection *c, const uint8_t *data, uint16_t size);

/* Hero System */
void RequestHeroInfo(Connection *c);
void RequestHeroEnhanceFinish(Connection *c, uint16_t hero_id);
void RecvHeroInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvHeroEnhanceFinish(Connection *c, const uint8_t *data, uint16_t size);

/* Item Crafting */
void RequestItemCraftInfo(Connection *c);
void RequestItemCraftStart(Connection *c, uint16_t recipe_id, uint16_t count);
void RequestItemCraftFinish(Connection *c, uint16_t recipe_id);
void RecvItemCraftInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvItemCraftStart(Connection *c, const uint8_t *data, uint16_t size);
void RecvItemCraftFinish(Connection *c, const uint8_t *data, uint16_t size);

/* Achievement */
void RequestAchievementInfo(Connection *c);
void RequestAchievementPrize(Connection *c, uint8_t kind, uint16_t activity_id, uint16_t reward_id);
void RecvAchievementInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvAchievementPrize(Connection *c, const uint8_t *data, uint16_t size);

/* Quest Chapter */
void RequestQuestChapterInfo(Connection *c);
void RequestQuestChapterReward(Connection *c, uint16_t quest_id);
void RecvQuestChapterInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvQuestChapterReward(Connection *c, const uint8_t *data, uint16_t size);

/* Expedition */
void RequestExpeditionInfo(Connection *c);
void RequestExpeditionPrize(Connection *c, uint16_t expedition_id);
void RecvExpeditionInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvExpeditionPrize(Connection *c, const uint8_t *data, uint16_t size);

/* Valhalla */
void RequestValhallaInfo(Connection *c);
void RequestValhallaPrize(Connection *c);
void RequestValhallaInstantRevive(Connection *c);
void RequestValhallaDivineRevive(Connection *c);
void RecvValhallaInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvValhallaPrize(Connection *c, const uint8_t *data, uint16_t size);
void RecvValhallaInstantRevive(Connection *c, const uint8_t *data, uint16_t size);
void RecvValhallaDivineRevive(Connection *c, const uint8_t *data, uint16_t size);

/* Adventure */
void RequestAdventureMissionInfo(Connection *c);
void RequestAdventureMissionPrize(Connection *c, uint16_t mission_id);
void RequestAdventureStartQuest(Connection *c, uint16_t mission_id);
void RequestAdventureHunt(Connection *c, uint16_t monster_id);
void RecvAdventureMissionInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvAdventureMissionPrize(Connection *c, const uint8_t *data, uint16_t size);
void RecvAdventureHunt(Connection *c, const uint8_t *data, uint16_t size);

/* Relics */
void RequestRelicsInfo(Connection *c);
void RequestRelicsGacha(Connection *c, uint16_t gacha_id);
void RequestRelicsSynthesize(Connection *c, uint16_t relic_id);
void RequestRelicsEnhance(Connection *c, uint16_t relic_id);
void RecvRelicsInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvRelicsGacha(Connection *c, const uint8_t *data, uint16_t size);
void RecvRelicsSynthesize(Connection *c, const uint8_t *data, uint16_t size);
void RecvRelicsEnhance(Connection *c, const uint8_t *data, uint16_t size);

/* Serial Gift */
void RequestSerialGiftList(Connection *c);
void RequestSerialGiftGet(Connection *c, uint8_t stage);
void RecvSerialGiftList(Connection *c, const uint8_t *data, uint16_t size);
void RecvSerialGiftGet(Connection *c, const uint8_t *data, uint16_t size);

/* Old Player Back (claim-only) */
void RequestOldPlayerBackGetGift(Connection *c);
void RecvOldPlayerBackInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvOldPlayerBackGetGift(Connection *c, const uint8_t *data, uint16_t size);

/* Gift Activity (claim-only) */
void RequestGiftActivityList(Connection *c);
void RequestGiftActivityOpenBox(Connection *c, uint8_t box_id);
void RecvGiftActivityList(Connection *c, const uint8_t *data, uint16_t size);
void RecvGiftActivityOpenBox(Connection *c, const uint8_t *data, uint16_t size);

/* Week Challenge */
void RequestWeekChallengeInfo(Connection *c);
void RequestWeekChallengeBuy(Connection *c, uint16_t item_id);
void RequestWeekChallengePrize(Connection *c);
void RecvWeekChallengeInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvWeekChallengeBuy(Connection *c, const uint8_t *data, uint16_t size);
void RecvWeekChallengePrize(Connection *c, const uint8_t *data, uint16_t size);

/* Lucky Card */
void RequestLuckyCardInfo(Connection *c);
void RequestLuckyCardExchange(Connection *c, uint8_t card_index);
void RecvLuckyCardInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvLuckyCardExchange(Connection *c, const uint8_t *data, uint16_t size);

/* Dark Nest */
void RequestDarkNestRallyList(Connection *c);
void RequestDarkNestJoinRally(Connection *c, uint32_t rally_id, const uint32_t troop_array[16]);
void RecvDarkNestRallyList(Connection *c, const uint8_t *data, uint16_t size);

/* Fantasy Realm */
void RequestFantasyRealmInfo(Connection *c);
void RequestFantasyRealmHunt(Connection *c, uint16_t monster_id);
void RequestFantasyRealmSummon(Connection *c);
void RecvFantasyRealmInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvFantasyRealmHunt(Connection *c, const uint8_t *data, uint16_t size);

/* Growth Fund */
void RequestGrowthFundInfo(Connection *c);
void RequestGrowthFundPrize(Connection *c, uint16_t level);
void RecvGrowthFundInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvGrowthFundPrize(Connection *c, const uint8_t *data, uint16_t size);

/* Treasure Back Event */
void RequestTreasureBackEventInfo(Connection *c);
void RequestTreasureBackEventPrize(Connection *c, uint16_t prize_id);
void RecvTreasureBackEventInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvTreasureBackEventPrize(Connection *c, const uint8_t *data, uint16_t size);

/* Newbie Challenge */
void RequestNewbieChallengeInfo(Connection *c);
void RequestNewbieChallengePrize(Connection *c, uint16_t prize_id, bool is_vip);
void RecvNewbieChallengeInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvNewbieChallengePrize(Connection *c, const uint8_t *data, uint16_t size);

/* Cycle Mission */
void RequestCycleMissionInfo(Connection *c);
void RequestCycleMissionPrize(Connection *c, uint16_t combo_id);
void RecvCycleMissionInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvCycleMissionPrize(Connection *c, const uint8_t *data, uint16_t size);

/* Custom Mission */
void RequestCustomMissionInfo(Connection *c);
void RequestCustomMissionPrize(Connection *c, uint16_t mission_id);
void RecvCustomMissionInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvCustomMissionPrize(Connection *c, const uint8_t *data, uint16_t size);

/* Mobilization (Alliance Mobilization) */
void RequestMobilizationMissionData(Connection *c);
void RequestMobilizationMissionRefresh(Connection *c, uint8_t mission_pos, uint8_t mission_kind, uint8_t reset);
void RequestMobilizationMissionBuy(Connection *c);
void RequestMobilizationMissionGet(Connection *c, uint8_t mission_pos, uint8_t mission_kind);
void RequestMobilizationMissionDel(Connection *c, uint8_t mission_pos, uint8_t mission_kind);
void RequestMobilizationMissionFinish(Connection *c, uint8_t mission_pos, uint8_t mission_kind);
void RequestMobilizationLegendMissionGetScore(Connection *c);
void RequestActivityAmDegeePrize(Connection *c);
void RequestActivityAmGetDegreePrize(Connection *c);
void RequestActivityAmGetPersonalPrize(Connection *c);
void RecvMobilizationMissionData(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionRefresh(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionBuy(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionGet(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionDel(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionFinish(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionUpdate(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationMissionDone(Connection *c, const uint8_t *data, uint16_t size);
void RecvMobilizationLegendMissionGetScore(Connection *c, const uint8_t *data, uint16_t size);
void RecvActivityAmDegeePrize(Connection *c, const uint8_t *data, uint16_t size);
void RecvActivityAmGetDegreePrize(Connection *c, const uint8_t *data, uint16_t size);
void RecvActivityAmGetPersonalPrize(Connection *c, const uint8_t *data, uint16_t size);
void RecvActivityAmUpdateInfo(Connection *c, const uint8_t *data, uint16_t size);

/* Alliance Gather Point */
void RequestAllianceGatherPointInfo(Connection *c);
void RequestAllianceGatherPointSet(Connection *c, uint16_t zone_id, uint8_t point_id);
void RequestAllianceGatherPointFreeTeleport(Connection *c);
void RecvAllianceGatherPointUpdate(Connection *c, const uint8_t *data, uint16_t size);
void RecvAllianceGatherPointSet(Connection *c, const uint8_t *data, uint16_t size);
void RecvAllianceGatherPointFreeTeleport(Connection *c, const uint8_t *data, uint16_t size);

/* Alliance WhiteList */
void RequestAllianceWhiteListInfo(Connection *c);
void RequestAllianceWhiteListAdd(Connection *c, int64_t user_id);
void RequestAllianceWhiteListRemove(Connection *c, int64_t user_id);
void RecvAllianceWhiteListInfo(Connection *c, const uint8_t *data, uint16_t size);
void RecvAllianceWhiteListAdd(Connection *c, const uint8_t *data, uint16_t size);
void RecvAllianceWhiteListRemove(Connection *c, const uint8_t *data, uint16_t size);

#endif