#include "k.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include "solace_k.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


/* Context */
solClient_opaqueContext_pt             context_p;
solClient_context_createFuncInfo_t     contextFuncInfo = SOLCLIENT_CONTEXT_CREATEFUNC_INITIALIZER;

/* Session */
solClient_opaqueSession_pt             session_p;
solClient_session_createFuncInfo_t     sessionFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;
char *sessionProps[50]; 

/* Message */
solClient_destination_t                destination;

void configure_session_properties();

K decodeFromSolace(K,solClient_opaqueMsg_pt);

solClient_returnCode_t encodeKdbPrimitiveToSolaceContainer(K,solClient_opaqueContainer_pt);

solClient_returnCode_t encodeKdbDictionaryToSolaceContainer(K,solClient_opaqueContainer_pt);

solClient_returnCode_t encodeKdbVectorToSolaceContainer(K,solClient_opaqueContainer_pt);

solClient_returnCode_t encodeKdbVectorElementToSolaceContainer(K x,char* name,int index,solClient_opaqueContainer_pt container);

solClient_returnCode_t encodeKdbVectorAtomToSolaceContainer(K x,char* name,solClient_opaqueContainer_pt container);

solClient_returnCode_t encodeKdbAtomToSolaceContainer(K, char*,solClient_opaqueContainer_pt);

K decodeFromSolaceContainer(solClient_field_t, solClient_opaqueMsg_pt);

K make_msg_dict(K x, solClient_opaqueMsg_pt msg_p, solClient_session_eventCallbackInfo_pt eventInfo_p, solClient_errorInfo_pt errorInfo_p, char msg_type);

void write_to_buffer(K x);

/* cleanup
 * Clean up and return OK on success, error code on failure
 * */
solClient_returnCode_t cleanup () {

    	if ( ( rc = solClient_cleanup (  ) ) != SOLCLIENT_OK ) {
    
		logtofile ( QCLIENT_LOG_ERR, "ERROR : solClient_cleanup() CODE:%s\n",solClient_returnCodeToString(rc) );
        
	}

        return rc;
}

/*
 * logtofile
 * Logs given message
 */
void logtofile(qclient_log_level_t loglvl, char *str, ...) {

	if(loglvl > opts.loglvl) return;

    	char buf[4096];				// buffer for format string with args inserted
    	char tm[100];				// buffer for time string
    	time_t now = time(NULL);                // get current time
    	struct tm *timenow = localtime(&now);   // convert to local time
    
    	strftime(tm,100,"%D %T",timenow);       // format for log file
    	
	va_list args;				// list of arguments passed in
    	va_start(args, str);
    	vsnprintf(buf,sizeof(buf),str,args);    // insert additional args to format sting
    	va_end(args);

    	printf("[%s] %s",tm,buf);          	// output message with timestamp to log filSession Called
  	
	return;
}


/** CALLBACK FUNCTIONS **/

