#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sctpThread.h"
#include <sys/epoll.h>
#include"E2AP-PDU.h"

using namespace testing;

TEST(sctp, TEST1) {
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
    int epoll_fd = epoll_create1(0);
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
    int epoll_fd = epoll_create1(0);
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
handleConfigChange(sctp);
}



TEST(sctp, TEST6) {
int epoll_fd = epoll_create1(0);
ConnectedCU_t cu;
ConnectedCU_t* peerinfo = &cu;
Sctp_Map_t m1;
Sctp_Map_t *m = &m1;
modifyToEpoll(epoll_fd,peerinfo,2,m,"enodeb1",2);
}


int main(int argc, char **argv) {

   testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();

}

