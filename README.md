# TorQ-Solace

## Solace

Solace provides a broker that handles messages sent between processes. This application is comparable to a standard kdb+ tickerplant, that publishes messages to real-time subscribers filtered by topic. The code in this repo allows kdb+ to communicate with any solace messaging framework. The API provides a translation from most solace datatypes to kdb+ and vice versa.

## Set up Instructions

### Install Solace Broker

Information on how to set up and install a solace broker can be found [here](https://docs.solace.com/Solace-SW-Broker-Set-Up/Docker-Containers/Set-Up-Single-Linux-Container.htm)

Once installed, run the below command to find the ip of the broker.
```
@homer:~$ ps -aef | grep docker
root      1068 32392  0 Apr03 ?        00:00:00 /usr/bin/docker-proxy -proto tcp -host-ip 0.0.0.0 -host-port 55559 -container-ip 172.17.0.6 -container-port 55555
```
Check that the container is running correctly
```
solace@homer:~$ docker container ls
CONTAINER ID        IMAGE                           COMMAND               CREATED             STATUS              PORTS                                                        NAMES
415cbaf393c3        solace/solace-pubsub-standard   "/usr/sbin/boot.sh"   10 days ago         Up 7 days           2222/tcp, 0.0.0.0:8084->8080/tcp, 0.0.0.0:55559->55555/tcp   solace_test
```
### Install Solace Kdb+ Client

Once the container is running correctly, clone the TorQ-Solace repository
```
solace@homer:~$ git clone https://github.com/AquaQAnalytics/TorQ-Solace
Cloning into 'TorQ-Solace'...
Username for 'https://github.com': user
Password for 'https://user@github.com':
remote: Enumerating objects: 30, done.
remote: Counting objects: 100% (30/30), done.
remote: Compressing objects: 100% (22/22), done.
remote: Total 833 (delta 14), reused 21 (delta 8), pack-reused 803
Receiving objects: 100% (833/833), 12.42 MiB | 7.09 MiB/s, done.
Resolving deltas: 100% (502/502), done.
```
The library should have the following structure
```
solace@homer:~$ ls
TorQ-Solace
solace@homer:~$ cd TorQ-Solace/
solace@homer:~/TorQ-Solace$ ls
include  LICENSE  Makefile  obj_Linux26-x86_64_debug  q  README.md  solclient  src  tests
solace@homer:~/TorQ-Solace$ ls -la
total 76
drwxrwxr-x 9 solace solace  4096 Apr 17 14:37 .
drwxrwxr-x 5 solace solace  4096 Apr 17 14:37 ..
drwxrwxr-x 8 solace solace  4096 Apr 17 11:52 .git
-rw-rw-r-- 1 solace solace    50 Apr  6 16:33 .gitignore
drwxrwxr-x 3 solace solace  4096 Apr 15 14:18 include
-rw-rw-r-- 1 solace solace  1072 Apr  6 12:39 LICENSE
-rw-rw-r-- 1 solace solace   714 Apr 17 11:52 Makefile
drwxrwxr-x 6 solace solace  4096 Apr  6 12:39 solapi
drwxrwxr-x 2 solace solace  4096 Apr 15 14:20 q
-rw-rw-r-- 1 solace solace 23597 Apr 16 11:28 README.md
drwxrwxr-x 2 solace solace  4096 Apr 17 11:52 src
drwxrwxr-x 4 solace solace  4096 Apr  9 12:20 tests
```
Copy the contents of the unzipped solace C lib package (https://solace.com/downloads/) to the solapi directory.
```
solace@homer:~/TorQ-Solace/$ ls -al solapi/
total 28
drwxrwxr-x  6 solace solace 4096 May 12 22:47 .
drwxrwxr-x 12 solace solace 4096 May 12 22:44 ..
drwxrwxr-x  2 solace solace 4096 May 12 22:25 ex
drwxrwxr-x  3 solace solace 4096 May 12 22:25 include
-rw-rw-r--  1 solace solace  186 May 12 22:47 info.txt
drwxrwxr-x  2 solace solace 4096 May 12 22:25 Intro
drwxrwxr-x  3 solace solace 4096 May 12 22:40 lib
```

Inside the main directory, run the `make` command to build the library files needed for the application to run.
```
solace@homer:~/TorQ-Solace$ make
if ! test -d lib;then mkdir lib/;fi
rm -f        lib/solace.*
gcc  -Lobj_Linux26-x86_64_debug/lib  -shared -DKXVER=3 -fPIC  -D_LINUX_X86_64 -lpthread -lsolclient -Iinclude -DPROVIDE_LOG_UTILITIES  src/solace.c src/solace_k.c src/cb.c -o lib/solace.so -lsolclient -lrt -Wno-discarded-qualifiers -Wno-incompatible-pointer-types
```
Finally set some environment variables to allow kdb+ to find the relevant shared objects.
```
solace@homer:~/TorQ-Solace$ source setenv.sh
```
Running sample programs from the `ex` directory can be used to test that the broker is running. Run the `make` command to build the object files and executable files.
```
solace@homer:~/TorQ-Solace/solapi/ex$ make
gcc -g  -I.. -I../include  -DSOLCLIENT_EXCLUDE_DEPRECATED -DPROVIDE_LOG_UTILITIES -DSOLCLIENT_CONST_PROPERTIES -g   -c directPubSub.c -o directPubSub.o
gcc -g  -I.. -I../include  -DSOLCLIENT_EXCLUDE_DEPRECATED -DPROVIDE_LOG_UTILITIES -DSOLCLIENT_CONST_PROPERTIES -g   -c common.c -o common.o

...

gcc -g  -I.. -I../include  -DSOLCLIENT_EXCLUDE_DEPRECATED -DPROVIDE_LOG_UTILITIES -DSOLCLIENT_CONST_PROPERTIES -g   -c transactions.c -o transactions.o
gcc -g -o transactions transactions.o common.o os.o -L../lib -L../obj_Linux26-x86_64_debug/lib -lsolclient  -lrt -pthread
```

Usage statements for each executable can be found by running 
```
solace@homer:~/TorQ-Solace/solapi/ex$ ./executablefile
```
For example,
```
solace@homer:~/TorQ-Solace/solapi/ex$ ./directPubSub

directPubSub.c (Copyright 2009-2019 Solace Corporation. All rights reserved.)
Missing required parameter '--cu'

Usage: ./directPubSub PARAMETERS [OPTIONS]

Where PARAMETERS are:
        -u, --cu=user[@vpn] Client username and Mesage VPN name. The VPN name is optional and
                              only used in a Solace messaging appliance running SolOS-TR.
Where OPTIONS are:
        -c, --cip=[Protocol:]Host[:Port] Protocol, host and port of the messaging appliance (e.g. --cip=tcp:192.168.160.101).
        -t, --topic=Topic   Topic or Destination String.
        -p, --cp=password   Client password.
        -n, --mn            Number of Messages.
        -l, --log=loglevel  API and application logging level (debug, info, notice, warn, error, critical).
        -g, --gss           Use GSS (Kerberos) authentication. When specified the '--cu' option is ignored.
        -z, --zip           Enable compression (set compress level=9 for SolOS-TR appliances only).
```
You can execute this sample program by specifying the IP address of the broker
```
solace@homer:~/TorQ-Solace/solapi/ex$ ./directPubSub -u admin -c tcp:172.17.0.6

directPubSub.c (Copyright 2009-2019 Solace Corporation. All rights reserved.)
CCSMP Version 7.11.0.8 (Aug  7 2019 16:39:16)   Variant: Linux26-x86_64_opt - C SDK

Received message:
Destination:                            Topic 'my/sample/topic'
SenderTimestamp:                        1587390692877 (Mon Apr 20 2020 14:51:32)
RcvTimestamp:                           1587390692878 (Mon Apr 20 2020 14:51:32)
Class Of Service:                       COS_1
DeliveryMode:                           DIRECT
Binary Attachment:                      len=16
  6d 79 20 61 74 74 61 63  68 65 64 20 64 61 74 61      my attac   hed data
```
Provided it is run on the same machine as the Solace broker, a connection to the broker can be initialised by running the `test_conf.q` script (otherwise the host will have to be changed)
```
solace@homer:~/TorQ-Solace$ q q/test_conf.q
KDB+ 3.5 2017.11.30 Copyright (C) 1993-2017 Kx Systems

[04/17/20 14:09:03] INFO : Initializing session...
[04/17/20 14:09:03] INFO : Session initialization complete
[04/17/20 14:09:03] INFO : --------- Argument Display -------------
[04/17/20 14:09:03] INFO : USER                       - admin
[04/17/20 14:09:03] INFO : HOST                       - 127.0.0.1
[04/17/20 14:09:03] INFO : CALLBACK FUNCTION          - f
[04/17/20 14:09:03] INFO : PORT                       - 55555
[04/17/20 14:09:03] INFO : SOLACE LOG LEVEL           - 4
[04/17/20 14:09:03] INFO : VPN                        - default
[04/17/20 14:09:03] INFO : CONNECT RETRIES            - 3
[04/17/20 14:09:03] INFO : MESSAGES TO SEND           - 10
[04/17/20 14:09:03] INFO : ENABLE COMPRESSION         - false
[04/17/20 14:09:03] INFO : USE GSS                    - false
[04/17/20 14:09:03] INFO : WRITE HANDLE               - 0
[04/17/20 14:09:03] INFO : TABLE NAME                 - table
[04/17/20 14:09:03] INFO : TABLE SCHEMA COUNT         - 0
[04/17/20 14:09:03] INFO : MESSAGE RECEIVE FUNCTION   - insert
[04/17/20 14:09:03] INFO : Q LOG LEVEL                - 4
[04/17/20 14:09:03] INFO : DUMP TO SCREEN             - false
[04/17/20 14:09:03] INFO : KDB Lib version            - 1.0.0.1
[04/17/20 14:09:03] INFO : Solace Lib version         - 7.11.0.8
[04/17/20 14:09:03] INFO : Solace date                - Aug  7 2019 16:39:16
[04/17/20 14:09:03] INFO : Solace Lib                 - Linux26-x86_64_opt - C SDK
[04/17/20 14:09:03] INFO : ---------------------------------------
[04/17/20 14:09:03] INFO : Connecting to Solace...
[04/17/20 14:09:03] INFO : Session up
[04/17/20 14:09:03] INFO : Connection to Solace complete
```
 The functions located in the `.solace` namespace can then be used to send and receive messages between processes.

Solace Lib API Function | Description | Arguments | Example usage
------------------------|-------------|-----------|--------------
init | Initialises a connection to the broker | Dictionary `o` of inputs (see Table 2) | ```.solace.init[d:(`user`password`host`port`loglevel)!(`admin;`admin;`$"192.168.1.48";55555;2)]```
getstats | Returns a dictionary of number of messages received, sent, batched and processed | Niladic | `.solace.getstats[]`
subscribe | Subscribes to a topic | Topic name | `.solace.subscribe["Default"]`
printargs | Prints all library settings and current values | Niladic | `.solace.printargs[]`
disconnect | Disconnects from broker | Niladic | `.solace.disconnect[]`
unsubscribe | Unsubscribes from a topic | Topic name | `.solace.unsubscibe["Default"]`
publish | Publishes a message for a particular topic | \[topic name; message\] | `.solace.publish["Default"; 1 2 3]`
checkconnect | Returns a boolean based on connection status | Niladic | `.solace.checkconnect[]`
gettopics | Returns a table of topics and subscription status booleans | Niladic | `.solace.gettopics[]`
getcbstats | Returns a metric relating to the flow of messages between the feed threads | Niladic | `.solace.getcbstats[]`
getopts | Returns a dictionary of the broker's arguments and their values | Niladic | `.solace.getopts[]`
changeloglevel | Changes the q log level to a specified level | Integer between 0 and 4 (see loglevel in Table 2) | `.solace.changeloglevel[2]`
<p align="center"> <strong> Table 1 </strong> </p>


When calling .solace.init, the inputs are passed in as a dictionary. The various inputs are explained in Table 2.
  

Input | Description | Default value
------|-------------|--------------
user | Maps to [SOLCLIENT_SESSION_PROP_USERNAME](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#gae4dd9e2e99fcfc59c4a4036ec946fde6) | admin
password | Maps to [SOLCLIENT_SESSION_PROP_PASSWORD](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#gaedfa8ed47e09fd05b80a040ae9620455) | admin
host | Maps to [SOLCLIENT_SESSION_PROP_HOST](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#ga4d10162a3615679d39e5ee8565155393) | 192.168.1.48
port | Maps to [SOLCLIENT_SESSION_PROP_HOST](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#ga4d10162a3615679d39e5ee8565155393) | 55555
callback | The message handler for events received from the Solace library | f
solaceloglevel | Maps to one of 8 [levels](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#a00cbbbb086c6f55edef77db1b0d800ce) | 4
vpn | Maps to [SOLCLIENT_SESSION_PROP_VPN_NAME](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#gab465acb2d0e99ee3e78d7d4ab7b93e62) | default
connectretries | Maps to [SOLCLIENT_SESSION_PROP_CONNECT_RETRIES](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#ga690506f8e550535c960f35bdd4d1dbe5) | 3
enablecompression | Maps to [SOLCLIENT_SESSION_PROP_COMPRESSION_LEVEL](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/group___session_props.html#gab78b2c0a97386d1d4c85c7be6a0c1299) | false
usegss | If true, uses [gss authentication scheme](https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/sol_client_8h.html#af687c82c49bd38011ae45bceb48ba869) | false
loglevel | Maps to one of 4 levels, see Table 3 | 4
dumptoscreen | If true, calls a function messageReceiveCallbackDump which dumps the message to screen instead of sending to buffer. Useful for doing an initial check of the connection | false

<p align="center"> <strong> Table 2 </strong> </p>

loglevel | Enumerator | Description
---------|------------|------------
0 | QCLIENT_LOG_NON | No logging
1 | QCLIENT_LOG_ERR | Errors only
2 | QCLIENT_LOG_WRN | Warnings and errors
3 | QCLIENT_LOG_INF | Information on progress, warnings and errors
4 | QCLIENT_LOG_ALL | All log messages

<p align="center"> <strong> Table 3 </strong> </p>

To subscribe to the topic "Default", run
```
q).solace.subscribe["Default"]
[04/14/20 13:03:37] INFO : Subscribing to topic...
[04/14/20 13:03:37] INFO : Subscribed to topic: Default
```
In a separate process, to publish a message on the topic "Default", for example the list of numbers 1,2 and 3, run 
```
q).solace.publish["Default";1 2 3]
[04/14/20 13:06:20] INFO : Publishing to Solace...
[04/14/20 13:06:20] INFO : Publishing complete
```
The latest message that has been published is recorded and stored in `.l.l`, which can be viewed on the subscriber process
```
q).l.l
type seqn dest    sndt          rect          swbt                          msg   erbt
---------------------------------------------------------------------------------------------------------------
s    0    Default 1587379630040 1587379630040 2020.04.20D10:47:10.040752114 1 2 3 2020.04.20D10:47:10.040805302

```
### Run tests

In order to run tests, we run the `solacetest.q` file inside the tests directory. Adding the flag `usage` in the command line will explain what optional arguments it takes.
```
solace@homer:~/TorQ-Solace/tests$ q solacetest.q -usage
KDB+ 3.5 2017.11.30 Copyright (C) 1993-2017 Kx Systems

Usage: q solacetest.q [OPTIONS]

Where OPTIONS are:

     -testsrc,    Runs all tests when set to csv. To run an individual test, set to csv/csvname.csv (Default: csv)
     -bport,      Client processes will run on ports bport+1, bport+2. (Default: 9080)
     -noexit,     Stays in q session after tests have run. (Default: 1b)
     -noload,     Loads k4unit tests when false. (Default: 1b)
     -runtests,   Runs tests. (Default: 1b)
     -init,       Initialises and opens connections to the client servers on ports bport+1, bport+2. (Default: 1b)
     -testhost,   Sets the host. (Default: 127.0.0.1)
```
In order to run all tests in the csv directory, use the command
```
solace@homer:~/TorQ-Solace/tests$ q solacetest.q

...

TEST RESULTS:

STATUS TYPE  FILE                  NUM TEST-CODE

PASSED RUN   :csv/checkpubsub.csv   0   d:(`user`password`host`port`loglvl)!(`admin;`admin;cmdl[`testhost];55555;2)
PASSED RUN   :csv/checkpubsub.csv   1   stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   2   start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   3   send[(`SOL_1);(`.solace.init;d)]
PASSED RUN   :csv/checkpubsub.csv   4   stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   5   start[cmdl[`bport]+1;`SOL_1]

...

PASSED RUN   :csv/pubsubpayload.csv 468 start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/pubsubpayload.csv 469 start[cmdl[`bport]+2;`SOL_2]
PASSED RUN   :csv/pubsubpayload.csv 470 send[(`SOL_1);(`.solace.init;d)]
PASSED RUN   :csv/pubsubpayload.csv 471 send[(`SOL_2);(`.solace.init;d)]
PASSED RUN   :csv/pubsubpayload.csv 472 send[(`SOL_2);(`.solace.subscribe;"Default")]
PASSED FAIL  :csv/pubsubpayload.csv 473 send[(`SOL_1);(`.solace.publish;"Default";(`a`b)!(enlist 1;enlist 2))]
PASSED RUN   :csv/pubsubpayload.csv 474 stop `SOL_1
PASSED RUN   :csv/pubsubpayload.csv 475 stop `SOL_2
PASSED RUN   :csv/pubsubpayload.csv 476 start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/pubsubpayload.csv 477 start[cmdl[`bport]+2;`SOL_2]
PASSED RUN   :csv/pubsubpayload.csv 478 send[(`SOL_1);(`.solace.init;d)]
PASSED RUN   :csv/pubsubpayload.csv 479 send[(`SOL_2);(`.solace.init;d)]
PASSED RUN   :csv/pubsubpayload.csv 480 send[(`SOL_2);(`.solace.subscribe;"Default")]
PASSED FAIL  :csv/pubsubpayload.csv 481 send[(`SOL_1);(`.solace.publish;"Default";([a:enlist 1];b:enlist 2))]
PASSED RUN   :csv/pubsubpayload.csv 482 stop `SOL_1
PASSED RUN   :csv/pubsubpayload.csv 483 stop `SOL_2
PASSED RUN   :csv/pubsubpayload.csv 484 start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/pubsubpayload.csv 485 start[cmdl[`bport]+2;`SOL_2]
PASSED RUN   :csv/pubsubpayload.csv 486 send[(`SOL_1);(`.solace.init;d)]
PASSED RUN   :csv/pubsubpayload.csv 487 send[(`SOL_2);(`.solace.init;d)]
PASSED RUN   :csv/pubsubpayload.csv 488 send[(`SOL_2);(`.solace.subscribe;"Default")]
PASSED RUN   :csv/pubsubpayload.csv 489 send[(`SOL_1);(`.solace.publish;"Default";1000000#1)]
PASSED FAIL  :csv/pubsubpayload.csv 490 send[(`SOL_2);(`.l.l)]
PASSED RUN   :csv/throughput.csv    491 d:(`user`password`host`port`loglvl)!(`admin;`admin;`$"172.17.0.4";55555;2)
PASSED RUN   :csv/throughput.csv    492 stop `SOL_1
PASSED RUN   :csv/throughput.csv    493 stop `SOL_2
PASSED RUN   :csv/throughput.csv    494 start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/throughput.csv    495 send[(`SOL_1);(`.solace.init;d)]
PASSED RUN   :csv/throughput.csv    496 tp:system"t  do[10000;send[(`SOL_1);(`.solace.publish;\"default\";1 2 3 4 5 6)]]"
PASSED TRUE  :csv/throughput.csv    497 tp<5000


********** All TESTS PASSED ***********
```
In order to run individual csvs, use the command
```
solace@homer:~/TorQ-Solace/tests$ q solacetest.q -testsrc csv/csvname.csv
```
For example,
```
solace@homer:~/TorQ-Solace/tests$ q solacetest.q -testsrc csv/checkpubsub.csv
KDB+ 3.5 2017.11.30 Copyright (C) 1993-2017 Kx Systems
l64/ 24()core 128387MB solace homer 127.0.1.1 EXPIRE 2020.06.30 AquaQ #55345

...

TEST RESULTS:

STATUS TYPE  FILE                  NUM TEST-CODE

PASSED RUN   :csv/checkpubsub.csv   0   d:(`user`password`host`port`loglvl)!(`admin;`admin;cmdl[`testhost];55555;2)
PASSED RUN   :csv/checkpubsub.csv   1   stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   2   start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   3   send[(`SOL_1);(`.solace.init;d)]
PASSED RUN   :csv/checkpubsub.csv   4   stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   5   start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   6   send[(`SOL_1);(`.solace.init;d)]
PASSED FAIL  :csv/checkpubsub.csv   7   send[(`SOL_1);(`.solace.subscribe;`default)]
PASSED RUN   :csv/checkpubsub.csv   8   stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   9   start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   10  send[(`SOL_1);(`.solace.init;d)]
PASSED FAIL  :csv/checkpubsub.csv   11  send[(`SOL_1);(`.solace.subscribe;"")]
PASSED RUN   :csv/checkpubsub.csv   12  stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   13  start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   14  send[(`SOL_1);(`.solace.init;d)]
PASSED FAIL  :csv/checkpubsub.csv   15  send[(`SOL_1);(`.solace.subscribe;()!())]
PASSED RUN   :csv/checkpubsub.csv   16  stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   17  start[cmdl[`bport]+1;`SOL_1]
PASSED FAIL  :csv/checkpubsub.csv   18  send[(`SOL_1);(`.solace.subscribe;"Default")]
PASSED RUN   :csv/checkpubsub.csv   19  stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   20  start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   21  send[(`SOL_1);(`.solace.init;d)]
PASSED FAIL  :csv/checkpubsub.csv   22  send[(`SOL_1);(`.solace.publish;"Default";())]
PASSED TRUE  :csv/checkpubsub.csv   23  0=getfield[send[(`SOL_1);(`.solace.getstats;[])];0]
PASSED RUN   :csv/checkpubsub.csv   24  stop `SOL_1
PASSED RUN   :csv/checkpubsub.csv   25  start[cmdl[`bport]+1;`SOL_1]
PASSED RUN   :csv/checkpubsub.csv   26  send[(`SOL_1);(`.solace.init;d)]
PASSED FAIL  :csv/checkpubsub.csv   27  send[(`SOL_1);(`.solace.publish;"Default";1]
PASSED TRUE  :csv/checkpubsub.csv   28  0=getfield[send[(`SOL_1);(`.solace.getstats;[])];0]



++++++++++ ALL TESTS PASSED ++++++++++
```
### Buffering

This feed handler is dual threaded and uses the sd0 functionality of kdb+ with a socket pair to signal that data is available from the solace thread. However the data itself, is not written to the socket pair. Instead it is written to a buffer which the q main thread inspects. This has a number of advantages, it bypasses the overhead of transferring the data over a network pipe, and it allows the q main thread to conflate updates if possible.  

### Useful links

Solace website - https://solace.com/

Setting up a pubsub broker - https://docs.solace.com/Solace-SW-Broker-Set-Up/Docker-Containers/Set-Up-Single-Linux-Container.htm

Documentation on the Messaging API for C (also referred to as SolClient) - https://docs.solace.com/API-Developer-Online-Ref-Documentation/c/main.html

C API for kdb+ - https://code.kx.com/q/wp/capi/

AquaQ website - https://www.aquaq.co.uk/

### More information 

For more information, contact info@aquaq.co.uk

### MIT License

MIT License

Copyright (c) 2019 AquaQ Analytics

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
