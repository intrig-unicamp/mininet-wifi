#include <hiredis.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define REDISCOMMAND_MAX_LEN 1000

int RedisGetExecRowId(redisContext *c, char* Channel,int Multi);
int RedisGetReply_ex(redisContext *c, redisReply **reply);
