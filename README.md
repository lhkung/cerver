# cerver

<code>cerver</code> is a web server built in plain C++. No dependency is used beyond C++ standard library.

The server uses a thread pool to handle incoming http connections and maintains an <code>LRUCache</code> to store resources in memory for quick access.

All C++ program is written in compliance with RAII paradigm.

To run the server:
<code>httpserver run <resource_directory></code>

To see the status of a running server:
<code>httpserver stat</code>

To terminate the server:
<code>httpserver end</code>

Optional arguments:
<code>-p <port></code>: specify the listening port.
<code>-t <port></code>: attach process to terminal.
