// Copyright 2019 AT&T Intellectual Property
// Copyright 2019 Nokia
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// TODO: High-level file comment.


#include "sctpThread.h"


using namespace std::placeholders;
using namespace boost::filesystem;

#ifdef __TRACING__
using namespace opentracing;
#endif
//#ifdef __cplusplus
//extern "C"
//{
//#endif

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)

boost::shared_ptr<sinks::synchronous_sink<sinks::text_file_backend>> boostLogger;
double cpuClock = 0.0;
bool jsonTrace = true;

void init_log() {
    mdclog_attr_t *attr;
    mdclog_attr_init(&attr);
    mdclog_attr_set_ident(attr, "E2Terminator");
    mdclog_init(attr);
    mdclog_attr_destroy(attr);
}
auto start_time = std::chrono::high_resolution_clock::now();
typedef std::chrono::duration<double, std::ratio<1,1>> seconds_t;

double age() {
    return seconds_t(std::chrono::high_resolution_clock::now() - start_time).count();
}

double approx_CPU_MHz(unsigned sleeptime) {
    using namespace std::chrono_literals;
    uint32_t aux = 0;
    uint64_t cycles_start = rdtscp(aux);
    double time_start = age();
    std::this_thread::sleep_for(sleeptime * 1ms);
    uint64_t elapsed_cycles = rdtscp(aux) - cycles_start;
    double elapsed_time = age() - time_start;
    return elapsed_cycles / elapsed_time;
}

//std::atomic<int64_t> rmrCounter{0};
std::atomic<int64_t> num_of_messages{0};
std::atomic<int64_t> num_of_XAPP_messages{0};
static long transactionCounter = 0;


int main(const int argc, char **argv) {
    sctp_params_t sctpParams;
#ifdef __TRACING__
    opentracing::Tracer::InitGlobal(tracelibcpp::createTracer("E2 Terminator"));
    auto span = opentracing::Tracer::Global()->StartSpan(__FUNCTION__);
#else
    otSpan span = 0;
#endif

    {
        std::random_device device{};
        std::mt19937 generator(device());
        std::uniform_int_distribution<long> distribution(1, (long) 1e12);
        transactionCounter = distribution(generator);
    }

    uint64_t st = 0,en = 0;
    uint32_t aux1 = 0;
    uint32_t aux2 = 0;
    st = rdtscp(aux1);

    unsigned num_cpus = std::thread::hardware_concurrency();
    init_log();
    mdclog_level_set(MDCLOG_INFO);

    cpuClock = approx_CPU_MHz(100);

    mdclog_write(MDCLOG_DEBUG, "CPU speed %11.11f", cpuClock);
    auto result = parse(argc, argv, sctpParams);

    path p = (sctpParams.configFilePath + "/" + sctpParams.configFileName).c_str();
    if (exists(p)) {
        const int size = 2048;
        auto fileSize = file_size(p);
        if (fileSize > size) {
            mdclog_write(MDCLOG_ERR, "File %s larger than %d", p.string().c_str(), size);
            exit(-1);
        }
    } else {
        mdclog_write(MDCLOG_ERR, "Configuration File %s not exists", p.string().c_str());
        exit(-1);
    }


    ReadConfigFile conf;
    if (conf.openConfigFile(p.string()) == -1) {
        mdclog_write(MDCLOG_ERR, "Filed to open config file %s, %s",
                     p.string().c_str(), strerror(errno));
        exit(-1);
    }
    int rmrPort = conf.getIntValue("nano");
    if (rmrPort == -1) {
        mdclog_write(MDCLOG_ERR, "illigal RMR port ");
        exit(-1);
    }
    sctpParams.rmrPort = (uint16_t)rmrPort;
    snprintf(sctpParams.rmrAddress, sizeof(sctpParams.rmrAddress), "%d", (int) (sctpParams.rmrPort));

    auto tmpStr = conf.getStringValue("loglevel");
    if (tmpStr.length() == 0) {
        mdclog_write(MDCLOG_ERR, "illigal loglevel. Set loglevel to MDCLOG_INFO");
        tmpStr = "info";
    }
    transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

    if ((tmpStr.compare("debug")) == 0) {
        sctpParams.logLevel = MDCLOG_DEBUG;
    } else if ((tmpStr.compare("info")) == 0) {
        sctpParams.logLevel = MDCLOG_INFO;
    } else if ((tmpStr.compare("warning")) == 0) {
        sctpParams.logLevel = MDCLOG_WARN;
    } else if ((tmpStr.compare("error")) == 0) {
        sctpParams.logLevel = MDCLOG_ERR;
    } else {
        mdclog_write(MDCLOG_ERR, "illigal loglevel = %s. Set loglevel to MDCLOG_INFO", tmpStr.c_str());
        sctpParams.logLevel = MDCLOG_INFO;
    }
    mdclog_level_set(sctpParams.logLevel);

    tmpStr = conf.getStringValue("volume");
    if (tmpStr.length() == 0) {
        mdclog_write(MDCLOG_ERR, "illigal volume.");
        exit(-1);
    }

    char tmpLogFilespec[VOLUME_URL_SIZE];
    tmpLogFilespec[0] = 0;
    sctpParams.volume[0] = 0;
    snprintf(sctpParams.volume, VOLUME_URL_SIZE, "%s", tmpStr.c_str());
    // copy the name to temp file as well
    snprintf(tmpLogFilespec, VOLUME_URL_SIZE, "%s", tmpStr.c_str());


    // define the file name in the tmp directory under the volume
    strcat(tmpLogFilespec,"/tmp/E2Term_%Y-%m-%d_%H-%M-%S.%N.tmpStr");

    sctpParams.myIP = conf.getStringValue("local-ip");
    if (sctpParams.myIP.length() == 0) {
        mdclog_write(MDCLOG_ERR, "illigal local-ip.");
        exit(-1);
    }

    sctpParams.myIP = conf.getStringValue("external-fqdn");
    if (sctpParams.myIP.length() == 0) {
        mdclog_write(MDCLOG_ERR, "illigal external-fqdn.");
        exit(-1);
    }

    tmpStr = conf.getStringValue("trace");
    transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);
    if ((tmpStr.compare("start")) == 0) {
        mdclog_write(MDCLOG_INFO, "Trace set to: start");
        sctpParams.trace = true;
    } else if ((tmpStr.compare("stop")) == 0) {
        mdclog_write(MDCLOG_INFO, "Trace set to: stop");
        sctpParams.trace = false;
    }
    jsonTrace = sctpParams.trace;

    en = rdtscp(aux2);

    mdclog_write(MDCLOG_INFO, "start = %lx end = %lx diff = %lx\n", st, en, en - st);
    mdclog_write(MDCLOG_INFO, "start high = %lx start lo = %lx end high = %lx end lo = %lx\n",
            st >> 32, st & 0xFFFFFFFF, (int64_t)en >> 32, en & 0xFFFFFFFF);
    mdclog_write(MDCLOG_INFO, "ellapsed time = %5.9f\n", (double)(en - st)/cpuClock);

    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_mdc_add("RMR Port", to_string(sctpParams.rmrPort).c_str());
        mdclog_mdc_add("LogLevel", to_string(sctpParams.logLevel).c_str());
        mdclog_mdc_add("volume", sctpParams.volume);
        mdclog_mdc_add("tmpLogFilespec", tmpLogFilespec);
        mdclog_mdc_add("my ip", sctpParams.myIP.c_str());

        mdclog_write(MDCLOG_INFO, "running parameters");
    }
    mdclog_mdc_clean();
    sctpParams.ka_message_length = snprintf(sctpParams.ka_message, 4096, "{\"address\": \"%s:%d\","
                                                                         "\"fqdn\": \"%s\"}",
                                            (const char *)sctpParams.myIP.c_str(),
                                            sctpParams.rmrPort,
                                            sctpParams.fqdn.c_str());


    // Files written to the current working directory
    boostLogger = logging::add_file_log(
            keywords::file_name = tmpLogFilespec, // to temp directory
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_interval(posix_time::hours(1)),
            keywords::format = "%Message%"
            //keywords::format = "[%TimeStamp%]: %Message%" // use each tmpStr with time stamp
    );

    // Setup a destination folder for collecting rotated (closed) files --since the same volumn can use rename()
    boostLogger->locked_backend()->set_file_collector(sinks::file::make_collector(
            keywords::target = sctpParams.volume
    ));

    // Upon restart, scan the directory for files matching the file_name pattern
    boostLogger->locked_backend()->scan_for_files();

    // Enable auto-flushing after each tmpStr record written
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
    	boostLogger->locked_backend()->auto_flush(true);
    }

    // start epoll
    sctpParams.epoll_fd = epoll_create1(0);
    if (sctpParams.epoll_fd == -1) {
        mdclog_write(MDCLOG_ERR, "failed to open epoll descriptor");
        exit(-1);
    }

    getRmrContext(sctpParams, &span);
    if (sctpParams.rmrCtx == nullptr) {
        close(sctpParams.epoll_fd);
        exit(-1);
    }

    if (buildInotify(sctpParams) == -1) {
        close(sctpParams.rmrListenFd);
        rmr_close(sctpParams.rmrCtx);
        close(sctpParams.epoll_fd);
        exit(-1);
     }

    sctpParams.sctpMap = new mapWrapper();

    std::vector<std::thread> threads(num_cpus);
//    std::vector<std::thread> threads;

    num_cpus = 1;
    for (unsigned int i = 0; i < num_cpus; i++) {
        threads[i] = std::thread(listener, &sctpParams);

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            mdclog_write(MDCLOG_ERR, "Error calling pthread_setaffinity_np: %d", rc);
        }
    }

    //loop over term_init until first message from xApp
    handleTermInit(sctpParams);

    for (auto &t : threads) {
        t.join();
    }

#ifdef __TRACING__
    opentracing::Tracer::Global()->Close();
#endif
    return 0;
}

void handleTermInit(sctp_params_t &sctpParams) {
    sendTermInit(sctpParams);
    //send to e2 manager init of e2 term
    //E2_TERM_INIT

    int count = 0;
    auto exitCond = true;
    while (exitCond) {
        auto xappMessages = num_of_XAPP_messages.load(std::memory_order_acquire);
        if (xappMessages > 0) {
            exitCond = false;
            continue;
        }
        usleep(10000);
        count++;
        if (count % 100 == 0) {
            mdclog_write(MDCLOG_ERR, "No messages from any xApp : %ld", xappMessages);
            sendTermInit(sctpParams);
        }
    }
}

