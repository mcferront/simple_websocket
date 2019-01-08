#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "sha1.h"

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN  512
#define DEFAULT_PORT    "80"
#define WEBSOCKET_KEY   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// WHAT IT IS:
// Server created to listen for a single connection
// A websocket is then established and the server echos back the client's text

// WHAT IT IS NOT:
// Robust or meant for anything other than a basic understanding of a C/C++ websocket implementation



// websocket RFC:
// https://tools.ietf.org/html/rfc6455#section-5.2

// basic winsock server code taken from:
// https://docs.microsoft.com/en-us/windows/desktop/WinSock/complete-server-code


// Header comes in most significant bit as left most
// In practice this means each group is endian swapped
// So I've modified this header accordingly
#pragma pack (push, 1)
struct _websocket_header
{
    unsigned char opcode : 4;

    unsigned char rsv3 : 1;
    unsigned char rsv2 : 1;
    unsigned char rsv1 : 1;
    unsigned char fin : 1;

    unsigned char len : 7;
    unsigned char mask : 1;
};

struct _extended_16
{
    unsigned char value[2];
};

struct _extended_64
{
    unsigned char value[8];
};

struct _mask_key
{
    unsigned char value[4];
};
#pragma pack (pop)

int main( int argc, const char *pArgv )
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if ( iResult != 0 ) {
        printf( "WSAStartup failed with error: %d\n", iResult );
        return 1;
    }

    ZeroMemory( &hints, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo( NULL, DEFAULT_PORT, &hints, &result );
    if ( iResult != 0 ) {
        printf( "getaddrinfo failed with error: %d\n", iResult );
        WSACleanup( );
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket( result->ai_family, result->ai_socktype, result->ai_protocol );
    if ( ListenSocket == INVALID_SOCKET ) {
        printf( "socket failed with error: %ld\n", WSAGetLastError( ) );
        freeaddrinfo( result );
        WSACleanup( );
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int) result->ai_addrlen );
    if ( iResult == SOCKET_ERROR ) {
        printf( "bind failed with error: %d\n", WSAGetLastError( ) );
        freeaddrinfo( result );
        closesocket( ListenSocket );
        WSACleanup( );
        return 1;
    }

    freeaddrinfo( result );

    iResult = listen( ListenSocket, SOMAXCONN );
    if ( iResult == SOCKET_ERROR ) {
        printf( "listen failed with error: %d\n", WSAGetLastError( ) );
        closesocket( ListenSocket );
        WSACleanup( );
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept( ListenSocket, NULL, NULL );
    if ( ClientSocket == INVALID_SOCKET ) {
        printf( "accept failed with error: %d\n", WSAGetLastError( ) );
        closesocket( ListenSocket );
        WSACleanup( );
        return 1;
    }

    // No longer need server socket
    closesocket( ListenSocket );

    // Receive until the peer shuts down the connection
    while ( true )
    {
        iResult = recv( ClientSocket, recvbuf, recvbuflen, 0 );
        
        if ( iResult == 0 )
        {
            printf( "Connection closing...\n" );
            break;
        }
        else if ( iResult < 0 )
        {
            printf( "recv failed with error: %d\n", WSAGetLastError( ) );
            closesocket( ClientSocket );
            WSACleanup( );
            return 1;
        }

        recvbuf[iResult] = 0;
        
        char sendbuf[1024];
        size_t sendbuf_size = 0;

        // see if it's requesting a key
        char *pKey = strstr(recvbuf, "Sec-WebSocket-Key:");
            
        if ( pKey )
        {
            // parse just the key part
            pKey = strchr(pKey, ' ') + 1;
            char *pEnd = strchr(pKey, '\r');
            *pEnd = 0;

            char key[256];
            _snprintf_s(key, _countof(key), "%s%s", pKey, WEBSOCKET_KEY);

            unsigned char result[20];
            const unsigned char *pSha1Key = sha1(key);
            
            // endian swap each of the 5 ints
            for (int i = 0; i < 5; i++)
            {
                for (int c = 0; c < 4; c++)
                    result[i * 4 + c] = pSha1Key[i * 4 + (4 - c - 1)];
            }

            pKey = base64_encode(result, 20);

            const char *pTemplateResponse = "HTTP/1.1 101 Switching Protocols\r\n"
                                            "Upgrade: websocket\r\n"
                                            "Connection: Upgrade\r\n"
                                            "Sec-WebSocket-Accept: %s\r\n\r\n";

            _snprintf_s(sendbuf, _countof(sendbuf), pTemplateResponse, pKey);
            sendbuf_size = strlen(sendbuf);
        }
        else
        {
            // else read, print the response, and echo it back to the server
            _websocket_header *h = (_websocket_header *) recvbuf;

            _mask_key *mask_key;

            unsigned long long length;

            if (h->len < 126)
            {
                length = h->len;
                mask_key = (_mask_key *) (recvbuf + sizeof(_websocket_header));
            }
            else if (h->len == 126)
            {
                _extended_16 *extended = (_extended_16 *) (recvbuf + sizeof(_websocket_header));

                length = (extended->value[0] << 8) | extended->value[1];
                mask_key = (_mask_key *) (recvbuf + sizeof(_websocket_header) + sizeof(_extended_16));
            }
            else
            {
                _extended_64 *extended = (_extended_64 *) (recvbuf + sizeof(_websocket_header));

                length = (((unsigned long long) extended->value[0]) << 56) | (((unsigned long long) extended->value[1]) << 48) | (((unsigned long long) extended->value[2]) << 40) |
                         (((unsigned long long) extended->value[3]) << 32) | (((unsigned long long) extended->value[4]) << 24) | (((unsigned long long) extended->value[5]) << 16) |
                         (((unsigned long long) extended->value[6]) <<  8) | (((unsigned long long) extended->value[7]) << 0);
                
                mask_key = (_mask_key *) (recvbuf + sizeof(_websocket_header) + sizeof(_extended_64));
            }

            char *client_msg = ((char *)mask_key) + sizeof(_mask_key);

            if (h->mask)
            {
                for (int i = 0; i < length; i++)
                    client_msg[i] = client_msg[i] ^ mask_key->value[i % 4];
            }

            printf("Client: %s\r\n", client_msg);
    
            char *pData;

            h = (_websocket_header *) sendbuf;
            *h = _websocket_header{};

            h->opcode = 0x1; //0x1 = text, 0x2 = blob
            h->fin = 1;
                
            char text[256];
            _snprintf_s(text, _countof(text), "Server Echo: %s", client_msg );

            unsigned long long msg_length = strlen(text);

            sendbuf_size = sizeof(_websocket_header);

            if (msg_length <= 125)
            {
                pData = sendbuf + sizeof(_websocket_header);
                h->len = msg_length;
            }
            else if (msg_length <= 0xffff)
            {
                h->len = 126;

                _extended_16 *extended = (_extended_16 *) (sendbuf + sendbuf_size);
                sendbuf_size += sizeof(_extended_16);

                extended->value[0] = (msg_length >> 8) & 0xff;
                extended->value[1] = msg_length & 0xff;
            }
            else
            {
                h->len = 127;

                _extended_64 *extended = (_extended_64 *) (sendbuf + sendbuf_size);
                sendbuf_size += sizeof(_extended_64);

                extended->value[0] = ((msg_length >> 56) & 0xff);
                extended->value[1] = ((msg_length >> 48) & 0xff);
                extended->value[2] = ((msg_length >> 40) & 0xff);
                extended->value[3] = ((msg_length >> 32) & 0xff);
                extended->value[4] = ((msg_length >> 24) & 0xff);
                extended->value[5] = ((msg_length >> 16) & 0xff);
                extended->value[6] = ((msg_length >> 8)  & 0xff);
                extended->value[7] = ((msg_length >> 0)  & 0xff);
            }

            pData = sendbuf + sendbuf_size;
            
            memcpy(pData, text, (size_t) msg_length);
            sendbuf_size += (size_t) msg_length;
        }


        iSendResult = send( ClientSocket, sendbuf, (int) sendbuf_size, 0 );

        if ( iSendResult == SOCKET_ERROR ) 
        {
            printf( "send failed with error: %d\n", WSAGetLastError( ) );
            closesocket( ClientSocket );
            WSACleanup( );
            return 1;
        }

    } while ( iResult > 0 );

    // shutdown the connection since we're done
    iResult = shutdown( ClientSocket, SD_SEND );
    if ( iResult == SOCKET_ERROR ) {
        printf( "shutdown failed with error: %d\n", WSAGetLastError( ) );
        closesocket( ClientSocket );
        WSACleanup( );
        return 1;
    }

    // cleanup
    closesocket( ClientSocket );    
    WSACleanup( );

    return 0;
}