void common_eventCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_session_eventCallbackInfo_pt eventInfo_p, void *user_p ) {
    solClient_errorInfo_pt errorInfo_p;

    switch ( eventInfo_p->sessionEvent ) {
        case SOLCLIENT_SESSION_EVENT_UP_NOTICE:
	
            /* Session up notice log at INFO level as a simple message. */
            logtofile ( QCLIENT_LOG_INF, "INFO : %s\n",
                            solClient_session_eventToString ( eventInfo_p->sessionEvent ));
            break;
	
        case SOLCLIENT_SESSION_EVENT_ACKNOWLEDGEMENT:
        case SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_OK:
        case SOLCLIENT_SESSION_EVENT_CAN_SEND:
        case SOLCLIENT_SESSION_EVENT_RECONNECTING_NOTICE:
        case SOLCLIENT_SESSION_EVENT_RECONNECTED_NOTICE:
        case SOLCLIENT_SESSION_EVENT_PROVISION_OK:
        case SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_OK:

            /* Non-error events are logged at the INFO level. */
            logtofile ( QCLIENT_LOG_INF, "INFO :  %s\n",
                            solClient_session_eventToString ( eventInfo_p->sessionEvent ));
            break;

        case SOLCLIENT_SESSION_EVENT_DOWN_ERROR:
            
	    errorInfo_p = solClient_getLastErrorInfo (  );
            /* Error events are output to STDOUT. */
            logtofile (QCLIENT_LOG_WRN, "INFO :  %s; subCode %s, responseCode %d, reason %s\n",
                     solClient_session_eventToString ( eventInfo_p->sessionEvent ),
                     solClient_subCodeToString ( errorInfo_p->subCode ), errorInfo_p->responseCode, errorInfo_p->errorStr );
            initiated = false; 
	    break;

        case SOLCLIENT_SESSION_EVENT_CONNECT_FAILED_ERROR:
	case SOLCLIENT_SESSION_EVENT_REJECTED_MSG_ERROR:
        case SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_ERROR:
        case SOLCLIENT_SESSION_EVENT_RX_MSG_TOO_BIG_ERROR:
        case SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_ERROR:
        case SOLCLIENT_SESSION_EVENT_PROVISION_ERROR:

            /* Extra error information is available on error events */
            errorInfo_p = solClient_getLastErrorInfo (  );
            /* Error events are output to STDOUT. */
            logtofile (QCLIENT_LOG_INF, "INFO :  %s; subCode %s, responseCode %d, reason %s\n",
                     solClient_session_eventToString ( eventInfo_p->sessionEvent ),
                     solClient_subCodeToString ( errorInfo_p->subCode ), errorInfo_p->responseCode, errorInfo_p->errorStr );
            break;

        default:
            /* Unrecognized or deprecated events are output to STDOUT. */
            logtofile (QCLIENT_LOG_WRN, "INFO : %s.  Unrecognized or deprecated event.\n",
                     solClient_session_eventToString ( eventInfo_p->sessionEvent ) );
            break;
    }

    K kmsg_dict = make_msg_dict((K)NULL, NULL, eventInfo_p, errorInfo_p, 'a');
    // Serialize
    K kmsg_ser = b9(1,kmsg_dict);
    if(kmsg_ser != NULL) {
	// Write to buffer
	write_to_buffer(kmsg_ser);
    }
}

uint8_t msgbuff[1000000];

// write_to_buffer
// Write a serialized message to the buffer
// Notify the receiver that something has been written
// by adding a single character to one end of the socket pair
void write_to_buffer(K x) {

	long msize;
	char dot = '.';

  	// Start writing to buffer
    	msize = x->n;
    	memcpy(msgbuff, &msize, sizeof(long));
    	memcpy(msgbuff + sizeof(long), kG(x), msize);
        if( msize >= MAX_MSG_SIZE)
	{
         logtofile(QCLIENT_LOG_INF,"Message size too big on msg_recv %i. Message dropped. Please check configuration.\n", msg_recv);
	}else
	{	
	// Write message to queue
	cb_write(&queue, msgbuff, msize + sizeof(long), 1);
	
	// Tell other side that a message is being written
    	write(fd[0], &dot, 1);
        }
        r0(x);
    }