void sendTermInit(sctp_params_t &sctpParams) {
    auto term_init = false;
    rmr_mbuf_t *msg = rmr_alloc_msg(sctpParams.rmrCtx, sctpParams.ka_message_length);
    auto count = 0;
    while (!term_init) {
        msg->mtype = E2_TERM_INIT;
        msg->state = 0;
        rmr_bytes2payload(msg, (unsigned char *)sctpParams.ka_message, sctpParams.ka_message_length);
        static unsigned char tx[32];
        auto txLen = snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
        rmr_bytes2xact(msg, tx, txLen);
        msg = rmr_send_msg(sctpParams.rmrCtx, msg);
        if (msg == nullptr) {
            msg = rmr_alloc_msg(sctpParams.rmrCtx, sctpParams.myIP.length());
        } else if (msg->state == 0) {
            term_init = true;
            rmr_free_msg(msg);
            //break;
        } else {
            if (count % 100 == 0) {
                mdclog_write(MDCLOG_ERR, "Error sending E2_TERM_INIT cause : %d ", msg->state);
            }
            sleep(1);
        }
        count++;
    }

}

/**
 *
 * @param argc
 * @param argv
 * @param sctpParams
 * @return
 */
cxxopts::ParseResult parse(int argc, char *argv[], sctp_params_t &sctpParams) {
    cxxopts::Options options(argv[0], "e2 term help");
    options.positional_help("[optional args]").show_positional_help();
    options.allow_unrecognised_options().add_options()
            ("p,path", "config file path", cxxopts::value<std::string>(sctpParams.configFilePath)->default_value("config"))
            ("f,file", "config file name", cxxopts::value<std::string>(sctpParams.configFileName)->default_value("config.conf"))
            ("h,help", "Print help");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help({""}) << std::endl;
        exit(0);
    }
    return result;
}

/**
 *
 * @param sctpParams
 * @return -1 failed 0 success
 */
int buildInotify(sctp_params_t &sctpParams) {
    sctpParams.inotifyFD = inotify_init1(IN_NONBLOCK);
    if (sctpParams.inotifyFD == -1) {
        mdclog_write(MDCLOG_ERR, "Failed to init inotify (inotify_init1) %s", strerror(errno));
        close(sctpParams.rmrListenFd);
        rmr_close(sctpParams.rmrCtx);
        close(sctpParams.epoll_fd);
        return -1;
    }

    sctpParams.inotifyWD = inotify_add_watch(sctpParams.inotifyFD,
                                              (const char *)sctpParams.configFilePath.c_str(),
                                              IN_OPEN | IN_CLOSE);
    if (sctpParams.inotifyWD == -1) {
        mdclog_write(MDCLOG_ERR, "Failed to add directory : %s to  inotify (inotify_add_watch) %s",
                sctpParams.configFilePath.c_str(),
                strerror(errno));
        close(sctpParams.inotifyFD);
        return -1;
    }

    struct epoll_event event{};
    event.events = (EPOLLIN);
    event.data.fd = sctpParams.inotifyFD;
    // add listening RMR FD to epoll
    if (epoll_ctl(sctpParams.epoll_fd, EPOLL_CTL_ADD, sctpParams.inotifyFD, &event)) {
        mdclog_write(MDCLOG_ERR, "Failed to add inotify FD to epoll");
        close(sctpParams.inotifyFD);
        return -1;
    }
    return 0;
}

/**
 *
 * @param args
 * @return
 */
void listener(sctp_params_t *params) {
#ifdef __TRACING__
    auto span = opentracing::Tracer::Global()->StartSpan(__FUNCTION__);
#else
    otSpan span = 0;
#endif
    int num_of_SCTP_messages = 0;
    auto totalTime = 0.0;
    mdclog_mdc_clean();
    mdclog_level_set(params->logLevel);

    std::thread::id this_id = std::this_thread::get_id();
    //save cout
    streambuf *oldCout = cout.rdbuf();
    ostringstream memCout;
    // create new cout
    cout.rdbuf(memCout.rdbuf());
    cout << this_id;
    //return to the normal cout
    cout.rdbuf(oldCout);

    char tid[32];
    memcpy(tid, memCout.str().c_str(), memCout.str().length() < 32 ? memCout.str().length() : 31);
    tid[memCout.str().length()] = 0;
    mdclog_mdc_add("thread id", tid);

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "started thread number %s", tid);
    }

    RmrMessagesBuffer_t rmrMessageBuffer{};
    //create and init RMR
    rmrMessageBuffer.rmrCtx = params->rmrCtx;

    auto *events = (struct epoll_event *) calloc(MAXEVENTS, sizeof(struct epoll_event));
    struct timespec end{0, 0};
    struct timespec start{0, 0};

    rmrMessageBuffer.rcvMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
    rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);

    memcpy(rmrMessageBuffer.ka_message, params->ka_message, params->ka_message_length);
    rmrMessageBuffer.ka_message_len = params->ka_message_length;
    rmrMessageBuffer.ka_message[rmrMessageBuffer.ka_message_len] = 0;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "keep alive message is : %s", rmrMessageBuffer.ka_message);
    }

    ReportingMessages_t message {};

    for (int i = 0; i < MAX_RMR_BUFF_ARRY; i++) {
        rmrMessageBuffer.rcvBufferedMessages[i] = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
        rmrMessageBuffer.sendBufferedMessages[i] = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
    }

    while (true) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Start EPOLL Wait");
        }
        auto numOfEvents = epoll_wait(params->epoll_fd, events, MAXEVENTS, -1);
        if (numOfEvents < 0 && errno == EINTR) {
            if (mdclog_level_get() >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "got EINTR : %s", strerror(errno));
            }
            continue;
        }
        if (numOfEvents < 0) {
            mdclog_write(MDCLOG_ERR, "Epoll wait failed, errno = %s", strerror(errno));
            return;
        }
        for (auto i = 0; i < numOfEvents; i++) {
            if (mdclog_level_get() >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "handling epoll event %d out of %d", i + 1, numOfEvents);
            }
            clock_gettime(CLOCK_MONOTONIC, &message.message.time);
            start.tv_sec = message.message.time.tv_sec;
            start.tv_nsec = message.message.time.tv_nsec;


            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                handlepoll_error(events[i], message, rmrMessageBuffer, params, &span);
            } else if (events[i].events & EPOLLOUT) {
                handleEinprogressMessages(events[i], message, rmrMessageBuffer, params, &span);
            } else if (params->rmrListenFd == events[i].data.fd) {
                // got message from XAPP
                num_of_XAPP_messages.fetch_add(1, std::memory_order_release);
                num_of_messages.fetch_add(1, std::memory_order_release);
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "new message from RMR");
                }
                if (receiveXappMessages(params->epoll_fd,
                                        params->sctpMap,
                                        rmrMessageBuffer,
                                        message.message.time,
                                        &span) != 0) {
                    mdclog_write(MDCLOG_ERR, "Error handling Xapp message");
                }
            } else if (params->inotifyFD == events[i].data.fd) {
                mdclog_write(MDCLOG_INFO, "Got event from inotify (configuration update)");
                handleConfigChange(params);
            } else {
                /* We RMR_ERR_RETRY have data on the fd waiting to be read. Read and display it.
                 * We must read whatever data is available completely, as we are running
                 *  in edge-triggered mode and won't get a notification again for the same data. */
                num_of_messages.fetch_add(1, std::memory_order_release);
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "new message from SCTP, epoll flags are : %0x", events[i].events);
                }
                receiveDataFromSctp(&events[i],
                                    params->sctpMap,
                                    num_of_SCTP_messages,
                                    rmrMessageBuffer,
                                    message.message.time,
                                    &span);
            }

            clock_gettime(CLOCK_MONOTONIC, &end);
            if (mdclog_level_get() >= MDCLOG_INFO) {
                totalTime += ((end.tv_sec + 1.0e-9 * end.tv_nsec) -
                              ((double) start.tv_sec + 1.0e-9 * start.tv_nsec));
            }
            if (mdclog_level_get() >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "message handling is %ld seconds %ld nanoseconds",
                             end.tv_sec - start.tv_sec,
                             end.tv_nsec - start.tv_nsec);
            }
        }
    }
#ifdef __TRACING__
    span->Finish();
#else

#endif
}

/**
 *
 * @param sctpParams
 */
