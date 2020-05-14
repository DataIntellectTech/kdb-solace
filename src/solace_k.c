// K functions for solace
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "k.h"
#include "solace_k.h"

int callbackcounter,
    msg_recv,
    msg_sent,
    callbackcounter,
    msg_processed;
       

struct {

	int    size;
	time_t subscribed[MAX_TOPIC_COUNT];
	time_t unsubscribed[MAX_TOPIC_COUNT];
	char   *topics[MAX_TOPIC_COUNT];

} topicindex;

solClient_version_info_pt version_p;

char buffer[1024];
struct sockaddr_in serverAddr;
socklen_t addr_size;
int fd[2];

struct kopts opts = {
        "admin",
        "admin",
        "192.168.1.57",
        "f",
	SOLCLIENT_LOG_WARNING,
        0,
        "",
        "default",
        3,
        10,
        false,
        false,
        (K)NULL,
        "table",
        0,
        "insert",
        QCLIENT_LOG_ALL,
	false
};


K setup_schema(K x);
K config_opts(K x);
K printargs(K x);
void setup_socket(void);

// Return error to kdb
K kdberr(char* err) {
   	krr(err);
	return (K)0;
}

void feedreset()
{
	msg_recv = 0;
	msg_sent = 0;
	callbackcounter = 0;
	msg_processed = 0;
	return;
}

/** INITIALIZATION AND CONFIGURATION FUNCTIONS **/

/* init
 * Initialization of opts values
 * */
K init(K x) {
        
	logtofile(QCLIENT_LOG_INF, "INFO : Initializing session...\n");
        solClient_version_get ( &version_p );
	if(initiated>0) return kdberr("alreadyinit");	
	feedreset();
        topicindex.size = 0;
	logfp = stdout;
	x = setup_schema(x);
	if( x->t != XD ) return kdberr("expecteddict");

	config_opts(x);

	setup_socket();

	logtofile(QCLIENT_LOG_INF, "INFO : Session initialization complete\n");
	
	printargs((K)NULL);

	logtofile(QCLIENT_LOG_INF, "INFO : Connecting to Solace...\n");

	if( ( rc = connectToSolace() ) != SOLCLIENT_OK ) {
		return kdberr((char*) solClient_returnCodeToString(rc));
	}

	logtofile(QCLIENT_LOG_INF, "INFO : Connection to Solace complete\n");
	initiated = true;

	return (K)0;

}

/* config_opts
 * Read configuration values from K object into opts
 * */
K config_opts(K x) {

	K keys = kK(x)[0];
	K values = kK(x)[1];

	// Iterate over key-value pairs
	// Check key and updates opts accordingly
	for( int i = 0, n = (int) keys->n; i < n; i++ ) {
		
		S key = kS(keys)[i];
		K value = kK(values)[i];

		if( strcmp( key, "user" ) == 0 ) {
			 
			if( value->t != -11 ) return kdberr("user");
			 opts.username = value->s;
		
		} else if ( strcmp( key, "password" ) == 0 ) {

			if( value->t != -11 ) return kdberr("password");
			opts.password = value->s;

		} else if ( strcmp( key, "host" ) == 0 ) {
		      if( value->t != -11 ) return kdberr("host");
	               opts.hostname = value->s;	      
		} else if ( strcmp( key, "callback" ) == 0 ) {

			if( value->t != -11 ) return kdberr("callback");
			opts.callback = value->s;
			
		} else if ( strcmp( key, "port" ) == 0 ) {

			if( value->t != -7 && value->t != -6 ) return kdberr("port");
			opts.port = value->j;

		} else if ( strcmp( key, "vpn" ) == 0 ) {

			if( value->t != -11 ) return kdberr("loglevel");
			opts.vpn = value->s;

		} else if ( strcmp( key, "loglevel" ) == 0 ) {

			if( value->t != -7 && value->t != -6 ) return kdberr("loglevel");
			opts.loglevel = value->j;

		} else if ( strcmp( key, "topic" ) == 0 ) {

			if( value->t != -11 ) return kdberr("topic");
			opts.topic = value->s;

		} else if ( strcmp( key, "connectretries" ) == 0 ) {

			if( value->t != -7 && value->t != -6 ) return kdberr("connectretries");
			opts.connectretries = value->j;

		} else if ( strcmp( key, "msgstosend" ) == 0 ) {

			if ( value->t != -7 && value->t != -6 ) return kdberr("msgstosend");
			opts.nummsgstosend = value->j;

		} else if ( strcmp( key, "enablecompression" ) == 0 ) {

			if( value->t != -1 ) return kdberr("enablecompression");
			opts.enablecompression = value->g;

		} else if ( strcmp( key, "usegss" ) == 0 ) {

			if( value->t != -1 ) return kdberr("usegss");
			opts.usegss = value->g;

		} else if ( strcmp( key, "schema" ) == 0 ) {

			if( value->t != 99 ) return kdberr("schema");
			opts.schema = xD(kK(value)[0], kK(value)[1]);

		} else if ( strcmp( key, "tablename" ) == 0 ) {

			if( value->t != -11 ) return kdberr("tablename");
			opts.tablename = value->s;

		} else if ( strcmp( key, "handle" ) == 0 ) {

			if( value->t != -6 || value->t != -7 ) return kdberr("handle");
			opts.handle = value->j;

		} else if ( strcmp( key, "rcvfunc" ) == 0 ) {

			if( value->t != -11 ) return kdberr("rcvfunc");
			opts.rcvfunc = value->s;

		} else if ( strcmp( key, "loglvl" ) == 0) {

			if( value->t != -7 && value->t != -6 ) return kdberr("loglvl");
			opts.loglvl = value->j;
		
		} else if ( strcmp( key, "dumptoscreen" ) == 0 ) {

			if( value->t != -1 ) return kdberr("dumptoscreen");
			opts.dumptoscreen = value->g;
		
		}
	}

	return (K)NULL;

}