K make_msg_dict(K x, solClient_opaqueMsg_pt msg_p, solClient_session_eventCallbackInfo_pt eventInfo_p, solClient_errorInfo_pt errorInfo_p, char msg_type) {

	solClient_destination_t destination;
	solClient_int64_t sendTimestamp, rcvTimestamp;

	if(msg_type == 'a') {

		switch ( eventInfo_p->sessionEvent ) {
        
			case SOLCLIENT_SESSION_EVENT_DOWN_ERROR:
        		case SOLCLIENT_SESSION_EVENT_CONNECT_FAILED_ERROR:
			case SOLCLIENT_SESSION_EVENT_REJECTED_MSG_ERROR:
        		case SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_ERROR:
        		case SOLCLIENT_SESSION_EVENT_RX_MSG_TOO_BIG_ERROR:
        		case SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_ERROR:
        		case SOLCLIENT_SESSION_EVENT_PROVISION_ERROR:
				return knk(7, kc(msg_type), kj(msg_recv), ks((S)"-"), 
					current_time(), current_time(), current_time(), 
					ks(errorInfo_p->errorStr) );
            			break;

        		case SOLCLIENT_SESSION_EVENT_UP_NOTICE:
        		case SOLCLIENT_SESSION_EVENT_ACKNOWLEDGEMENT:
        		case SOLCLIENT_SESSION_EVENT_TE_UNSUBSCRIBE_OK:
        		case SOLCLIENT_SESSION_EVENT_CAN_SEND:
        		case SOLCLIENT_SESSION_EVENT_RECONNECTING_NOTICE:
        		case SOLCLIENT_SESSION_EVENT_RECONNECTED_NOTICE:
        		case SOLCLIENT_SESSION_EVENT_PROVISION_OK:
        		case SOLCLIENT_SESSION_EVENT_SUBSCRIPTION_OK:
        		default:
				return knk(7, kc(msg_type), kj(msg_recv), ks((S)"-"), 
					current_time(), current_time(), current_time(), 
					ks(solClient_session_eventToString(eventInfo_p->sessionEvent)));
            			break;

		}

	}else {

		// Get message destination
		if ( ( rc = solClient_msg_getDestination ( msg_p, &destination, sizeof ( destination ) ) ) != SOLCLIENT_OK ) {
			logtofile (QCLIENT_LOG_WRN, "WARNING : solClient_msg_getDestination() CODE:%s\n", 
							solClient_returnCodeToString(rc) );
    	        	destination.dest = "";
		}
		// Get time stamps
    		if ( ( rc = solClient_msg_getSenderTimestamp(msg_p, &sendTimestamp) ) != SOLCLIENT_OK ) {
			logtofile (QCLIENT_LOG_WRN, "WARNING : solClient_msg_getSenderTimestamp() CODE:%s\n",
				 			solClient_returnCodeToString(rc) );
	    		sendTimestamp = 0;
    		}
		// Get time stamps
    		if ( SOLCLIENT_OK != solClient_msg_getRcvTimestamp(msg_p, &rcvTimestamp) ) {
			logtofile (QCLIENT_LOG_WRN, "WARNING : solClient_msg_getRcvTimestamp() CODE:%s\n",
				 			solClient_returnCodeToString(rc) );
	    		rcvTimestamp = 0;
		}
		
		return knk(7, kc(msg_type), kj(msg_recv), ks((S) destination.dest), 
				kj(sendTimestamp), kj(rcvTimestamp), current_time(), 
				x);
	}

	return (K) NULL;

}

/* messageReceiveCallback
 * Called when a message is received 
 * */
solClient_rxMsgCallback_returnCode_t messageReceiveCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{

	K kload;


        // Decode into q type	
    	K kmsg = decodeFromSolace(kload, msg_p);
       
	if(kmsg == (K)NULL){

		logtofile(QCLIENT_LOG_ERR, "ERROR : Unable to read data\n");
		return SOLCLIENT_FAIL;

	}

	// Make into dictionary
	K kmsg_dict = make_msg_dict(kmsg, msg_p, NULL, NULL, 's');

	// Serialize
	K kmsg_ser = b9(1,kmsg_dict);
	if(kmsg_ser == NULL) {
	 	return SOLCLIENT_FAIL;
	}

	// Write to buffer
	write_to_buffer(kmsg_ser);

	// Update message stats    
    	msg_recv++;
    	
	return SOLCLIENT_OK;
}

/*
 * messageReceiveCallbackDump
 * Called when a message is received
 * Dumps message to screen
 * Used for testing purposes
 */
solClient_rxMsgCallback_returnCode_t messageReceiveCallbackDump ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p ) {

	if ( ( rc = solClient_msg_dump ( msg_p, NULL, 0 ) ) != SOLCLIENT_OK ) {
	    	logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_msg_dump() CODE:%s\n",solClient_returnCodeToString(rc) );
        	return rc;
    	}

	return SOLCLIENT_OK;

}

/*
 * setSolaceCallback
 * Set the callback used when a message is received
 * Value depends on opts.dumptoscreen
 */
void setSolaceCallback() {

    if(opts.dumptoscreen == true) {
    
    	    sessionFuncInfo.rxMsgInfo.callback_p = messageReceiveCallbackDump;

    }else {
    
    	    sessionFuncInfo.rxMsgInfo.callback_p = messageReceiveCallback;
    
    }

}

/** CONNECTION MANAGEMENT **/

/* connectToSolace 
 * Set up connection to solace
 * Return OK on success, or error code otherwise
 * */