void handleConfigChange(sctp_params_t *sctpParams) {
    char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    char *ptr;

    path p = (sctpParams->configFilePath + "/" + sctpParams->configFileName).c_str();
    auto endlessLoop = true;
    while (endlessLoop) {
        auto len = read(sctpParams->inotifyFD, buf, sizeof buf);
        if (len == -1) {
            if (errno != EAGAIN) {
                mdclog_write(MDCLOG_ERR, "read %s ", strerror(errno));
                endlessLoop = false;
                continue;
            }
            else {
                endlessLoop = false;
                continue;
            }
        }

        for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event *)ptr;
            if (event->mask & (uint32_t)IN_ISDIR) {
                continue;
            }

            // the directory name
            if (sctpParams->inotifyWD == event->wd) {
                // not the directory
            }
            if (event->len) {
                if (!(sctpParams->configFileName.compare(event->name))) {
                    continue;
                }
            }
            // only the file we want
            if (event->mask & (uint32_t)IN_CLOSE_WRITE) {
                if (exists(p)) {
                    const int size = 2048;
                    auto fileSize = file_size(p);
                    if (fileSize > size) {
                        mdclog_write(MDCLOG_ERR, "File %s larger than %d", p.string().c_str(), size);
                        return;
                    }
                } else {
                    mdclog_write(MDCLOG_ERR, "Configuration File %s not exists", p.string().c_str());
                    return;
                }

                ReadConfigFile conf;
                if (conf.openConfigFile(p.string()) == -1) {
                    mdclog_write(MDCLOG_ERR, "Filed to open config file %s, %s",
                                 p.string().c_str(), strerror(errno));
                    return;
                }

                auto tmpStr = conf.getStringValue("loglevel");
                if (tmpStr.length() == 0) {
                    mdclog_write(MDCLOG_ERR, "illigal loglevel. Set loglevel to MDCLOG_INFO");
                    tmpStr = "info";
                }
                transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

                if ((tmpStr.compare("debug")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_DEBUG");
                    sctpParams->logLevel = MDCLOG_DEBUG;
                } else if ((tmpStr.compare("info")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_INFO");
                    sctpParams->logLevel = MDCLOG_INFO;
                } else if ((tmpStr.compare("warning")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_WARN");
                    sctpParams->logLevel = MDCLOG_WARN;
                } else if ((tmpStr.compare("error")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_ERR");
                    sctpParams->logLevel = MDCLOG_ERR;
                } else {
                    mdclog_write(MDCLOG_ERR, "illigal loglevel = %s. Set loglevel to MDCLOG_INFO", tmpStr.c_str());
                    sctpParams->logLevel = MDCLOG_INFO;
                }
                mdclog_level_set(sctpParams->logLevel);


                tmpStr = conf.getStringValue("trace");
                if (tmpStr.length() == 0) {
                    mdclog_write(MDCLOG_ERR, "illigal trace. Set trace to stop");
                    tmpStr = "stop";
                }

                transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);
                if ((tmpStr.compare("start")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Trace set to: start");
                    sctpParams->trace = true;
                } else if ((tmpStr.compare("stop")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Trace set to: stop");
                    sctpParams->trace = false;
                } else {
                    mdclog_write(MDCLOG_ERR, "Trace was set to wrong value %s, set to stop", tmpStr.c_str());
                    sctpParams->trace = false;
                }
                jsonTrace = sctpParams->trace;
                endlessLoop = false;
            }
        }
    }
}

/**
 *
 * @param event
 * @param message
 * @param rmrMessageBuffer
 * @param params
 * @param pSpan
 */
void handleEinprogressMessages(struct epoll_event &event,
                               ReportingMessages_t &message,
                               RmrMessagesBuffer_t &rmrMessageBuffer,
                               sctp_params_t *params,
                               otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    auto *peerInfo = (ConnectedCU_t *)event.data.ptr;
    memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));

    mdclog_write(MDCLOG_INFO, "file descriptor %d got EPOLLOUT", peerInfo->fileDescriptor);
    auto retVal = 0;
    socklen_t retValLen = 0;
    auto rc = getsockopt(peerInfo->fileDescriptor, SOL_SOCKET, SO_ERROR, &retVal, &retValLen);
    if (rc != 0 || retVal != 0) {
        if (rc != 0) {
            rmrMessageBuffer.sendMessage->len = snprintf((char *)rmrMessageBuffer.sendMessage->payload, 256,
                                                         "%s|Failed SCTP Connection, after EINPROGRESS the getsockopt%s",
                                                         peerInfo->enodbName, strerror(errno));
        } else if (retVal != 0) {
            rmrMessageBuffer.sendMessage->len = snprintf((char *)rmrMessageBuffer.sendMessage->payload, 256,
                                                         "%s|Failed SCTP Connection after EINPROGRESS, SO_ERROR",
                                                         peerInfo->enodbName);
        }

        message.message.asndata = rmrMessageBuffer.sendMessage->payload;
        message.message.asnLength = rmrMessageBuffer.sendMessage->len;
        mdclog_write(MDCLOG_ERR, "%s", rmrMessageBuffer.sendMessage->payload);
        message.message.direction = 'N';
        if (sendRequestToXapp(message, RIC_SCTP_CONNECTION_FAILURE, rmrMessageBuffer, &lspan) != 0) {
            mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
        }
        memset(peerInfo->asnData, 0, peerInfo->asnLength);
        peerInfo->asnLength = 0;
        peerInfo->mtype = 0;
#ifdef __TRACING__
        lspan->Finish();
#endif
        return;
    }

    peerInfo->isConnected = true;

    if (modifyToEpoll(params->epoll_fd, peerInfo, (EPOLLIN | EPOLLET), params->sctpMap, peerInfo->enodbName,
                      peerInfo->mtype, &lspan) != 0) {
        mdclog_write(MDCLOG_ERR, "epoll_ctl EPOLL_CTL_MOD");
#ifdef __TRACING__
        lspan->Finish();
#endif
        return;
    }

    message.message.asndata = (unsigned char *)peerInfo->asnData;
    message.message.asnLength = peerInfo->asnLength;
    message.message.messageType = peerInfo->mtype;
    memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));
    num_of_messages.fetch_add(1, std::memory_order_release);
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "send the delayed SETUP/ENDC SETUP to sctp for %s",
                     message.message.enodbName);
    }
    if (sendSctpMsg(peerInfo, message, params->sctpMap, &lspan) != 0) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Error write to SCTP  %s %d", __func__, __LINE__);
        }
#ifdef __TRACING__
        lspan->Finish();
#endif
        return;
    }

    memset(peerInfo->asnData, 0, peerInfo->asnLength);
    peerInfo->asnLength = 0;
    peerInfo->mtype = 0;
#ifdef __TRACING__
    lspan->Finish();
#endif
}


void handlepoll_error(struct epoll_event &event,
                      ReportingMessages_t &message,
                      RmrMessagesBuffer_t &rmrMessageBuffer,
                      sctp_params_t *params,
                      otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    if (event.data.fd != params->rmrListenFd) {
        auto *peerInfo = (ConnectedCU_t *)event.data.ptr;
        mdclog_write(MDCLOG_ERR, "epoll error, events %0x on fd %d, RAN NAME : %s",
                     event.events, peerInfo->fileDescriptor, peerInfo->enodbName);

        rmrMessageBuffer.sendMessage->len = snprintf((char *)rmrMessageBuffer.sendMessage->payload, 256,
                                                     "%s|Failed SCTP Connection",
                                                     peerInfo->enodbName);
        message.message.asndata = rmrMessageBuffer.sendMessage->payload;
        message.message.asnLength = rmrMessageBuffer.sendMessage->len;

        memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));
        message.message.direction = 'N';
        if (sendRequestToXapp(message, RIC_SCTP_CONNECTION_FAILURE, rmrMessageBuffer, &lspan) != 0) {
            mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
        }

        close(peerInfo->fileDescriptor);
        cleanHashEntry((ConnectedCU_t *) event.data.ptr, params->sctpMap, &lspan);
    } else {
        mdclog_write(MDCLOG_ERR, "epoll error, events %0x on RMR FD", event.events);
    }
#ifdef __TRACING__
    lspan->Finish();
#endif

}
/**
 *
 * @param socket
 * @return
 */
int setSocketNoBlocking(int socket) {
    auto flags = fcntl(socket, F_GETFL, 0);

    if (flags == -1) {
        mdclog_mdc_add("func", "fcntl");
        mdclog_write(MDCLOG_ERR, "%s, %s", __FUNCTION__, strerror(errno));
        mdclog_mdc_clean();
        return -1;
    }

    flags = (unsigned) flags | (unsigned) O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flags) == -1) {
        mdclog_mdc_add("func", "fcntl");
        mdclog_write(MDCLOG_ERR, "%s, %s", __FUNCTION__, strerror(errno));
        mdclog_mdc_clean();
        return -1;
    }

    return 0;
}

/**
 *
 * @param val
 * @param m
 * @param pSpan
 */
void cleanHashEntry(ConnectedCU_t *val, Sctp_Map_t *m, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
//    otSpan lspan = 0;
#endif
    char *dummy;
    auto port = (uint16_t) strtol(val->portNumber, &dummy, 10);
    char searchBuff[256]{};

    snprintf(searchBuff, sizeof searchBuff, "host:%s:%d", val->hostName, port);
    m->erase(searchBuff);

    m->erase(val->enodbName);
    free(val);
#ifdef __TRACING__
    lspan->Finish();
#endif
}

/**
 *
 * @param fd file discriptor
 * @param data the asn data to send
 * @param len  length of the data
 * @param enodbName the enodbName as in the map for printing purpose
 * @param m map host information
 * @param mtype message number
 * @return 0 success, anegative number on fail
 */
int sendSctpMsg(ConnectedCU_t *peerInfo, ReportingMessages_t &message, Sctp_Map_t *m, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    auto loglevel = mdclog_level_get();
    int fd = peerInfo->fileDescriptor;
    if (loglevel >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Send SCTP message for CU %s, %s",
                     message.message.enodbName, __FUNCTION__);
    }

    while (true) {
        //TODO add send to VES client or KAFKA
        //format ts|mtype|direction(D/U)|length of asn data|raw data
//        auto length = sizeof message.message.time
//                      + sizeof message.message.enodbName
//                      + sizeof message.message.messageType
//                      + sizeof message.message.direction
//                      + sizeof message.message.asnLength
//                      + message.message.asnLength;

        if (send(fd,message.message.asndata, message.message.asnLength,MSG_NOSIGNAL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            mdclog_write(MDCLOG_ERR, "error writing to CU a message, %s ", strerror(errno));
	    // Prevent double free() of peerInfo in the event of connection failure.
	    // Returning failure will trigger, in x2/endc setup flow, RIC_SCTP_CONNECTION_FAILURE rmr message causing the E2M to retry.
    	    if (!peerInfo->isConnected){
    		mdclog_write(MDCLOG_ERR, "connection to CU %s is still in progress.", message.message.enodbName);
#ifdef __TRACING__
            lspan->Finish();
#endif
		return -1;
	    }
            cleanHashEntry(peerInfo, m, &lspan);
            close(fd);
            char key[MAX_ENODB_NAME_SIZE * 2];
            snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", message.message.enodbName,
                     message.message.messageType);
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "remove key = %s from %s at line %d", key, __FUNCTION__, __LINE__);
            }
    	    auto tmp = m->find(key);
            if (tmp) {
                free(tmp);
            }
            m->erase(key);
#ifdef __TRACING__
            lspan->Finish();
#endif
            return -1;
        }
        message.message.direction = 'D';
        // send report.buffer of size
        buildJsonMessage(message);

        if (loglevel >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG,
                         "SCTP message for CU %s sent from %s",
                         message.message.enodbName,
                         __FUNCTION__);
        }
#ifdef __TRACING__
        lspan->Finish();
#endif

        return 0;
    }
}

/**
 *
 * @param message
 * @param rmrMessageBuffer
 * @param pSpan
 */
void getRequestMetaData(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
//    otSpan lspan = 0;
#endif
    rmr_get_meid(rmrMessageBuffer.rcvMessage, (unsigned char *)(message.message.enodbName));

    message.message.asndata = rmrMessageBuffer.rcvMessage->payload;
    message.message.asnLength = rmrMessageBuffer.rcvMessage->len;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Message from Xapp RAN name = %s message length = %ld",
                     message.message.enodbName, (unsigned long) message.message.asnLength);
    }
#ifdef __TRACING__
    lspan->Finish();
#endif

}


/**
 *
 * @param metaData all the data strip to structure
 * @param data the data recived from xAPP
 * @return 0 success all other values are fault
 */
int getSetupRequestMetaData(ReportingMessages_t &message, char *data, char *host, uint16_t &port, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
//    otSpan lspan = 0;
#endif
    auto loglevel = mdclog_level_get();

    char delimiter[4] {};
    memset(delimiter, 0, (size_t)4);
    delimiter[0] = '|';
    char *tmp;

    char *val = strtok_r(data, delimiter, &tmp);
    if (val != nullptr) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "SCTP ADDRESS parameter from message = %s", val);
        }
        memcpy(host, val, tmp - val );
    } else {
        mdclog_write(MDCLOG_ERR, "wrong Host Name for setup request %s", data);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    val = strtok_r(nullptr, delimiter, &tmp);
    if (val != nullptr) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "PORT parameter from message = %s", val);
        }
        char *dummy;
        port = (uint16_t)strtol(val, &dummy, 10);
    } else {
        mdclog_write(MDCLOG_ERR, "wrong Port for setup request %s", data);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -2;
    }

    val = strtok_r(nullptr, delimiter, &tmp);
    if (val != nullptr) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "RAN NAME parameter from message = %s", val);
        }
        memcpy(message.message.enodbName, val, tmp - val);
    } else {
        mdclog_write(MDCLOG_ERR, "wrong gNb/Enodeb name for setup request %s", data);
#ifdef __TRACING__
        lspan->Finish();
#endif

        return -3;
    }
    val = strtok_r(nullptr, delimiter, &tmp);
    if (val != nullptr) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "ASN length parameter from message = %s", val);
        }
        char *dummy;
        message.message.asnLength = (uint16_t) strtol(val, &dummy, 10);
    } else {
        mdclog_write(MDCLOG_ERR, "wrong ASN length for setup request %s", data);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -4;
    }

    message.message.asndata = (unsigned char *)tmp;  // tmp is local but point to the location in data

    if (loglevel >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Message from Xapp RAN name = %s host address = %s port = %d",
                     message.message.enodbName, host, port);
    }
