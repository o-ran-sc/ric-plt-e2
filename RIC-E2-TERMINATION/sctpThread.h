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

/*
 * This source code is part of the near-RT RIC (RAN Intelligent Controller)
 * platform project (RICP).
 */


#ifndef X2_SCTP_THREAD_H
#define X2_SCTP_THREAD_H

#include <algorithm>

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <random>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/sctp.h>
#include <thread>
#include <atomic>
#include <sys/param.h>
#include <sys/file.h>
#include <ctime>
#include <netdb.h>
#include <sys/epoll.h>
#include <mutex>
#include <shared_mutex>
#include <iterator>
#include <map>

#include <rmr/rmr.h>
#include <rmr/RIC_message_types.h>
#include <mdclog/mdclog.h>
#include <functional>
#include <iostream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/move/utility.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>


#include <mdclog/mdclog.h>

#include "3rdparty/asn1cFiles/E2AP-PDU.h"
#include <3rdparty/asn1cFiles/ProtocolIE-Container.h>
#include "3rdparty/asn1cFiles/InitiatingMessage.h"
#include "3rdparty/asn1cFiles/SuccessfulOutcome.h"
#include "3rdparty/asn1cFiles/UnsuccessfulOutcome.h"
#include "3rdparty/asn1cFiles/ProtocolIE-Container.h"
#include "3rdparty/asn1cFiles/ProtocolIE-Field.h"

#ifdef __TRACING__
#include "openTracing.h"
#endif

#include "mapWrapper.h"

#include "base64.h"

using namespace std;
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace posix_time = boost::posix_time;
namespace expr = boost::log::expressions;

#define SRC_PORT 36422
#define SA      struct sockaddr
#define MAX_ENODB_NAME_SIZE 64

#define MAXEVENTS 128

#define RECEIVE_SCTP_BUFFER_SIZE (64*1024)
#define RECEIVE_XAPP_BUFFER_SIZE RECEIVE_SCTP_BUFFER_SIZE 

typedef mapWrapper Sctp_Map_t;

#ifdef __TRACING__
typedef const std::unique_ptr<opentracing::Span> otSpan;
#else
typedef const int otSpan;
#endif

#define VOLUME_URL_SIZE 256

typedef struct sctp_params {
    uint16_t rmrPort = 0;
    int      epoll_fd = 0;
    int      rmrListenFd = 0;
    void     *rmrCtx = nullptr;
    Sctp_Map_t *sctpMap = nullptr;
    char       rmrAddress[256] {}; // "tcp:portnumber" "tcp:5566" listen to all address on port 5566
    mdclog_severity_t logLevel = MDCLOG_INFO;
    char volume[VOLUME_URL_SIZE];
    //shared_timed_mutex fence; // moved to mapWrapper
} sctp_params_t;

typedef struct ConnectedCU {
    int fileDescriptor = 0;
    char hostName[NI_MAXHOST] {};
    char portNumber[NI_MAXSERV] {};
    char enodbName[MAX_ENODB_NAME_SIZE] {};
    char asnData[RECEIVE_SCTP_BUFFER_SIZE] {};
    size_t asnLength = 0;
    int mtype = 0;
    bool isConnected = false;
} ConnectedCU_t ;

#define MAX_RMR_BUFF_ARRY 32
typedef struct RmrMessagesBuffer {
    void *rmrCtx;
    rmr_mbuf_t *sendMessage;
    rmr_mbuf_t *sendBufferedMessages[MAX_RMR_BUFF_ARRY];
    rmr_mbuf_t *rcvMessage;
    rmr_mbuf_t *rcvBufferedMessages[MAX_RMR_BUFF_ARRY];
} RmrMessagesBuffer_t;

typedef struct formatedMessage {
    char enodbName[MAX_ENODB_NAME_SIZE];
    struct timespec time;
    int messageType;
    char direction;
    ssize_t asnLength;
    unsigned char *asndata;
} FormatedMessage_t;

typedef struct ReportingMessages {
    FormatedMessage_t message;
    int outLen;
    unsigned char base64Data[RECEIVE_SCTP_BUFFER_SIZE * 2];
    char buffer[RECEIVE_SCTP_BUFFER_SIZE * 8];
    size_t bufferLen;
} ReportingMessages_t;


void listener(sctp_params_t *params);

int setSocketNoBlocking(int socket);

void cleanHashEntry(ConnectedCU_t *peerInfo, Sctp_Map_t *m, otSpan *pSpan);

int getSetupRequestMetaData(ReportingMessages_t &message, char *data, char *host, uint16_t &port, otSpan *pSpan);

/**
 *
 * @param message
 * @param rmrMessageBuffer
 * @param pSpan
 */
void getRequestMetaData(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer, otSpan *pSpan);

/**
 *
 * @param sctpMap
 * @param messagBuffer
 * @param message
 * @param failedMesgId
 * @param pSpan
 * @return
 */
int sendMessagetoCu(Sctp_Map_t *sctpMap,
                    RmrMessagesBuffer_t &messagBuffer,
                    ReportingMessages_t &message,
                    int failedMesgId, otSpan *pSpan);

void sendFailedSendingMessagetoXapp(RmrMessagesBuffer_t &rmrMessageBuffer,
                                    ReportingMessages_t &message,
                                    int failedMesgId,
                                    otSpan *pSpan);

