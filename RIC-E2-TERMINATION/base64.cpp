/*
 * Copyright 2019 AT&T Intellectual Property
 * Copyright 2019 Nokia
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created by adi ENZEL on 9/26/19.
//

#include "base64.h"

void base64::encode(const unsigned char *src, const int srcLen, char unsigned *dst, int &dstLen) {
    unsigned char *pos;
    const unsigned char *end, *in;
    if (dstLen < srcLen) {
        mdclog_write(MDCLOG_ERR, "Destination size %d must be at least 140 percent from source size %d",
                     dstLen, srcLen);
        return;
    }
    if (dst == nullptr) {
        mdclog_write(MDCLOG_ERR, "Destination must be allocated and freed by caller the function not allocate the memory");
        return;
    }

    end = src + srcLen;
    in = src;
    pos = dst;
    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> (unsigned int)2];
        *pos++ = base64_table[((in[0] & 0x03) << (unsigned int)4) | (in[1] >> (unsigned int)4)];
        *pos++ = base64_table[((in[1] & 0x0f) << (unsigned int)2) | (in[2] >> (unsigned int)6)];
        *pos++ = base64_table[in[2] & (unsigned int)0x3f];
        in += 3;
    }

    if (end - in) {
        *pos++ = base64_table[in[0] >> (unsigned int)2];
        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << (unsigned int)4];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> (unsigned int)4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    *pos = '\0';
    dstLen = pos - dst;
}

