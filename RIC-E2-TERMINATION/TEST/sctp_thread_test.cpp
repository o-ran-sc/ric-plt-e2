#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sctpThread.h"
#include <sys/epoll.h>
#include "E2AP-PDU.h"

using namespace testing;

typedef struct {
    int32_t mtype;                      // message type  ("long" network integer)
    int32_t	plen;                       // payload length (sender data length in payload)
    int32_t rmr_ver;                    // our internal message version number
    unsigned char xid[RMR_MAX_XID];	    // space for user transaction id or somesuch
    unsigned char sid[RMR_MAX_SID];	    // sender ID for return to sender needs
    unsigned char src[RMR_MAX_SRC];	    // name:port of the sender (source)
    unsigned char meid[RMR_MAX_MEID];   // managed element id.
    struct timespec	ts;                 // timestamp ???
    int32_t	flags;                      // HFL_* constants
    int32_t	len0;                       // length of the RMr header data
    int32_t	len1;                       // length of the tracing data
    int32_t	len2;                       // length of data 1 (d1)
    int32_t	len3;                       // length of data 2 (d2)
    int32_t	sub_id;                     // subscription id (-1 invalid)

    unsigned char srcip[RMR_MAX_SRC];   // ip address and port of the source
} uta_mhdr_t;

constexpr int numberSix = 6;
constexpr int numberSeven = 7;
constexpr int numberEight = 8;
constexpr int numberNine = 9;
constexpr int numberTen = 10;
constexpr int numberEleven = 11;
constexpr int numberTwelve = 12;
constexpr int numberThirteen = 13;
constexpr int numberFourteen = 14;
constexpr int numberFifteen = 15;
constexpr int numberSixteen = 16;
constexpr int numberSeventeen = 17;
constexpr int numberForty = 40;
constexpr int numberHundred = 100;
constexpr int rmrPort = 38000;
constexpr auto dummyIp = "1.2.3.4";
constexpr int ricRequestorId = 12345;

extern uint64_t e2RateLimitCount;
extern int e2RateLimitMax;
extern std::map<std::string, E2NodeConnectionHandling> connectionHandlingPerE2NodeMap;
extern e_TimeToWait timeToWait;

void init_memories(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer, sctp_params_t &sctp_ut_params);
void delete_memories_initiatingMessage(E2AP_PDU_t *pdu, RmrMessagesBuffer_t &rmrMessageBuffer,bool IsRICIndication, bool IsE2SetupReq, bool IsErrorIndication);
void delete_memories_successfulOutcome(E2AP_PDU_t *pdu, RmrMessagesBuffer_t &rmrMessageBuffer);
void delete_memories_unsuccessfulOutcome(E2AP_PDU_t *pdu, RmrMessagesBuffer_t &rmrMessageBuffer);


TEST(sctp, TEST1) {
    mdclog_level_set(MDCLOG_DEBUG);
    string s;
    s = translateRmrErrorMessages(numberZero);
    EXPECT_THAT(s, HasSubstr("RMR_OK"));
    s = translateRmrErrorMessages(numberOne);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_BADARG"));
    s = translateRmrErrorMessages(numberTwo);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOENDPT"));
    s = translateRmrErrorMessages(numberThree);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_EMPTY"));
    s = translateRmrErrorMessages(numberFour);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOHDR"));
    s = translateRmrErrorMessages(numberFive);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_SENDFAILED"));
    s = translateRmrErrorMessages(numberSix);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_CALLFAILED"));
    s = translateRmrErrorMessages(numberSeven);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOWHOPEN"));
    s = translateRmrErrorMessages(numberEight);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_WHID"));
    s = translateRmrErrorMessages(numberNine);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_OVERFLOW"));
    s = translateRmrErrorMessages(numberTen);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_RETRY"));
    s = translateRmrErrorMessages(numberEleven);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_RCVFAILED"));
    s = translateRmrErrorMessages(numberTwelve);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_TIMEOUT"));
    s = translateRmrErrorMessages(numberThirteen);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_UNSET"));
    s = translateRmrErrorMessages(numberFourteen);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_TRUNC"));
    s = translateRmrErrorMessages(numberFifteen);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_INITFAILED"));
    s = translateRmrErrorMessages(numberSixteen);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOTSUPP"));
    s = translateRmrErrorMessages(numberSeventeen);
    EXPECT_THAT(s, HasSubstr("UNDOCUMENTED RMR_ERR"));
}

auto *peerInfo = (ConnectedCU_t *)calloc(numberOne, sizeof(ConnectedCU_t));

TEST(sctp, TEST2) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = numberZero;
    ConnectedCU_t data1;
    ConnectedCU_t *data = &data1;
    event.data.ptr = (void *)data;
    sctp_params_t sctp_ut_params;
    sctp_params_t* sctp = &sctp_ut_params;
    ReportingMessages_t reporting_msg;
    RmrMessagesBuffer_t rmrmessagesbuffer;
    handleEinprogressMessages(event,reporting_msg,rmrmessagesbuffer,sctp);
}

