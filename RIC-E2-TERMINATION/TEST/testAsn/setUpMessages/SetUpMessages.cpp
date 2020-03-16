/*
 * Copyright 2020 AT&T Intellectual Property
 * Copyright 2020 Nokia
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

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <iostream>


//#include <mdclog/mdclog.h>


#include "oranE2/E2AP-PDU.h"
#include "oranE2/InitiatingMessage.h"
#include "oranE2/SuccessfulOutcome.h"
#include "oranE2/UnsuccessfulOutcome.h"

#include "oranE2/ProtocolIE-Field.h"
#include "oranE2/ENB-ID.h"
#include "oranE2/GlobalENB-ID.h"
#include "oranE2/GlobalE2node-gNB-ID.h"
#include "oranE2/constr_TYPE.h"

#include "E2Builder.h"

using namespace std;


#include "BuildRunName.h"

void buildRanName(E2AP_PDU_t *pdu, unsigned char *buffer) {
    for (auto i = 0; i < pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.count; i++) {
        auto *ie = pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[i];
        if (ie->id == ProtocolIE_ID_id_GlobalE2node_ID) {
            if (ie->value.present == E2setupRequestIEs__value_PR_GlobalE2node_ID) {
                memset(buffer, 0, 128);
                buildRanName( (char *) buffer, ie);
            }
        }
    }

}

void extractPdu(E2AP_PDU_t *pdu, unsigned char *buffer, int buffer_size) {
    asn_enc_rval_t er;
    er = asn_encode_to_buffer(nullptr, ATS_BASIC_XER, &asn_DEF_E2AP_PDU, pdu, buffer, buffer_size);
    if (er.encoded == -1) {
        cerr << "encoding of " << asn_DEF_E2AP_PDU.name << " failed, " << strerror(errno) << endl;
        exit(-1);
    } else if (er.encoded > (ssize_t) buffer_size) {
        cerr << "Buffer of size " << buffer_size << " is to small for " << asn_DEF_E2AP_PDU.name << endl;
        exit(-1);
    } else {
        cout << "XML result = " << buffer << endl;
    }

}

auto main(const int argc, char **argv) -> int {
    E2AP_PDU_t pdu;
    char *printBuffer;
    size_t size;
    FILE *stream = open_memstream(&printBuffer, &size);
    auto buffer_size =  8192;
    unsigned char buffer[8192] = {};

    buildSetupRequest(&pdu, 311, 410);
    asn_fprint(stream, &asn_DEF_E2AP_PDU, &pdu);
    cout << "Encoding E2AP PDU of size  " << size << endl << printBuffer << endl;
    fseek(stream,0,SEEK_SET);

    extractPdu(&pdu, buffer, buffer_size);
    buildRanName(&pdu, buffer);
    cout << "Ran name = " << buffer << endl;

    ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, &pdu);
    memset(buffer, 0, buffer_size);

    buildSetupRequestWithFunc(&pdu, 311, 410);
    extractPdu(&pdu, buffer, buffer_size);

    buildRanName(&pdu, buffer);
    cout << "Ran name = " << buffer << endl;

    cout << "Sucessesfull outcome" << endl;
    ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, &pdu);
    memset(buffer, 0, buffer_size);
    uint8_t data[4] = {0x99, 0xAA, 0xBB, 0};

    buildSetupSuccsessfulResponse(&pdu, 311, 410, data);

    asn_fprint(stream, &asn_DEF_E2AP_PDU, &pdu);
    cout << "Encoding E2AP PDU of size  " << size << endl << printBuffer << endl;
    fseek(stream,0,SEEK_SET);

    extractPdu(&pdu, buffer, buffer_size);

    cout << "Failure outcome" << endl;
    ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, &pdu);
    memset(buffer, 0, buffer_size);

    buildSetupUnSuccsessfulResponse(&pdu);
    asn_fprint(stream, &asn_DEF_E2AP_PDU, &pdu);
    cout << "Encoding E2AP PDU of size  " << size << endl << printBuffer << endl;
    fseek(stream,0,SEEK_SET);

    extractPdu(&pdu, buffer, buffer_size);
}
