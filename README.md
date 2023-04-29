# cerver

<code>cerver</code> is a dynamic web server and a web dev framework built in plain C++. No dependency is used beyond C++ standard library.

The server uses a thread pool to handle incoming http connections. Routes can be defined using lambda expressions.

All C++ program is written in compliance with RAII paradigm.

To run the server with self-defined route:<br>
```
#include "httpserver.h"

using namespace Cerver;

int main(int argc, char** argv) {
  server->GET("/", [](const HttpRequest& req, HttpResponse* res) {
    return "Hello World";
  });
  return 0;
}
```

There is also a example server:

To run the server:<br>
<code>bin/webserver run</code>

To see the status of a running server:<br>
<code>bin/webserver stats</code>

To terminate the server:<br>
<code>bin/webserver end</code>

Optional arguments:<br>
<code>-p [port]</code>: specify the listening port.<br>
<code>-t</code>: attach process to terminal.<br>