K setup_schema(K x) {

	// Check parameter is a K object - create empty dictionary if not
	if( x->t == 101 ) x = xD( ktn( KS, 0 ), ktn( KS, 0 ) );

	opts.schema = xD( ktn( KS, 0 ), ktn( KS, 0 ) );

	return x;

}


void* noop(void* x) {

	while(1) usleep(1000);
}

void setup_socket(void) {

	pthread_t tid;
	
	cb_init(&queue, 1000000);
	
	socketpair(AF_LOCAL, SOCK_STREAM, 0 , fd);
	
	fcntl(fd[0], F_SETFL, O_NONBLOCK);
	
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	addr_size = sizeof serverAddr;
	
	sd1(fd[1], callback);
	
	pthread_create(&tid, NULL, &noop, (void*) &fd[0]);

	return;

}

/** MANAGE CONNECTION **/

K checkconnect (K x) {

	return kb(initiated ? 1:0);

}

K sessiondisconnect (K x) {

	logtofile(QCLIENT_LOG_INF, "INFO : Disconnecting from server...\n");
       	
	if(!initiated) {
	
	 	return kdberr("nonconnected");
        
	}

	if( ( rc = disconnectFromSolace() ) != SOLCLIENT_OK ) {
	
		return kdberr((char*) solClient_returnCodeToString(rc));
	
	}

	initiated = false;

        // Free up memory
	int ii=0;
	for(ii; ii<topicindex.size; ii++) {
		free(topicindex.topics[ii]);
	}

	logtofile(QCLIENT_LOG_INF, "INFO : Disconnection complete\n");

	return (K)NULL;
}

K changeloglevel(K x) {

	logtofile(QCLIENT_LOG_INF, "INFO : Changing log level...\n");
	
	if( x->t != -7 && x->t != -6 ) return kdberr("loglevel");
	opts.loglvl = x->j;
	
	logtofile(QCLIENT_LOG_INF, "INFO : Log level is now: %d\n", opts.loglvl);

	return (K)0;

}

/** SEND AND RECEIVE MESSAGES **/

K add_rec_time(K x) {

	K keys = ktn(KS,0);

	// Create keys for all fields
        js(&keys,ss("type")); // Message type - admin or stream
	js(&keys,ss("seqn")); // Sequence number
	js(&keys,ss("dest")); // Topic
	js(&keys,ss("sndt")); // Message Sent Time
	js(&keys,ss("rect")); // Message Received Time
	js(&keys,ss("swbt")); // Start Write Buffer Time
	js(&keys,ss("msg"));  // Message
	js(&keys,ss("erbt")); // End Read Buffer Time
	
	// Add end read buffer time to vals
	jk(&x,current_time());

	// Make dictionary
	return xD(keys,x);

}

