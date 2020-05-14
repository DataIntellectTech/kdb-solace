SOLHOME:getenv`SOLHOME;
d:(`user`password`host`port`init`loglvl)!(`admin;`admin;`$"127.0.0.1";55555;0b;2);

o:.Q.def[d;.Q.opt[.z.x]]
.solace:((`$":",SOLHOME,"/lib/solace") 2:((`getsolacelib);1))[];
f:{.l.l: x}

if[o[`init];.solace.init[o]];