#ifdef __TRACING__
    lspan->Finish();
#endif

    return 0;
}

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
                        otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    /* We have data on the fd waiting to be read. Read and display it.
 * We must read whatever data is available completely, as we are running
 *  in edge-triggered mode and won't get a notification again for the same data. */
    int done = 0;
    auto loglevel = mdclog_level_get();
    // get the identity of the interface
    auto *peerInfo = (ConnectedCU_t *)events->data.ptr;
    struct timespec start{0, 0};
    struct timespec decodestart{0, 0};
    struct timespec end{0, 0};

    E2AP_PDU_t *pdu = nullptr;

    ReportingMessages_t message {};

    while (true) {
        if (loglevel >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Start Read from SCTP %d fd", peerInfo->fileDescriptor);
            clock_gettime(CLOCK_MONOTONIC, &start);
        }
        // read the buffer directly to rmr payload
        message.message.asndata = rmrMessageBuffer.sendMessage->payload;
        message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                read(peerInfo->fileDescriptor, rmrMessageBuffer.sendMessage->payload, RECEIVE_SCTP_BUFFER_SIZE);
        if (loglevel >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Finish Read from SCTP %d fd message length = %ld",
                    peerInfo->fileDescriptor, message.message.asnLength);
        }
        memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));
        message.message.direction = 'U';
        message.message.time.tv_nsec = ts.tv_nsec;
        message.message.time.tv_sec = ts.tv_sec;

        if (message.message.asnLength < 0) {
            if (errno == EINTR) {
                continue;
            }
            /* If errno == EAGAIN, that means we have read all
               data. So go back to the main loop. */
            if (errno != EAGAIN) {
                mdclog_write(MDCLOG_ERR, "Read error, %s ", strerror(errno));
                done = 1;
            } else if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "EAGAIN - descriptor = %d", peerInfo->fileDescriptor);
            }
            break;
        } else if (message.message.asnLength == 0) {
            /* End of file. The remote has closed the connection. */
            if (loglevel >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "END of File Closed connection - descriptor = %d",
                             peerInfo->fileDescriptor);
            }
            done = 1;
            break;
        }

        asn_dec_rval_t rval;
        if (loglevel >= MDCLOG_DEBUG) {
            char printBuffer[4096]{};
            char *tmp = printBuffer;
            for (size_t i = 0; i < (size_t)message.message.asnLength; ++i) {
                snprintf(tmp, 2, "%02x", message.message.asndata[i]);
                tmp += 2;
            }
            printBuffer[message.message.asnLength] = 0;
            clock_gettime(CLOCK_MONOTONIC, &end);
            mdclog_write(MDCLOG_DEBUG, "Before Encoding E2AP PDU for : %s, Read time is : %ld seconds, %ld nanoseconds",
                         peerInfo->enodbName, end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
            mdclog_write(MDCLOG_DEBUG, "PDU buffer length = %ld, data =  : %s", message.message.asnLength,
                         printBuffer);
            clock_gettime(CLOCK_MONOTONIC, &decodestart);
        }

        rval = asn_decode(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, (void **) &pdu,
                          message.message.asndata, message.message.asnLength);
        if (rval.code != RC_OK) {
            mdclog_write(MDCLOG_ERR, "Error %d Decoding (unpack) E2AP PDU from RAN : %s", rval.code,
                         peerInfo->enodbName);
            break;
        }

        if (loglevel >= MDCLOG_DEBUG) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            mdclog_write(MDCLOG_DEBUG, "After Encoding E2AP PDU for : %s, Read time is : %ld seconds, %ld nanoseconds",
                         peerInfo->enodbName, end.tv_sec - decodestart.tv_sec, end.tv_nsec - decodestart.tv_nsec);
            char *printBuffer;
            size_t size;
            FILE *stream = open_memstream(&printBuffer, &size);
            asn_fprint(stream, &asn_DEF_E2AP_PDU, pdu);
            mdclog_write(MDCLOG_DEBUG, "Encoding E2AP PDU past : %s", printBuffer);
            clock_gettime(CLOCK_MONOTONIC, &decodestart);
        }

        switch (pdu->present) {
            case E2AP_PDU_PR_initiatingMessage: {//initiating message
                asnInitiatingRequest(pdu, message, rmrMessageBuffer, &lspan);
                break;
            }
            case E2AP_PDU_PR_successfulOutcome: { //successful outcome
                asnSuccsesfulMsg(pdu, message, sctpMap, rmrMessageBuffer, &lspan);
                break;
            }
            case E2AP_PDU_PR_unsuccessfulOutcome: { //Unsuccessful Outcome
                asnUnSuccsesfulMsg(pdu, message, sctpMap, rmrMessageBuffer, &lspan);
                break;
            }
            default:
                mdclog_write(MDCLOG_ERR, "Unknown index %d in E2AP PDU", pdu->present);
                break;
        }
        if (loglevel >= MDCLOG_DEBUG) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            mdclog_write(MDCLOG_DEBUG,
                         "After processing message and sent to rmr for : %s, Read time is : %ld seconds, %ld nanoseconds",
                         peerInfo->enodbName, end.tv_sec - decodestart.tv_sec, end.tv_nsec - decodestart.tv_nsec);

        }
        numOfMessages++;
        // remove the break for EAGAIN
        //break;
        if (pdu != nullptr) {
            //TODO need to test ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, pdu); to get better performance
            //ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, pdu);
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        //clock_gettime(CLOCK_MONOTONIC, &start);
    }
    // in case of break to avoid memory leak
    if (pdu != nullptr) {
        ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
        pdu = nullptr;
    }

    if (done) {
        if (loglevel >= MDCLOG_INFO) {
            mdclog_write(MDCLOG_INFO, "Closed connection - descriptor = %d", peerInfo->fileDescriptor);
        }
        message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                snprintf((char *)rmrMessageBuffer.sendMessage->payload,
                        256,
                        "%s|CU disconnected unexpectedly",
                        peerInfo->enodbName);
        message.message.asndata = rmrMessageBuffer.sendMessage->payload;

        if (sendRequestToXapp(message,
                              RIC_SCTP_CONNECTION_FAILURE,
                              rmrMessageBuffer,
                              &lspan) != 0) {
            mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
        }

        /* Closing descriptor make epoll remove it from the set of descriptors which are monitored. */
        close(peerInfo->fileDescriptor);
        cleanHashEntry((ConnectedCU_t *) events->data.ptr, sctpMap, &lspan);
    }
    if (loglevel >= MDCLOG_DEBUG) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        mdclog_write(MDCLOG_DEBUG, "from receive SCTP to send RMR time is %ld seconds and %ld nanoseconds",
                     end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    }
#ifdef __TRACING__
    lspan->Finish();
