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
// Created by adi ENZEL on 12/10/19.
//

#include "E2Builder.h"

#include "asn1cFiles/ProtocolIE-Field.h"

template<typename T>
void buildX2SetupIE(X2SetupRequest_IEs_t *x2SetupIE,
        ProtocolIE_ID_t id,
        Criticality_t criticality,
        X2SetupRequest_IEs__value_PR present,
        T *value) {
    x2SetupIE->id = id;
    x2SetupIE->criticality = criticality;
    x2SetupIE->value.present = present;

    switch (present) {
        case X2SetupRequest_IEs__value_PR_GlobalENB_ID: {
            memcpy(&x2SetupIE->value.choice.GlobalENB_ID, value, sizeof(GlobalENB_ID_t));
            break;
        }
        case X2SetupRequest_IEs__value_PR_ServedCells: {
            memcpy(&x2SetupIE->value.choice.ServedCells, value, sizeof(*value));
            break;
        }
        case X2SetupRequest_IEs__value_PR_GUGroupIDList: {
            memcpy(&x2SetupIE->value.choice.GUGroupIDList, value, sizeof(*value));
            break;
        }
        case X2SetupRequest_IEs__value_PR_LHN_ID: {
            memcpy(&x2SetupIE->value.choice.LHN_ID, value, sizeof(*value));
            break;
        }
        case X2SetupRequest_IEs__value_PR_NOTHING:
        default:
            break;
    }
}

void buildE2SetupRequest(X2SetupRequest_t *x2Setup) {

}