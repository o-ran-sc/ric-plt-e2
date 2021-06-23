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

TEST(sctp, TEST1) {
    mdclog_level_set(MDCLOG_DEBUG);
    string s;
    s = translateRmrErrorMessages(0);
    EXPECT_THAT(s, HasSubstr("RMR_OK"));
    s = translateRmrErrorMessages(1);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_BADARG"));
    s = translateRmrErrorMessages(2);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOENDPT"));
    s = translateRmrErrorMessages(3);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_EMPTY"));
    s = translateRmrErrorMessages(4);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOHDR"));
    s = translateRmrErrorMessages(5);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_SENDFAILED"));
    s = translateRmrErrorMessages(6);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_CALLFAILED"));
    s = translateRmrErrorMessages(7);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOWHOPEN"));
    s = translateRmrErrorMessages(8);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_WHID"));
    s = translateRmrErrorMessages(9);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_OVERFLOW"));
    s = translateRmrErrorMessages(10);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_RETRY"));
    s = translateRmrErrorMessages(11);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_RCVFAILED"));
    s = translateRmrErrorMessages(12);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_TIMEOUT"));
    s = translateRmrErrorMessages(13);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_UNSET"));
    s = translateRmrErrorMessages(14);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_TRUNC"));
    s = translateRmrErrorMessages(15);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_INITFAILED"));
    s = translateRmrErrorMessages(16);
    EXPECT_THAT(s, HasSubstr("RMR_ERR_NOTSUPP"));
    s = translateRmrErrorMessages(17);
    EXPECT_THAT(s, HasSubstr("UNDOCUMENTED RMR_ERR"));
}

auto *peerInfo = (ConnectedCU_t *)calloc(1, sizeof(ConnectedCU_t));

TEST(sctp, TEST2) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    ConnectedCU_t data1;
    ConnectedCU_t *data = &data1;
    event.data.ptr = (void *)data;
    sctp_params_t sctp_ut_params;
    sctp_params_t* sctp = &sctp_ut_params;
    ReportingMessages_t reporting_msg;
    RmrMessagesBuffer_t rmrmessagesbuffer;
    handleEinprogressMessages(event,reporting_msg,rmrmessagesbuffer,sctp);
}

TEST(sctp, TEST3) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    ConnectedCU_t data1;
    ConnectedCU_t *data = &data1;
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
    ConnectedCU_t* connected_cu = &cu;
    Sctp_Map_t m1;
    Sctp_Map_t *m = &m1;
    cleanHashEntry(connected_cu,m);
}

TEST(sctp, TEST5) {
    sctp_params_t sctp_ut_params;
    sctp_params_t* sctp = &sctp_ut_params;
    sctp->configFilePath.assign("/opt/e2/RIC-E2-TERMINATION/config/");
    sctp->configFileName.assign("config.conf");
    handleConfigChange(sctp);
}

TEST(sctp, TEST6) {
    int epoll_fd = epoll_create1(0);
    ConnectedCU_t cu;
    ConnectedCU_t* peerinfo = &cu;
    Sctp_Map_t m1;
    Sctp_Map_t *m = &m1;
    modifyToEpoll(epoll_fd,peerinfo,2,m, (char*)"enodeb1",2);
}

