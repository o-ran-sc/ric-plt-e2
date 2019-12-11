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

#ifndef E2_E2BUILDER_H
#define E2_E2BUILDER_H

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <error.h>
#include <mdclog/mdclog.h>


#include <mdclog/mdclog.h>


#include "asn1cFiles/E2AP-PDU.h"
#include "asn1cFiles/InitiatingMessage.h"
#include "asn1cFiles/SuccessfulOutcome.h"
#include "asn1cFiles/UnsuccessfulOutcome.h"

#include "asn1cFiles/ProtocolIE-Field.h"

#include "asn1cFiles/constr_TYPE.h"

using namespace std;

#define printEntry(type, function) \
    if (mdclog_level_get() >= MDCLOG_DEBUG) { \
        mdclog_write(MDCLOG_DEBUG, "start Test %s , %s", type, function); \
    }


static void checkAndPrint(asn_TYPE_descriptor_t *typeDescriptor, void *data, char *dataType, const char *function) {
    char errbuf[128]; /* Buffer for error message */
    size_t errlen = sizeof(errbuf); /* Size of the buffer */
    if (asn_check_constraints(typeDescriptor, data, errbuf, &errlen) != 0) {
        mdclog_write(MDCLOG_ERR, "%s Constraint validation failed: %s", dataType, errbuf);
    } else if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "%s successes function %s", dataType, function);
    }
}


PLMN_Identity_t *createPLMN_ID(const unsigned char *data) {
    printEntry("PLMN_Identity_t", __func__)
    auto *plmnId = (PLMN_Identity_t *)calloc(1, sizeof(PLMN_Identity_t));
    ASN_STRUCT_RESET(asn_DEF_PLMN_Identity, plmnId);
    plmnId->size = 3;
    plmnId->buf = (uint8_t *)calloc(1, 3);
    memcpy(plmnId->buf, data, 3);

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_PLMN_Identity, plmnId, (char *)"PLMN_Identity_t", __func__);
    }

    return plmnId;
}

ENB_ID_t *createENB_ID(ENB_ID_PR enbType, unsigned char *data) {
    printEntry("ENB_ID_t", __func__)
    auto *enb = (ENB_ID_t *)calloc(1, sizeof(ENB_ID_t));
    ASN_STRUCT_RESET(asn_DEF_ENB_ID, enb);

    enb->present = enbType;

    switch (enbType) {
        case ENB_ID_PR_macro_eNB_ID: { // 20 bit 3 bytes
            enb->choice.macro_eNB_ID.size = 3;
            enb->choice.macro_eNB_ID.bits_unused = 4;

            enb->present = ENB_ID_PR_macro_eNB_ID;

            enb->choice.macro_eNB_ID.buf = (uint8_t *)calloc(1, enb->choice.macro_eNB_ID.size);
            data[enb->choice.macro_eNB_ID.size - 1] = ((unsigned)(data[enb->choice.macro_eNB_ID.size - 1] >>
                    (unsigned)enb->choice.macro_eNB_ID.bits_unused) << (unsigned)enb->choice.macro_eNB_ID.bits_unused);
            memcpy(enb->choice.macro_eNB_ID.buf, data, enb->choice.macro_eNB_ID.size);

            break;
        }
        case ENB_ID_PR_home_eNB_ID: { // 28 bit 4 bytes
            enb->choice.home_eNB_ID.size = 4;
            enb->choice.home_eNB_ID.bits_unused = 4;
            enb->present = ENB_ID_PR_home_eNB_ID;

            enb->choice.home_eNB_ID.buf = (uint8_t *)calloc(1, enb->choice.home_eNB_ID.size);
            data[enb->choice.home_eNB_ID.size - 1] = ((unsigned)(data[enb->choice.home_eNB_ID.size - 1] >>
                    (unsigned)enb->choice.home_eNB_ID.bits_unused) << (unsigned)enb->choice.home_eNB_ID.bits_unused);
            memcpy(enb->choice.home_eNB_ID.buf, data, enb->choice.home_eNB_ID.size);
            break;
        }
        case ENB_ID_PR_short_Macro_eNB_ID: { // 18 bit - 3 bytes
            enb->choice.short_Macro_eNB_ID.size = 3;
            enb->choice.short_Macro_eNB_ID.bits_unused = 6;
            enb->present = ENB_ID_PR_short_Macro_eNB_ID;

            enb->choice.short_Macro_eNB_ID.buf = (uint8_t *)calloc(1, enb->choice.short_Macro_eNB_ID.size);
            data[enb->choice.short_Macro_eNB_ID.size - 1] = ((unsigned)(data[enb->choice.short_Macro_eNB_ID.size - 1] >>
                    (unsigned)enb->choice.short_Macro_eNB_ID.bits_unused) << (unsigned)enb->choice.short_Macro_eNB_ID.bits_unused);
            memcpy(enb->choice.short_Macro_eNB_ID.buf, data, enb->choice.short_Macro_eNB_ID.size);
            break;
        }
        case ENB_ID_PR_long_Macro_eNB_ID: { // 21
            enb->choice.long_Macro_eNB_ID.size = 3;
            enb->choice.long_Macro_eNB_ID.bits_unused = 3;
            enb->present = ENB_ID_PR_long_Macro_eNB_ID;

            enb->choice.long_Macro_eNB_ID.buf = (uint8_t *)calloc(1, enb->choice.long_Macro_eNB_ID.size);
            data[enb->choice.long_Macro_eNB_ID.size - 1] = ((unsigned)(data[enb->choice.long_Macro_eNB_ID.size - 1] >>
                    (unsigned)enb->choice.long_Macro_eNB_ID.bits_unused) << (unsigned)enb->choice.long_Macro_eNB_ID.bits_unused);
            memcpy(enb->choice.long_Macro_eNB_ID.buf, data, enb->choice.long_Macro_eNB_ID.size);
            break;
        }
        default:
            free(enb);
            return nullptr;
    }

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_ENB_ID, enb, (char *)"ENB_ID_t", __func__);
    }
    return enb;
}