TEST(sctp, TestSctpAbortThenE2tShouldNotCrash) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = numberZero;
    ConnectedCU_t data1;
    ConnectedCU_t *data = (ConnectedCU_t *) calloc(1, sizeof(ConnectedCU_t));
    event.data.ptr = (void *)data;
    sctp_params_t sctp_ut_params;
    sctp_params_t* sctp = &sctp_ut_params;
    Sctp_Map_t m1;
    sctp ->sctpMap =  &m1;
    ReportingMessages_t reporting_msg;
    RmrMessagesBuffer_t rmrmessagesbuffer;
    handlepoll_error(event,reporting_msg,rmrmessagesbuffer,sctp);
}

TEST(sctp, TEST4) {
    ConnectedCU_t cu;
    ConnectedCU_t* connected_cu1 = (ConnectedCU_t *) calloc(1, sizeof(ConnectedCU_t));
    Sctp_Map_t m1;
    Sctp_Map_t *m = &m1;
    cleanHashEntry(connected_cu1,m);
}

TEST(sctp, TEST5) {
    sctp_params_t sctp_ut_params;
    sctp_params_t* sctp = &sctp_ut_params;
    sctp->configFilePath.assign("/opt/e2/RIC-E2-TERMINATION/config");
    sctp->configFileName.assign("config.conf");
    handleConfigChange(sctp);
}

TEST(sctp, TEST6) {
    int epoll_fd = epoll_create1(numberZero);
    ConnectedCU_t cu;
    ConnectedCU_t* peerinfo = (ConnectedCU_t *) calloc(1, sizeof(ConnectedCU_t));
    Sctp_Map_t m1;
    Sctp_Map_t *m = &m1;
    modifyToEpoll(epoll_fd,peerinfo,numberTwo,m, (char*)"enodeb1",numberTwo);
}

TEST(sctp, TEST16) {
unsigned sleepTime;
sleepTime = numberOne;
approx_CPU_MHz(sleepTime);
getinterfaceip();
}

TEST(sctp, TEST17) {
sctp_params_t sctpParams;
sctpParams.inotifyFD = negativeOne;
buildInotify(sctpParams);
}

TEST(sctp, TEST18) {
sctp_params_t sctpParams;
sctpParams.inotifyWD = negativeOne;
buildInotify(sctpParams);
}


/* TEST7 Begin: */
void init_memories(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer, sctp_params_t &sctp_ut_params) {
    message.peerInfo = peerInfo;
    message.peerInfo->sctpParams = &sctp_ut_params;
    snprintf(message.message.enodbName, strlen("Nokia_enb "), "%s", (char*)"Nokia_enb");

    sctp_ut_params.myIP = dummyIp;
    sctp_ut_params.rmrPort = rmrPort;
    snprintf(sctp_ut_params.rmrAddress, sizeof(sctp_ut_params.rmrAddress), "%d", (int) (sctp_ut_params.rmrPort));

    rmrMessageBuffer.sendMessage = (rmr_mbuf_t*) malloc(sizeof(rmr_mbuf_t));
    rmrMessageBuffer.sendMessage->header = (uta_mhdr_t*) malloc(sizeof(uta_mhdr_t));
    rmrMessageBuffer.sendMessage->len = strlen("Saying Hello from NOKIA ");
    rmrMessageBuffer.sendMessage->payload = (unsigned char*)strdup("Saying Hello from NOKIA");
}

void deleteMemoryForE2SetupReq(E2AP_PDU_t *pdu)
{
    if( (pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array) &&
        (pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero]) ) {
        free(pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero]);
        pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero] = NULL;
    }
    if( (pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array) &&
        (pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberOne]) ) {
        free(pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberOne]);
        pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberOne] = NULL;
    }
    if(pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array) {
        free(pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array);
        pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array = NULL;
    }

    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.count = numberZero;
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.size = numberZero;
}

void deleteMemoryForRicIndication(E2AP_PDU_t *pdu)
{
    if( (pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array) &&
        (pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[numberZero]) ) {
        free(pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[numberZero]);
        pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[numberZero] = NULL;
    }
    if(pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array) {
        free(pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array);
        pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array = NULL;
    }
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count = numberZero;
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.size = numberZero;
}

void deleteMemoryForErrorIndication(E2AP_PDU_t *pdu)
{
    if( (pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array) &&
        (pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array[numberZero]) ) {
        free(pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array[numberZero]);
        pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array[numberZero] = NULL;
    }
    if(pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array) {
        free(pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array);
        pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array = NULL;
    }
    pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.count = numberZero;
    pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.size = numberZero;
}

void delete_memories_initiatingMessage(E2AP_PDU_t *pdu, RmrMessagesBuffer_t &rmrMessageBuffer,bool IsRICIndication, bool IsE2SetupReq, bool IsErrorIndication) {
    if(IsE2SetupReq){
        deleteMemoryForE2SetupReq(pdu);
    }
    if(IsRICIndication){
        deleteMemoryForRicIndication(pdu);
    }
    if(IsErrorIndication){
        deleteMemoryForErrorIndication(pdu);
    }
    if(rmrMessageBuffer.sendMessage && rmrMessageBuffer.sendMessage->header) {
        free(rmrMessageBuffer.sendMessage->header);
        rmrMessageBuffer.sendMessage->header = NULL;
    }
    if(rmrMessageBuffer.sendMessage->payload) {
        free(rmrMessageBuffer.sendMessage->payload);
        rmrMessageBuffer.sendMessage->payload = NULL;
    }
    if(rmrMessageBuffer.sendMessage) {
        free(rmrMessageBuffer.sendMessage);
        rmrMessageBuffer.sendMessage = NULL;
    }
    if(rmrMessageBuffer.rmrCtx) {
        rmr_close(rmrMessageBuffer.rmrCtx);
        rmrMessageBuffer.rmrCtx = NULL;
    }
}