/* TEST7 Begin: */
void create_asnInitiatingReq_Procedure_RICserviceUpdate(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICserviceUpdate */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_RICserviceUpdate; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_RICserviceUpdate;
    
    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnInitiatingReq_Procedure_RICindication(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICindication */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_RICindication; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_RICindication;
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array = (RICindication_IEs**) malloc(1 * sizeof(RICindication_IEs*));
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count = 1;
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.size = sizeof(RICindication_IEs);
    pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[0] = (RICindication_IEs*) malloc(sizeof(RICindication_IEs));
    RICindication_IEs_t *ie = pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[0];
    ie->id = ProtocolIE_ID_id_RICrequestID;
    ie->value.present = RICindication_IEs__value_PR_RICrequestID;
    ie->value.choice.RICrequestID.ricRequestorID = 12345;
    ie->value.choice.RICrequestID.ricInstanceID = 1;

    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnInitiatingReq_Procedure_ErrorIndication(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {

    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_ErrorIndication */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_ErrorIndication; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_ErrorIndication;
    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnInitiatingReq_Procedure_Reset(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_Reset */
    pdu->choice.initiatingMessage->procedureCode = ProcedureCode_id_Reset; 
    pdu->choice.initiatingMessage->value.present = InitiatingMessage__value_PR_ResetRequest;
    asnInitiatingRequest(pdu, sctpMap, message, rmrMessageBuffer);
}

TEST(sctp, TEST7) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    
    pdu.present = E2AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage = (InitiatingMessage*) malloc(sizeof(InitiatingMessage));
    memset( (void*)pdu.choice.initiatingMessage, 0, sizeof(pdu.choice.initiatingMessage));
    memset( (void*)&message, 0, sizeof(message));
    memset( (void*)&rmrMessageBuffer, 0, sizeof(rmrMessageBuffer));
    
    message.peerInfo = peerInfo;
    message.peerInfo->sctpParams = &sctp_ut_params;
    snprintf(message.message.enodbName, strlen("Nokia_enb "), "%s", (char*)"Nokia_enb");
    
    sctp_ut_params.myIP = "1.2.3.4";
    sctp_ut_params.rmrPort = 38000;
    snprintf(sctp_ut_params.rmrAddress, sizeof(sctp_ut_params.rmrAddress), "%d", (int) (sctp_ut_params.rmrPort));
    sctp_ut_params.prometheusRegistry = std::make_shared<Registry>();
    startPrometheus(sctp_ut_params);
    
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, RMRFL_NONE);
    rmrMessageBuffer.sendMessage = (rmr_mbuf_t*) malloc(sizeof(rmr_mbuf_t));
    rmrMessageBuffer.sendMessage->header = (uta_mhdr_t*) malloc(sizeof(uta_mhdr_t));
    rmrMessageBuffer.sendMessage->len = strlen("Saying Hello from Ramji ");
    rmrMessageBuffer.sendMessage->payload = (unsigned char*)strdup("Saying Hello from Ramji");

    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICserviceUpdate */
    create_asnInitiatingReq_Procedure_RICserviceUpdate(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_RICindication */
    create_asnInitiatingReq_Procedure_RICindication(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_ErrorIndication */
    create_asnInitiatingReq_Procedure_ErrorIndication(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_initiatingMessage and procedure code as: ProcedureCode_id_Reset */
    create_asnInitiatingReq_Procedure_Reset(&pdu, sctpMap, message, rmrMessageBuffer);
    /* For Procedure's Default case. */
    pdu.choice.initiatingMessage->procedureCode = ((ProcedureCode_t)100);
    asnInitiatingRequest(&pdu, sctpMap, message, rmrMessageBuffer);
    
    /* Put some usleep... */
    // usleep(2);
    if(pdu.choice.initiatingMessage)
        free(pdu.choice.initiatingMessage);
    if(rmrMessageBuffer.sendMessage->header)
        free(rmrMessageBuffer.sendMessage->header);
    if(rmrMessageBuffer.sendMessage)
        free(rmrMessageBuffer.sendMessage);
}

/* TEXT8 Begin: */
void create_asnSuccessfulMsg_Procedure_Reset(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_Reset */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_Reset; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnSuccessfulMsg_Procedure_RICcontrol(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {

    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array = (RICcontrolAcknowledge_IEs_t**) malloc(1 * sizeof(RICcontrolAcknowledge_IEs_t*));
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.count = 1;
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.size = sizeof(RICcontrolAcknowledge_IEs_t);
    pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[0] = (RICcontrolAcknowledge_IEs_t*) malloc(sizeof(RICcontrolAcknowledge_IEs_t));
    RICcontrolAcknowledge_IEs_t *ie = pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[0];

    ie->id = ProtocolIE_ID_id_RICrequestID;
    ie->value.present = RICcontrolAcknowledge_IEs__value_PR_RICrequestID;
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_RICcontrol; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnSuccessfulMsg_Procedure_RICsubscription(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_RICsubscription; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnSuccessfulMsg_Procedure_RICsubscriptionDelete(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    pdu->choice.successfulOutcome->procedureCode = ProcedureCode_id_RICsubscriptionDelete; 
    asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

TEST(sctp, TEST8) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    
    pdu.present = E2AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome = (SuccessfulOutcome*) malloc(sizeof(SuccessfulOutcome));
    memset( (void*)pdu.choice.successfulOutcome, 0, sizeof(pdu.choice.successfulOutcome));
    memset( (void*)&message, 0, sizeof(message));
    memset( (void*)&rmrMessageBuffer, 0, sizeof(rmrMessageBuffer));
    
    message.peerInfo = peerInfo;
    message.peerInfo->sctpParams = &sctp_ut_params;
    snprintf(message.message.enodbName, strlen("Nokia_enb "), "%s", (char*)"Nokia_enb");
    
    sctp_ut_params.myIP = "1.2.3.4";
    sctp_ut_params.rmrPort = 38000;
    snprintf(sctp_ut_params.rmrAddress, sizeof(sctp_ut_params.rmrAddress), "%d", (int) (sctp_ut_params.rmrPort));
    
    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, RMRFL_NONE);
    rmrMessageBuffer.sendMessage = (rmr_mbuf_t*) malloc(sizeof(rmr_mbuf_t));
    rmrMessageBuffer.sendMessage->header = (uta_mhdr_t*) malloc(sizeof(uta_mhdr_t));
    rmrMessageBuffer.sendMessage->len = strlen("Saying Hello from Ramji ");
    rmrMessageBuffer.sendMessage->payload = (unsigned char*)strdup("Saying Hello from Ramji");
    
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_Reset */
    create_asnSuccessfulMsg_Procedure_Reset(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    create_asnSuccessfulMsg_Procedure_RICcontrol(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    create_asnSuccessfulMsg_Procedure_RICsubscription(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_successfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    create_asnSuccessfulMsg_Procedure_RICsubscriptionDelete(&pdu, sctpMap, message, rmrMessageBuffer);
    /* For Procedure's Default case. */
    pdu.choice.successfulOutcome->procedureCode = ((ProcedureCode_t)100); 
    asnSuccessfulMsg(&pdu, sctpMap, message, rmrMessageBuffer);

    /*Put some usleep... */
    // usleep(2);
    if(pdu.choice.successfulOutcome)
        free(pdu.choice.successfulOutcome);
    if(rmrMessageBuffer.sendMessage->header)
        free(rmrMessageBuffer.sendMessage->header);
    if(rmrMessageBuffer.sendMessage)
        free(rmrMessageBuffer.sendMessage);
}

/* TEST9 Begin: */
void create_asnUnSuccsesfulMsg_Procedure_RICcontrol(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {

    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array = (RICcontrolFailure_IEs_t**) malloc(1*sizeof(RICcontrolFailure_IEs_t*));
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.count = 1;
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.size = sizeof(RICcontrolFailure_IEs_t);
    pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[0] = (RICcontrolFailure_IEs_t*) malloc(sizeof(RICcontrolFailure_IEs_t));
    RICcontrolFailure_IEs_t *ie = pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[0];

    ie->id = ProtocolIE_ID_id_RICrequestID;
    ie->value.present = RICcontrolFailure_IEs__value_PR_RICrequestID;
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_RICcontrol;
    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnUnSuccsesfulMsg_Procedure_RICsubscription(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_RICsubscription; 
    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

void create_asnUnSuccsesfulMsg_Procedure_RICsubscriptionDelete(E2AP_PDU_t *pdu, Sctp_Map_t *sctpMap, ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    pdu->choice.unsuccessfulOutcome->procedureCode = ProcedureCode_id_RICsubscriptionDelete; 
    asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
}

TEST(sctp, TEST9) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;
    
    pdu.present = E2AP_PDU_PR_unsuccessfulOutcome;
    pdu.choice.unsuccessfulOutcome = (UnsuccessfulOutcome*) malloc(sizeof(UnsuccessfulOutcome));
    memset( (void*)pdu.choice.unsuccessfulOutcome, 0, sizeof(pdu.choice.unsuccessfulOutcome));
    memset( (void*)&message, 0, sizeof(message));
    memset( (void*)&rmrMessageBuffer, 0, sizeof(rmrMessageBuffer));
    
    message.peerInfo = peerInfo;
    message.peerInfo->sctpParams = &sctp_ut_params;
    snprintf(message.message.enodbName, strlen("Nokia_enb "), "%s", (char*)"Nokia_enb");

    sctp_ut_params.myIP = "1.2.3.4";
    sctp_ut_params.rmrPort = 38000;
    snprintf(sctp_ut_params.rmrAddress, sizeof(sctp_ut_params.rmrAddress), "%d", (int) (sctp_ut_params.rmrPort));

    rmrMessageBuffer.rmrCtx = rmr_init(sctp_ut_params.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, RMRFL_NONE);
    rmrMessageBuffer.sendMessage = (rmr_mbuf_t*) malloc(sizeof(rmr_mbuf_t));
    rmrMessageBuffer.sendMessage->header = (uta_mhdr_t*) malloc(sizeof(uta_mhdr_t));
    rmrMessageBuffer.sendMessage->len = strlen("Saying Hello from Ramji ");
    rmrMessageBuffer.sendMessage->payload = (unsigned char*)strdup("Saying Hello from Ramji");

    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICcontrol */
    create_asnUnSuccsesfulMsg_Procedure_RICcontrol(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscription */
    create_asnUnSuccsesfulMsg_Procedure_RICsubscription(&pdu, sctpMap, message, rmrMessageBuffer);
    /* Sending E2AP_PDU_PR_unsuccessfulOutcome and procedure code as: ProcedureCode_id_RICsubscriptionDelete */
    create_asnUnSuccsesfulMsg_Procedure_RICsubscriptionDelete(&pdu, sctpMap, message, rmrMessageBuffer);
    /* For Procedure's Default case. */
    pdu.choice.unsuccessfulOutcome->procedureCode = ((ProcedureCode_t)100); 
    asnUnSuccsesfulMsg(&pdu, sctpMap, message, rmrMessageBuffer);

    if(pdu.choice.unsuccessfulOutcome)
        free(pdu.choice.unsuccessfulOutcome);
    if(rmrMessageBuffer.sendMessage->header)
        free(rmrMessageBuffer.sendMessage->header);
    if(rmrMessageBuffer.sendMessage)
        free(rmrMessageBuffer.sendMessage);
}

TEST(sctp, TEST10) {
    int epoll_fd = epoll_create1(0);
    ConnectedCU_t cu;
    ConnectedCU_t* peerinfo = &cu;
    Sctp_Map_t m1;
    Sctp_Map_t *m = &m1;
    addToEpoll(epoll_fd, peerinfo, 2, m, (char*)"enodeb1", 0);
}

TEST(sctp, TEST11) {
    sctp_params_t sctpParams;
    int argc = 5;
    char **argv = (char**) malloc(argc * sizeof(char*));
    argv[0] = "./e2";
    argv[1] = "-p";
    argv[2] = "/opt/e2/RIC-E2-TERMINATION/config/";
    argv[3] = "-f";
    argv[4] = "config.conf";
    
    auto result = parse(argc, argv, sctpParams);
    sctpParams.podName.assign("E2TermAlpha_pod");
    buildConfiguration(sctpParams);
    sctpParams.epoll_fd = epoll_create1(0);
    // getRmrContext(sctpParams);
    buildInotify(sctpParams);
    buildListeningPort(sctpParams);
    sctpParams.sctpMap = new mapWrapper();
    listener(&sctpParams);
}

TEST(sctp, TEST12) {
    ReportingMessages_t     reporting_msg;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    sendSctpMsg(peerInfo, reporting_msg, sctpMap);
}

/*TEST(sctp, TEST13) {
    E2AP_PDU_t              pdu;
    Sctp_Map_t              *sctpMap = new Sctp_Map_t();
    ReportingMessages_t     message;
    RmrMessagesBuffer_t     rmrMessageBuffer;
    sctp_params_t           sctp_ut_params;

    pdu.present = E2AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome = (SuccessfulOutcome*) malloc(sizeof(SuccessfulOutcome));
    memset( (void*)pdu.choice.successfulOutcome, 0, sizeof(pdu.choice.successfulOutcome));
    memset( (void*)&message, 0, sizeof(message));
    memset( (void*)&rmrMessageBuffer, 0, sizeof(rmrMessageBuffer));

    message.peerInfo = peerInfo;
    message.peerInfo->sctpParams = &sctp_ut_params;
    snprintf(message.message.enodbName, strlen("Nokia_enb "), "%s", (char*)"Nokia_enb");

    sctp_ut_params.myIP = "1.2.3.4";
    sctp_ut_params.rmrPort = 38000;
    snprintf(sctp_ut_params.rmrAddress, sizeof(sctp_ut_params.rmrAddress), "%d", (int) (sctp_ut_params.rmrPort));
    rmrMessageBuffer.rcvMessage = rmr_rcv_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.rcvMessage);
    rmr_get_meid(rmrMessageBuffer.rcvMessage, (unsigned char *)message.message.enodbName);
    message.peerInfo = (ConnectedCU_t *) sctpMap->find(message.message.enodbName);
    pdu.choice.successfulOutcome->procedureCode = ProcedureCode_id_E2setup;
    receiveXappMessages(sctp_ut_params.sctpMap, rmrMessageBuffer, message.message.time);
}*/
int main(int argc, char **argv) {

   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();

}