solClient_returnCode_t connectToSolace(){

    initiated = false;

    /* Initialize Solace */
    if ( ( rc = solClient_initialize ( SOLCLIENT_LOG_DEFAULT_FILTER, NULL ) ) != SOLCLIENT_OK ) {
        
	    logtofile (QCLIENT_LOG_ERR, "\nERROR : solClient_initialize() CODE:%s\n",solClient_returnCodeToString(rc) );
	    return rc;

    }
   
    /* Set logging level */
    solClient_log_setFilterLevel ( SOLCLIENT_LOG_CATEGORY_ALL, opts.loglevel );
   
    /* Create context */ 
    if ( ( rc = solClient_context_create ( SOLCLIENT_CONTEXT_PROPS_DEFAULT_WITH_CREATE_THREAD,
                                           &context_p, &contextFuncInfo, sizeof ( contextFuncInfo ) ) ) != SOLCLIENT_OK ) {
    
	    logtofile (QCLIENT_LOG_ERR, "\nERROR : solClient_context_create() code: %s\n", solClient_returnCodeToString(rc) );
            cleanup();
	    return SOLCLIENT_FAIL;
    
    }

    /* Set up session */
    setSolaceCallback(); // Set up appropriate callback function, depending on value of opts.dumptoscreen

    sessionFuncInfo.rxMsgInfo.user_p = NULL;
    sessionFuncInfo.eventInfo.callback_p = common_eventCallback;
    sessionFuncInfo.eventInfo.user_p = NULL;
    
    configure_session_properties();
    
    /* Create session. */
    if ( ( rc = solClient_session_create ( ( char** ) sessionProps, context_p,
                                           &session_p, &sessionFuncInfo, sizeof ( sessionFuncInfo ) ) ) != SOLCLIENT_OK ) {
	    
	    logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_session_create() CODE:%s\n",solClient_returnCodeToString(rc) );
	    cleanup();
	    return SOLCLIENT_FAIL;
    
    }

    /* Connect the session. */
    if ( ( rc = solClient_session_connect ( session_p ) ) != SOLCLIENT_OK ) {
    
	    logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_session_connect() CODE:%s\n",solClient_returnCodeToString(rc) );
            cleanup();
	    return SOLCLIENT_FAIL;

    }

    return SOLCLIENT_OK;

}

/* configure_session_properties()
 * Set up sessionProps using values in opts
 * */
void configure_session_properties() {

    /* Session Properties */
    int      propIndex = 0;

    /* Configure the Session properties. */
    propIndex = 0;
    char tmpstr[10];
    char port[256];
    
    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_HOST;
    snprintf(port, sizeof(port),"%s:%i",opts.hostname,opts.port);
    sessionProps[propIndex++] = port;

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_USERNAME;
    sessionProps[propIndex++] = opts.username;

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_PASSWORD;
    sessionProps[propIndex++] = opts.password;

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_VPN_NAME;
    sessionProps[propIndex++] = opts.vpn;

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_RECONNECT_RETRIES;
    snprintf(tmpstr, sizeof(opts.connectretries),"%d",opts.connectretries);
    sessionProps[propIndex++] = tmpstr;

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_COMPRESSION_LEVEL;
    sessionProps[propIndex++] = ( opts.enablecompression ) ? "9" : "0";

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_REAPPLY_SUBSCRIPTIONS;
    sessionProps[propIndex++] = SOLCLIENT_PROP_ENABLE_VAL;

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_SSL_VALIDATE_CERTIFICATE;
    sessionProps[propIndex++] = SOLCLIENT_PROP_DISABLE_VAL;

    if ( opts.usegss ) {
        sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_AUTHENTICATION_SCHEME;
        sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_AUTHENTICATION_SCHEME_GSS_KRB;
    }
    
    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_GENERATE_SEND_TIMESTAMPS;
    sessionProps[propIndex++] = SOLCLIENT_PROP_ENABLE_VAL;
    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_GENERATE_RCV_TIMESTAMPS;
    sessionProps[propIndex++] = SOLCLIENT_PROP_ENABLE_VAL;
    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_SEND_BLOCKING;
    sessionProps[propIndex++] = SOLCLIENT_PROP_DISABLE_VAL;

    sessionProps[propIndex] = NULL;
    
}

/* disconnectFromSolace
 * Disconnect the connection to Solace
 * Returns OK on success, error code otherwise
 * */
solClient_returnCode_t disconnectFromSolace() {


	if( ( rc = solClient_session_disconnect ( session_p ) ) != SOLCLIENT_OK ) {

		logtofile (QCLIENT_LOG_ERR, "\nERROR : solClient_session_disconnect() CODE: %s\n",
				solClient_returnCodeToString(rc) );

	}

	return rc;

}

