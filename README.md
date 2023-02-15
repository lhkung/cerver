# cerver

<code>cerver</code> is a web server built in plain C++. No dependency is used beyond C++ standard library.

The server uses a thread pool to handle incoming http connections and maintains an <code>LRUCache</code> to store resources in memory for quick access.

All C++ program is written in compliance with RAII paradigm.

I like pure C++. Simple and elegant.
