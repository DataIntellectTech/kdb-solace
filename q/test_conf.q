// Retrieve the path to the install directory.
SOLHOME:getenv`SOLHOME;

// Default input values for .solace.init
d:(`user`password`host`port`init)!(`admin;`admin;`$"127.0.0.1";55555;1b);

// Replace any key-value pairs in d with ones were entered as command line parameters.
o:.Q.def[d;.Q.opt[.z.x]]

// Load functions from the solace library.
.solace:((`$":",SOLHOME,"/lib/solace") 2:((`getsolacelib);1))[];

// User configured call back function.
f:{.l.l: x}

// Automatically start.
if[o[`init];.solace.init[o]];