GlobalENB_ID_t *createGlobalENB_ID(PLMN_Identity_t *plmnIdentity, ENB_ID_t *enbId) {
    printEntry("GlobalENB_ID_t", __func__)
    auto *genbId = (GlobalENB_ID_t *)calloc(1, sizeof(GlobalENB_ID_t));
    ASN_STRUCT_RESET(asn_DEF_GlobalENB_ID, genbId);
    memcpy(&genbId->pLMN_Identity, plmnIdentity, sizeof(PLMN_Identity_t));
    memcpy(&genbId->eNB_ID, enbId, sizeof(ENB_ID_t));

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_GlobalENB_ID, genbId, (char *)"GlobalENB_ID_t", __func__);
    }
    return genbId;
}

//ServedCell-Information ::= SEQUENCE {
//        pCI					PCI,
//        cellId				ECGI,
//        tAC					TAC,
//        broadcastPLMNs		BroadcastPLMNs-Item,
//        eUTRA-Mode-Info		EUTRA-Mode-Info,
//        iE-Extensions		ProtocolExtensionContainer { {ServedCell-Information-ExtIEs} } OPTIONAL,
//        ...
//}
//
//ServedCell-Information-ExtIEs X2AP-PROTOCOL-EXTENSION ::= {
//        { ID id-Number-of-Antennaports				CRITICALITY ignore	EXTENSION Number-of-Antennaports					PRESENCE optional}|
//        { ID id-PRACH-Configuration					CRITICALITY ignore	EXTENSION PRACH-Configuration						PRESENCE optional}|
//        { ID id-MBSFN-Subframe-Info					CRITICALITY ignore	EXTENSION MBSFN-Subframe-Infolist				PRESENCE optional}|
//        { ID id-CSG-Id								CRITICALITY ignore	EXTENSION CSG-Id									PRESENCE optional}|
//        { ID id-MBMS-Service-Area-List				CRITICALITY ignore	EXTENSION MBMS-Service-Area-Identity-List		PRESENCE optional}|
//        { ID id-MultibandInfoList					CRITICALITY ignore	EXTENSION MultibandInfoList							PRESENCE optional}|
//        { ID id-FreqBandIndicatorPriority			CRITICALITY ignore	EXTENSION FreqBandIndicatorPriority				PRESENCE optional}|
//        { ID id-BandwidthReducedSI					CRITICALITY ignore	EXTENSION BandwidthReducedSI						PRESENCE optional}|
//        { ID id-ProtectedEUTRAResourceIndication	CRITICALITY ignore	EXTENSION ProtectedEUTRAResourceIndication	PRESENCE optional}|
//        { ID id-BPLMN-ID-Info-EUTRA					CRITICALITY ignore 	EXTENSION BPLMN-ID-Info-EUTRA						PRESENCE optional},
//        ...
//}