/** SUBSCRIBE AND UNSUBSCRIBE TO/FROM TOPICS **/

/* subscribeToTopic
 * Subscribe to the topic contained in opts.topic
 * */
solClient_returnCode_t subscribeToTopic(char * topic) {
	if ( ( rc = solClient_session_topicSubscribeExt ( session_p,
                                                        SOLCLIENT_SUBSCRIBE_FLAGS_WAITFORCONFIRM,
                                                        topic ) ) != SOLCLIENT_OK ) {
		logtofile (QCLIENT_LOG_ERR, "\nERROR : solClient_session_topicSubscribe() CODE:%s\n",solClient_returnCodeToString(rc) );
        }

        return rc;
}

/* unsubscribeFromTopic
 * Unsubscribe from the topic contained in opts.topic
 * */
solClient_returnCode_t unsubscribeFromTopic(char * topic) {
	if ( ( rc = solClient_session_topicUnsubscribeExt ( session_p,
                                                        SOLCLIENT_SUBSCRIBE_FLAGS_WAITFORCONFIRM,
                                                        topic ) ) != SOLCLIENT_OK ) {
		logtofile (QCLIENT_LOG_ERR, "\nERROR : solClient_session_topicUnsubscribe() CODE:%s\n",solClient_returnCodeToString(rc) );
        }

        return rc;
}


/** SEND AND RECEIVE MESSAGES **/


K decodeFromSolace(K x,solClient_opaqueMsg_pt msg_p) {
	// Function takes a k object and translates this to a a solace container.
    	// We should only take items which are lists or vectors initially.
    	// No lists of lists for now.
      	K kobj = NULL;	    
	K ans;

    	solClient_field_t  field;
     	solClient_msg_getBinaryAttachmentField ( msg_p, &field, sizeof(solClient_field_t));
       	
     	ans = decodeFromSolaceContainer(field, msg_p);

     	return ans;

}

K  decodeFromSolaceMap(solClient_opaqueContainer_pt container) {

	int i = 0;
       	char* name;
	K myobj = NULL;
 	K keys = ktn(KS,0);
        K vals = ktn(0,0);
	solClient_field_t field_p;
	while(solClient_container_hasNextField ( container )) {

        	solClient_container_getNextField ( container, &field_p, sizeof(solClient_field_t), &name);
                js( &keys, ss(name) );
                myobj = decodeFromSolaceContainer(field_p, NULL);
	
		jk( &vals, myobj );
        
		i = i+1;

	}

 	return xD(keys,vals);

}

K  decodeFromSolaceStream(solClient_opaqueContainer_pt container) {
	
	int i = 0;
      	char* name;
      	K myobj = ktn(0,0), myelement;
      	solClient_field_t field_p;
      
	while(solClient_container_hasNextField ( container)) {

		solClient_container_getNextField ( container,&field_p,sizeof(solClient_field_t),&name);		      
		myelement= decodeFromSolaceContainer( field_p, NULL);
		    
		jk(&myobj,myelement);
		    
		i = i+1;

	}

	return myobj;

}