uint8_t rbuf[1000000];

/* callback
 * */
K callback(I d) {

	char c;
	K msg, dmsgs;
	long qsize, msize;

	recv(d, &c, 1, 0);
	qsize = cb_size(&queue);

	if(qsize > 0) {

		dmsgs = ktn(0,0);
		cb_read(&queue, rbuf, qsize, 0);
		long offset = 0;
		
		while(offset < qsize ) {

			memcpy(&msize, rbuf + offset, sizeof(long));
			assert((msize > 0) && (msize < MAX_MSG_SIZE));
			msg = ktn(KG, msize);
			memcpy(kG(msg), rbuf + offset + sizeof(long), msize);
			K msg_des = d9(msg);
			K msg_final = add_rec_time(msg_des);
			jk(&dmsgs,msg_final);
			r0(msg);
			offset += msize + sizeof(long);
			msg_processed++;
		}

		k(0, opts.callback, dmsgs, (K)0);
	}
	
	callbackcounter++;
	return (K)0;
}

K publishtosolace (K topic,K message) {

	if( !initiated ) {
		return kdberr("noinit");
	}

        if( (topic->t)!=10 ) {
		return kdberr("topictype");
	}
	if( (message->t)<0 ) {
                return kdberr("messagetype");
        }
        
	logtofile(QCLIENT_LOG_INF, "INFO : Publishing to Solace...\n");
	
	char* _topic = "";
	
	_topic = (char*) kC(topic);
	_topic[topic->n] = '\0';
	
	if( ( rc = publishToSolace( _topic, message) ) != SOLCLIENT_OK ) {
	return kdberr((char*) solClient_returnCodeToString(rc));

	}
        msg_sent++;
	logtofile(QCLIENT_LOG_INF, "INFO : Publishing complete\n");
	return (K)0;

}

/** SUBSCRIBE AND UNSUBSCRIBE **/

int findTopicInList(char * needle) {

	int ii = 0;
	for(ii; ii < topicindex.size; ii++) {
		if(strcmp(needle, topicindex.topics[ii]) == 0) return ii;
	}

	return -1;
}

K subscribe (K topic) {

        if( !initiated ) {
		return kdberr("noinit");
	}

        if( (topic->t) != 10 ) {
        	return kdberr("topictype");
	}

        if(topicindex.size >= MAX_TOPIC_COUNT) return kdberr("maxtopics");
	
	logtofile(QCLIENT_LOG_INF, "INFO : Subscribing to topic...\n");
        
	char* _topic = "";
	_topic = (char*) kC(topic);
	_topic[topic->n] = '\0';
	 
	int pos = findTopicInList(_topic);
       	if( ( pos < -1 ) || ( pos >= topicindex.size ) ) return kdberr("outofrange");

	if(pos == -1) {

        	// Allocate space to store new topic
		topicindex.topics[topicindex.size] = malloc(sizeof(char) * (strlen(_topic) + 1));

		if(topicindex.topics[topicindex.size] == NULL) {
			logtofile(QCLIENT_LOG_ERR,"\nERROR :  Unable to allocate memory\n");
			return kdberr("memory");
		}
                // Copy topic name into struct
                strcpy(topicindex.topics[topicindex.size], _topic);

                // Update subscribed time to now and unsubscribed time to null
		topicindex.subscribed[topicindex.size] = time(NULL);
		topicindex.unsubscribed[topicindex.size] = (time_t) NULL;

		topicindex.size++;	
	}else {

		// Otherwise, update subscribed time at given position
		topicindex.subscribed[pos] = time(NULL);
		// Update unsubscribed time to null
		topicindex.unsubscribed[pos] = (time_t) NULL;
	
	}

	if( ( rc = subscribeToTopic(_topic ) ) != SOLCLIENT_OK) {

	        return kdberr((char*) solClient_returnCodeToString(rc));

	}

	logtofile(QCLIENT_LOG_INF, "INFO : Subscribed to topic: %s\n", _topic);

	return (K)0;

}