#endif

    return 0;
}

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
                          otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif

    auto procedureCode = ((InitiatingMessage_t *) pdu->choice.initiatingMessage)->procedureCode;
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Initiating message %ld", procedureCode);
    }
    switch (procedureCode) {
        case ProcedureCode_id_x2Setup: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Setup Initiating  message from CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_endcX2Setup: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got X2 EN-DC Setup Request from CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_ricSubscription: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got RIC Subscription Request message from CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_ricSubscriptionDelete: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got RIC Subscription Delete Request message from CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_endcConfigurationUpdate: {
            if (sendRequestToXapp(message, RIC_ENDC_CONF_UPDATE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "E2 EN-DC CONFIGURATION UPDATE message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_eNBConfigurationUpdate: {
            if (sendRequestToXapp(message, RIC_ENB_CONF_UPDATE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "E2 EN-BC CONFIGURATION UPDATE message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_x2Removal: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got E2 Removal Initiating  message from CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_loadIndication: {
            if (sendRequestToXapp(message, RIC_ENB_LOAD_INFORMATION, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Load indication message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_resourceStatusReportingInitiation: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Status reporting initiation message from CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_resourceStatusReporting: {
            if (sendRequestToXapp(message, RIC_RESOURCE_STATUS_UPDATE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Resource Status Reporting message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_reset: {
            if (sendRequestToXapp(message, RIC_X2_RESET, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "RIC_X2_RESET message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_ricIndication: {
            for (int i = 0; i < pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count; i++) {
                auto messageSent = false;
                RICindication_IEs_t *ie = pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[i];
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "ie type (ProtocolIE_ID) = %ld", ie->id);
                }
                if (ie->id == ProtocolIE_ID_id_RICrequestID) {
                    if (mdclog_level_get() >= MDCLOG_DEBUG) {
                        mdclog_write(MDCLOG_DEBUG, "Got RIC requestId entry, ie type (ProtocolIE_ID) = %ld", ie->id);
                    }
                    if (ie->value.present == RICindication_IEs__value_PR_RICrequestID) {
                        static unsigned char tx[32];
                        message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_INDICATION;
                        snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
                        rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, strlen((const char *) tx));
    			rmr_bytes2meid(rmrMessageBuffer.sendMessage, (unsigned char *)message.message.enodbName, strlen(message.message.enodbName));
                        rmrMessageBuffer.sendMessage->state = 0;
                        rmrMessageBuffer.sendMessage->sub_id = (int) ie->value.choice.RICrequestID.ricRequestorID;
                        sendRmrMessage(rmrMessageBuffer, message, &lspan);
                        messageSent = true;
                    } else {
                        mdclog_write(MDCLOG_ERR, "RIC request id missing illigal request");
                    }
                }
                if (messageSent) {
                    break;
                }
            }
            break;
        }
        case ProcedureCode_id_errorIndication: {
            if (sendRequestToXapp(message, RIC_ERROR_INDICATION, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Error Indication message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_ricServiceUpdate : {
            if (sendRequestToXapp(message, RIC_SERVICE_UPDATE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Service Update message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_gNBStatusIndication : {
            if (sendRequestToXapp(message, RIC_GNB_STATUS_INDICATION, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "RIC_GNB_STATUS_INDICATION failed to send to xAPP");
            }
            break;
        }
        default: {
            mdclog_write(MDCLOG_ERR, "Undefined or not supported message = %ld", procedureCode);
            message.message.messageType = 0; // no RMR message type yet

            buildJsonMessage(message);

            break;
        }
    }
#ifdef __TRACING__
    lspan->Finish();
#endif

}

/**
 *
 * @param pdu
 * @param message
 * @param sctpMap
 * @param rmrMessageBuffer
 * @param pSpan
 */
void asnSuccsesfulMsg(E2AP_PDU_t *pdu, ReportingMessages_t &message, Sctp_Map_t *sctpMap,
                      RmrMessagesBuffer_t &rmrMessageBuffer, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    auto procedureCode = pdu->choice.successfulOutcome->procedureCode;
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Successful Outcome %ld", procedureCode);
    }
    switch (procedureCode) {
        case ProcedureCode_id_x2Setup: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Succesful Setup response from CU - %s",
                             message.message.enodbName);
            }
            if (sendResponseToXapp(message, RIC_X2_SETUP_RESP,
                                   RIC_X2_SETUP_REQ, rmrMessageBuffer, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Succesful Setup response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_endcX2Setup: { //X2_EN_DC_SETUP_REQUEST_FROM_CU
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Succesful E2 EN-DC Setup response from CU - %s",
                             message.message.enodbName);
            }
            if (sendResponseToXapp(message, RIC_ENDC_X2_SETUP_RESP,
                                   RIC_ENDC_X2_SETUP_REQ, rmrMessageBuffer, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Succesful X2 EN DC Setup response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_endcConfigurationUpdate: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Succesful E2 EN-DC CONFIGURATION UPDATE from CU - %s",
                             message.message.enodbName);
            }
            if (sendRequestToXapp(message, RIC_ENDC_CONF_UPDATE_ACK, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Succesful E2 EN DC CONFIGURATION response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_eNBConfigurationUpdate: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Succesful E2 ENB CONFIGURATION UPDATE from CU - %s",
                             message.message.enodbName);
            }
            if (sendRequestToXapp(message, RIC_ENB_CONF_UPDATE_ACK, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Succesful E2 ENB CONFIGURATION response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_reset: {
            if (sendRequestToXapp(message, RIC_X2_RESET_RESP, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Succesful E2_RESET response for CU - %s",
                             message.message.enodbName);
            }
            break;

        }
        case ProcedureCode_id_resourceStatusReportingInitiation: {
            if (sendRequestToXapp(message, RIC_RES_STATUS_RESP, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR,
                             "Failed to send Succesful 2_REQUEST_STATUS_REPORTING_INITIATION response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_ricSubscription: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Succesful RIC Subscription response from CU - %s",
                             message.message.enodbName);
            }
            if (sendRequestToXapp(message, RIC_SUB_RESP, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription successful message failed to send to xAPP");
            }
            break;

        }
        case ProcedureCode_id_ricSubscriptionDelete: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO,
                             "Got Succesful RIC Subscription Delete response from CU - %s",
                             message.message.enodbName);
            }
            if (sendRequestToXapp(message, RIC_SUB_DEL_RESP, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription delete successful message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_ricControl: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO,
                             "Got Succesful RIC control response from CU - %s",
                             message.message.enodbName);
            }
            for (int i = 0;
                 i < pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.count; i++) {
                auto messageSent = false;
                RICcontrolAcknowledge_IEs_t *ie = pdu->choice.successfulOutcome->value.choice.RICcontrolAcknowledge.protocolIEs.list.array[i];
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "ie type (ProtocolIE_ID) = %ld", ie->id);
                }
                if (ie->id == ProtocolIE_ID_id_RICrequestID) {
                    if (mdclog_level_get() >= MDCLOG_DEBUG) {
                        mdclog_write(MDCLOG_DEBUG, "Got RIC requestId entry, ie type (ProtocolIE_ID) = %ld", ie->id);
                    }
                    if (ie->value.present == RICcontrolAcknowledge_IEs__value_PR_RICrequestID) {
                        message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_CONTROL_ACK;
                        rmrMessageBuffer.sendMessage->state = 0;
                        rmrMessageBuffer.sendMessage->sub_id = (int) ie->value.choice.RICrequestID.ricRequestorID;
                        static unsigned char tx[32];
                        snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
                        rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, strlen((const char *) tx));
                        rmr_bytes2meid(rmrMessageBuffer.sendMessage,
                                (unsigned char *)message.message.enodbName,
                                strlen(message.message.enodbName));

                        sendRmrMessage(rmrMessageBuffer, message, &lspan);
                        messageSent = true;
                    } else {
                        mdclog_write(MDCLOG_ERR, "RIC request id missing illigal request");
                    }
                }
                if (messageSent) {
                    break;
                }
            }
            break;
        }
        default: {
            mdclog_write(MDCLOG_WARN, "Undefined or not supported message = %ld", procedureCode);
            message.message.messageType = 0; // no RMR message type yet
            buildJsonMessage(message);

            break;
        }
    }
#ifdef __TRACING__
    lspan->Finish();
#endif

}

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
                        otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    auto procedureCode = pdu->choice.unsuccessfulOutcome->procedureCode;
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Unsuccessful Outcome %ld", procedureCode);
    }
    switch (procedureCode) {
        case ProcedureCode_id_x2Setup: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO,
                             "Got Unsuccessful Setup response from CU - %s",
                             message.message.enodbName);
            }
            if (sendResponseToXapp(message,
                                   RIC_X2_SETUP_FAILURE, RIC_X2_SETUP_REQ,
                                   rmrMessageBuffer,
                                   sctpMap,
                                   &lspan) != 0) {
                mdclog_write(MDCLOG_ERR,
                             "Failed to send Unsuccessful Setup response for CU - %s",
                             message.message.enodbName);
                break;
            }
            break;
        }
        case ProcedureCode_id_endcX2Setup: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO,
                             "Got Unsuccessful E2 EN-DC Setup response from CU - %s",
                             message.message.enodbName);
            }
            if (sendResponseToXapp(message, RIC_ENDC_X2_SETUP_FAILURE,
                                   RIC_ENDC_X2_SETUP_REQ,
                                   rmrMessageBuffer,
                                   sctpMap,
                                   &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Unsuccessful E2 EN DC Setup response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_endcConfigurationUpdate: {
            if (sendRequestToXapp(message, RIC_ENDC_CONF_UPDATE_FAILURE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Unsuccessful E2 EN DC CONFIGURATION response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_eNBConfigurationUpdate: {
            if (sendRequestToXapp(message, RIC_ENB_CONF_UPDATE_FAILURE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send Unsuccessful E2 ENB CONFIGURATION response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_resourceStatusReportingInitiation: {
            if (sendRequestToXapp(message, RIC_RES_STATUS_FAILURE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR,
                             "Failed to send Succesful E2_REQUEST_STATUS_REPORTING_INITIATION response for CU - %s",
                             message.message.enodbName);
            }
            break;
        }
        case ProcedureCode_id_ricSubscription: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Unsuccessful RIC Subscription Response from CU - %s",
                             message.message.enodbName);
            }
            if (sendRequestToXapp(message, RIC_SUB_FAILURE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription unsuccessful message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_ricSubscriptionDelete: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Unsuccessful RIC Subscription Delete Response from CU - %s",
                             message.message.enodbName);
            }
            if (sendRequestToXapp(message, RIC_SUB_DEL_FAILURE, rmrMessageBuffer, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription Delete unsuccessful message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_ricControl: {
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got UNSuccesful RIC control response from CU - %s",
                             message.message.enodbName);
            }
            for (int i = 0;
                 i < pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.count; i++) {
                auto messageSent = false;
                RICcontrolFailure_IEs_t *ie = pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[i];
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "ie type (ProtocolIE_ID) = %ld", ie->id);
                }
                if (ie->id == ProtocolIE_ID_id_RICrequestID) {
                    if (mdclog_level_get() >= MDCLOG_DEBUG) {
                        mdclog_write(MDCLOG_DEBUG, "Got RIC requestId entry, ie type (ProtocolIE_ID) = %ld", ie->id);
                    }
                    if (ie->value.present == RICcontrolFailure_IEs__value_PR_RICrequestID) {
                        message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_CONTROL_FAILURE;
                        rmrMessageBuffer.sendMessage->state = 0;
                        rmrMessageBuffer.sendMessage->sub_id = (int) ie->value.choice.RICrequestID.ricRequestorID;
                        static unsigned char tx[32];
                        snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
                        rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, strlen((const char *) tx));
    			rmr_bytes2meid(rmrMessageBuffer.sendMessage, (unsigned char *)message.message.enodbName, strlen(message.message.enodbName));
                        sendRmrMessage(rmrMessageBuffer, message, &lspan);
                        messageSent = true;
                    } else {
                        mdclog_write(MDCLOG_ERR, "RIC request id missing illigal request");
                    }
                }
                if (messageSent) {
                    break;
                }
            }
            break;
        }
        default: {
            mdclog_write(MDCLOG_WARN, "Undefined or not supported message = %ld", procedureCode);
            message.message.messageType = 0; // no RMR message type yet

            buildJsonMessage(message);

            break;
        }
    }
#ifdef __TRACING__
    lspan->Finish();
#endif

}

/**
 *
 * @param message
 * @param requestId
 * @param rmrMmessageBuffer
 * @param pSpan
 * @return
 */
int sendRequestToXapp(ReportingMessages_t &message,
                      int requestId,
                      RmrMessagesBuffer_t &rmrMmessageBuffer,
                      otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    rmr_bytes2meid(rmrMmessageBuffer.sendMessage,
                   (unsigned char *)message.message.enodbName,
                   strlen(message.message.enodbName));
    message.message.messageType = rmrMmessageBuffer.sendMessage->mtype = requestId;
    rmrMmessageBuffer.sendMessage->state = 0;
    static unsigned char tx[32];
    snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
    rmr_bytes2xact(rmrMmessageBuffer.sendMessage, tx, strlen((const char *) tx));

    auto rc = sendRmrMessage(rmrMmessageBuffer, message, &lspan);
#ifdef __TRACING__
    lspan->Finish();
#endif

    return rc;
}


void getRmrContext(sctp_params_t &pSctpParams, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
//    otSpan lspan = 0;
#endif
    pSctpParams.rmrCtx = nullptr;
    pSctpParams.rmrCtx = rmr_init(pSctpParams.rmrAddress, RMR_MAX_RCV_BYTES, RMRFL_NONE);
    if (pSctpParams.rmrCtx == nullptr) {
        mdclog_write(MDCLOG_ERR, "Failed to initialize RMR");
#ifdef __TRACING__
        lspan->Finish();
#endif
        return;
    }

    rmr_set_stimeout(pSctpParams.rmrCtx, 0);    // disable retries for any send operation
    // we need to find that routing table exist and we can run
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "We are after RMR INIT wait for RMR_Ready");
    }
    int rmrReady = 0;
    int count = 0;
    while (!rmrReady) {
        if ((rmrReady = rmr_ready(pSctpParams.rmrCtx)) == 0) {
            sleep(1);
        }
        count++;
        if (count % 60 == 0) {
            mdclog_write(MDCLOG_INFO, "waiting to RMR ready state for %d seconds", count);
        }
    }
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "RMR running");
    }
#ifdef __TRACING__
    lspan->Finish();
#endif
    rmr_init_trace(pSctpParams.rmrCtx, 200);
    // get the RMR fd for the epoll
    pSctpParams.rmrListenFd = rmr_get_rcvfd(pSctpParams.rmrCtx);
    struct epoll_event event{};
    // add RMR fd to epoll
    event.events = (EPOLLIN);
    event.data.fd = pSctpParams.rmrListenFd;
    // add listening RMR FD to epoll
    if (epoll_ctl(pSctpParams.epoll_fd, EPOLL_CTL_ADD, pSctpParams.rmrListenFd, &event)) {
        mdclog_write(MDCLOG_ERR, "Failed to add RMR descriptor to epoll");
        close(pSctpParams.rmrListenFd);
        rmr_close(pSctpParams.rmrCtx);
        pSctpParams.rmrCtx = nullptr;
    }
}

/**
 *
 * @param epoll_fd
 * @param sctpMap
 * @param rmrMessageBuffer
 * @param ts
 * @param pSpan
 * @return
 */
int receiveXappMessages(int epoll_fd,
                        Sctp_Map_t *sctpMap,
                        RmrMessagesBuffer_t &rmrMessageBuffer,
                        struct timespec &ts,
                        otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    if (rmrMessageBuffer.rcvMessage == nullptr) {
        //we have error
        mdclog_write(MDCLOG_ERR, "RMR Allocation message, %s", strerror(errno));
#ifdef __TRACING__
        lspan->Finish();
#endif

        return -1;
    }

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Call to rmr_rcv_msg");
    }
    rmrMessageBuffer.rcvMessage = rmr_rcv_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.rcvMessage);
    if (rmrMessageBuffer.rcvMessage == nullptr) {
        mdclog_write(MDCLOG_ERR, "RMR Receving message with null pointer, Realloc rmr mesage buffer");
        rmrMessageBuffer.rcvMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
#ifdef __TRACING__
        lspan->Finish();
#endif

        return -2;
    }
    ReportingMessages_t message;
    message.message.direction = 'D';
    message.message.time.tv_nsec = ts.tv_nsec;
    message.message.time.tv_sec = ts.tv_sec;

    // get message payload
    //auto msgData = msg->payload;
    if (rmrMessageBuffer.rcvMessage->state != 0) {
        mdclog_write(MDCLOG_ERR, "RMR Receving message with stat = %d", rmrMessageBuffer.rcvMessage->state);
#ifdef __TRACING__
        lspan->Finish();
#endif

        return -1;
    }
    switch (rmrMessageBuffer.rcvMessage->mtype) {
        case RIC_X2_SETUP_REQ: {
            if (connectToCUandSetUp(rmrMessageBuffer, message, epoll_fd, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "ERROR in connectToCUandSetUp on RIC_X2_SETUP_REQ");
                message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_SCTP_CONNECTION_FAILURE;
                message.message.direction = 'N';
                message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                        snprintf((char *)rmrMessageBuffer.sendMessage->payload,
                                256,
                                "ERROR in connectToCUandSetUp on RIC_X2_SETUP_REQ");
                rmrMessageBuffer.sendMessage->state = 0;
                message.message.asndata = rmrMessageBuffer.sendMessage->payload;

                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "start writing to rmr buffer");
                }
                rmr_bytes2xact(rmrMessageBuffer.sendMessage, rmrMessageBuffer.rcvMessage->xaction, RMR_MAX_XID);
                rmr_str2meid(rmrMessageBuffer.sendMessage, (unsigned char *)message.message.enodbName);

                sendRmrMessage(rmrMessageBuffer, message, &lspan);
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -3;
            }
            break;
        }
        case RIC_ENDC_X2_SETUP_REQ: {
            if (connectToCUandSetUp(rmrMessageBuffer, message, epoll_fd, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "ERROR in connectToCUandSetUp on RIC_ENDC_X2_SETUP_REQ");
                message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_SCTP_CONNECTION_FAILURE;
                message.message.direction = 'N';
                message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                        snprintf((char *)rmrMessageBuffer.sendMessage->payload, 256,
                                 "ERROR in connectToCUandSetUp on RIC_ENDC_X2_SETUP_REQ");
                rmrMessageBuffer.sendMessage->state = 0;
                message.message.asndata = rmrMessageBuffer.sendMessage->payload;

                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "start writing to rmr buffer");
                }

                rmr_bytes2xact(rmrMessageBuffer.sendMessage, rmrMessageBuffer.rcvMessage->xaction, RMR_MAX_XID);
                rmr_str2meid(rmrMessageBuffer.sendMessage, (unsigned char *) message.message.enodbName);

                sendRmrMessage(rmrMessageBuffer, message, &lspan);
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -3;
            }
            break;
        }
        case RIC_ENDC_CONF_UPDATE: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ENDC_CONF_UPDATE");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -4;
            }
            break;
        }
        case RIC_ENDC_CONF_UPDATE_ACK: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ENDC_CONF_UPDATE_ACK");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -4;
            }
            break;
        }
        case RIC_ENDC_CONF_UPDATE_FAILURE: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ENDC_CONF_UPDATE_FAILURE");