K  decodeFromSolaceContainer(solClient_field_t  field, solClient_opaqueMsg_pt msg_p) {

	void *binaryAttachmentBuffer_p;
	solClient_uint32_t binaryAttachmentBufferSize;
       	size_t  fieldSize=24;
       	const char *  name;
       	K myobj=(K)0;       
         
       	solClient_fieldType_t datatype=field.type;
      
       	switch(datatype) {

		case SOLCLIENT_BOOL:
	       		myobj=kb(field.value.boolean);
	       		break;
		case SOLCLIENT_UINT8:
	       		myobj=ki((int)field.value.uint8);
	       		break;
		case SOLCLIENT_INT8:
	       		myobj=kh((int)field.value.int8);
	       		break;
		case SOLCLIENT_UINT16:
	       		myobj=ki((int)field.value.uint16);
               		break;
        	case SOLCLIENT_INT16:
	       		myobj=ki((int)field.value.int16);
               		break;
        	case SOLCLIENT_UINT32:
	       		myobj=kj((int)field.value.uint32);
               		break;
        	case SOLCLIENT_INT32:
	       		myobj=ki((int)field.value.int32);
               		break;
        	case SOLCLIENT_UINT64:
	       		myobj=kf((float)field.value.uint64);
               		break;
        	case SOLCLIENT_INT64:
	       		myobj=kj(field.value.uint64);
               		break;
        	case SOLCLIENT_WCHAR:
              		myobj=kc(field.value.wchar); 
	      		break;
        	case SOLCLIENT_STRING:
              		myobj=kp(field.value.string);
               		break;
        	case SOLCLIENT_BYTEARRAY:
                	myobj=(K)0;
	       		break;
        	case SOLCLIENT_FLOAT:
	       		myobj=ke((float)field.value.float32);
               		break;
        	case SOLCLIENT_DOUBLE:
	       		myobj=kf((float)field.value.float64);
               		break;
        	case SOLCLIENT_MAP:
	       		myobj=decodeFromSolaceMap(field.value.map);
               		break;
        	case SOLCLIENT_STREAM:
                	myobj=decodeFromSolaceStream(field.value.stream);
	       		break;
        	case SOLCLIENT_UNKNOWN:
               		logtofile (QCLIENT_LOG_ERR," UNKNOWN obtained\n" );
	        	myobj=kp("UNKNOWN_DATA_TYPE");
               		break;
        	case SOLCLIENT_NULL:
        	case SOLCLIENT_DESTINATION:
        	case SOLCLIENT_SMF:
		default:
			if(msg_p) {
				// Attempt to get a pointer to the binary attachment of the message		
				if ((rc = solClient_msg_getBinaryAttachmentPtr( msg_p, &binaryAttachmentBuffer_p, &binaryAttachmentBufferSize)) != SOLCLIENT_OK) {
					logtofile(QCLIENT_LOG_ERR, "ERROR : Unknown message type - unable to retrieve message\n");
					return (K)NULL;
    				}
				myobj = ks((char *) binaryAttachmentBuffer_p);
			}else {
				return (K)NULL;
			}
			break;
       	}
       
       	return myobj;
}


solClient_returnCode_t encodeToSolaceContainer(K x,solClient_opaqueContainer_pt container) {
    //Function takes a k object and translates this to a a solace container. 
    //We should only take items whicha re lists or vectors initially. 
    solClient_returnCode_t rc=SOLCLIENT_FAIL;
    int elementcount=-0,typenumber=0;
    typenumber=x->t;
    char map[1024];
    solClient_opaqueContainer_pt newcontainer = NULL;
     // Special handling for dictionaries as I want them to gett hrough.
     

    if(typenumber==99)
    {
    rc = solClient_container_createMap( &newcontainer, map, sizeof ( map ) );
     rc+=encodeKdbPrimitiveToSolaceContainer( x,newcontainer);
      rc+=solClient_container_addContainer(container,newcontainer,NULL);
     return rc; 
    }
    
    
      if(abs(typenumber)>0)
     {
     //In the end here I have to make the assumtion that everything is alist. Ambiguity between lists and atoms on decoding side.
       rc=      encodeKdbPrimitiveToSolaceContainer( x,container);
      return rc;
     }
     else
     {
     int length=x->n;
     int i=0;
     for(i=0;i<length;i++)
     {K myobj=kK(x)[i];
       if((myobj->t)<0)
       {
        rc=encodeKdbAtomToSolaceContainer( kK(x)[i],NULL,container); 
       }
       if((myobj->t)>0)
       { 
          if(((myobj)->t)==99)
	  {
	  rc = solClient_container_createMap( &newcontainer, map, sizeof ( map ) );
	  } else
	  {
	   rc = solClient_container_createStream( &newcontainer, map, sizeof ( map ) );
          }
      	  rc= encodeKdbVectorToSolaceContainer(kK(x)[i],newcontainer);
        rc +=  solClient_container_addContainer(container,newcontainer,NULL);
       }
        
       
       if((myobj->t)==0)
       {
          solClient_opaqueContainer_pt newcontainer = NULL;
	  rc += solClient_container_createStream ( &newcontainer, map, sizeof ( map ) );
          rc+= encodeToSolaceContainer( kK(x)[i], newcontainer);
	  rc=solClient_container_addContainer(container,newcontainer,NULL);
	}
     }
     }

      return rc;  
     }   
   

 solClient_returnCode_t encodeKdbAtomToSolaceContainer(K x, char* name,solClient_opaqueContainer_pt container)     
 {        int typenumber= x->t; 
         solClient_returnCode_t rc=SOLCLIENT_FAIL;
	 switch(typenumber)
      {       
	     case -11:
                  rc=solClient_container_addString(container,  (char*)x->s,name);
                     break;
	     case -10:
                 rc=solClient_container_addChar(container,  (char)x->g,name);
                     break;
	     case -9:
	      rc=solClient_container_addDouble(container, x->f,name);
	    	     break;
             case -8:
	     case -15:
		  rc=   solClient_container_addFloat(container, x->e,name);
	             break;
	     case -7:
	     case -12:
	     case -16:	     
               rc=solClient_container_addInt64(container, x->j,name);	
       		     break;
             case -6:
             case -13:
             case -14:
             case -17:
             case -18:
             case -19:

               rc=solClient_container_addInt32(container, x->i,name);  
		     break;
             case -5:
		  rc=   solClient_container_addInt16(container, x->h,name);
                      break;
	     case -4:
                      break;

		      case -3:
                      break;
		      case -2:
                      break;
		      case -1:
                     rc= solClient_container_addBoolean(container, x->g ,name);                     
                       break;
	      default :
                  
              break;
     }
     return rc;
    }

 solClient_returnCode_t encodeKdbDictionaryToSolaceContainer(K x,solClient_opaqueContainer_pt container) 
{      
        K keys = kK(x)[0];
        K values = kK(x)[1];
	int i,length=keys->n,valuetype=values->t;
        char map[1024];     
	if(KS!=keys->t)
	{       //keys for now must be symbols.
		return SOLCLIENT_FAIL;
	}
	//solClient_opaqueContainer_pt newcontainer = NULL;
        //solClient_returnCode_t  rc = solClient_container_createMap ( &newcontainer, map, sizeof ( map ) );        
	if(rc==SOLCLIENT_FAIL){
         return rc;
	}
        for(i=0;i<length;i++){
   if(valuetype==0)
   {
	   K obf=kK(values)[i];
      rc=	encodeKdbAtomToSolaceContainer( (K)obf,(char *)kS(keys)[i], container);       
   } else if(valuetype>0)
  {      
         rc=encodeKdbVectorElementToSolaceContainer(values ,(char *)kS(keys)[i], i,container);
  }else
  {

  }
   }
	return rc;

}