K unsubscribe (K topic) {

	if( !initiated ) {
		return kdberr("noinit");
	}

        if( (topic->t) != 10 ) {
        	return kdberr("topictype");
	}

	logtofile(QCLIENT_LOG_INF, "INFO : Unsubscribing from topic...\n");

        char* _topic = "";
	_topic = (char*) kC(topic);
	_topic[topic->n] = '\0';

	int pos = findTopicInList(_topic);

        // Find topic in list and update unsubscribed time to now
       	if( ( pos < -1 ) || ( pos >= topicindex.size ) ) return kdberr("outofrange");
        
	if( pos != -1 ) {
                topicindex.unsubscribed[pos] = time(NULL);
        
	}

        if( ( rc = unsubscribeFromTopic(_topic) ) != SOLCLIENT_OK ) {

                return kdberr((char*) solClient_returnCodeToString(rc));

	}

	logtofile(QCLIENT_LOG_INF, "INFO : Unsubscribed from topic: %s\n", _topic);
	
	return (K)0;

}

/** DISPLAY FUNCTIONS **/

K getcbstats(K x) {
 
	K keys = ktn(KS,4);
        K vals = ktn(KJ,4);

        kS(keys)[0] = ss("h");
        kS(keys)[1] = ss("t");
        kS(keys)[2] = ss("n");
        kS(keys)[3] = ss("size");

        kJ(vals)[0] = queue.h;
        kJ(vals)[1] = queue.t;
        kJ(vals)[2] = queue.n;
        kJ(vals)[3] = cb_size(&queue);

        K dict = xD(keys,vals);

        return dict;
}

K getstats (K x) {

        K sent,recv,batched; 
        
	K keys = ktn(KS,4),
	  vals = ktn(0,0);

        kS(keys)[0]=ss("recv");      // Messages recieved from the solace broker.
        kS(keys)[1]=ss("sent");      // Messages send by this API via calls to publishtosolace
	kS(keys)[2]=ss("batched");   // Number of times q main thread has called k(0,.....).
        kS(keys)[3]=ss("processed"); // Number of messages sent to q main thread	

        jk(&vals,ki(msg_recv));
	jk(&vals,ki(msg_sent));
	jk(&vals,ki(callbackcounter));
        jk(&vals,ki(msg_processed));

	return xD(keys,vals);

}


K gettopics (K x) {

	if( !initiated ) {
		return kdberr("noinit");
	}

        K subscribed = ktn(KI,topicindex.size);
        K topics = ktn(KS,topicindex.size);
        int ii;
	char buff[20];
	struct tm* tm_info;

	for(ii=0; ii<topicindex.size; ii++) {
         
		kS(topics)[ii] = ss(topicindex.topics[ii]);
                
		K sub;

		if(topicindex.unsubscribed[ii] != (time_t) NULL) {
			kI(subscribed)[ii] = 0;
		}else {
			kI(subscribed)[ii] = 1;
		}
	}

	K keys = ktn(KS,2); 
     	kS(keys)[0] = ss("topic");
     	kS(keys)[1] = ss("subscribed");

	K vals= ktn(0,2);
    	kK(vals)[0] = topics;
    	kK(vals)[1] = subscribed;

	return  xT(xD(keys,vals));
}

/* printargs
 * Prints current values in opts struct
 * */
K printargs (K x) {

	logtofile(QCLIENT_LOG_REQ, "INFO : --------- Argument Display -------------\n");
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                       - %s\n", "USER", opts.username);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                       - %s\n", "HOST", opts.hostname);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s          - %s\n", "CALLBACK FUNCTION", opts.callback);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                       - %d\n", "PORT", opts.port);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s           - %d\n", "SOLACE LOG LEVEL", opts.loglevel);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                        - %s\n", "VPN", opts.vpn);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s            - %d\n", "CONNECT RETRIES", opts.connectretries);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s           - %d\n", "MESSAGES TO SEND", opts.nummsgstosend);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s         - %s\n", "ENABLE COMPRESSION", opts.enablecompression ? "true" : "false");
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                    - %s\n", "USE GSS", opts.usegss ? "true" : "false");
	logtofile(QCLIENT_LOG_REQ, "INFO : %s               - %d\n", "WRITE HANDLE", opts.handle);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                 - %s\n", "TABLE NAME", opts.tablename);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s         - %d\n", "TABLE SCHEMA COUNT", (int) (kK(opts.schema)[0])->n);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s   - %s\n", "MESSAGE RECEIVE FUNCTION", opts.rcvfunc);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s                - %d\n", "Q LOG LEVEL", opts.loglvl);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s             - %s\n", "DUMP TO SCREEN", opts.dumptoscreen ? "true" : "false");
	logtofile(QCLIENT_LOG_REQ, "INFO : %s            - %s\n", "KDB Lib version", KDB_LIB_VERSION);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s         - %s\n", "Solace Lib version",version_p->version_p);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s         - %s\n", "Solace Lib version",version_p->dateTime_p);
	logtofile(QCLIENT_LOG_REQ, "INFO : %s         - %s\n", "Solace Lib version",version_p->variant_p);
	logtofile(QCLIENT_LOG_REQ, "INFO : ---------------------------------------\n");

	return (K)NULL;
}


