#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "k.h"
#include <time.h>

#ifndef _LOGHEADER_
#define _LOGHEADER_

static FILE *logfp;

typedef enum qclient_log_level {
	QCLIENT_LOG_NON = 0,		/**< No logging shown (only requested info, eg getstats) */
	QCLIENT_LOG_REQ = 0,		/**< Used internally to denote requested info) */
    	QCLIENT_LOG_ERR = 1,		/**< Show errors only */
    	QCLIENT_LOG_WRN = 2,    	/**< Show warnings and errors */
	QCLIENT_LOG_INF = 3,		/**< Show information on progress as well as warnings & errors */
	QCLIENT_LOG_ALL = 4,  		/**< Show all log messages */
} qclient_log_level_t;      		/**< Type for log levels. */

void logtofile(qclient_log_level_t loglvl, char *str, ...);
#endif