int sendRequestToXapp(ReportingMessages_t &message,
                      int requestId,
                      RmrMessagesBuffer_t &rmrMmessageBuffer,
                      otSpan *pSpan);

/**
 *
 * @param message
 * @param msgType
 * @param requestType
 * @param rmrMessageBuffer
 * @param sctpMap
 * @param pSpan
 * @return
 */
int sendResponseToXapp(ReportingMessages_t &message,
                       int msgType,
                       int requestType,
                       RmrMessagesBuffer_t &rmrMessageBuffer,
                       Sctp_Map_t *sctpMap,
                       otSpan *pSpan);

/**
 *
 * @param peerInfo
 * @param message
 * @param m
 * @param pSpan
 * @return
 */
int sendSctpMsg(ConnectedCU_t *peerInfo,
                ReportingMessages_t &message,
                Sctp_Map_t *m,
                otSpan *pSpan);

/**
 *
 * @param events
 * @param sctpMap
 * @param numOfMessages
 * @param rmrMessageBuffer
 * @param ts
 * @param pSpan
 * @return
 */
int receiveDataFromSctp(struct epoll_event *events,
                        Sctp_Map_t *sctpMap,
                        int &numOfMessages,
                        RmrMessagesBuffer_t &rmrMessageBuffer,
                        struct timespec &ts,
                        otSpan *pSpan);

/**
 *
 * @param rmrAddress
 * @param pSpan
 * @return
 */
void *getRmrContext(char *rmrAddress, otSpan *pSpan);

/**
 *
 * @param epoll_fd
 * @param rmrCtx
 * @param sctpMap
 * @param messagBuffer
 * @param pSpan
 * @return
 */
int receiveXappMessages(int epoll_fd,
                        Sctp_Map_t *sctpMap,
                        RmrMessagesBuffer_t &rmrMessageBuffer,
                        struct timespec &ts,
                        otSpan *pSpan);

/**
 *
 * @param rmrMessageBuffer
 * @param message
 * @param epoll_fd
 * @param sctpMap
 * @param pSpan
 * @return
 */
int connectToCUandSetUp(RmrMessagesBuffer_t &rmrMessageBuffer,
                           ReportingMessages_t &message,
                           int epoll_fd,
                           Sctp_Map_t *sctpMap,
                           otSpan *pSpan);

/**
 *
 * @param messagBuffer
 * @param failedMsgId
 * @param sctpMap
 * @param pSpan
 * @return
 */
int sendDirectionalSctpMsg(RmrMessagesBuffer_t &messagBuffer,
                           ReportingMessages_t &message,
                           int failedMsgId,
                           Sctp_Map_t *sctpMap,
                           otSpan *pSpan);
/**
 *
 * @param pdu
 * @param message
 * @param rmrMessageBuffer
 * @param pSpan
 */
void asnInitiatingRequest(E2AP_PDU_t *pdu,
                          ReportingMessages_t &message,
                          RmrMessagesBuffer_t &rmrMessageBuffer,
                          otSpan *pSpan);
/**
 *
 * @param pdu
 * @param message
 * @param sctpMap
 * @param rmrMessageBuffer
 * @param pSpan
 */
void asnSuccsesfulMsg(E2AP_PDU_t *pdu,
                      ReportingMessages_t &message,
                      Sctp_Map_t *sctpMap,
                      RmrMessagesBuffer_t &rmrMessageBuffer,
                      otSpan *pSpan);
/**
 *
 * @param pdu
 * @param message
 * @param sctpMap
 * @param rmrMessageBuffer
 * @param pSpan
 */
void asnUnSuccsesfulMsg(E2AP_PDU_t *pdu,
                        ReportingMessages_t &message,
                        Sctp_Map_t *sctpMap,
                        RmrMessagesBuffer_t &rmrMessageBuffer,
                        otSpan *pSpan);

/**
 *
 * @param rmrMessageBuffer
 * @param message
 * @param pSpan
 * @return
 */
int sendRmrMessage(RmrMessagesBuffer_t &rmrMessageBuffer, ReportingMessages_t &message, otSpan *pSpan);

/**
 *
 * @param epoll_fd
 * @param peerInfo
 * @param events
 * @param sctpMap
 * @param enodbName
 * @param msgType
 * @param pSpan
 * @returnsrc::logger_mt& lg = my_logger::get();
 */
int addToEpoll(int epoll_fd, ConnectedCU_t *peerInfo, uint32_t events, Sctp_Map_t *sctpMap, char *enodbName, int msgType, otSpan *pSpan);
/**
 *
 * @param epoll_fd
 * @param peerInfo
 * @param events
 * @param sctpMap
 * @param enodbName
 * @param msgType
 * @param pSpan
 * @return
 */
int modifyToEpoll(int epoll_fd, ConnectedCU_t *peerInfo, uint32_t events, Sctp_Map_t *sctpMap, char *enodbName, int msgType, otSpan *pSpan);

/**
 *
 * @param message
 */
void buildJsonMessage(ReportingMessages_t &message);

/**
 *
 *
 * @param state
 * @return
 */
string translateRmrErrorMessages(int state);

#ifndef RIC_SCTP_CONNECTION_FAILURE
#define RIC_SCTP_CONNECTION_FAILURE  10080
#endif

#endif //X2_SCTP_THREAD_H