solClient_returnCode_t encodeKdbVectorElementToSolaceContainer(K x,char* name,int index,solClient_opaqueContainer_pt container)
{
	solClient_returnCode_t rc=SOLCLIENT_FAIL; 
	int typenumber=x->t;    
         switch(typenumber)
      {
	      case 11:
                  rc=solClient_container_addString(container,  (char*)kS(x)[index],name);
                     break;

             case 10:
                     rc=solClient_container_addChar(container, (char)kC(x)[index],name);
                     break;
             case 9:
              rc=solClient_container_addDouble(container, kF(x)[index],name);
                     break;
             case 8:
             case 15:
                 rc=    solClient_container_addFloat(container, kE(x)[index],name);
                     break;
             case 7:
	     case 16:
	     case 12:
	 	    rc= solClient_container_addInt64(container, kJ(x)[index],name);
                     break;
             case 6:
             case 13:
             case 14:
             case 17:
             case 18:
             case 19:
               rc=solClient_container_addInt32(container, kI(x)[index],name);
                     break;
             case 5:
                  rc=   solClient_container_addInt16(container, kH(x)[index],name);
                      break;
             case 4:
                      break;

                case 3:
                      break;
                case 2:
                      break;
                      case 1:
                     rc= solClient_container_addBoolean(container, kG(x)[index] ,name);
                       break;
              default :
              		break;
     
	}

	return rc;

}

solClient_returnCode_t encodeKdbVectorToSolaceContainer(K x,solClient_opaqueContainer_pt container) {

        int typenumber= x->t;
        int length=x->n;
        int i=0;
        char            map[1024];

  if(typenumber==99)
  {
   return encodeKdbDictionaryToSolaceContainer(x, container);
  }

  if(typenumber==98)
  {
   //Tables not supported for now. [ Lazyness. Easily eimplementable. ]	  
   return SOLCLIENT_FAIL;
  }

   for(i=0;i<length;i++)
   {
     if(SOLCLIENT_FAIL==encodeKdbVectorElementToSolaceContainer(x,NULL,i, container))
     {
	     return SOLCLIENT_FAIL;
     };
     	}

      	return  SOLCLIENT_OK;
}

