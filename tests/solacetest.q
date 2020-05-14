// Default command line parameters.
defaultcmd:(!). flip (
  (`testsrc;`csv);
  (`bport;9080);
  (`noexit;1b);
  (`noload;0b);
  (`runtests;1b);
  (`init;1b);
  (`testhost;`$"127.0.0.1")
  );

// Usage statement triggered if -usage is entered on command line.
if["-usage" in .z.X;
   -1 "";
   -1 "Usage: q solacetest.q [OPTIONS]\n";
   -1 "Where OPTIONS are:\n";
   -1 "     -testsrc,    Runs all tests when set to csv. To run an individual test, set to csv/csvname.csv (Default: csv)";
   -1 "     -bport,      Client processes will run on ports bport+1, bport+2. (Default: 9080)";
   -1 "     -noexit,     Stays in q session after tests have run. (Default: 1b)";
   -1 "     -noload,     Loads k4unit tests when false. (Default: 1b)"; 
   -1 "     -runtests,   Runs tests. (Default: 1b)";
   -1 "     -init,       Initialises and opens connections to the client servers on ports bport+1, bport+2. (Default: 1b)";
   -1 "     -testhost,   Sets the host. (Default: 127.0.0.1)\n\n";
   exit 0;
  ];

// Get command line.
cmdl:.Q.def[defaultcmd;.Q.opt[.z.x]];

// Load k4unit script.
system"l k4unit.q";

// Create empty dictionary for connection handles.
.conn.h:(`symbol$())!`int$();

// Create logging function
.lg.o:{[m;x;y]0N!(.z.T;m;x;-3!y)};

// Sleep function
sleep:{[x] now:.z.P;while[.z.P<=now+`time$x;()];:()};

// Start server function. 
start:{[port;name]
  /- Start the session.
  .lg.o[`startproc;"Starting process: ",string[name]," on port: ",string[port]];
  system["q solaceconf.q -p ",string[port]];
  /- Sleep while session is starting.
  sleep[600];
  .lg.o[`startproc;"Process started:";name];
  /- Open a connection and name.
  h:hopen [port];
  .conn.h[name]:h;
  /- If master process disconnects, exit 0.
  h".z.pc:{[x;y] if[x=y;exit[0]]}[;.z.w]";
  .lg.o[`startproc;"Process connected to master:";name];
 };

// Stop server function.
stop:{[name]
  /- Exit on handle.
  .lg.o[`closeproc;"Exitting process:";name];
  neg[.conn.h[name]](exit;0);
  /- Flush messages.
  neg[.conn.h[name]][];
  /- Delete handle name from dictionary.
  ![`.conn.h;();0b;enlist name];
  .lg.o[`closeproc;"Exitted process:";name];
 };

// Send message to server.
send:{[name;m]
  .conn.h[name][m]
  //.lg.o[`message;"Attempting to send following message to ",string[name];m];
  //status:.[{ans:x@y;(1b;ans)};(.conn.h[name];m);{(0b;x)}];
  //$[status[0];
    //[.lg.o[`message;"Message sent successfully";name];status[1]];
    //.lg.o[`message;"Error encountered while sending message: ",status[1];name]];
  //:status[0];
 };

getfield:{[d;f]
  d[(key d)[f]]
 }

// Initilise servers.
init:{[cmdl]
  .lg.o[`init;"Executing init script; init flag:";cmdl[`init]];
  start[cmdl[`bport]+1;`SOL_1];
  start[cmdl[`bport]+2;`SOL_2];
 };

//Load k4unit csv folder or file if $noload false
if[not cmdl[`noload];
  $[11h=type key hsym cmdl[`testsrc];
      [.lg.o[`loadtests;"Loading test folder: ";key hsym cmdl[`testsrc]];KUltd[hsym cmdl[`testsrc]]];
    neg[11h]=type key hsym cmdl[`testsrc];
      [.lg.o[`loadtests;"Loading test file: ";key hsym cmdl[`testsrc]];KUltf[hsym cmdl[`testsrc]]];
    .lg.o[`loadtests;"Testsrc provided cannot be found";cmdl[`testsrc]]
   ];
 ];

// Run init.
$[cmdl[`init];
 @[init;cmdl;{[x;cmdl] .lg.o[`init;"Error on init: ",x;cmdl]}[;cmdl]];
 .lg.o[`init;"Init disabled";cmdl[`init]]
  ];

// Run and format tests.
if[cmdl[`runtests];
  KUrt[];-1 "\n\n\n";
  -1 "TEST RESULTS:\n";
  -1 "STATUS TYPE  FILE                  NUM TEST-CODE\n";
  -1 {" " sv ("PASSED";(5$upper string[x[`action]]);(22$string[x[`file]]);(3$string[x[`x]]);string[x[`code]])}each select valid,file,action,code,i from KUTR where ok=1b;
  -1 "";
  -1 {" " sv ("FAILED";(5$upper string[x[`action]]);(22$string[x[`file]]);(3$string[x[`x]]);string[x[`code]])}each select valid,file,action,code,i from KUTR where ok=0b;
  -1 "\n";
  $[0=count select from KUTR where ok=0b;
    -1 "++++++++++ ALL TESTS PASSED ++++++++++\n\n\n";
    -1 "********** ",string[count select from KUTR where ok=0b]," TESTS FAILED ***********\n\n\n"]
  ];

// Exit solace.q
if[not cmdl[`noexit];exit 0];