#ifdef __TRACING__
                lspan->Finish();
#endif

                return -4;
            }
            break;
        }
        case RIC_ENB_CONF_UPDATE: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ENDC_CONF_UPDATE");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -4;
            }
            break;
        }
        case RIC_ENB_CONF_UPDATE_ACK: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ENB_CONF_UPDATE_ACK");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -4;
            }
            break;
        }
        case RIC_ENB_CONF_UPDATE_FAILURE: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ENB_CONF_UPDATE_FAILURE");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -4;
            }
            break;
        }
        case RIC_RES_STATUS_REQ: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_RES_STATUS_REQ");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_SUB_REQ: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SUB_REQ");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_SUB_DEL_REQ: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SUB_DEL_REQ");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_CONTROL_REQ: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_CONTROL_REQ");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_SERVICE_QUERY: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SERVICE_QUERY");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_SERVICE_UPDATE_ACK: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SERVICE_UPDATE_ACK");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_SERVICE_UPDATE_FAILURE: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SERVICE_UPDATE_FAILURE");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_X2_RESET: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_X2_RESET");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_X2_RESET_RESP: {
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap, &lspan) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_X2_RESET_RESP");
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -6;
            }
            break;
        }
        case RIC_SCTP_CLEAR_ALL: {
            mdclog_write(MDCLOG_INFO, "RIC_SCTP_CLEAR_ALL");
            // loop on all keys and close socket and then erase all map.
            vector<char *> v;
            sctpMap->getKeys(v);
            for (auto const &iter : v) { //}; iter != sctpMap.end(); iter++) {
                if (!boost::starts_with((string) (iter), "host:") && !boost::starts_with((string) (iter), "msg:")) {
                    auto *peerInfo = (ConnectedCU_t *) sctpMap->find(iter);
                    if (peerInfo == nullptr) {
                        continue;
                    }
                    close(peerInfo->fileDescriptor);
                    memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));
                    message.message.direction = 'D';
                    message.message.time.tv_nsec = ts.tv_nsec;
                    message.message.time.tv_sec = ts.tv_sec;

                    message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                            snprintf((char *)rmrMessageBuffer.sendMessage->payload,
                                                                   256,
                                                                   "%s|RIC_SCTP_CLEAR_ALL",
                                                                   peerInfo->enodbName);
                    message.message.asndata = rmrMessageBuffer.sendMessage->payload;
                    mdclog_write(MDCLOG_INFO, "%s", message.message.asndata);
                    if (sendRequestToXapp(message,
                                          RIC_SCTP_CONNECTION_FAILURE, rmrMessageBuffer, &lspan) != 0) {
                        mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
                    }
                    free(peerInfo);
                }
            }

            sleep(1);
            sctpMap->clear();
            break;
        }
        case E2_TERM_KEEP_ALIVE_REQ: {
            // send message back
            if (mdclog_level_get() >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got Keep Alive Request send : %s", rmrMessageBuffer.ka_message);
            }
            rmr_bytes2payload(rmrMessageBuffer.sendMessage,
                    (unsigned char *)rmrMessageBuffer.ka_message,
                    rmrMessageBuffer.ka_message_len);
            rmrMessageBuffer.sendMessage->mtype = E2_TERM_KEEP_ALIVE_RESP;
            rmrMessageBuffer.sendMessage->state = 0;
            static unsigned char tx[32];
            auto txLen = snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
            rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, txLen);
            rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);
            if (rmrMessageBuffer.sendMessage == nullptr) {
                rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
                mdclog_write(MDCLOG_ERR, "Failed to send E2_TERM_KEEP_ALIVE_RESP");
            } else if (rmrMessageBuffer.sendMessage->state != 0)  {
                mdclog_write(MDCLOG_ERR, "Failed to send E2_TERM_KEEP_ALIVE_RESP");
            }
            break;
        }
        default:
            mdclog_write(MDCLOG_WARN, "Message Type : %d is not seported", rmrMessageBuffer.rcvMessage->mtype);
            message.message.asndata = rmrMessageBuffer.rcvMessage->payload;
            message.message.asnLength = rmrMessageBuffer.rcvMessage->len;
            message.message.time.tv_nsec = ts.tv_nsec;
            message.message.time.tv_sec = ts.tv_sec;
            message.message.messageType = rmrMessageBuffer.rcvMessage->mtype;

            buildJsonMessage(message);


#ifdef __TRACING__
            lspan->Finish();
#endif
            return -7;
    }
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "EXIT OK from %s", __FUNCTION__);
    }
#ifdef __TRACING__
    lspan->Finish();
#endif
    return 0;
}

/**
 * Send message to the CU that is not expecting for successful or unsuccessful results
 * @param messageBuffer
 * @param message
 * @param failedMsgId
 * @param sctpMap
 * @param pSpan
 * @return
 */
int sendDirectionalSctpMsg(RmrMessagesBuffer_t &messageBuffer,
                           ReportingMessages_t &message,
                           int failedMsgId,
                           Sctp_Map_t *sctpMap,
                           otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif

    getRequestMetaData(message, messageBuffer, &lspan);
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "send message to %s address", message.message.enodbName);
    }

    auto rc = sendMessagetoCu(sctpMap, messageBuffer, message, failedMsgId, &lspan);
#ifdef __TRACING__
    lspan->Finish();
#endif

    return rc;
}

/**
 *
 * @param sctpMap
 * @param messageBuffer
 * @param message
 * @param failedMesgId
 * @param pSpan
 * @return
 */
int sendMessagetoCu(Sctp_Map_t *sctpMap,
                    RmrMessagesBuffer_t &messageBuffer,
                    ReportingMessages_t &message,
                    int failedMesgId,
                    otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    auto *peerInfo = (ConnectedCU_t *) sctpMap->find(message.message.enodbName);
    if (peerInfo == nullptr) {
        if (failedMesgId != 0) {
            sendFailedSendingMessagetoXapp(messageBuffer, message, failedMesgId, &lspan);
        } else {
            mdclog_write(MDCLOG_ERR, "Failed to send message no CU entry %s", message.message.enodbName);
        }
#ifdef __TRACING__
        lspan->Finish();
#endif

        return -1;
    }

    // get the FD
    message.message.messageType = messageBuffer.rcvMessage->mtype;
    auto rc = sendSctpMsg(peerInfo, message, sctpMap, &lspan);
#ifdef __TRACING__
    lspan->Finish();
#endif

    return rc;
}