void create_asnInitiatingReq_Procedure_E2Setup(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_E2setup */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_E2setup;
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_E2setupRequest;

    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array = (E2setupRequestIEs**) malloc(1 * sizeof(E2setupRequestIEs*));
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.count = 2;
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.size = sizeof(E2setupRequestIEs);
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero] = (E2setupRequestIEs*) malloc(sizeof(E2setupRequestIEs));
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberOne] = (E2setupRequestIEs*) malloc(sizeof(E2setupRequestIEs));
    E2setupRequestIEs_t *ie = pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero];
    ie->id = ProtocolIE_ID_id_GlobalE2node_ID;
    ie->value.present = E2setupRequestIEs__value_PR_GlobalE2node_ID;
    ie->value.choice.GlobalE2node_ID.present = GlobalE2node_ID_PR_gNB;

    ie = pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberOne];
    ie->id = ProtocolIE_ID_id_TransactionID;
    ie->value.present = E2setupRequestIEs__value_PR_TransactionID;
    ie->value.choice.TransactionID = numberOne;
   
    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer,streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, true, false);
}

void unsupportedRicSubscriptionDelReq(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICsubscriptionDeleteRequired */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_RICsubscriptionDeleteRequired;
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_RICsubscriptionDeleteRequired;

    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer,streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, false, false);
}

void unsupportedE2ConnectionUpdateAck(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_E2connectionUpdate */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_E2connectionUpdate;
    pdu->choice.successfulOutcome->value.present = SuccessfulOutcome__value_PR_E2connectionUpdateAcknowledge;

    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    // delete_memories_successfulOutcome(pdu, rmrMessageBuffer);
}

void deleteMemoriesForUnsupportedE2ConnectionUpdate(RmrMessagesBuffer_t &rmrMessageBuffer)
{
    if(rmrMessageBuffer.sendMessage && rmrMessageBuffer.sendMessage->header) {
        free(rmrMessageBuffer.sendMessage->header);
        rmrMessageBuffer.sendMessage->header = NULL;
    }
    if(rmrMessageBuffer.sendMessage->payload) {
        free(rmrMessageBuffer.sendMessage->payload);
        rmrMessageBuffer.sendMessage->payload = NULL;
    }
    if(rmrMessageBuffer.sendMessage) {
        free(rmrMessageBuffer.sendMessage);
        rmrMessageBuffer.sendMessage = NULL;
    }
}

void unsupportedE2ConnectionUpdateFail(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_E2connectionUpdate */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_E2connectionUpdate;
    pdu->choice.unsuccessfulOutcome->value.present = UnsuccessfulOutcome__value_PR_E2nodeConfigurationUpdateFailure;

    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);

    deleteMemoriesForUnsupportedE2ConnectionUpdate(rmrMessageBuffer);
}

void create_asnInitiatingReq_Procedure_E2SetupWithoutTransactionID(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_E2setup */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_E2setup;
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_E2setupRequest;

    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array = (E2setupRequestIEs**) malloc(1 * sizeof(E2setupRequestIEs*));
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.count = numberOne;
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.size = sizeof(E2setupRequestIEs);
    pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero] = (E2setupRequestIEs*) malloc(sizeof(E2setupRequestIEs));
    E2setupRequestIEs_t *ie = pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[numberZero];
    ie->id = ProtocolIE_ID_id_GlobalE2node_ID;
    ie->value.present = E2setupRequestIEs__value_PR_GlobalE2node_ID;
    ie->value.choice.GlobalE2node_ID.present = GlobalE2node_ID_PR_gNB;

    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer,streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, true, false);
}

void create_asnInitiatingReq_Procedure_RICserviceUpdate(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICserviceUpdate */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_RICserviceUpdate; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_RICserviceUpdate;

    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer,streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, false, false);
}

void create_asnInitiatingReq_Procedure_RICindication(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICindication */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_RICindication; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_RICindication;
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array = (RICindication_IEs**) malloc(1 * sizeof(RICindication_IEs*));
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count = numberOne;
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.size = sizeof(RICindication_IEs);
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[numberZero] = (RICindication_IEs*) malloc(sizeof(RICindication_IEs));
    RICindication_IEs_t *ie = pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[numberZero];
    ie->id = ProtocolIE_ID_id_RICrequestID;
    ie->value.present = RICindication_IEs__value_PR_RICrequestID;
    ie->value.choice.RICrequestID.ricRequestorID = ricRequestorId;
    ie->value.choice.RICrequestID.ricInstanceID = numberOne;
    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer,streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,true, false, false);
}

void create_asnInitiatingReq_Procedure_ErrorIndication(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_ErrorIndication */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_ErrorIndication; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_ErrorIndication;
    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer, streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, false, false);
}

void create_asnInitiatingReq_Procedure_Reset(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_Reset */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_Reset; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_ResetRequest;
    int streamId =numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer,streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, false, false);
}