solClient_returnCode_t encodeKdbPrimitiveToSolaceContainer(K x,solClient_opaqueContainer_pt container) {

        int typenumber= x->t;
	solClient_returnCode_t rc=SOLCLIENT_FAIL;

   	if(typenumber>0) {
  
             rc=encodeKdbVectorToSolaceContainer(x,container);
   }else
   {
	   
              rc=encodeKdbAtomToSolaceContainer(x,NULL,container);
   }
   return rc;



}

/* publishToSolace
 * Publish a message to solace
 * msg - Message to be sent
 * dest - Topic to publish to
 * */
solClient_returnCode_t publishToSolace (char* msg_destination, K message) {

	solClient_returnCode_t rc_temp;
        rc = SOLCLIENT_OK;

	solClient_opaqueMsg_pt msg_p = NULL;
	solClient_opaqueContainer_pt streamContainer = NULL;	


	// Allocate memory for the message that is to be sent
	if ( ( rc = solClient_msg_alloc ( &msg_p ) ) != SOLCLIENT_OK ) {
	
		logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_msg_allow() CODE:%s\n", solClient_returnCodeToString(rc) );
	        return rc;
	
	}

	// Set message delivery mode
	if ( ( rc = solClient_msg_setDeliveryMode ( msg_p, SOLCLIENT_DELIVERY_MODE_DIRECT ) ) != SOLCLIENT_OK ) {
	
		logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_msg_setDeliveryMode() CODE:%s\n", solClient_returnCodeToString(rc) );
		goto freemessage;
	
	}
	
	// Set the destination
	destination.destType = SOLCLIENT_TOPIC_DESTINATION;
	destination.dest = msg_destination;
	if ( ( rc = solClient_msg_setDestination ( msg_p, &destination, sizeof (destination ) ) ) != SOLCLIENT_OK ) {

		logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_msg_setDestination() CODE:%s\n", solClient_returnCodeToString(rc) );
                goto freemessage;

	}

        if ( ( rc = solClient_msg_createBinaryAttachmentStream ( msg_p, &streamContainer, 1024 ) ) != SOLCLIENT_OK ) {
        logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_msg_createBinaryAttachmentStream():%s\n", solClient_returnCodeToString(rc) );
        goto freemessage;
    }

       // Encode the K objected into a solace container
       if(  ( rc= encodeToSolaceContainer(message, streamContainer))!=SOLCLIENT_OK)
		       {
		logtofile(QCLIENT_LOG_ERR, "Error : Could not encode message\n");
                        goto freemessage;

		       }
	// Send the message
	if ( ( rc = solClient_session_sendMsg ( session_p, msg_p ) ) != SOLCLIENT_OK ) {
	
		logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_session_sendMsg() CODE:%s\n", solClient_returnCodeToString(rc) );
		goto freemessage;
	
	}

        freemessage:
		if ( ( rc_temp = solClient_msg_free ( &msg_p ) ) != SOLCLIENT_OK ) {
	
			logtofile (QCLIENT_LOG_ERR, "ERROR : solClient_msg_free() CODE:%s\n", solClient_returnCodeToString(rc_temp) );
	          	return rc_temp;
	  	}

	// Update message stats

	return rc;

}

/*
K getopts (K x) {
    //CURRENTLY BROKEN DO NOT USE SEGFAULT DUE TO WONKINESS WITH SCHEMA
    //function to return opts
    if (!initiated){
        return kdberr("noinit");
    }
    K y = ktn(0,15);
    x = ktn(KS,15);
    K tmp=xD(kK(opts.schema)[0],kK(opts.schema)[1]);
    //printdict(tmp);
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
    kS(x)[10]= ss("schema");
    kK(y)[10]= ks("opts.schema"); //to break replace with = tmp
    kS(x)[11]= ss("tablename");
    kK(y)[11]= ks(opts.tablename);
    kS(x)[12]= ss("handle");
    kK(y)[12]= kj(opts.handle);
    kS(x)[13]= ss("rcvfunc");
    kK(y)[13]= ks(opts.rcvfunc);
    kS(x)[14]= ss("loglevel");
    kK(y)[14]= kj(opts.loglevel);
    //opts.schema=tmp;
    //printdict(opts.schema);
    return xD(x,y);
}*/
