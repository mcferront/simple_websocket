/*
sha1.h - header of

============
SHA-1 in C++
============

100% Public Domain.

Original C Code
-- Steve Reid <steve@edmweb.com>
Small changes to fit into bglibs
-- Bruce Guenter <bruce@untroubled.org>
Translation to simpler C++ Code
-- Volker Grabsch <vog@notjusthosting.com>
*/

#ifndef SHA1_HPP
#define SHA1_HPP


#include <iostream>
#include <string>

class SHA1
{
public:
    SHA1();
    void update(const std::string &s);
    void update(std::istream &is);
    const unsigned char *final();

private:
    typedef unsigned long int uint32;   /* just needs to be at least 32bit */
    typedef unsigned long long uint64;  /* just needs to be at least 64bit */

    static const unsigned int DIGEST_INTS = 5;  /* number of 32bit integers per SHA1 digest */
    static const unsigned int BLOCK_INTS = 16;  /* number of 32bit integers per SHA1 block */
    static const unsigned int BLOCK_BYTES = BLOCK_INTS * 4;

    uint32 digest[DIGEST_INTS];
    std::string buffer;
    uint64 transforms;

    void reset();
    void transform(uint32 block[BLOCK_BYTES]);

    static void buffer_to_block(const std::string &buffer, uint32 block[BLOCK_BYTES]);
    static void read(std::istream &is, std::string &s, int max);
};

const unsigned char *sha1(const std::string &string);

/* 
base64.cpp and base64.h

base64 encoding and decoding with C++.

Version: 1.01.00

Copyright (C) 2004-2017 Ren� Nyffenegger

This source code is provided 'as-is', without any express or implied
warranty. In no event will the author be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this source code must not be misrepresented; you must not
claim that you wrote the original source code. If you use this source code
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original source code.

3. This notice may not be removed or altered from any source distribution.

Ren� Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

// trapper@trapzz.com...
//  Modifications to the original code: 
//  * use const char * instead of std::string
//  * remove .cpp and base64_decode

static char *base64_encode(const unsigned char *pIn, size_t length) 
{
#define align(x,a) (((x) + ((a)-1)) / (a) * (a))

    const char encoding[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static char *result;
    static size_t result_length;

    if ( nullptr == result || align(length * 2,4) > result_length )
    {
        result_length = align(length * 2, 4);
        result = (char *) realloc( result, result_length );
    }

    int result_index = 0;

    int val=0, valb=-6;
    for (size_t i = 0; i < length; i++) 
    {
        val = (val<<8) + pIn[i];
        valb += 8;
        while (valb>=0) 
        {
            result[result_index++] = encoding[(val>>valb)&0x3F];
            valb-=6;
        }
    }
    if (valb>-6) 
        result[result_index++] = encoding[((val<<8)>>(valb+8))&0x3F];

    while (result_index % 4) result[result_index++] = '=';
    result[result_index] = 0;

    return result;
}

#endif /* SHA1_HPP */
