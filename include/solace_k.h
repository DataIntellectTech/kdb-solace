#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"
#include <stdbool.h>
#include "cb.h"
#include "log.h"

#define MAX_TOPIC_COUNT 250
#define MAX_MSG_SIZE    500

#define KDB_LIB_VERSION    "1.0.0.1"

bool initiated;

solClient_returnCode_t rc;

K callback(I d);
void setSolaceCallback(void);
solClient_returnCode_t connectToSolace(void);
solClient_returnCode_t disconnectFromSolace(void);
solClient_returnCode_t publishToSolace(char*,K);
solClient_returnCode_t subscribeToTopic(char *);
solClient_returnCode_t unsubscribeFromTopic(char *);

K current_time();

typedef struct kopts {
        char *username;
        char *password;
        char *hostname;
	char *callback;
        solClient_log_level_t loglevel;
        int port;
        char *topic;
        char *vpn;
        int connectretries;
        int nummsgstosend;
        bool enablecompression;
        bool usegss;
        K schema;
        char *tablename;
        int handle;
        char *rcvfunc;
	qclient_log_level_t loglvl;
	bool dumptoscreen;
}kopts;

kopts opts;
int fd[2];
int msg_recv, msg_sent;

cb queue;
