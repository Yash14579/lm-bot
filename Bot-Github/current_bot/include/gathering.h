#ifndef GATHERING_H
#define GATHERING_H
#include "connection.h"
void GatheringInit(Connection *c);
void GatheringTick(Connection *c);
void GatheringOnMarchData(Connection *c, uint8_t server_current_marches);
void GatheringOnMarchAck(Connection *c, uint32_t event_type);
void GatheringOnMarchReject(Connection *c, uint8_t result_code);
void GatheringOnMarchEnd(Connection *c);
void RecvGatherMapInfoPlus(Connection *c, const uint8_t *data, uint16_t size);
void RecvGbgEventDetail(Connection *c, const uint8_t *data, uint16_t size);
void RecvChaosInfo(Connection *c, const uint8_t *data, uint16_t size);
int GatheringFindTiles(Connection *c, int kind, int level, char *out, size_t out_size);
int build_troop_array(const Connection *c, uint32_t out[16]);
#endif