/**
 *
 * @param rmrCtx the rmr context to send and receive
 * @param msg the msg we got fromxApp
 * @param metaData data from xApp in ordered struct
 * @param failedMesgId the return message type error
 */
void
sendFailedSendingMessagetoXapp(RmrMessagesBuffer_t &rmrMessageBuffer, ReportingMessages_t &message, int failedMesgId,
                               otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    rmr_mbuf_t *msg = rmrMessageBuffer.sendMessage;
    msg->len = snprintf((char *) msg->payload, 200, "the gNb/eNode name %s not found",
                        message.message.enodbName);
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "%s", msg->payload);
    }
    msg->mtype = failedMesgId;
    msg->state = 0;

    static unsigned char tx[32];
    snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
    rmr_bytes2xact(msg, tx, strlen((const char *) tx));

    sendRmrMessage(rmrMessageBuffer, message, &lspan);
#ifdef __TRACING__
    lspan->Finish();pLogSink
#endif

}

/**
 * Send Response back to xApp, message is used only when there was a request from the xApp
 *
 * @param enodbName the name of the gNb/eNodeB
 * @param msgType  the value of the message to the xApp
 * @param requestType The request that was sent by the xAPP
 * @param rmrCtx the rmr identifier
 * @param sctpMap hash map holds data on the requestrs
 * @param buf  the buffer to send to xAPP
 * @param size size of the buffer to send
 * @return
 */
int sendResponseToXapp(ReportingMessages_t &message,
                       int msgType,
                       int requestType,
                       RmrMessagesBuffer_t &rmrMessageBuffer,
                       Sctp_Map_t *sctpMap,
                       otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    char key[MAX_ENODB_NAME_SIZE * 2];
    snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", message.message.enodbName, requestType);

    auto xact = sctpMap->find(key);
    if (xact == nullptr) {
        mdclog_write(MDCLOG_ERR, "NO Request %s found for this response from CU: %s", key,
                     message.message.enodbName);
#ifdef __TRACING__
        lspan->Finish();
#endif

        return -1;
    }
    sctpMap->erase(key);

    message.message.messageType = rmrMessageBuffer.sendMessage->mtype = msgType; //SETUP_RESPONSE_MESSAGE_TYPE;
    rmr_bytes2payload(rmrMessageBuffer.sendMessage, (unsigned char *) message.message.asndata,
                      message.message.asnLength);
    rmr_bytes2xact(rmrMessageBuffer.sendMessage, (const unsigned char *)xact, strlen((const char *)xact));
    rmr_str2meid(rmrMessageBuffer.sendMessage, (unsigned char *) message.message.enodbName);
    rmrMessageBuffer.sendMessage->state = 0;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "remove key = %s from %s at line %d", key, __FUNCTION__, __LINE__);
    }
    free(xact);

    auto rc = sendRmrMessage(rmrMessageBuffer, message, &lspan);
#ifdef __TRACING__
    lspan->Finish();
#endif
    return rc;
}

/**
 * build the SCTP connection to eNodB or gNb
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
                        otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    struct sockaddr_in6 servaddr{};
    struct addrinfo hints{}, *result;
    auto msgData = rmrMessageBuffer.rcvMessage->payload;
    unsigned char meid[RMR_MAX_MEID]{};
    char host[256]{};
    uint16_t port = 0;

    message.message.messageType = rmrMessageBuffer.rcvMessage->mtype;
    rmr_mbuf_t *msg = rmrMessageBuffer.rcvMessage;
    rmr_get_meid(msg, meid);

    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "message %d Received for MEID :%s. SETUP/EN-DC Setup Request from xApp, Message = %s",
                     msg->mtype, meid, msgData);
    }
    if (getSetupRequestMetaData(message, (char *)msgData, host, port, &lspan) < 0) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Error in setup parameters %s, %d", __func__, __LINE__);
        }
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    //// message asndata points to the start of the asndata of the message and not to start of payload
    // search if the same host:port but not the same enodbname
    char searchBuff[256]{};
    snprintf(searchBuff, sizeof searchBuff, "host:%s:%d", host, port);
    auto e = (char *)sctpMap->find(searchBuff);
    if (e != nullptr) {
        // found one compare if not the same
        if (strcmp(message.message.enodbName, e) != 0) {
            mdclog_write(MDCLOG_ERR,
                         "Try to connect CU %s to Host %s but %s already connected",
                         message.message.enodbName, host, e);
#ifdef __TRACING__
            lspan->Finish();
#endif
            return -1;
        }
    }

    // check if not alread connected. if connected send the request and return
    auto *peerInfo = (ConnectedCU_t *)sctpMap->find(message.message.enodbName);
    if (peerInfo != nullptr) {
//        snprintf(strErr,
//                128,
//                "Device %s already connected please remove and then setup again",
//                message.message.enodbName);
        if (mdclog_level_get() >= MDCLOG_INFO) {
            mdclog_write(MDCLOG_INFO,
                         "Device already connected to %s",
                         message.message.enodbName);
        }
        message.message.messageType = msg->mtype;
        auto rc = sendSctpMsg(peerInfo, message, sctpMap, &lspan);
        if (rc != 0) {
            mdclog_write(MDCLOG_ERR, "failed write to SCTP %s, %d", __func__, __LINE__);
#ifdef __TRACING__
            lspan->Finish();
#endif
            return -1;
        }

        char key[MAX_ENODB_NAME_SIZE * 2];
        snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", message.message.enodbName, msg->mtype);
        int xaction_len = strlen((const char *) msg->xaction);
        auto *xaction = (unsigned char *) calloc(1, xaction_len);
        memcpy(xaction, msg->xaction, xaction_len);
        sctpMap->setkey(key, xaction);
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "set key = %s from %s at line %d", key, __FUNCTION__, __LINE__);
        }
#ifdef __TRACING__
        lspan->Finish();
#endif
        return 0;
    }

    peerInfo = (ConnectedCU_t *) calloc(1, sizeof(ConnectedCU_t));
    memcpy(peerInfo->enodbName, message.message.enodbName, sizeof(message.message.enodbName));

    // new connection
    if ((peerInfo->fileDescriptor = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
        mdclog_write(MDCLOG_ERR, "Socket Error, %s %s, %d", strerror(errno), __func__, __LINE__);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    auto optval = 1;
    if (setsockopt(peerInfo->fileDescriptor, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval) != 0) {
        mdclog_write(MDCLOG_ERR, "setsockopt SO_REUSEPORT Error, %s %s, %d", strerror(errno), __func__, __LINE__);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }
    optval = 1;
    if (setsockopt(peerInfo->fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0) {
        mdclog_write(MDCLOG_ERR, "setsockopt SO_REUSEADDR Error, %s %s, %d", strerror(errno), __func__, __LINE__);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }
    servaddr.sin6_family = AF_INET6;

    struct sockaddr_in6 localAddr {};
    localAddr.sin6_family = AF_INET6;
    localAddr.sin6_addr = in6addr_any;
    localAddr.sin6_port = htons(SRC_PORT);

    if (bind(peerInfo->fileDescriptor, (struct sockaddr*)&localAddr , sizeof(struct sockaddr_in6)) < 0) {
        mdclog_write(MDCLOG_ERR, "bind Socket Error, %s %s, %d", strerror(errno), __func__, __LINE__);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }//Ends the binding.

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_NUMERICHOST;
    if (getaddrinfo(host, nullptr, &hints, &result) < 0) {
        close(peerInfo->fileDescriptor);
        mdclog_write(MDCLOG_ERR, "getaddrinfo error for %s, Error = %s", host, strerror(errno));
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }
    memcpy(&servaddr, result->ai_addr, sizeof(struct sockaddr_in6));
    freeaddrinfo(result);

    servaddr.sin6_port = htons(port);      /* daytime server */
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Send Connect FD = %d host : %s port %d",
                     peerInfo->fileDescriptor,
                     host,
                     port);
    }

    // Add to Epol
    if (addToEpoll(epoll_fd, peerInfo, (EPOLLOUT | EPOLLIN | EPOLLET), sctpMap, message.message.enodbName,
                   msg->mtype, &lspan) != 0) {
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    char hostBuff[NI_MAXHOST];
    char portBuff[NI_MAXHOST];

    if (getnameinfo((SA *) &servaddr, sizeof(servaddr),
                    hostBuff, sizeof(hostBuff),
                    portBuff, sizeof(portBuff),
                    (uint) (NI_NUMERICHOST) | (uint) (NI_NUMERICSERV)) != 0) {
        mdclog_write(MDCLOG_ERR, "getnameinfo() Error, %s  %s %d", strerror(errno), __func__, __LINE__);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    if (setSocketNoBlocking(peerInfo->fileDescriptor) != 0) {
        mdclog_write(MDCLOG_ERR, "setSocketNoBlocking failed to set new connection %s on sctpPort %s", hostBuff,
                     portBuff);
        close(peerInfo->fileDescriptor);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    memcpy(peerInfo->hostName, hostBuff, strlen(hostBuff));
    peerInfo->hostName[strlen(hostBuff)] = 0;
    memcpy(peerInfo->portNumber, portBuff, strlen(portBuff));
    peerInfo->portNumber[strlen(portBuff)] = 0;

    // map by enoodb/gnb name
    sctpMap->setkey(message.message.enodbName, peerInfo);
    //map host and port to enodeb
    sctpMap->setkey(searchBuff, message.message.enodbName);

    // save message for the return values
    char key[MAX_ENODB_NAME_SIZE * 2];
    snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", message.message.enodbName, msg->mtype);
    int xaction_len = strlen((const char *) msg->xaction);
    auto *xaction = (unsigned char *) calloc(1, xaction_len);
    memcpy(xaction, msg->xaction, xaction_len);
    sctpMap->setkey(key, xaction);
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "End building peerinfo: %s for CU %s", key, message.message.enodbName);
    }

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Send connect to FD %d, %s, %d",
                     peerInfo->fileDescriptor, __func__, __LINE__);
    }
    if (connect(peerInfo->fileDescriptor, (SA *) &servaddr, sizeof(servaddr)) < 0) {
        if (errno != EINPROGRESS) {
            mdclog_write(MDCLOG_ERR, "connect FD %d to host : %s port %d, %s",
                         peerInfo->fileDescriptor, host, port, strerror(errno));
            close(peerInfo->fileDescriptor);
#ifdef __TRACING__
            lspan->Finish();
#endif
            return -1;
        }
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG,
                         "Connect to FD %d returned with EINPROGRESS : %s",
                         peerInfo->fileDescriptor, strerror(errno));
        }
        // since message.message.asndata is pointing to the asndata in the rmr message payload we copy it like this
        memcpy(peerInfo->asnData, message.message.asndata, message.message.asnLength);
        peerInfo->asnLength = message.message.asnLength;
        peerInfo->mtype = msg->mtype;
