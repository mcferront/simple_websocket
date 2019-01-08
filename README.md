# Simple websocket server in C++
I couldn't find public implementation of a websocket server in C++, so I spent a few hours figuring it out and writing it.  I wanted to post it just in case it saves anyone else some time.

## What it is:
* Winsock server created to listen for a single connection
* A websocket is then established and the server echos back the client's text

## What it is not:
* Robust or meant for anything other than a basic understanding of a C/C++ websocket implementation

### Links
* Websocket RFC: https://tools.ietf.org/html/rfc6455#section-5.2
* Basic winsock server code: https://docs.microsoft.com/en-us/windows/desktop/WinSock/complete-server-code