ServedCell_Information_t *createervedCellIno(long pci) {
    printEntry("ServedCell_Information_t", __func__)
    auto servedCellinfo = (ServedCell_Information_t *)calloc(1, sizeof(ServedCell_Information_t));

    servedCellinfo->pCI = pci;
//    servedCellinfo->cellId =

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_ServedCell_Information, servedCellinfo, (char *)"ServedCell_Information_t", __func__);
    }

    return servedCellinfo;
}


ServedCells__Member *createServedCellsMember() {
    printEntry("ServedCells__Member", __func__)
    auto servedCellMember = (ServedCells__Member *)calloc(1, sizeof(ServedCells__Member));

    //servedCellMember->servedCellInfo
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_ServedCells, servedCellMember, (char *)"ServedCells__Member", __func__);
    }

    return servedCellMember;
}


static void buildInitiatingMessagePDU(E2AP_PDU_t &pdu, InitiatingMessage_t *initMsg) {
    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = initMsg;
}

template<typename T>
static void buildInitMsg(InitiatingMessage_t &initMsg,
                         InitiatingMessage__value_PR present,
                         ProcedureCode_t procedureCode,
                         Criticality_t criticality,
                         T *value) {
    initMsg.value.present = present;
    initMsg.procedureCode = procedureCode;
    initMsg.criticality = criticality;

    switch (present) {
        case InitiatingMessage__value_PR_RICsubscriptionRequest: {
            memcpy(&initMsg.value.choice.RICsubscriptionRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_RICsubscriptionDeleteRequest: {
            memcpy(&initMsg.value.choice.RICsubscriptionDeleteRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_RICserviceUpdate: {
            memcpy(&initMsg.value.choice.RICserviceUpdate, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_RICcontrolRequest: {
            memcpy(&initMsg.value.choice.RICcontrolRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_X2SetupRequest: {
            memcpy(&initMsg.value.choice.X2SetupRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ENDCX2SetupRequest: {
            memcpy(&initMsg.value.choice.ENDCX2SetupRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ResourceStatusRequest: {
            memcpy(&initMsg.value.choice.ResourceStatusRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ENBConfigurationUpdate: {
            memcpy(&initMsg.value.choice.ENBConfigurationUpdate, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ENDCConfigurationUpdate: {
            memcpy(&initMsg.value.choice.ENDCConfigurationUpdate, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ResetRequest: {
            memcpy(&initMsg.value.choice.ResetRequest, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_RICindication: {
            memcpy(&initMsg.value.choice.RICindication, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_RICserviceQuery: {
            memcpy(&initMsg.value.choice.RICserviceQuery, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_LoadInformation: {
            memcpy(&initMsg.value.choice.LoadInformation, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_GNBStatusIndication: {
            memcpy(&initMsg.value.choice.GNBStatusIndication, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ResourceStatusUpdate: {
            memcpy(&initMsg.value.choice.ResourceStatusUpdate, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_ErrorIndication: {
            memcpy(&initMsg.value.choice.ErrorIndication, value, sizeof(*value));
            break;
        }
        case InitiatingMessage__value_PR_NOTHING:
        default : {
            break;
        }
    }
}

static void buildSuccsesfulMessagePDU(E2AP_PDU_t &pdu, SuccessfulOutcome_t *succMsg) {
    pdu.present = E2AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome = succMsg;
}

template<typename T>
static void buildSuccMsg(SuccessfulOutcome_t &succMsg,
                         SuccessfulOutcome__value_PR present,
                         ProcedureCode_t procedureCode,
                         Criticality_t criticality,
                         T *value) {
    succMsg.value.present = present;
    succMsg.procedureCode = procedureCode;
    succMsg.criticality = criticality;

    switch (present) {
        case SuccessfulOutcome__value_PR_RICsubscriptionResponse: {
            memcpy(&succMsg.value.choice.RICsubscriptionResponse, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_RICsubscriptionDeleteResponse: {
            memcpy(&succMsg.value.choice.RICsubscriptionDeleteResponse, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_RICserviceUpdateAcknowledge: {
            memcpy(&succMsg.value.choice.RICserviceUpdateAcknowledge, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_RICcontrolAcknowledge: {
            memcpy(&succMsg.value.choice.RICcontrolAcknowledge, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_X2SetupResponse: {
            memcpy(&succMsg.value.choice.X2SetupResponse, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_ENDCX2SetupResponse: {
            memcpy(&succMsg.value.choice.ENDCX2SetupResponse, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_ResourceStatusResponse: {
            memcpy(&succMsg.value.choice.ResourceStatusResponse, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_ENBConfigurationUpdateAcknowledge: {
            memcpy(&succMsg.value.choice.ENBConfigurationUpdateAcknowledge, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_ENDCConfigurationUpdateAcknowledge: {
            memcpy(&succMsg.value.choice.ENDCConfigurationUpdateAcknowledge, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_ResetResponse: {
            memcpy(&succMsg.value.choice.ResetResponse, value, sizeof(*value));
            break;
        }
        case SuccessfulOutcome__value_PR_NOTHING:
        default:
            break;
    }
}


static void buildUnSucssesfullMessagePDU(E2AP_PDU_t &pdu, UnsuccessfulOutcome_t *unSuccMsg) {
    pdu.present = E2AP_PDU_PR_unsuccessfulOutcome;
    pdu.choice.unsuccessfulOutcome = unSuccMsg;
}

template<typename T>
static void buildUnSuccMsg(UnsuccessfulOutcome_t &unSuccMsg,
                           UnsuccessfulOutcome__value_PR present,
                           ProcedureCode_t procedureCode,
                           Criticality_t criticality,
                           T *value) {
    unSuccMsg.value.present = present;
    unSuccMsg.procedureCode = procedureCode;
    unSuccMsg.criticality = criticality;

    switch (present) {
        case UnsuccessfulOutcome__value_PR_RICsubscriptionFailure: {
            memcpy(&unSuccMsg.value.choice.RICsubscriptionFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_RICsubscriptionDeleteFailure: {
            memcpy(&unSuccMsg.value.choice.RICsubscriptionDeleteFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_RICserviceUpdateFailure: {
            memcpy(&unSuccMsg.value.choice.RICserviceUpdateFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_RICcontrolFailure: {
            memcpy(&unSuccMsg.value.choice.RICcontrolFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_X2SetupFailure: {
            memcpy(&unSuccMsg.value.choice.X2SetupFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_ENDCX2SetupFailure: {
            memcpy(&unSuccMsg.value.choice.ENDCX2SetupFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_ResourceStatusFailure: {
            memcpy(&unSuccMsg.value.choice.ResourceStatusFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_ENBConfigurationUpdateFailure: {
            memcpy(&unSuccMsg.value.choice.ENBConfigurationUpdateFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_ENDCConfigurationUpdateFailure: {
            memcpy(&unSuccMsg.value.choice.ENDCConfigurationUpdateFailure, value, sizeof(*value));
            break;
        }
        case UnsuccessfulOutcome__value_PR_NOTHING:
        default:
            break;
    }
}


static void createPLMN_ID(PLMN_Identity_t &plmnId, const unsigned char *data) {
    //printEntry("PLMN_Identity_t", __func__)
    //PLMN_Identity_t *plmnId = calloc(1, sizeof(PLMN_Identity_t));
    ASN_STRUCT_RESET(asn_DEF_PLMN_Identity, &plmnId);
    plmnId.size = 3;
    plmnId.buf = (uint8_t *) calloc(1, 3);
    memcpy(plmnId.buf, data, 3);

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_PLMN_Identity, &plmnId, (char *) "PLMN_Identity_t", __func__);
    }

}

static void createENB_ID(ENB_ID_t &enb, ENB_ID_PR enbType, unsigned char *data) {
    //printEntry("ENB_ID_t", __func__)
    ASN_STRUCT_RESET(asn_DEF_ENB_ID, &enb);
    enb.present = enbType;
    switch (enbType) {
        case ENB_ID_PR_macro_eNB_ID: { // 20 bit 3 bytes
            enb.choice.macro_eNB_ID.size = 3;
            enb.choice.macro_eNB_ID.bits_unused = 4;

            enb.present = ENB_ID_PR_macro_eNB_ID;

            enb.choice.macro_eNB_ID.buf = (uint8_t *) calloc(1, enb.choice.macro_eNB_ID.size);
            data[enb.choice.macro_eNB_ID.size - 1] = ((unsigned) (data[enb.choice.macro_eNB_ID.size - 1]
                    >> (unsigned) enb.choice.macro_eNB_ID.bits_unused)
                    << (unsigned) enb.choice.macro_eNB_ID.bits_unused);
            memcpy(enb.choice.macro_eNB_ID.buf, data, enb.choice.macro_eNB_ID.size);

            break;
        }
        case ENB_ID_PR_home_eNB_ID: { // 28 bit 4 bytes
            enb.choice.home_eNB_ID.size = 4;
            enb.choice.home_eNB_ID.bits_unused = 4;
            enb.present = ENB_ID_PR_home_eNB_ID;

            enb.choice.home_eNB_ID.buf = (uint8_t *) calloc(1, enb.choice.home_eNB_ID.size);
            data[enb.choice.home_eNB_ID.size - 1] = ((unsigned) (data[enb.choice.home_eNB_ID.size - 1]
                    >> (unsigned) enb.choice.home_eNB_ID.bits_unused)
                    << (unsigned) enb.choice.home_eNB_ID.bits_unused);
            memcpy(enb.choice.home_eNB_ID.buf, data, enb.choice.home_eNB_ID.size);
            break;
        }
        case ENB_ID_PR_short_Macro_eNB_ID: { // 18 bit - 3 bytes
            enb.choice.short_Macro_eNB_ID.size = 3;
            enb.choice.short_Macro_eNB_ID.bits_unused = 6;
            enb.present = ENB_ID_PR_short_Macro_eNB_ID;

            enb.choice.short_Macro_eNB_ID.buf = (uint8_t *) calloc(1, enb.choice.short_Macro_eNB_ID.size);
            data[enb.choice.short_Macro_eNB_ID.size - 1] = ((unsigned) (data[enb.choice.short_Macro_eNB_ID.size - 1]
                    >> (unsigned) enb.choice.short_Macro_eNB_ID.bits_unused)
                    << (unsigned) enb.choice.short_Macro_eNB_ID.bits_unused);
            memcpy(enb.choice.short_Macro_eNB_ID.buf, data, enb.choice.short_Macro_eNB_ID.size);
            break;
        }
        case ENB_ID_PR_long_Macro_eNB_ID: { // 21
            enb.choice.long_Macro_eNB_ID.size = 3;
            enb.choice.long_Macro_eNB_ID.bits_unused = 3;
            enb.present = ENB_ID_PR_long_Macro_eNB_ID;

            enb.choice.long_Macro_eNB_ID.buf = (uint8_t *) calloc(1, enb.choice.long_Macro_eNB_ID.size);
            data[enb.choice.long_Macro_eNB_ID.size - 1] = ((unsigned) (data[enb.choice.long_Macro_eNB_ID.size - 1]
                    >> (unsigned) enb.choice.long_Macro_eNB_ID.bits_unused)
                    << (unsigned) enb.choice.long_Macro_eNB_ID.bits_unused);
            memcpy(enb.choice.long_Macro_eNB_ID.buf, data, enb.choice.long_Macro_eNB_ID.size);
            break;
        }
        default:
            break;
    }

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_ENB_ID, &enb, (char *) "ENB_ID_t", __func__);
    }
}


static void buildGlobalENB_ID(GlobalENB_ID_t *gnbId,
                              const unsigned char *gnbData,
                              ENB_ID_PR enbType,
                              unsigned char *enbData) {
    createPLMN_ID(gnbId->pLMN_Identity, gnbData);
    createENB_ID(gnbId->eNB_ID, enbType, enbData);
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        checkAndPrint(&asn_DEF_GlobalENB_ID, gnbId, (char *) "GlobalENB_ID_t", __func__);
    }
}

#endif //E2_E2BUILDER_H