K current_time() {

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	
	return ktj(-KP,(ts.tv_sec-10957*86400)*1000000000LL+ts.tv_nsec);

}

K getopts (K x) {
    //function to return opts
    if (!initiated) {
        return kdberr("noinit");
    }
    K y = ktn(0, 17);
    x = ktn(KS, 17);
    
    kS(x)[0] = ss("user");
    kK(y)[0] = ks(opts.username);
    kS(x)[1] = ss("password");
    kK(y)[1] = ks(opts.password);
    kS(x)[2] = ss("host");
    kK(y)[2] = ks(opts.hostname);
    kS(x)[3] = ss("port");
    kK(y)[3] = kj(opts.port);
    kS(x)[4] = ss("topic");
    kK(y)[4] = ks(opts.topic);
    kS(x)[5] = ss("vpn");
    kK(y)[5] = ks(opts.vpn);
    kS(x)[6] = ss("connectretries");
    kK(y)[6] = kj(opts.connectretries);
    kS(x)[7] = ss("msgstosend");
    kK(y)[7] = kj(opts.nummsgstosend);
    kS(x)[8] = ss("enablecompression");
    kK(y)[8] = kb(opts.enablecompression);
    kS(x)[9] = ss("usegss");
    kK(y)[9] = kb(opts.usegss);
    kS(x)[10]= ss("callback");
    kK(y)[10]= ks(opts.callback);
    kS(x)[11]= ss("tablename");
    kK(y)[11]= ks(opts.tablename);
    kS(x)[12]= ss("handle");
    kK(y)[12]= kj(opts.handle);
    kS(x)[13]= ss("rcvfunc");
    kK(y)[13]= ks(opts.rcvfunc);
    kS(x)[14]= ss("solaceloglevel");
    kK(y)[14]= kj(opts.loglevel);
    kS(x)[15]= ss("clientloglevel");
    kK(y)[15]= kj(opts.loglvl);
    kS(x)[16]= ss("dumptoscreen");
    kK(y)[16]= kb(opts.dumptoscreen);

    return xD(x,y);
}


K getsolacelib (K x) {

        K y = ktn( 0, 12 );
        x = ktn( KS, 12 );

        kS(x)[0] = ss("init");
        kK(y)[0] = dl( init, 1 );
        kS(x)[1] = ss("getstats");
        kK(y)[1] = dl( getstats, 1 );
        kS(x)[2] = ss("subscribe");
        kK(y)[2] = dl( subscribe, 1 );
        kS(x)[3] = ss("printargs");
        kK(y)[3] = dl( printargs, 1 );
        kS(x)[4] = ss("disconnect");
        kK(y)[4] = dl( sessiondisconnect, 1 );
        kS(x)[5] = ss("unsubscribe");
        kK(y)[5] = dl( unsubscribe, 1 );
        kS(x)[6] = ss("publish");
        kK(y)[6] = dl( publishtosolace, 2 );
        kS(x)[7] = ss("checkconnect");
        kK(y)[7] = dl( checkconnect, 1 );
        kS(x)[8] = ss("gettopics");
        kK(y)[8] = dl( gettopics, 1 );
        kS(x)[9] = ss("getcbstats");
        kK(y)[9] = dl( getcbstats, 1 );
        kS(x)[10] = ss("getopts");
        kK(y)[10] = dl( getopts, 1 );
	kS(x)[11] = ss("changeloglevel");
	kK(y)[11] = dl( changeloglevel, 1);

	return xD(x,y);
}