TEST(sctp, TESTBeforeE2SetupReqThenDropTheMessage) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICserviceUpdate */
    create_asnInitiatingReq_Procedure_RICserviceUpdate(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICindication */
    create_asnInitiatingReq_Procedure_RICindication(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_ErrorIndication */
    create_asnInitiatingReq_Procedure_ErrorIndication(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_Reset */
    create_asnInitiatingReq_Procedure_Reset(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* For Procedure's Default case. */
    pdu.choice.initiatingMessage->procedureCode = ((ProcedureCode_t)100);
    int streamId =numberZero;

    asnInitiatingRequest(&pdu, sctpMap, message, rmrMessageBuffer,streamId);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

/* TEST8 Begin: */
void delete_memories_successfulOutcome(E2AP_PDU_t *pdu, RmrMessagesBuffer_t &rmrMessageBuffer) {

    if( (pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array) &&
        (pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[numberZero]) ) {
        free(pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[numberZero]);
        pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[numberZero] = NULL;
    }
    if(pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array) {
        free(pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array);
        pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array = NULL;
    }
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.count = numberZero;
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.size = numberZero;

    if(rmrMessageBuffer.sendMessage && rmrMessageBuffer.sendMessage->header) {
        free(rmrMessageBuffer.sendMessage->header);
        rmrMessageBuffer.sendMessage->header = NULL;
    }
    if(rmrMessageBuffer.sendMessage->payload) {
        free(rmrMessageBuffer.sendMessage->payload);
        rmrMessageBuffer.sendMessage->payload = NULL;
    }
    if(rmrMessageBuffer.sendMessage) {
        free(rmrMessageBuffer.sendMessage);
        rmrMessageBuffer.sendMessage = NULL;
    }
}


void create_asnSuccessfulMsg_Procedure_Reset(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {
    
    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_Reset */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_Reset; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_successfulOutcome(pdu, rmrMessageBuffer);
}

void create_asnSuccessfulMsg_Procedure_RICcontrol(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);

    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array = (RICcontrolAcknowledge_IEs_t**) malloc(1 * sizeof(RICcontrolAcknowledge_IEs_t*));
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.count = numberOne;
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.size = sizeof(RICcontrolAcknowledge_IEs_t);
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[numberZero] = (RICcontrolAcknowledge_IEs_t*) malloc(sizeof(RICcontrolAcknowledge_IEs_t));
    RICcontrolAcknowledge_IEs_t *ie = pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[numberZero];

    ie->id = ProtocolIE_ID_id_RICrequestID;
    ie->value.present = RICcontrolAcknowledge_IEs__value_PR_RICrequestID;
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_RICcontrol; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_successfulOutcome(pdu, rmrMessageBuffer);
}

void create_asnSuccessfulMsg_Procedure_RICsubscription(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap,
        ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_RICsubscription; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_successfulOutcome(pdu, rmrMessageBuffer);
}

void create_asnSuccessfulMsg_Procedure_RICsubscriptionDelete(E2AP_PDU_t *pdu,
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_RICsubscriptionDelete; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_successfulOutcome(pdu, rmrMessageBuffer);
}

TEST(sctp, TEST8) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    pdu.present = E2AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome = (SuccessfulOutcome*) malloc(sizeof(SuccessfulOutcome));
    memset( (void*)pdu.choice.successfulOutcome, numberZero, sizeof(pdu.choice.successfulOutcome));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("127.0.0.1 "), "%s", (char*)"127.0.0.1");

    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_Reset */
    create_asnSuccessfulMsg_Procedure_Reset(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    create_asnSuccessfulMsg_Procedure_RICcontrol(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    create_asnSuccessfulMsg_Procedure_RICsubscription(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    create_asnSuccessfulMsg_Procedure_RICsubscriptionDelete(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* For Procedure's Default case. */
    pdu.choice.successfulOutcome->procedureCode = ((ProcedureCode_t)numberHundred); 
    asnSuccessfulMsg(&pdu, sctpMap, message, rmrMessageBuffer);

    if(pdu.choice.successfulOutcome) {
        free(pdu.choice.successfulOutcome);
        pdu.choice.successfulOutcome = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

/* TEST9 Begin: */
void delete_memories_unsuccessfulOutcome(E2AP_PDU_t *pdu, RmrMessagesBuffer_t &rmrMessageBuffer) {

    if( (pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array) &&
        (pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[numberZero]) ) {
        free(pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[numberZero]);
        pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[numberZero] = NULL;
    }
    if(pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array) {
        free(pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array);
        pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array = NULL;
    }
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.count = numberZero;
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.size = numberZero;

    if(rmrMessageBuffer.sendMessage && rmrMessageBuffer.sendMessage->header) {
        free(rmrMessageBuffer.sendMessage->header);
        rmrMessageBuffer.sendMessage->header = NULL;
    }
    if(rmrMessageBuffer.sendMessage->payload) {
        free(rmrMessageBuffer.sendMessage->payload);
        rmrMessageBuffer.sendMessage->payload = NULL;
    }
    if(rmrMessageBuffer.sendMessage) {
        free(rmrMessageBuffer.sendMessage);
        rmrMessageBuffer.sendMessage = NULL;
    }
}

void create_asnUnSuccsesfulMsg_Procedure_RICcontrol(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);

    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array = (RICcontrolFailure_IEs_t**) malloc(1*sizeof(RICcontrolFailure_IEs_t*));
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.count = numberOne;
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.size = sizeof(RICcontrolFailure_IEs_t);
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[numberZero] = (RICcontrolFailure_IEs_t*) malloc(sizeof(RICcontrolFailure_IEs_t));
    RICcontrolFailure_IEs_t *ie = pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[numberZero];

    ie->id = ProtocolIE_ID_id_RICrequestID;
    ie->value.present = RICcontrolFailure_IEs__value_PR_RICrequestID;
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_RICcontrol;
    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_unsuccessfulOutcome(pdu, rmrMessageBuffer);
}

void create_asnUnSuccsesfulMsg_Procedure_RICsubscription(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_RICsubscription; 
    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_unsuccessfulOutcome(pdu, rmrMessageBuffer);
}

void create_asnUnSuccsesfulMsg_Procedure_RICsubscriptionDelete(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {
    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_RICsubscriptionDelete; 
    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
    delete_memories_unsuccessfulOutcome(pdu, rmrMessageBuffer);
}

TEST(sctp, TEST9) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    pdu.present = E2AP_PDU_PR_unsuccessfulOutcome;
    pdu.choice.unsuccessfulOutcome = (UnsuccessfulOutcome*) malloc(sizeof(UnsuccessfulOutcome));
    memset( (void*)pdu.choice.unsuccessfulOutcome, numberZero, sizeof(pdu.choice.unsuccessfulOutcome));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("127.0.0.1 "), "%s", (char*)"127.0.0.1");

    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    create_asnUnSuccsesfulMsg_Procedure_RICcontrol(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    create_asnUnSuccsesfulMsg_Procedure_RICsubscription(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    create_asnUnSuccsesfulMsg_Procedure_RICsubscriptionDelete(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    /* For Procedure's Default case. */
    pdu.choice.unsuccessfulOutcome->procedureCode = ((ProcedureCode_t)numberHundred); 
    asnUnSuccsesfulMsg(&pdu, sctpMap, message, rmrMessageBuffer);

    if(pdu.choice.unsuccessfulOutcome) {
        free(pdu.choice.unsuccessfulOutcome);
        pdu.choice.unsuccessfulOutcome = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}


TEST(sctp, TEST10) {
    int epoll_fd = epoll_create1(numberZero);
    ConnectedCU_t cu;
    ConnectedCU_t* peerinfo = (ConnectedCU_t *) calloc(1, sizeof(ConnectedCU_t));
    Sctp_Map_t m1;
    Sctp_Map_t *m = &m1;
    addToEpoll(epoll_fd, peerinfo, numberTwo, m, (char*)"enodeb1", numberZero);
}

TEST(sctp, TEST11) {
    sctp_params_t sctpParams;
    int argc = 5;
    char **argv = (char**) calloc(argc, sizeof(char*));
    argv[numberZero] = (char*) malloc(numberForty * sizeof(char));
    argv[numberOne] = (char*) malloc(numberForty * sizeof(char));
    argv[numberTwo] = (char*) malloc(numberForty * sizeof(char));
    argv[numberThree] = (char*) malloc(numberForty * sizeof(char));
    argv[numberFour] = (char*) malloc(numberForty * sizeof(char));
    snprintf(argv[numberZero], strlen("./e2 "), "%s", (char*)"./e2");
    snprintf(argv[numberOne], strlen("-p "), "%s", (char*)"-p");
    snprintf(argv[numberTwo], strlen("/opt/e2/RIC-E2-TERMINATION/config "), "%s", (char*)"/opt/e2/RIC-E2-TERMINATION/config");
    snprintf(argv[numberThree], strlen("-f "), "%s", (char*)"-f");
    snprintf(argv[numberFour], strlen("config.conf "), "%s", (char*)"config.conf");

    auto result = parse(argc, argv, sctpParams);
    sctpParams.podName.assign("E2TermAlpha_pod");
    sctpParams.sctpMap = new mapWrapper();
    sctpParams.epoll_fd = epoll_create1(numberZero);
    buildConfiguration(sctpParams);
    // getRmrContext(sctpParams);
    buildInotify(sctpParams);
    sctpParams.inotifyFD = negativeOne;
    buildInotify(sctpParams);
    sctpParams.inotifyWD = negativeOne;
    buildInotify(sctpParams);
    buildListeningPort(sctpParams);
    listener(&sctpParams);

    if(sctpParams.sctpMap) {
        delete sctpParams.sctpMap;
        sctpParams.sctpMap = NULL;
    }
    if(argv) {
        free(argv[numberZero]);
        argv[numberZero] = NULL;
        free(argv[numberOne]);
        argv[numberOne] = NULL;
        free(argv[numberTwo]);
        argv[numberTwo] = NULL;
        free(argv[numberThree]);
        argv[numberThree] = NULL;
        free(argv[numberFour]);
        argv[numberFour] = NULL;
        free(argv);
        argv=NULL;
    }
}

TEST(sctp, TEST12) {
    ReportingMessages_t     reporting_msg;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    sendSctpMsg(peerInfo, reporting_msg, sctpMap);

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

/*TEST13 Begin*/

void inti_buffers_rcv(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    message.peerInfo = peerInfo;
    snprintf(message.message.enodbName, strlen("Nokia_enb "), "%s", (char*)"Nokia_enb");

    rmrMessageBuffer.rcvMessage = (rmr_mbuf_t*) malloc(sizeof(rmr_mbuf_t));
    rmrMessageBuffer.rcvMessage->header = (uta_mhdr_t*) malloc(sizeof(uta_mhdr_t));
    rmrMessageBuffer.rcvMessage->len = strlen("Saying Hello from Ramji ");
    rmrMessageBuffer.rcvMessage->payload = (unsigned char*)strdup("Saying Hello from Ramji");

    rmrMessageBuffer.sendMessage = (rmr_mbuf_t*) malloc(sizeof(rmr_mbuf_t));
    rmrMessageBuffer.sendMessage->header = (uta_mhdr_t*) malloc(sizeof(uta_mhdr_t));
    rmrMessageBuffer.sendMessage->len = strlen("Saying Hello from Ramji ");
    rmrMessageBuffer.sendMessage->payload = (unsigned char*)strdup("Saying Hello from Ramji");

}

void delete_memories_rcv(RmrMessagesBuffer_t &rmrMessageBuffer) {

    if(rmrMessageBuffer.rcvMessage && rmrMessageBuffer.rcvMessage->header) {
        free(rmrMessageBuffer.rcvMessage->header);
        rmrMessageBuffer.rcvMessage->header = NULL;
    }
    if(rmrMessageBuffer.rcvMessage) {
        free(rmrMessageBuffer.rcvMessage);
        rmrMessageBuffer.rcvMessage = NULL;
    }
    if(rmrMessageBuffer.sendMessage && rmrMessageBuffer.sendMessage->header) {
        free(rmrMessageBuffer.sendMessage->header);
        rmrMessageBuffer.sendMessage->header = NULL;
    }
    if(rmrMessageBuffer.sendMessage) {
        free(rmrMessageBuffer.sendMessage);
        rmrMessageBuffer.sendMessage = NULL;
    }

}

void create_receiveXappMessages_RIC_ERROR_INDICATION(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_ERROR_INDICATION;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);

}
void create_receiveXappMessages_RIC_E2_SETUP_FAILURE(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_E2_SETUP_FAILURE;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_SUB_REQ(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_SUB_REQ;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_CONTROL_REQ(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_CONTROL_REQ;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_SERVICE_QUERY(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_SERVICE_QUERY;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_SERVICE_UPDATE_ACK(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_SERVICE_UPDATE_ACK;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_SERVICE_UPDATE_FAILURE(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_SERVICE_UPDATE_FAILURE;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}


void create_receiveXappMessages_RIC_E2_RESET_REQ(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_E2_RESET_REQ;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_E2_RESET_RESP(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_E2_RESET_RESP;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_SCTP_CLEAR_ALL(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_SCTP_CLEAR_ALL;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_HEALTH_CHECK_REQ(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = RIC_HEALTH_CHECK_REQ;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_E2_TERM_KEEP_ALIVE_REQ(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = E2_TERM_KEEP_ALIVE_REQ;
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_SUB_DEL_REQ(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
        inti_buffers_rcv(message, rmrMessageBuffer);
        rmrMessageBuffer.rcvMessage->mtype = RIC_SUB_DEL_REQ;
        receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
        delete_memories_rcv(rmrMessageBuffer);
}        

void create_receiveXappMessages_RIC_E2_SETUP_RESP(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
        inti_buffers_rcv(message, rmrMessageBuffer);
        rmrMessageBuffer.rcvMessage->mtype = RIC_E2_SETUP_RESP;
        receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
        delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_E2NODE_CONFIG_UPDATE_ACK(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
        inti_buffers_rcv(message, rmrMessageBuffer);
        rmrMessageBuffer.rcvMessage->mtype = RIC_E2NODE_CONFIG_UPDATE_ACK;
        receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
        delete_memories_rcv(rmrMessageBuffer);
}

void create_receiveXappMessages_RIC_E2NODE_CONFIG_UPDATE_FAILURE(Sctp_Map_t *sctpMap, ReportingMessages_t &message,
        RmrMessagesBuffer_t &rmrMessageBuffer) {
        inti_buffers_rcv(message, rmrMessageBuffer);
        rmrMessageBuffer.rcvMessage->mtype = RIC_E2NODE_CONFIG_UPDATE_FAILURE;
        receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
        delete_memories_rcv(rmrMessageBuffer);
}

TEST(sctp, TEST13) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;

    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    // Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_Reset
    create_receiveXappMessages_RIC_E2_SETUP_FAILURE(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_ERROR_INDICATION(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_SUB_REQ(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_CONTROL_REQ(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_HEALTH_CHECK_REQ(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_E2_TERM_KEEP_ALIVE_REQ(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_SCTP_CLEAR_ALL(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_E2_RESET_RESP(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_E2_RESET_REQ(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_SERVICE_UPDATE_FAILURE(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_SERVICE_UPDATE_ACK(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_SERVICE_QUERY(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_SUB_DEL_REQ(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_E2_SETUP_RESP(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_E2NODE_CONFIG_UPDATE_ACK(sctpMap, message, rmrMessageBuffer);
    create_receiveXappMessages_RIC_E2NODE_CONFIG_UPDATE_FAILURE(sctpMap, message, rmrMessageBuffer);

    inti_buffers_rcv(message, rmrMessageBuffer);
    rmrMessageBuffer.rcvMessage->mtype = 52345; /*Dummy Integer Value for default case*/
    receiveXappMessages(sctpMap, rmrMessageBuffer, message.message.time);
    delete_memories_rcv(rmrMessageBuffer);

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

TEST(sctp, TESTForDebug) {
char* log_level = "4";
update_mdc_log_level_severity(log_level);
}

TEST(sctp, TestE2SetupRequest) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestWhenReceivedWithoutTransactionIDThenIgnoreE2SetupProcedure) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_E2SetupWithoutTransactionID(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}


TEST(sctp, TestErrorIndicationWhenE2SetupOngoingProcedureOccurred) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");

    create_receiveXappMessages_RIC_E2_SETUP_RESP(sctpMap, message, rmrMessageBuffer);

    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_ErrorIndication(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}


TEST(sctp, TestE2SetupRequestReceivedThenErrorIndicationMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnInitiatingReq_Procedure_ErrorIndication(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenRICindicationMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnInitiatingReq_Procedure_RICindication(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}
TEST(sctp, TestE2SetupRequestReceivedThenRICserviceUpdateMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnInitiatingReq_Procedure_RICserviceUpdate(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenResetMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnInitiatingReq_Procedure_Reset(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}
TEST(sctp, TestE2SetupRequestReceivedThenRICcontrolMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnSuccessfulMsg_Procedure_RICcontrol(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenRICcontrolMessageTriggeredAndGotControlFailure) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnUnSuccsesfulMsg_Procedure_RICcontrol(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenRICsubscriptionMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnSuccessfulMsg_Procedure_RICsubscription(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenRICsubscriptionMessageTriggeredAndGotSubsFailure) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnUnSuccsesfulMsg_Procedure_RICsubscription(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenRICsubscriptionDeleteMessageTriggered) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnSuccessfulMsg_Procedure_RICsubscriptionDelete(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    resetToDefaultValueAsTeardown(message.message.enodbName);

    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestE2SetupRequestReceivedThenRICsubscriptionDeleteMessageTriggeredAndGotSubDelFailure) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    create_asnUnSuccsesfulMsg_Procedure_RICsubscriptionDelete(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    resetToDefaultValueAsTeardown(message.message.enodbName);

    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestBuildE2SetupFailureWhenRateLimitCountIsGreaterThanE2RateLimitMax) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;

    pdu.present = E2AP_PDU_PR_unsuccessfulOutcome;
    pdu.choice.unsuccessfulOutcome = (UnsuccessfulOutcome*) malloc(sizeof(UnsuccessfulOutcome));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.unsuccessfulOutcome, numberZero, sizeof(pdu.choice.unsuccessfulOutcome));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    e2RateLimitCount = numberThree;
    long transactionID = numberOne;
    int streamId = numberZero;

    handleRejectionCase(message, rmrMessageBuffer, &pdu, transactionID, streamId, sctp_ut_params, sctpMap);

    delete_memories_initiatingMessage(&pdu, rmrMessageBuffer,false, false, false);

    if(pdu.choice.unsuccessfulOutcome) {
        free(pdu.choice.unsuccessfulOutcome);
        pdu.choice.unsuccessfulOutcome = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

void errorIndicationBasedOnProcedureCodeOrProcedureStatus(E2AP_PDU_t *pdu, 
        Sctp_Map_t *sctpMap, 
        ReportingMessages_t &message, 
        RmrMessagesBuffer_t &rmrMessageBuffer,
        sctp_params_t &sctp_ut_params) {

    init_memories(message, rmrMessageBuffer, sctp_ut_params);
    
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_ErrorIndication;
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_ErrorIndication;

    pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array = (ErrorIndication_IEs**) malloc(1 * sizeof(ErrorIndication_IEs*));
    pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.count = 1;
    pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.size = sizeof(ErrorIndication_IEs);
    pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array[numberZero] = (ErrorIndication_IEs*) malloc(sizeof(ErrorIndication_IEs));

    ErrorIndication_IEs_t *ie = pdu->choice.initiatingMessage->value.choice.ErrorIndication.protocolIEs.list.array[numberZero];

    ie->id = ProtocolIE_ID_id_TransactionID;
    ie->criticality = Criticality_reject;
    ie->value.present = ErrorIndication_IEs__value_PR_TransactionID;
    ie->value.choice.TransactionID = numberThree;

    int streamId = numberZero;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer, streamId);
    delete_memories_initiatingMessage(pdu, rmrMessageBuffer,false, false, true);
}


TEST(sctp, TestDropErrorIndicationReceivedWithInvalidTidWhenE2SetupOngoingProcedureOccurredBasedOnOngoingProcedureStatus) {
    Sctp_Map_t *sctpMap = new Sctp_Map_t();
    ReportingMessages_t message;
    RmrMessagesBuffer_t rmrMessageBuffer;
    E2AP_PDU_t              pdu;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);
    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    errorIndicationBasedOnProcedureCodeOrProcedureStatus(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }

    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestRemovalOfE2ConnectionEntryFromMap) {

    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    removeE2ConnectionEntryFromMap(message.message.enodbName);

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}

TEST(sctp, TestUnsupportedRicSubsDelRequiredBeforeE2SetupReqThenE2TShouldNotCrash) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    unsupportedRicSubscriptionDelReq(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

TEST(sctp, TestUnsupportedRicSubsDelRequiredAfterE2SetupReqThenE2TShouldNotCrash) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    unsupportedRicSubscriptionDelReq(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

TEST(sctp, TestUnsupportedE2ConnectionUpdateAckBeforeE2SetupReqThenE2TShouldNotCrash) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.successfulOutcome = (SuccessfulOutcome*) malloc(sizeof(SuccessfulOutcome));
    memset( (void*)pdu.choice.successfulOutcome, numberZero, sizeof(pdu.choice.successfulOutcome));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    unsupportedE2ConnectionUpdateAck(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    if(pdu.choice.successfulOutcome) {
        free(pdu.choice.successfulOutcome);
        pdu.choice.successfulOutcome = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

TEST(sctp, TestUnsupportedE2ConnectionUpdateAckAfterE2SetupReqThenE2TShouldNotCrash) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    pdu.choice.successfulOutcome = (SuccessfulOutcome*) malloc(sizeof(SuccessfulOutcome));
    memset( (void*)pdu.choice.successfulOutcome, numberZero, sizeof(pdu.choice.successfulOutcome));

    unsupportedE2ConnectionUpdateAck(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(pdu.choice.successfulOutcome) {
        free(pdu.choice.successfulOutcome);
        pdu.choice.successfulOutcome = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }

}

TEST(sctp, TestUnsupportedE2ConnectionUpdateFailBeforeE2SetupReqThenE2TShouldNotCrash) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.unsuccessfulOutcome = (UnsuccessfulOutcome*) malloc(sizeof(UnsuccessfulOutcome));
    memset( (void*)pdu.choice.unsuccessfulOutcome, numberZero, sizeof(pdu.choice.unsuccessfulOutcome));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    unsupportedE2ConnectionUpdateFail(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    if(pdu.choice.unsuccessfulOutcome) {
        free(pdu.choice.unsuccessfulOutcome);
        pdu.choice.unsuccessfulOutcome = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

TEST(sctp, TestUnsupportedE2ConnectionUpdateFailAfterE2SetupReqThenE2TShouldNotCrash) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    pdu.choice.unsuccessfulOutcome = (UnsuccessfulOutcome*) malloc(sizeof(UnsuccessfulOutcome));
    memset( (void*)pdu.choice.unsuccessfulOutcome, numberZero, sizeof(pdu.choice.unsuccessfulOutcome));

    unsupportedE2ConnectionUpdateFail(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(pdu.choice.unsuccessfulOutcome) {
        free(pdu.choice.unsuccessfulOutcome);
        pdu.choice.unsuccessfulOutcome = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
}

TEST(sctp, TestTimeToWaitValueWhenRejectedE2SetupReq) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    struct E2NodeConnectionHandling e2NodeConnectionHandling;
    memset(&e2NodeConnectionHandling, numberZero, sizeof(e2NodeConnectionHandling));
    e2NodeConnectionHandling.e2tProcedureOngoingStatus = E2_SETUP_PROCEDURE_NOT_INITIATED;
    e2NodeConnectionHandling.e2SetupProcedureTransactionId = numberOne;

    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, numberZero, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, numberZero, sizeof(message));
    memset( (void*)&rmrMessageBuffer, numberZero, sizeof(rmrMessageBuffer));

    snprintf(sctp_ut_params.rmrAddress, strlen("tcp:4560 "), "%s", (char*)"tcp:4560");
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, 0x01);

    e2RateLimitCount = numberFour;

    create_asnInitiatingReq_Procedure_E2Setup(&pdu, sctpMap, message, rmrMessageBuffer, sctp_ut_params);

     timeToWait = TimeToWait_v1s;
    updateTimeToWaitValueToHigherWaitTime();

    timeToWait = TimeToWait_v2s;
    updateTimeToWaitValueToHigherWaitTime();

    timeToWait = TimeToWait_v5s;
    updateTimeToWaitValueToHigherWaitTime();

    timeToWait = TimeToWait_v10s;
    updateTimeToWaitValueToHigherWaitTime();

    timeToWait = TimeToWait_v20s;
    updateTimeToWaitValueToHigherWaitTime();

    timeToWait = TimeToWait_v60s;
    updateTimeToWaitValueToHigherWaitTime();

    resetToDefaultValueAsTeardown(message.message.enodbName);
    ASSERT_EQ(e2RateLimitCount, numberZero);
    ASSERT_TRUE(currentE2tProcedureOngoingStatus(message.message.enodbName) == E2T_Procedure_States::E2_SETUP_PROCEDURE_NOT_INITIATED);
    removeE2ConnectionEntryFromMap(message.message.enodbName);


    if(pdu.choice.initiatingMessage) {
        free(pdu.choice.initiatingMessage);
        pdu.choice.initiatingMessage = NULL;
    }
    if(sctpMap) {
        delete sctpMap;
        sctpMap = NULL;
    }
}


TEST(sctp, TestForInfo) {
char* log_level = "3";
update_mdc_log_level_severity(log_level);
}

TEST(sctp, TestForWarn) {
char* log_level = "2";
update_mdc_log_level_severity(log_level);
}

TEST(sctp, TestForErr) {
char* log_level = "1";
update_mdc_log_level_severity(log_level);
}

int main(int argc, char **argv) {

   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
