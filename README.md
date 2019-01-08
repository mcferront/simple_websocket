# Simple websocket server in C++
I couldn't find public implementation of a websocket server in C++, so I spent a few hours figuring it out and writing it.  I'm posting it in hopes it saves someone else time in figuring it out.

## What it is:
* Winsock server created to listen for a single connection
* A websocket is then established and the server echos back the client's text

## What it is not:
* Robust or meant for anything other than a basic understanding of a C/C++ websocket implementation

## How to use it:
* Compile and run Server/websockets.sln
* Open Client/socket_test.html in a browser

The client will send a string to the server, the server will echo it back and the client will print the response via console.log

### Links
* Websocket RFC: https://tools.ietf.org/html/rfc6455#section-5.2
* Basic winsock server code: https://docs.microsoft.com/en-us/windows/desktop/WinSock/complete-server-code