#ifdef __TRACING__
        lspan->Finish();
#endif
        return 0;
    }

    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Connect to FD %d returned OK without EINPROGRESS", peerInfo->fileDescriptor);
    }

    peerInfo->isConnected = true;

    if (modifyToEpoll(epoll_fd, peerInfo, (EPOLLIN | EPOLLET), sctpMap, message.message.enodbName, msg->mtype,
                      &lspan) != 0) {
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Connected to host : %s port %d", host, port);
    }

    message.message.messageType = msg->mtype;
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Send SCTP message to FD %d", peerInfo->fileDescriptor);
    }
    if (sendSctpMsg(peerInfo, message, sctpMap, &lspan) != 0) {
        mdclog_write(MDCLOG_ERR, "Error write to SCTP  %s %d", __func__, __LINE__);
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }
    memset(peerInfo->asnData, 0, message.message.asnLength);
    peerInfo->asnLength = 0;
    peerInfo->mtype = 0;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Sent message to SCTP for %s", message.message.enodbName);
    }
#ifdef __TRACING__
    lspan->Finish();
#endif
    return 0;
}

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
int addToEpoll(int epoll_fd,
               ConnectedCU_t *peerInfo,
               uint32_t events,
               Sctp_Map_t *sctpMap,
               char *enodbName,
               int msgType,
               otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    // Add to Epol
    struct epoll_event event{};
    event.data.ptr = peerInfo;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, peerInfo->fileDescriptor, &event) < 0) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "epoll_ctl EPOLL_CTL_ADD (may chack not to quit here), %s, %s %d",
                         strerror(errno), __func__, __LINE__);
        }
        close(peerInfo->fileDescriptor);
        cleanHashEntry(peerInfo, sctpMap, &lspan);
        char key[MAX_ENODB_NAME_SIZE * 2];
        snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", enodbName, msgType);
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "remove key = %s from %s at line %d", key, __FUNCTION__, __LINE__);
        }
    	auto tmp = sctpMap->find(key);
        if (tmp) {
            free(tmp);
        }
        sctpMap->erase(key);
        mdclog_write(MDCLOG_ERR, "epoll_ctl EPOLL_CTL_ADD (may chack not to quit here)");
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }
#ifdef __TRACING__
    lspan->Finish();
#endif
    return 0;
}

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
int modifyToEpoll(int epoll_fd,
                  ConnectedCU_t *peerInfo,
                  uint32_t events,
                  Sctp_Map_t *sctpMap,
                  char *enodbName,
                  int msgType,
                  otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
    otSpan lspan = 0;
#endif
    // Add to Epol
    struct epoll_event event{};
    event.data.ptr = peerInfo;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, peerInfo->fileDescriptor, &event) < 0) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "epoll_ctl EPOLL_CTL_MOD (may chack not to quit here), %s, %s %d",
                         strerror(errno), __func__, __LINE__);
        }
        close(peerInfo->fileDescriptor);
        cleanHashEntry(peerInfo, sctpMap, &lspan);
        char key[MAX_ENODB_NAME_SIZE * 2];
        snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", enodbName, msgType);
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "remove key = %s from %s at line %d", key, __FUNCTION__, __LINE__);
        }
    	auto tmp = sctpMap->find(key);
        if (tmp) {
            free(tmp);
        }
        sctpMap->erase(key);
        mdclog_write(MDCLOG_ERR, "epoll_ctl EPOLL_CTL_ADD (may chack not to quit here)");
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }
#ifdef __TRACING__
    lspan->Finish();
#endif
    return 0;
}


int sendRmrMessage(RmrMessagesBuffer_t &rmrMessageBuffer, ReportingMessages_t &message, otSpan *pSpan) {
#ifdef __TRACING__
    auto lspan = opentracing::Tracer::Global()->StartSpan(
            __FUNCTION__, { opentracing::ChildOf(&pSpan->get()->context()) });
#else
//    otSpan lspan = 0;
#endif
    //serialize the span
#ifdef __TRACING__
    std::unordered_map<std::string, std::string> data;
    RICCarrierWriter carrier(data);
    opentracing::Tracer::Global()->Inject((lspan.get())->context(), carrier);
    nlohmann::json j = data;
    std::string str = j.dump();
    static auto maxTraceLength = 0;

    maxTraceLength = str.length() > maxTraceLength ? str.length() : maxTraceLength;
    // serialized context can be put to RMR message using function:
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "max trace length is %d trace data length = %ld data = %s", maxTraceLength,
                     str.length(), str.c_str());
    }
    rmr_set_trace(rmrMessageBuffer.sendMessage, (const unsigned char *) str.c_str(), str.length());
#endif
    buildJsonMessage(message);

    rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);

    if (rmrMessageBuffer.sendMessage == nullptr) {
        rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
        mdclog_write(MDCLOG_ERR, "RMR failed send message returned with NULL pointer");
#ifdef __TRACING__
        lspan->Finish();
#endif
        return -1;
    }

    if (rmrMessageBuffer.sendMessage->state != 0) {
        char meid[RMR_MAX_MEID]{};
        if (rmrMessageBuffer.sendMessage->state == RMR_ERR_RETRY) {
            usleep(5);
            rmrMessageBuffer.sendMessage->state = 0;
            mdclog_write(MDCLOG_INFO, "RETRY sending Message type %d to Xapp from %s",
                         rmrMessageBuffer.sendMessage->mtype,
                         rmr_get_meid(rmrMessageBuffer.sendMessage, (unsigned char *)meid));
            rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);
            if (rmrMessageBuffer.sendMessage == nullptr) {
                mdclog_write(MDCLOG_ERR, "RMR failed send message returned with NULL pointer");
                rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
#ifdef __TRACING__
                lspan->Finish();
#endif
                return -1;
            } else if (rmrMessageBuffer.sendMessage->state != 0) {
                mdclog_write(MDCLOG_ERR,
                             "Message state %s while sending request %d to Xapp from %s after retry of 10 microseconds",
                             translateRmrErrorMessages(rmrMessageBuffer.sendMessage->state).c_str(),
                             rmrMessageBuffer.sendMessage->mtype,
                             rmr_get_meid(rmrMessageBuffer.sendMessage, (unsigned char *)meid));
                auto rc = rmrMessageBuffer.sendMessage->state;
#ifdef __TRACING__
                lspan->Finish();
#endif
                return rc;
            }
        } else {
            mdclog_write(MDCLOG_ERR, "Message state %s while sending request %d to Xapp from %s",
                         translateRmrErrorMessages(rmrMessageBuffer.sendMessage->state).c_str(),
                         rmrMessageBuffer.sendMessage->mtype,
                         rmr_get_meid(rmrMessageBuffer.sendMessage, (unsigned char *)meid));
#ifdef __TRACING__
            lspan->Finish();
#endif
            return rmrMessageBuffer.sendMessage->state;
        }
    }
    return 0;
}

void buildJsonMessage(ReportingMessages_t &message) {
    if (jsonTrace) {
        message.outLen = sizeof(message.base64Data);
        base64::encode((const unsigned char *) message.message.asndata,
                       (const int) message.message.asnLength,
                       message.base64Data,
                       message.outLen);
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "asn data length = %d, base64 message length = %d ",
                         (int) message.message.asnLength,
                         (int) message.outLen);
        }

//    char buff[256];
//    // build day time to seconds from epoc
//    strftime(buff, sizeof message.message.time, "%D %T", gmtime(&message.message.time.tv_sec));
//    // add nanosecond
//    snprintf(buff, sizeof buff, "%s.%09ld UTC\n", buff, message.message.time.tv_nsec);

        message.bufferLen = snprintf(message.buffer, sizeof(message.buffer),
                                     "{\"header\": {\"ts\": \"%ld.%09ld\","
                                     "\"ranName\": \"%s\","
                                     "\"messageType\": %d,"
                                     "\"direction\": \"%c\"},"
                                     "\"base64Length\": %d,"
                                     "\"asnBase64\": \"%s\"}",
                                     message.message.time.tv_sec,
                                     message.message.time.tv_nsec,
                                     message.message.enodbName,
                                     message.message.messageType,
                                     message.message.direction,
                                     (int) message.outLen,
                                     message.base64Data);
        static src::logger_mt &lg = my_logger::get();

        BOOST_LOG(lg) << message.buffer;
    }
}


/**
 * take RMR error code to string
 * @param state
 * @return
 */
string translateRmrErrorMessages(int state) {
    string str = {};
    switch (state) {
        case RMR_OK:
            str = "RMR_OK - state is good";
            break;
        case RMR_ERR_BADARG:
            str = "RMR_ERR_BADARG - argument passd to function was unusable";
            break;
        case RMR_ERR_NOENDPT:
            str = "RMR_ERR_NOENDPT - send//call could not find an endpoint based on msg type";
            break;
        case RMR_ERR_EMPTY:
            str = "RMR_ERR_EMPTY - msg received had no payload; attempt to send an empty message";
            break;
        case RMR_ERR_NOHDR:
            str = "RMR_ERR_NOHDR - message didn't contain a valid header";
            break;
        case RMR_ERR_SENDFAILED:
            str = "RMR_ERR_SENDFAILED - send failed; errno has nano reason";
            break;
        case RMR_ERR_CALLFAILED:
            str = "RMR_ERR_CALLFAILED - unable to send call() message";
            break;
        case RMR_ERR_NOWHOPEN:
            str = "RMR_ERR_NOWHOPEN - no wormholes are open";
            break;
        case RMR_ERR_WHID:
            str = "RMR_ERR_WHID - wormhole id was invalid";
            break;
        case RMR_ERR_OVERFLOW:
            str = "RMR_ERR_OVERFLOW - operation would have busted through a buffer/field size";
            break;
        case RMR_ERR_RETRY:
            str = "RMR_ERR_RETRY - request (send/call/rts) failed, but caller should retry (EAGAIN for wrappers)";
            break;
        case RMR_ERR_RCVFAILED:
            str = "RMR_ERR_RCVFAILED - receive failed (hard error)";
            break;
        case RMR_ERR_TIMEOUT:
            str = "RMR_ERR_TIMEOUT - message processing call timed out";
            break;
        case RMR_ERR_UNSET:
            str = "RMR_ERR_UNSET - the message hasn't been populated with a transport buffer";
            break;
        case RMR_ERR_TRUNC:
            str = "RMR_ERR_TRUNC - received message likely truncated";
            break;
        case RMR_ERR_INITFAILED:
            str = "RMR_ERR_INITFAILED - initialisation of something (probably message) failed";
            break;
        case RMR_ERR_NOTSUPP:
            str = "RMR_ERR_NOTSUPP - the request is not supported, or RMr was not initialised for the request";
            break;
        default:
            char buf[128]{};
            snprintf(buf, sizeof buf, "UNDOCUMENTED RMR_ERR : %d", state);
            str = buf;
            break;
    }
    return str;
}


