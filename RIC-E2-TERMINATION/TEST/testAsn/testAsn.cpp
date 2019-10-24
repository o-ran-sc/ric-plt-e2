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
// Created by adi ENZEL on 7/8/19.
//

#include <mdclog/mdclog.h>
#include "../../asn1cFiles/E2AP-PDU.h"


void init_log()
{
    mdclog_attr_t *attr;
    mdclog_attr_init(&attr);
    mdclog_attr_set_ident(attr, "e2smTests");
    mdclog_init(attr);
    mdclog_attr_destroy(attr);
}

int main(const int argc, char **argv) {
    init_log();
    //mdclog_level_set(MDCLOG_WARN);
    mdclog_level_set(MDCLOG_DEBUG);

    E2AP_PDU_t *pdu = calloc(1, sizeof(E2AP_PDU_t));
    ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, pdu);

}