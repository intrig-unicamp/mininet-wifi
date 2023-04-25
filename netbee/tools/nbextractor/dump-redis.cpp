#include "dump-redis.h"

int RedisGetExecRowId(redisContext *c, char* Channel,int Multi)
{
	char RedisCommandBuffer[REDISCOMMAND_MAX_LEN];
	redisReply *reply;
	int ExecRowId;

	sprintf(RedisCommandBuffer,"INCRBY %s:nextid %i",Channel,Multi);
	redisAppendCommand(c,RedisCommandBuffer);
	redisGetReply(c,(void**)&reply);
	if ( reply->type != REDIS_REPLY_INTEGER )
	{
		fprintf(stderr,"Error during fetching next Redis ID\n%s\n",c->errstr);
		exit(1);
	}
	ExecRowId = reply->integer;
	freeReplyObject(reply);
	return (ExecRowId);
}

int RedisGetReply_ex(redisContext *c, redisReply **reply)
{
	redisGetReply(c,(void**)reply);
	if ( (*reply)->type == REDIS_REPLY_ERROR )
	{
		fprintf(stderr,"Redis Error: %s\n",(*reply)->str);
		exit(1);
	}
}

