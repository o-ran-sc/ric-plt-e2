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

//  This source code is part of the near-RT RIC (RAN Intelligent Controller)
//  platform project (RICP).

// TODO: High-level file comment.



#include <3rdparty/oranE2/RANfunctions-List.h>
#include "sctpThread.h"
#include "BuildRunName.h"
#include <unistd.h>
//#include "3rdparty/oranE2SM/E2SM-gNB-NRT-RANfunction-Definition.h"
//#include "BuildXml.h"
//#include "pugixml/src/pugixml.hpp"
#include <pthread.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include <errno.h>
#include <sys/stat.h>

using namespace std;
//using namespace std::placeholders;
using namespace boost::filesystem;
using namespace prometheus;


//#ifdef __cplusplus
//extern "C"
//{
//#endif

// need to expose without the include of gcov
extern "C" void __gcov_flush(void);
#define LOG_FILE_CONFIG_MAP "CONFIG_MAP_NAME"

static void catch_function(int signal) {
    __gcov_flush();
    exit(signal);
}


BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)

boost::shared_ptr<sinks::synchronous_sink<sinks::text_file_backend>> boostLogger;
double cpuClock = 0.0;
bool jsonTrace = false;


static int enable_log_change_notify(const char* fileName)
{
    int ret = -1;
    struct stat fileInfo;
    if ( lstat(fileName,&fileInfo) == 0 )
    {
        ret = register_log_change_notify(fileName);
    }
    return ret;
}


static int register_log_change_notify(const char *fileName)
{
    pthread_attr_t cb_attr;
    pthread_t tid;
    pthread_attr_init(&cb_attr);
    pthread_attr_setdetachstate(&cb_attr,PTHREAD_CREATE_DETACHED);
    return pthread_create(&tid, &cb_attr,&monitor_loglevel_change_handler,(void *)fileName);
}


static void * monitor_loglevel_change_handler(void* arg)
{
    char *fileName = (char*) arg;
    int ifd;                   // the inotify file des
    int wfd;                   // the watched file des
    ssize_t n = 0;
    char rbuf[4096];           // large read buffer as the event is var len
    fd_set fds;
    int res = 0;
    struct timeval timeout;
    char* dname=NULL;          // directory name
    char* bname = NULL;        // basename
    char* tok=NULL;
    char* log_level=NULL;

    dname = strdup( fileName); // defrock the file name into dir and basename
    if( (tok = strrchr( dname, '/' )) != NULL ) {
        *tok = '\0';
        bname = strdup( tok+1 );
    }


    ifd = inotify_init1( 0 ); // initialise watcher setting blocking read (no option)
    if( ifd < 0 ) {
        fprintf( stderr, "### ERR ### unable to initialise file watch %s\n", strerror( errno ) );
    } else {
        wfd = inotify_add_watch( ifd, dname, IN_MOVED_TO | IN_CLOSE_WRITE ); // we only care about close write changes

        if( wfd < 0 ) {
            fprintf( stderr, "### ERR ### unable to add watch on config file %s: %s\n", fileName, strerror( errno ) );
        } else {
           

            memset( &timeout, 0, sizeof(timeout) );
            while( 1 ) {
                FD_ZERO (&fds);
                FD_SET (ifd, &fds);
                timeout.tv_sec=1;
                res = select (ifd + 1, &fds, NULL, NULL, &timeout);
                if(res)
                {
                    n = read( ifd, rbuf, sizeof( rbuf ) ); // read the event
                    if( n < 0  ) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                        
                        if( errno == EAGAIN ) {
                        } else {
                            fprintf( stderr, "### CRIT ### config listener read err: %s\n", strerror( errno ) );
                        }
                        continue;
#endif                        
                    }

                    //Retrieving Log Level from configmap by parsing configmap file
                    log_level = parse_file(fileName);
                    update_mdc_log_level_severity(log_level); //setting log level
                    free(log_level);
                }
            }
            inotify_rm_watch(ifd,wfd);
        }
        close(ifd);
    }
    free(bname);
    free(dname);

    pthread_exit(NULL);
}

void  update_mdc_log_level_severity(char* log_level)
{
    mdclog_severity_t level = MDCLOG_ERR;

    if(log_level == NULL)
    {
        printf("### ERR ### Invalid Log-Level Configuration in ConfigMap, Default Log-Level Applied:   %d\n",level);
    }
    else if(strcasecmp(log_level,"1")==0)
    {
        level = MDCLOG_ERR;
    }
    else if(strcasecmp(log_level,"2")==0)
    {
        level = MDCLOG_WARN;
    }
    else if(strcasecmp(log_level,"3")==0)
    {
        level = MDCLOG_INFO;
    }
    else if(strcasecmp(log_level,"4")==0)
    {
        level = MDCLOG_DEBUG;
    }

    mdclog_level_set(level);
}
static char* parse_file(char* filename)
{
    char *token=NULL;
    char *search = ": ";
    char *string_match = "log-level";
    bool found = false;
    FILE *file = fopen ( filename, "r" );
    if ( file != NULL )
    {
        char line [ 128 ];
        while ( fgets ( line, sizeof line, file ) != NULL )
        {
            token = strtok(line, search);
            if(strcmp(token,string_match)==0)
            {
                found = true;
                token = strtok(NULL, search);
                token = strtok(token, "\n");//removing newline if any
                break;
            }
        }
        fclose ( file );
     }
     if(found)
         return(strdup(token));
     else
         return(NULL);
}

char *read_env_param(const char*envkey)
{
    if(envkey)
    {
        char *value = getenv(envkey);
        if(value)
            return strdup(value);
    }
    return NULL;
}

void dynamic_log_level_change()
{
    char *logFile_Name = read_env_param(LOG_FILE_CONFIG_MAP);
    char* log_level_init=NULL;
    if(logFile_Name)
    {
        log_level_init = parse_file(logFile_Name);
        update_mdc_log_level_severity(log_level_init); //setting log level
        free(log_level_init);

    }
    enable_log_change_notify(logFile_Name);
    free(logFile_Name);

}

void init_log() {
    int log_change_monitor = 0;
    mdclog_attr_t *attr;
    mdclog_attr_init(&attr);
    mdclog_attr_set_ident(attr, "E2Terminator");
    mdclog_init(attr);
    if(mdclog_format_initialize(log_change_monitor)!=0)
        mdclog_write(MDCLOG_ERR, "Failed to intialize MDC log format !!!");
    dynamic_log_level_change();
    mdclog_attr_destroy(attr);
}
auto start_time = std::chrono::high_resolution_clock::now();
typedef std::chrono::duration<double, std::ratio<1,1>> seconds_t;

double age() {
    return seconds_t(std::chrono::high_resolution_clock::now() - start_time).count();
}

double approx_CPU_MHz(unsigned sleepTime) {
    using namespace std::chrono_literals;
    uint32_t aux = 0;
    uint64_t cycles_start = rdtscp(aux);
    double time_start = age();
    std::this_thread::sleep_for(sleepTime * 1ms);
    uint64_t elapsed_cycles = rdtscp(aux) - cycles_start;
    double elapsed_time = age() - time_start;
    return elapsed_cycles / elapsed_time;
}

//std::atomic<int64_t> rmrCounter{0};
std::atomic<int64_t> num_of_messages{0};
std::atomic<int64_t> num_of_XAPP_messages{0};
static long transactionCounter = 0;

int buildListeningPort(sctp_params_t &sctpParams) {
    sctpParams.listenFD = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
    if (sctpParams.listenFD <= 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "Error Opening socket, %s", strerror(errno));
        return -1;
#endif        
    }

    struct sockaddr_in6 serverAddress {};
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_addr   = in6addr_any;
    serverAddress.sin6_port = htons(sctpParams.sctpPort);
    if (bind(sctpParams.listenFD, (SA *)&serverAddress, sizeof(serverAddress)) < 0 ) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "Error binding port %d. %s", sctpParams.sctpPort, strerror(errno));
        return -1;
#endif        
    }
    if (setSocketNoBlocking(sctpParams.listenFD) == -1) {
        //mdclog_write(MDCLOG_ERR, "Error binding. %s", strerror(errno));
        return -1;
    }
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        struct sockaddr_in6 clientAddress {};
        socklen_t len = sizeof(clientAddress);
        getsockname(sctpParams.listenFD, (SA *)&clientAddress, &len);
        char buff[1024] {};
        inet_ntop(AF_INET6, &clientAddress.sin6_addr, buff, sizeof(buff));
        mdclog_write(MDCLOG_DEBUG, "My address: %s, port %d\n", buff, htons(clientAddress.sin6_port));
    }

    if (listen(sctpParams.listenFD, SOMAXCONN) < 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))     
        mdclog_write(MDCLOG_ERR, "Error listening. %s\n", strerror(errno));
        return -1;
#endif        
    }
    struct epoll_event event {};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = sctpParams.listenFD;

    // add listening port to epoll
    if (epoll_ctl(sctpParams.epoll_fd, EPOLL_CTL_ADD, sctpParams.listenFD, &event)) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))
        printf("Failed to add descriptor to epoll\n");
        mdclog_write(MDCLOG_ERR, "Failed to add descriptor to epoll. %s\n", strerror(errno));
        return -1;
#endif        
    }

    return 0;
}

int buildConfiguration(sctp_params_t &sctpParams) {
    path p = (sctpParams.configFilePath + "/" + sctpParams.configFileName).c_str();
    if (exists(p)) {
        const int size = 2048;
        auto fileSize = file_size(p);
        if (fileSize > size) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            mdclog_write(MDCLOG_ERR, "File %s larger than %d", p.string().c_str(), size);
            return -1;
#endif            
        }
    } else {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "Configuration File %s not exists", p.string().c_str());
        return -1;
#endif        
    }

    ReadConfigFile conf;
    if (conf.openConfigFile(p.string()) == -1) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "Filed to open config file %s, %s",
                     p.string().c_str(), strerror(errno));
        return -1;
#endif        
    }
    int rmrPort = conf.getIntValue("nano");
    if (rmrPort == -1) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))
        mdclog_write(MDCLOG_ERR, "illegal RMR port ");
        return -1;
#endif        
    }
    sctpParams.rmrPort = (uint16_t)rmrPort;
    snprintf(sctpParams.rmrAddress, sizeof(sctpParams.rmrAddress), "%d", (int) (sctpParams.rmrPort));
    auto tmpStr = conf.getStringValue("volume");
    if (tmpStr.length() == 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "illegal volume.");
        return -1;
#endif        
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
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "illegal local-ip.");
        return -1;
#endif        
    }

    int sctpPort = conf.getIntValue("sctp-port");
    if (sctpPort == -1) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "illegal SCTP port ");
        return -1;
#endif        
    }
    sctpParams.sctpPort = (uint16_t)sctpPort;

    sctpParams.fqdn = conf.getStringValue("external-fqdn");
    if (sctpParams.fqdn.length() == 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "illegal external-fqdn");
        return -1;
#endif        
    }

    std::string pod = conf.getStringValue("pod_name");
#ifndef UNIT_TEST
    if (pod.length() == 0) {
        mdclog_write(MDCLOG_ERR, "illegal pod_name in config file");
        return -1;
    }
    auto *podName = getenv(pod.c_str());
    if (podName == nullptr) {
        mdclog_write(MDCLOG_ERR, "illegal pod_name or environment variable not exists : %s", pod.c_str());
        return -1;

    } else {
        sctpParams.podName.assign(podName);
        if (sctpParams.podName.length() == 0) {
            mdclog_write(MDCLOG_ERR, "illegal pod_name");
            return -1;
        }
    }
#endif
    tmpStr = conf.getStringValue("trace");
    transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);
    if ((tmpStr.compare("start")) == 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_INFO, "Trace set to: start");
        sctpParams.trace = true;
#endif        
    } else if ((tmpStr.compare("stop")) == 0) {
        mdclog_write(MDCLOG_INFO, "Trace set to: stop");
        sctpParams.trace = false;
    } else {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))      
        mdclog_write(MDCLOG_ERR, "Trace was set to wrong value %s, set to stop", tmpStr.c_str());
        sctpParams.trace = false;
#endif        
    }
    jsonTrace = sctpParams.trace;

    sctpParams.epollTimeOut = -1;

    tmpStr = conf.getStringValue("prometheusPort");
    if (tmpStr.length() != 0) {
        sctpParams.prometheusPort = tmpStr;
    }

    sctpParams.ka_message_length = snprintf(sctpParams.ka_message, KA_MESSAGE_SIZE, "{\"address\": \"%s:%d\","
                                                                                    "\"fqdn\": \"%s\","
                                                                                    "\"pod_name\": \"%s\"}",
                                            (const char *)sctpParams.myIP.c_str(),
                                            sctpParams.rmrPort,
                                            sctpParams.fqdn.c_str(),
                                            sctpParams.podName.c_str());

    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_DEBUG,"RMR Port: %s", to_string(sctpParams.rmrPort).c_str());
        mdclog_write(MDCLOG_DEBUG,"LogLevel: %s", to_string(sctpParams.logLevel).c_str());
        mdclog_write(MDCLOG_DEBUG,"volume: %s", sctpParams.volume);
        mdclog_write(MDCLOG_DEBUG,"tmpLogFilespec: %s", tmpLogFilespec);
        mdclog_write(MDCLOG_DEBUG,"my ip: %s", sctpParams.myIP.c_str());
        mdclog_write(MDCLOG_DEBUG,"pod name: %s", sctpParams.podName.c_str());

        mdclog_write(MDCLOG_INFO, "running parameters for instance : %s", sctpParams.ka_message);
    }

    // Files written to the current working directory
    boostLogger = logging::add_file_log(
            keywords::file_name = tmpLogFilespec, // to temp directory
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_interval(posix_time::hours(1)),
            keywords::format = "%Message%"
            //keywords::format = "[%TimeStamp%]: %Message%" // use each tmpStr with time stamp
    );

    // Setup a destination folder for collecting rotated (closed) files --since the same volume can use rename()
    boostLogger->locked_backend()->set_file_collector(sinks::file::make_collector(
            keywords::target = sctpParams.volume
    ));

    // Upon restart, scan the directory for files matching the file_name pattern
    boostLogger->locked_backend()->scan_for_files();

    // Enable auto-flushing after each tmpStr record written
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        boostLogger->locked_backend()->auto_flush(true);
    }

    return 0;
}

void startPrometheus(sctp_params_t &sctpParams) {
    auto podName = std::getenv("POD_NAME");
    string metric = "e2t_beta";
    if (strstr(podName, "alpha") != NULL) {
        metric = "e2t_alpha";
    }

    sctpParams.prometheusFamily = &BuildCounter()
            .Name(metric.c_str())
            .Help("E2T instance metrics")
            .Labels({{"POD_NAME", sctpParams.podName}})
            .Register(*sctpParams.prometheusRegistry);

    // Build E2T instance level metrics
    buildE2TPrometheusCounters(sctpParams);

    string prometheusPath = sctpParams.prometheusPort + "," + "[::]:" + sctpParams.prometheusPort;
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Start Prometheus Pull mode on %s", prometheusPath.c_str());
    }
    sctpParams.prometheusExposer = new Exposer(prometheusPath, 1);
    sctpParams.prometheusExposer->RegisterCollectable(sctpParams.prometheusRegistry);
}
#ifndef UNIT_TEST

int main(const int argc, char **argv) {
    sctp_params_t sctpParams;
    {
        std::random_device device{};
        std::mt19937 generator(device());
        std::uniform_int_distribution<long> distribution(1, (long) 1e12);
        transactionCounter = distribution(generator);
    }

//    uint64_t st = 0;
//    uint32_t aux1 = 0;
//   st = rdtscp(aux1);

    unsigned num_cpus = std::thread::hardware_concurrency();
    init_log();
    if (std::signal(SIGINT, catch_function) == SIG_ERR) {
        mdclog_write(MDCLOG_ERR, "Error initializing SIGINT");
        exit(1);
    }
    if (std::signal(SIGABRT, catch_function)== SIG_ERR) {
        mdclog_write(MDCLOG_ERR, "Error initializing SIGABRT");
        exit(1);
    }
    if (std::signal(SIGTERM, catch_function)== SIG_ERR) {
        mdclog_write(MDCLOG_ERR, "Error initializing SIGTERM");
        exit(1);
    }

    cpuClock = approx_CPU_MHz(100);

    mdclog_write(MDCLOG_DEBUG, "CPU speed %11.11f", cpuClock);

    auto result = parse(argc, argv, sctpParams);

    if (buildConfiguration(sctpParams) != 0) {
        exit(-1);
    }

    //auto registry = std::make_shared<Registry>();
    sctpParams.prometheusRegistry = std::make_shared<Registry>();

    //sctpParams.prometheusFamily = new Family<Counter>("E2T", "E2T message counter", {{"E", sctpParams.podName}});

    startPrometheus(sctpParams);

    // start epoll
    sctpParams.epoll_fd = epoll_create1(0);
    if (sctpParams.epoll_fd == -1) {
        mdclog_write(MDCLOG_ERR, "failed to open epoll descriptor");
        exit(-1);
    }
    getRmrContext(sctpParams);
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

    if (buildListeningPort(sctpParams) != 0) {
        close(sctpParams.rmrListenFd);
        rmr_close(sctpParams.rmrCtx);
        close(sctpParams.epoll_fd);
        exit(-1);
    }

    sctpParams.sctpMap = new mapWrapper();

    std::vector<std::thread> threads(num_cpus);
//    std::vector<std::thread> threads;

    num_cpus = 3;
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

    return 0;
}
#endif
void handleTermInit(sctp_params_t &sctpParams) {
    sendTermInit(sctpParams);
    //send to e2 manager init of e2 term
    //E2_TERM_INIT

    int count = 0;
    while (true) {
        auto xappMessages = num_of_XAPP_messages.load(std::memory_order_acquire);
        if (xappMessages > 0) {
            if (mdclog_level_get() >=  MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "Got a message from some application, stop sending E2_TERM_INIT");
            }
            return;
        }
        usleep(100000);
        count++;
        if (count % 1000 == 0) {
            mdclog_write(MDCLOG_ERR, "GOT No messages from any xApp");
            sendTermInit(sctpParams);
        }
    }
}

void sendTermInit(sctp_params_t &sctpParams) {
    rmr_mbuf_t *msg = rmr_alloc_msg(sctpParams.rmrCtx, sctpParams.ka_message_length);
    auto count = 0;
    while (true) {
        msg->mtype = E2_TERM_INIT;
        msg->state = 0;
        rmr_bytes2payload(msg, (unsigned char *)sctpParams.ka_message, sctpParams.ka_message_length);
        static unsigned char tx[32];
        auto txLen = snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
        rmr_bytes2xact(msg, tx, txLen);
        msg = rmr_send_msg(sctpParams.rmrCtx, msg);
        if (msg == nullptr) {
            msg = rmr_alloc_msg(sctpParams.rmrCtx, sctpParams.ka_message_length);
        } else if (msg->state == 0) {
            rmr_free_msg(msg);
            if (mdclog_level_get() >=  MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "E2_TERM_INIT successfully sent ");
            }
            return;
        } else {
            if (count % 100 == 0) {
                mdclog_write(MDCLOG_ERR, "Error sending E2_TERM_INIT cause : %s ", translateRmrErrorMessages(msg->state).c_str());
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

    auto result = options.parse(argc, (const char **&)argv);

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
        return -1;
    }

    sctpParams.inotifyWD = inotify_add_watch(sctpParams.inotifyFD,
                                             (const char *)sctpParams.configFilePath.c_str(),
                                             (unsigned)IN_OPEN | (unsigned)IN_CLOSE_WRITE | (unsigned)IN_CLOSE_NOWRITE); //IN_CLOSE = (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
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
    int num_of_SCTP_messages = 0;
    auto totalTime = 0.0;
    std::thread::id this_id = std::this_thread::get_id();
    //save cout
    auto pod_name = std::getenv("POD_NAME");
    auto container_name = std::getenv("CONTAINER_NAME");
    auto service_name = std::getenv("SERVICE_NAME");
    auto host_name = std::getenv("HOST_NAME");
    auto system_name = std::getenv("SYSTEM_NAME");
    auto pid = std::to_string(getpid()).c_str();
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
    mdclog_mdc_add("SYSTEM_NAME", system_name);
    mdclog_mdc_add("HOST_NAME", host_name);
    mdclog_mdc_add("SERVICE_NAME", service_name);
    mdclog_mdc_add("CONTAINER_NAME", container_name);
    mdclog_mdc_add("POD_NAME", pod_name);
    mdclog_mdc_add("PID", pid);

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

//    for (int i = 0; i < MAX_RMR_BUFF_ARRAY; i++) {
//        rmrMessageBuffer.rcvBufferedMessages[i] = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
//        rmrMessageBuffer.sendBufferedMessages[i] = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
//    }

    while (true) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Start EPOLL Wait. Timeout = %d", params->epollTimeOut);
        }
#ifndef UNIT_TEST
        auto numOfEvents = epoll_wait(params->epoll_fd, events, MAXEVENTS, params->epollTimeOut);
#else
        auto numOfEvents = 1;
#endif
        if (numOfEvents == 0) { // time out
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            if (mdclog_level_get() >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "got epoll timeout");
            }
            continue;
        } else if (numOfEvents < 0) {
            if (errno == EINTR) {
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "got EINTR : %s", strerror(errno));
                }
                continue;
            }
            mdclog_write(MDCLOG_ERR, "Epoll wait failed, errno = %s", strerror(errno));
            return;
#endif            
        }
        for (auto i = 0; i < numOfEvents; i++) {
            if (mdclog_level_get() >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "handling epoll event %d out of %d", i + 1, numOfEvents);
            }
            clock_gettime(CLOCK_MONOTONIC, &message.message.time);
            start.tv_sec = message.message.time.tv_sec;
            start.tv_nsec = message.message.time.tv_nsec;


            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                handlepoll_error(events[i], message, rmrMessageBuffer, params);
            } else if (events[i].events & EPOLLOUT) {
                handleEinprogressMessages(events[i], message, rmrMessageBuffer, params);
            } else if (params->listenFD == events[i].data.fd) {
                if (mdclog_level_get() >= MDCLOG_INFO) {
                    mdclog_write(MDCLOG_INFO, "New connection request from sctp network\n");
                }
                // new connection is requested from RAN  start build connection
                while (true) {
                    struct sockaddr in_addr {};
                    socklen_t in_len;
                    char hostBuff[NI_MAXHOST];
                    char portBuff[NI_MAXSERV];

                    in_len = sizeof(in_addr);
                    auto *peerInfo = (ConnectedCU_t *)calloc(1, sizeof(ConnectedCU_t));
                    if(peerInfo == nullptr){
                        mdclog_write(MDCLOG_ERR, "calloc failed");
                        break;
                    }
                    peerInfo->sctpParams = params;
                    peerInfo->fileDescriptor = accept(params->listenFD, &in_addr, &in_len);
                    if (peerInfo->fileDescriptor == -1) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                        
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* We have processed all incoming connections. */
                            if(peerInfo)
                                free(peerInfo);
                            break;
                        } else {
                            if(peerInfo)
                                free(peerInfo);
                            mdclog_write(MDCLOG_ERR, "Accept error, errno = %s", strerror(errno));
                            break;
                        }
                    }
                    if (setSocketNoBlocking(peerInfo->fileDescriptor) == -1) {
                        mdclog_write(MDCLOG_ERR, "setSocketNoBlocking failed to set new connection %s on port %s\n", hostBuff, portBuff);
                        close(peerInfo->fileDescriptor);
                        if(peerInfo)
                            free(peerInfo);
                        break;
#endif                        
                    }
                    auto  ans = getnameinfo(&in_addr, in_len,
                                            peerInfo->hostName, NI_MAXHOST,
                                            peerInfo->portNumber, NI_MAXSERV, (unsigned )((unsigned int)NI_NUMERICHOST | (unsigned int)NI_NUMERICSERV));
                    if (ans < 0) {
                        mdclog_write(MDCLOG_ERR, "Failed to get info on connection request. %s\n", strerror(errno));
                        close(peerInfo->fileDescriptor);
                        if(peerInfo)
                            free(peerInfo);
                        break;
                    }
                    if (mdclog_level_get() >= MDCLOG_DEBUG) {
                        mdclog_write(MDCLOG_DEBUG, "Accepted connection on descriptor %d (host=%s, port=%s)\n", peerInfo->fileDescriptor, peerInfo->hostName, peerInfo->portNumber);
                    }
                    peerInfo->isConnected = false;
                    peerInfo->gotSetup = false;
                    if (addToEpoll(params->epoll_fd,
                                   peerInfo,
                                   (EPOLLIN | EPOLLET),
                                   params->sctpMap, nullptr,
                                   0) != 0) {
                        if(peerInfo)
                            free(peerInfo);
                        break;
                    }
                    break;
                }
            } else if (params->rmrListenFd == events[i].data.fd) {
                // got message from XAPP
                //num_of_XAPP_messages.fetch_add(1, std::memory_order_release);
                num_of_messages.fetch_add(1, std::memory_order_release);
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "new RMR message");
                }
                if (receiveXappMessages(params->sctpMap,
                                        rmrMessageBuffer,
                                        message.message.time) != 0) {
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
                                    message.message.time);
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
#ifdef UNIT_TEST
    break;
#endif
    }
}

/**
 *
 * @param sctpParams
 */
void handleConfigChange(sctp_params_t *sctpParams) {
    char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    char *ptr;
#ifdef UNIT_TEST
    struct inotify_event tmpEvent;
#endif
    path p = (sctpParams->configFilePath + "/" + sctpParams->configFileName).c_str();
    auto endlessLoop = true;
    while (endlessLoop) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))    
        auto len = read(sctpParams->inotifyFD, buf, sizeof buf);
#else
    auto len=10;
#endif
        if (len == -1) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
            if (errno != EAGAIN) {
                mdclog_write(MDCLOG_ERR, "read %s ", strerror(errno));
                endlessLoop = false;
                continue;
            }
            else {
                endlessLoop = false;
                continue;
            }
#endif            
        }

        for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
#ifndef UNIT_TEST
    event = (const struct inotify_event *)ptr;
#else
    tmpEvent.mask = (uint32_t)IN_CLOSE_WRITE;
    event = &tmpEvent;
#endif
            if (event->mask & (uint32_t)IN_ISDIR) {
                continue;
            }

            // the directory name
            if (sctpParams->inotifyWD == event->wd) {
                // not the directory
            }
            if (event->len) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                
                auto  retVal = strcmp(sctpParams->configFileName.c_str(), event->name);
                if (retVal != 0) {
                    continue;
                }
#endif                
            }
            // only the file we want
            if (event->mask & (uint32_t)IN_CLOSE_WRITE) {
                if (mdclog_level_get() >= MDCLOG_INFO) {
                    mdclog_write(MDCLOG_INFO, "Configuration file changed");
                }
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
                    mdclog_write(MDCLOG_ERR, "illegal loglevel. Set loglevel to MDCLOG_INFO");
                    tmpStr = "info";
                }
                transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

                if ((tmpStr.compare("debug")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_DEBUG");
                    sctpParams->logLevel = MDCLOG_DEBUG;
                } 
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                
                else if ((tmpStr.compare("info")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_INFO");
                    sctpParams->logLevel = MDCLOG_INFO;
                } else if ((tmpStr.compare("warning")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_WARN");
                    sctpParams->logLevel = MDCLOG_WARN;
                } else if ((tmpStr.compare("error")) == 0) {
                    mdclog_write(MDCLOG_INFO, "Log level set to MDCLOG_ERR");
                    sctpParams->logLevel = MDCLOG_ERR;
                } else {
                    mdclog_write(MDCLOG_ERR, "illegal loglevel = %s. Set loglevel to MDCLOG_INFO", tmpStr.c_str());
                    sctpParams->logLevel = MDCLOG_INFO;
                }
#endif                
                mdclog_level_set(sctpParams->logLevel);
                tmpStr = conf.getStringValue("trace");
                if (tmpStr.length() == 0) {
                    mdclog_write(MDCLOG_ERR, "illegal trace. Set trace to stop");
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
#ifdef UNIT_TEST
            break;
#endif
        }
    }
}

/**
 *
 * @param event
 * @param message
 * @param rmrMessageBuffer
 * @param params
 */
void handleEinprogressMessages(struct epoll_event &event,
                               ReportingMessages_t &message,
                               RmrMessagesBuffer_t &rmrMessageBuffer,
                               sctp_params_t *params) {
    auto *peerInfo = (ConnectedCU_t *)event.data.ptr;
    memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));

    mdclog_write(MDCLOG_INFO, "file descriptor %d got EPOLLOUT", peerInfo->fileDescriptor);
    auto retVal = 0;
    socklen_t retValLen = 0;
    auto rc = getsockopt(peerInfo->fileDescriptor, SOL_SOCKET, SO_ERROR, &retVal, &retValLen);
    if (rc != 0 || retVal != 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
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
        if (sendRequestToXapp(message, RIC_SCTP_CONNECTION_FAILURE, rmrMessageBuffer) != 0) {
            mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
        }
#endif
        memset(peerInfo->asnData, 0, peerInfo->asnLength);
        peerInfo->asnLength = 0;
        peerInfo->mtype = 0;
        return;
    }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))
    peerInfo->isConnected = true;

    if (modifyToEpoll(params->epoll_fd, peerInfo, (EPOLLIN | EPOLLET), params->sctpMap, peerInfo->enodbName,
                      peerInfo->mtype) != 0) {
        mdclog_write(MDCLOG_ERR, "epoll_ctl EPOLL_CTL_MOD");
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
    if (sendSctpMsg(peerInfo, message, params->sctpMap) != 0) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Error write to SCTP  %s %d", __func__, __LINE__);
        }
        return;
    }

    memset(peerInfo->asnData, 0, peerInfo->asnLength);
    peerInfo->asnLength = 0;
    peerInfo->mtype = 0;
#endif    
}


void handlepoll_error(struct epoll_event &event,
                      ReportingMessages_t &message,
                      RmrMessagesBuffer_t &rmrMessageBuffer,
                      sctp_params_t *params) {
    if (event.data.fd != params->rmrListenFd) {
        auto *peerInfo = (ConnectedCU_t *)event.data.ptr;
        mdclog_write(MDCLOG_ERR, "epoll error, events %0x on fd %d, RAN NAME : %s",
                     event.events, peerInfo->fileDescriptor, peerInfo->enodbName);
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))
        rmrMessageBuffer.sendMessage->len = snprintf((char *)rmrMessageBuffer.sendMessage->payload, 256,
                                                     "%s|Failed SCTP Connection",
                                                     peerInfo->enodbName);
        message.message.asndata = rmrMessageBuffer.sendMessage->payload;
        message.message.asnLength = rmrMessageBuffer.sendMessage->len;

        memcpy(message.message.enodbName, peerInfo->enodbName, sizeof(peerInfo->enodbName));
        message.message.direction = 'N';
        if (sendRequestToXapp(message, RIC_SCTP_CONNECTION_FAILURE, rmrMessageBuffer) != 0) {
            mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
        }
#endif
        close(peerInfo->fileDescriptor);
        params->sctpMap->erase(peerInfo->enodbName);
        cleanHashEntry((ConnectedCU_t *) event.data.ptr, params->sctpMap);
    } else {
        mdclog_write(MDCLOG_ERR, "epoll error, events %0x on RMR FD", event.events);
    }
}
/**
 *
 * @param socket
 * @return
 */
int setSocketNoBlocking(int socket) {
    auto flags = fcntl(socket, F_GETFL, 0);

    if (flags == -1) {
        mdclog_write(MDCLOG_ERR, "%s, %s", __FUNCTION__, strerror(errno));
        return -1;
    }

    flags = (unsigned) flags | (unsigned) O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flags) == -1) {
        mdclog_write(MDCLOG_ERR, "%s, %s", __FUNCTION__, strerror(errno));
        return -1;
    }

    return 0;
}

/**
 *
 * @param val
 * @param m
 */
void cleanHashEntry(ConnectedCU_t *val, Sctp_Map_t *m) {
    char *dummy;
    auto port = (uint16_t) strtol(val->portNumber, &dummy, 10);
    char searchBuff[2048]{};

    snprintf(searchBuff, sizeof searchBuff, "host:%s:%d", val->hostName, port);
    m->erase(searchBuff);

    m->erase(val->enodbName);
#ifndef UNIT_TEST
    free(val);
#endif
}

/**
 *
 * @param fd file descriptor
 * @param data the asn data to send
 * @param len  length of the data
 * @param enodbName the enodbName as in the map for printing purpose
 * @param m map host information
 * @param mtype message number
 * @return 0 success, a negative number on fail
 */
int sendSctpMsg(ConnectedCU_t *peerInfo, ReportingMessages_t &message, Sctp_Map_t *m) {
    auto loglevel = mdclog_level_get();
#ifndef UNIT_TEST    
    int fd = peerInfo->fileDescriptor;
#else
    int fd = FILE_DESCRIPTOR;
#endif    
    if (loglevel >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Send SCTP message for CU %s, %s",
                     message.message.enodbName, __FUNCTION__);
    }

    while (true) {
        if (send(fd,message.message.asndata, message.message.asnLength,MSG_NOSIGNAL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            mdclog_write(MDCLOG_ERR, "error writing to CU a message, %s ", strerror(errno));
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            if (!peerInfo->isConnected) {
                mdclog_write(MDCLOG_ERR, "connection to CU %s is still in progress.", message.message.enodbName);
                return -1;
            }
#endif
#ifndef UNIT_TEST            
            cleanHashEntry(peerInfo, m);
            close(fd);
#endif            
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
#ifndef UNIT_TEST
            return -1;
#endif
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
        return 0;
    }
}

/**
 *
 * @param message
 * @param rmrMessageBuffer
 */
void getRequestMetaData(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    message.message.asndata = rmrMessageBuffer.rcvMessage->payload;
    message.message.asnLength = rmrMessageBuffer.rcvMessage->len;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Message from Xapp RAN name = %s message length = %ld",
                     message.message.enodbName, (unsigned long) message.message.asnLength);
    }
}



/**
 *
 * @param events
 * @param sctpMap
 * @param numOfMessages
 * @param rmrMessageBuffer
 * @param ts
 * @return
 */
int receiveDataFromSctp(struct epoll_event *events,
                        Sctp_Map_t *sctpMap,
                        int &numOfMessages,
                        RmrMessagesBuffer_t &rmrMessageBuffer,
                        struct timespec &ts) {
    /* We have data on the fd waiting to be read. Read and display it.
 * We must read whatever data is available completely, as we are running
 *  in edge-triggered mode and won't get a notification again for the same data. */
    ReportingMessages_t message {};
    auto done = 0;
    auto loglevel = mdclog_level_get();

    // get the identity of the interface
    message.peerInfo = (ConnectedCU_t *)events->data.ptr;

    struct timespec start{0, 0};
    struct timespec decodeStart{0, 0};
    struct timespec end{0, 0};

    E2AP_PDU_t *pdu = nullptr;

    while (true) {
        if (loglevel >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Start Read from SCTP %d fd", message.peerInfo->fileDescriptor);
            clock_gettime(CLOCK_MONOTONIC, &start);
        }
        // read the buffer directly to rmr payload
        message.message.asndata = rmrMessageBuffer.sendMessage->payload;
#ifndef UNIT_TEST        
        message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                read(message.peerInfo->fileDescriptor, rmrMessageBuffer.sendMessage->payload, RECEIVE_SCTP_BUFFER_SIZE);
#else
        message.message.asnLength = rmrMessageBuffer.sendMessage->len;
#endif

        if (loglevel >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Finish Read from SCTP %d fd message length = %ld",
                         message.peerInfo->fileDescriptor, message.message.asnLength);
        }

        memcpy(message.message.enodbName, message.peerInfo->enodbName, sizeof(message.peerInfo->enodbName));
        message.message.direction = 'U';
        message.message.time.tv_nsec = ts.tv_nsec;
        message.message.time.tv_sec = ts.tv_sec;

        if (message.message.asnLength < 0) {
            if (errno == EINTR) {
                continue;
            }
            /* If errno == EAGAIN, that means we have read all
               data. So goReportingMessages_t back to the main loop. */
            if (errno != EAGAIN) {
                mdclog_write(MDCLOG_ERR, "Read error, %s ", strerror(errno));
                done = 1;
            } else if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "EAGAIN - descriptor = %d", message.peerInfo->fileDescriptor);
            }
            break;
        } else if (message.message.asnLength == 0) {
            /* End of file. The remote has closed the connection. */
            if (loglevel >= MDCLOG_INFO) {
                mdclog_write(MDCLOG_INFO, "END of File Closed connection - descriptor = %d",
                             message.peerInfo->fileDescriptor);
            }
            done = 1;
            break;
        }

        if (loglevel >= MDCLOG_DEBUG) {
            char printBuffer[RECEIVE_SCTP_BUFFER_SIZE]{};
            char *tmp = printBuffer;
            for (size_t i = 0; i < (size_t)message.message.asnLength; ++i) {
                snprintf(tmp, 3, "%02x", message.message.asndata[i]);
                tmp += 2;
            }
            printBuffer[message.message.asnLength] = 0;
            clock_gettime(CLOCK_MONOTONIC, &end);
            mdclog_write(MDCLOG_DEBUG, "Before Encoding E2AP PDU for : %s, Read time is : %ld seconds, %ld nanoseconds",
                         message.peerInfo->enodbName, end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
            mdclog_write(MDCLOG_DEBUG, "PDU buffer length = %ld, data =  : %s", message.message.asnLength,
                         printBuffer);
            clock_gettime(CLOCK_MONOTONIC, &decodeStart);
        }
#ifndef UNIT_TEST
        auto rval = asn_decode(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, (void **) &pdu,
                        message.message.asndata, message.message.asnLength);
#else
        asn_dec_rval_t rval = {RC_OK, 0};
        pdu = (E2AP_PDU_t*)rmrMessageBuffer.sendMessage->tp_buf;
#endif
        if (rval.code != RC_OK) {
            mdclog_write(MDCLOG_ERR, "Error %d Decoding (unpack) E2AP PDU from RAN : %s", rval.code,
                         message.peerInfo->enodbName);
            if (pdu != nullptr) {
                ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
                pdu = nullptr;
            }
            break;
        }

        if (loglevel >= MDCLOG_DEBUG) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            mdclog_write(MDCLOG_DEBUG, "After Encoding E2AP PDU for : %s, Read time is : %ld seconds, %ld nanoseconds",
                         message.peerInfo->enodbName, end.tv_sec - decodeStart.tv_sec, end.tv_nsec - decodeStart.tv_nsec);
            char *printBuffer;
            size_t size;
            FILE *stream = open_memstream(&printBuffer, &size);
            asn_fprint(stream, &asn_DEF_E2AP_PDU, pdu);
            mdclog_write(MDCLOG_DEBUG, "Encoding E2AP PDU past : %s", printBuffer);
            clock_gettime(CLOCK_MONOTONIC, &decodeStart);

            fclose(stream);
            free(printBuffer);
        }

        switch (pdu->present) {
            case E2AP_PDU_PR_initiatingMessage: {//initiating message
                asnInitiatingRequest(pdu, sctpMap,message, rmrMessageBuffer);
                break;
            }
            case E2AP_PDU_PR_successfulOutcome: { //successful outcome
                asnSuccessfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
                break;
            }
            case E2AP_PDU_PR_unsuccessfulOutcome: { //Unsuccessful Outcome
                asnUnSuccsesfulMsg(pdu, sctpMap, message, rmrMessageBuffer);
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
                         message.peerInfo->enodbName, end.tv_sec - decodeStart.tv_sec, end.tv_nsec - decodeStart.tv_nsec);
        }
        numOfMessages++;
#ifndef UNIT_TEST
        if (pdu != nullptr) {
            // ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, pdu); /* With reset we were not freeing the memory and was causing the leak here. */
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
#else
    done = 1;
    break;
#endif
    }

    if (done) {
        if (loglevel >= MDCLOG_INFO) {
            mdclog_write(MDCLOG_INFO, "Closed connection - descriptor = %d", message.peerInfo->fileDescriptor);
        }
        message.message.asnLength = rmrMessageBuffer.sendMessage->len =
                snprintf((char *)rmrMessageBuffer.sendMessage->payload,
                         256,
                         "%s|CU disconnected unexpectedly",
                         message.peerInfo->enodbName);
        message.message.asndata = rmrMessageBuffer.sendMessage->payload;
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        if (sendRequestToXapp(message,
                              RIC_SCTP_CONNECTION_FAILURE,
                              rmrMessageBuffer) != 0) {
            mdclog_write(MDCLOG_ERR, "SCTP_CONNECTION_FAIL message failed to send to xAPP");
        }
#endif        

        /* Closing descriptor make epoll remove it from the set of descriptors which are monitored. */
        close(message.peerInfo->fileDescriptor);
        cleanHashEntry((ConnectedCU_t *) events->data.ptr, sctpMap);
    }
    if (loglevel >= MDCLOG_DEBUG) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        mdclog_write(MDCLOG_DEBUG, "from receive SCTP to send RMR time is %ld seconds and %ld nanoseconds",
                     end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

    }
    return 0;
}

static void buildAndSendSetupRequest(ReportingMessages_t &message,
                                     RmrMessagesBuffer_t &rmrMessageBuffer,
                                     E2AP_PDU_t *pdu/*,
                                     string const &messageName,
                                     string const &ieName,
                                     vector<string> &functionsToAdd_v,
                                     vector<string> &functionsToModified_v*/) {
    auto logLevel = mdclog_level_get();
    // now we can send the data to e2Mgr

    asn_enc_rval_t er;
    auto buffer_size = RECEIVE_SCTP_BUFFER_SIZE * 2;
    unsigned char *buffer = nullptr;
    buffer = (unsigned char *) calloc(buffer_size, sizeof(unsigned char));
    if(!buffer)
    {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))        
        mdclog_write(MDCLOG_ERR, "Allocating buffer for %s failed, %s", asn_DEF_E2AP_PDU.name, strerror(errno));
        return;
#endif        
    }
    while (true) {
        er = asn_encode_to_buffer(nullptr, ATS_BASIC_XER, &asn_DEF_E2AP_PDU, pdu, buffer, buffer_size);
        if (er.encoded == -1) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))     
            mdclog_write(MDCLOG_ERR, "encoding of %s failed, %s", asn_DEF_E2AP_PDU.name, strerror(errno));
            return;
#endif            
        } else if (er.encoded > (ssize_t) buffer_size) {
            buffer_size = er.encoded + 128;
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            mdclog_write(MDCLOG_WARN, "Buffer of size %d is to small for %s. Reallocate buffer of size %d",
                         (int) buffer_size,
                         asn_DEF_E2AP_PDU.name, buffer_size);
            buffer_size = er.encoded + 128;

            unsigned char *newBuffer = nullptr;
            newBuffer = (unsigned char *) realloc(buffer, buffer_size);
            if (!newBuffer)
            {
                // out of memory
                mdclog_write(MDCLOG_ERR, "Reallocating buffer for %s failed, %s", asn_DEF_E2AP_PDU.name, strerror(errno));
                free(buffer);
                return;
            }
            buffer = newBuffer;
            continue;
#endif            
        }
        buffer[er.encoded] = '\0';
        break;
    }
    // encode to xml

    string res((char *)buffer);
    res.erase(std::remove(res.begin(), res.end(), '\n'), res.end());
    res.erase(std::remove(res.begin(), res.end(), '\t'), res.end());
    res.erase(std::remove(res.begin(), res.end(), ' '), res.end());

//    string res {};
//    if (!functionsToAdd_v.empty() || !functionsToModified_v.empty()) {
//        res = buildXmlData(messageName, ieName, functionsToAdd_v, functionsToModified_v, buffer, (size_t) er.encoded);
//    }
    rmr_mbuf_t *rmrMsg;
//    if (res.length() == 0) {
//        rmrMsg = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, buffer_size + 256);
//        rmrMsg->len = snprintf((char *) rmrMsg->payload, RECEIVE_SCTP_BUFFER_SIZE * 2, "%s:%d|%s",
//                               message.peerInfo->sctpParams->myIP.c_str(),
//                               message.peerInfo->sctpParams->rmrPort,
//                               buffer);
//    } else {
        rmrMsg = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, (int)res.length() + 256);
        rmrMsg->len = snprintf((char *) rmrMsg->payload, res.length() + 256, "%s:%d|%s",
                               message.peerInfo->sctpParams->myIP.c_str(),
                               message.peerInfo->sctpParams->rmrPort,
                               res.c_str());
//    }

    if (logLevel >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Setup request of size %d :\n %s\n", rmrMsg->len, rmrMsg->payload);
    }
    // send to RMR
    rmrMsg->mtype = message.message.messageType;
    rmrMsg->state = 0;
    rmr_bytes2meid(rmrMsg, (unsigned char *) message.message.enodbName, strlen(message.message.enodbName));

    static unsigned char tx[32];
    snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
    rmr_bytes2xact(rmrMsg, tx, strlen((const char *) tx));
#ifndef UNIT_TEST
    rmrMsg = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMsg);
#endif
    if (rmrMsg == nullptr) {
        mdclog_write(MDCLOG_ERR, "RMR failed to send returned nullptr");
    } else if (rmrMsg->state != 0) {
        char meid[RMR_MAX_MEID]{};
        if (rmrMsg->state == RMR_ERR_RETRY) {
            usleep(5);
            rmrMsg->state = 0;
            mdclog_write(MDCLOG_INFO, "RETRY sending Message %d to Xapp from %s",
                         rmrMsg->mtype, rmr_get_meid(rmrMsg, (unsigned char *) meid));
#ifndef UNIT_TEST
            rmrMsg = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMsg);
#endif
            if (rmrMsg == nullptr) {
                mdclog_write(MDCLOG_ERR, "RMR failed send returned nullptr");
            } else if (rmrMsg->state != 0) {
                mdclog_write(MDCLOG_ERR,
                             "RMR Retry failed %s sending request %d to Xapp from %s",
                             translateRmrErrorMessages(rmrMsg->state).c_str(),
                             rmrMsg->mtype,
                             rmr_get_meid(rmrMsg, (unsigned char *) meid));
            }
        } else {
            mdclog_write(MDCLOG_ERR, "RMR failed: %s. sending request %d to Xapp from %s",
                         translateRmrErrorMessages(rmrMsg->state).c_str(),
                         rmrMsg->mtype,
                         rmr_get_meid(rmrMsg, (unsigned char *) meid));
        }
    }
    message.peerInfo->gotSetup = true;
    buildJsonMessage(message);

    if (rmrMsg != nullptr) {
        rmr_free_msg(rmrMsg);
    }
    free(buffer);

    return;
}

#if 0
int RAN_Function_list_To_Vector(RANfunctions_List_t& list, vector <string> &runFunXML_v) {
    auto index = 0;
    runFunXML_v.clear();
    for (auto j = 0; j < list.list.count; j++) {
        auto *raNfunctionItemIEs = (RANfunction_ItemIEs_t *)list.list.array[j];
        if (raNfunctionItemIEs->id == ProtocolIE_ID_id_RANfunction_Item &&
            (raNfunctionItemIEs->value.present == RANfunction_ItemIEs__value_PR_RANfunction_Item)) {
            // encode to xml
            E2SM_gNB_NRT_RANfunction_Definition_t *ranFunDef = nullptr;
            auto rval = asn_decode(nullptr, ATS_ALIGNED_BASIC_PER,
                                   &asn_DEF_E2SM_gNB_NRT_RANfunction_Definition,
                                   (void **)&ranFunDef,
                                   raNfunctionItemIEs->value.choice.RANfunction_Item.ranFunctionDefinition.buf,
                                   raNfunctionItemIEs->value.choice.RANfunction_Item.ranFunctionDefinition.size);
            if (rval.code != RC_OK) {
                mdclog_write(MDCLOG_ERR, "Error %d Decoding (unpack) E2SM message from : %s",
                             rval.code,
                             asn_DEF_E2SM_gNB_NRT_RANfunction_Definition.name);
                return -1;
            }

            auto xml_buffer_size = RECEIVE_SCTP_BUFFER_SIZE * 2;
            unsigned char xml_buffer[RECEIVE_SCTP_BUFFER_SIZE * 2];
            memset(xml_buffer, 0, RECEIVE_SCTP_BUFFER_SIZE * 2);
            // encode to xml
            auto er = asn_encode_to_buffer(nullptr,
                                           ATS_BASIC_XER,
                                           &asn_DEF_E2SM_gNB_NRT_RANfunction_Definition,
                                           ranFunDef,
                                           xml_buffer,
                                           xml_buffer_size);
            if (er.encoded == -1) {
                mdclog_write(MDCLOG_ERR, "encoding of %s failed, %s",
                             asn_DEF_E2SM_gNB_NRT_RANfunction_Definition.name,
                             strerror(errno));
            } else if (er.encoded > (ssize_t)xml_buffer_size) {
                mdclog_write(MDCLOG_ERR, "Buffer of size %d is to small for %s, at %s line %d",
                             (int) xml_buffer_size,
                             asn_DEF_E2SM_gNB_NRT_RANfunction_Definition.name, __func__, __LINE__);
            } else {
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "Encoding E2SM %s PDU number %d : %s",
                                 asn_DEF_E2SM_gNB_NRT_RANfunction_Definition.name,
                                 index++,
                                 xml_buffer);
                }

                string runFuncs = (char *)(xml_buffer);
                runFunXML_v.emplace_back(runFuncs);
            }
        }
    }
    return 0;
}

int collectServiceUpdate_RequestData(E2AP_PDU_t *pdu,
                                     Sctp_Map_t *sctpMap,
                                     ReportingMessages_t &message,
                                     vector <string> &RANfunctionsAdded_v,
                                     vector <string> &RANfunctionsModified_v) {
    memset(message.peerInfo->enodbName, 0 , MAX_ENODB_NAME_SIZE);
    for (auto i = 0; i < pdu->choice.initiatingMessage->value.choice.RICserviceUpdate.protocolIEs.list.count; i++) {
        auto *ie = pdu->choice.initiatingMessage->value.choice.RICserviceUpdate.protocolIEs.list.array[i];
        if (ie->id == ProtocolIE_ID_id_RANfunctionsAdded) {
            if (ie->value.present == RICserviceUpdate_IEs__value_PR_RANfunctionsID_List) {
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "Run function list have %d entries",
                                 ie->value.choice.RANfunctions_List.list.count);
                }
                if (RAN_Function_list_To_Vector(ie->value.choice.RANfunctions_List, RANfunctionsAdded_v) != 0 ) {
                    return -1;
                }
            }
        } else if (ie->id == ProtocolIE_ID_id_RANfunctionsModified) {
            if (ie->value.present == RICserviceUpdate_IEs__value_PR_RANfunctions_List) {
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "Run function list have %d entries",
                                 ie->value.choice.RANfunctions_List.list.count);
                }
                if (RAN_Function_list_To_Vector(ie->value.choice.RANfunctions_List, RANfunctionsModified_v) != 0 ) {
                    return -1;
                }
            }
        }
    }
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Run function vector have %ld entries",
                     RANfunctionsAdded_v.size());
    }
    return 0;
}

#endif


void buildE2TPrometheusCounters(sctp_params_t &sctpParams) {
    sctpParams.e2tCounters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_E2setup)] = &sctpParams.prometheusFamily->Add({{"counter", "setup_request_messages_total"}});
    sctpParams.e2tCounters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_E2setup)] = &sctpParams.prometheusFamily->Add({{"counter", "setup_request_bytes_total"}});

    sctpParams.e2tCounters[OUT_SUCC][MSG_COUNTER][(ProcedureCode_id_E2setup)] = &sctpParams.prometheusFamily->Add({{"counter", "setup_response_messages_total"}});
    sctpParams.e2tCounters[OUT_SUCC][BYTES_COUNTER][(ProcedureCode_id_E2setup)] = &sctpParams.prometheusFamily->Add({{"counter", "setup_response_bytes_total"}});

    sctpParams.e2tCounters[OUT_UN_SUCC][MSG_COUNTER][ProcedureCode_id_E2setup] = &sctpParams.prometheusFamily->Add({{"counter", "setup_request_failure_messages_total"}});
    sctpParams.e2tCounters[OUT_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_E2setup] = &sctpParams.prometheusFamily->Add({{"counter", "setup_request_failure_bytes_total"}});

    sctpParams.e2tCounters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_ErrorIndication)] = &sctpParams.prometheusFamily->Add({{"counter", "error_indication_messages_total"}});
    sctpParams.e2tCounters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_ErrorIndication)] = &sctpParams.prometheusFamily->Add({{"counter", "error_indication_bytes_total"}});

    sctpParams.e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_Reset] = &sctpParams.prometheusFamily->Add({{"counter", "reset_request_messages_total"}});
    sctpParams.e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_Reset] = &sctpParams.prometheusFamily->Add({{"counter", "reset_request_bytes_total"}});

    sctpParams.e2tCounters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_Reset] = &sctpParams.prometheusFamily->Add({{"counter", "reset_ack_messages_total"}});
    sctpParams.e2tCounters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_Reset] = &sctpParams.prometheusFamily->Add({{"counter", "reset_ack_bytes_total"}});

    sctpParams.e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_update_messages_total"}});
    sctpParams.e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_update_bytes_total"}});

    sctpParams.e2tCounters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_update_response_messages_total"}});
    sctpParams.e2tCounters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_update_response_bytes_total"}});

    sctpParams.e2tCounters[OUT_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_update_failure_messages_total"}});
    sctpParams.e2tCounters[OUT_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_update_failure_bytes_total"}});

    sctpParams.e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICcontrol] = &sctpParams.prometheusFamily->Add({{"counter", "ric_control_messages_total"}});
    sctpParams.e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICcontrol] = &sctpParams.prometheusFamily->Add({{"counter", "ric_control_bytes_total"}});

    sctpParams.e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICcontrol] = &sctpParams.prometheusFamily->Add({{"counter", "ric_control_ack_messages_total"}});
    sctpParams.e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICcontrol] = &sctpParams.prometheusFamily->Add({{"counter", "ric_control_ack_bytes_total"}});

    sctpParams.e2tCounters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICcontrol] = &sctpParams.prometheusFamily->Add({{"counter", "ric_control_failure_messages_total"}});
    sctpParams.e2tCounters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICcontrol] = &sctpParams.prometheusFamily->Add({{"counter", "ric_control_failure_bytes_total"}});

    sctpParams.e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICsubscription] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_messages_total"}});
    sctpParams.e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICsubscription] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_bytes_total"}});

    sctpParams.e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscription] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_ack_messages_total"}});
    sctpParams.e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscription] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_ack_bytes_total"}});

    sctpParams.e2tCounters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscription] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_failure_messages_total"}});
    sctpParams.e2tCounters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscription] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_failure_bytes_total"}});

    sctpParams.e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_delete_messages_total"}});
    sctpParams.e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_delete_bytes_total"}});

    sctpParams.e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_delete_ack_messages_total"}});
    sctpParams.e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_delete_ack_bytes_total"}});

    sctpParams.e2tCounters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_delete_failure_messages_total"}});
    sctpParams.e2tCounters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete] = &sctpParams.prometheusFamily->Add({{"counter", "ric_subscription_delete_failure_bytes_total"}});

    sctpParams.e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_RICindication] = &sctpParams.prometheusFamily->Add({{"counter", "ric_indication_messages_total"}});
    sctpParams.e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_RICindication] = &sctpParams.prometheusFamily->Add({{"counter", "ric_indication_bytes_total"}});

    sctpParams.e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICserviceQuery] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_query_messages_total"}});
    sctpParams.e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICserviceQuery] = &sctpParams.prometheusFamily->Add({{"counter", "ric_service_query_bytes_total"}});
}

void buildPrometheusList(ConnectedCU_t *peerInfo, Family<Counter> *prometheusFamily) {
    peerInfo->counters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_E2setup)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"SetupRequest", "Messages"}});
    peerInfo->counters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_E2setup)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"SetupRequest", "Bytes"}});

    peerInfo->counters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_ErrorIndication)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"ErrorIndication", "Messages"}});
    peerInfo->counters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_ErrorIndication)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"ErrorIndication", "Bytes"}});

    peerInfo->counters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_RICindication)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICindication", "Messages"}});
    peerInfo->counters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_RICindication)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICindication", "Bytes"}});

    peerInfo->counters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"ResetRequest", "Messages"}});
    peerInfo->counters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"ResetRequest", "Bytes"}});

    peerInfo->counters[IN_INITI][MSG_COUNTER][(ProcedureCode_id_RICserviceUpdate)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICserviceUpdate", "Messages"}});
    peerInfo->counters[IN_INITI][BYTES_COUNTER][(ProcedureCode_id_RICserviceUpdate)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICserviceUpdate", "Bytes"}});
    // ---------------------------------------------
    peerInfo->counters[IN_SUCC][MSG_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"ResetACK", "Messages"}});
    peerInfo->counters[IN_SUCC][BYTES_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"ResetACK", "Bytes"}});

    peerInfo->counters[IN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICcontrol)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICcontrolACK", "Messages"}});
    peerInfo->counters[IN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICcontrol)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICcontrolACK", "Bytes"}});

    peerInfo->counters[IN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICsubscription)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionACK", "Messages"}});
    peerInfo->counters[IN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICsubscription)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionACK", "Bytes"}});

    peerInfo->counters[IN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICsubscriptionDelete)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionDeleteACK", "Messages"}});
    peerInfo->counters[IN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICsubscriptionDelete)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionDeleteACK", "Bytes"}});
    //-------------------------------------------------------------

    peerInfo->counters[IN_UN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICcontrol)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICcontrolFailure", "Messages"}});
    peerInfo->counters[IN_UN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICcontrol)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICcontrolFailure", "Bytes"}});

    peerInfo->counters[IN_UN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICsubscription)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionFailure", "Messages"}});
    peerInfo->counters[IN_UN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICsubscription)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionFailure", "Bytes"}});

    peerInfo->counters[IN_UN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICsubscriptionDelete)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionDeleteFailure", "Messages"}});
    peerInfo->counters[IN_UN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICsubscriptionDelete)] = &prometheusFamily->Add({{peerInfo->enodbName, "IN"}, {"RICsubscriptionDeleteFailure", "Bytes"}});

    //====================================================================================
    peerInfo->counters[OUT_INITI][MSG_COUNTER][(ProcedureCode_id_ErrorIndication)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"ErrorIndication", "Messages"}});
    peerInfo->counters[OUT_INITI][BYTES_COUNTER][(ProcedureCode_id_ErrorIndication)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"ErrorIndication", "Bytes"}});

    peerInfo->counters[OUT_INITI][MSG_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"ResetRequest", "Messages"}});
    peerInfo->counters[OUT_INITI][BYTES_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"ResetRequest", "Bytes"}});

    peerInfo->counters[OUT_INITI][MSG_COUNTER][(ProcedureCode_id_RICcontrol)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICcontrol", "Messages"}});
    peerInfo->counters[OUT_INITI][BYTES_COUNTER][(ProcedureCode_id_RICcontrol)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICcontrol", "Bytes"}});

    peerInfo->counters[OUT_INITI][MSG_COUNTER][(ProcedureCode_id_RICserviceQuery)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICserviceQuery", "Messages"}});
    peerInfo->counters[OUT_INITI][BYTES_COUNTER][(ProcedureCode_id_RICserviceQuery)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICserviceQuery", "Bytes"}});

    peerInfo->counters[OUT_INITI][MSG_COUNTER][(ProcedureCode_id_RICsubscription)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICsubscription", "Messages"}});
    peerInfo->counters[OUT_INITI][BYTES_COUNTER][(ProcedureCode_id_RICsubscription)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICsubscription", "Bytes"}});

    peerInfo->counters[OUT_INITI][MSG_COUNTER][(ProcedureCode_id_RICsubscriptionDelete)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICsubscriptionDelete", "Messages"}});
    peerInfo->counters[OUT_INITI][BYTES_COUNTER][(ProcedureCode_id_RICsubscriptionDelete)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICsubscriptionDelete", "Bytes"}});
    //---------------------------------------------------------------------------------------------------------
    peerInfo->counters[OUT_SUCC][MSG_COUNTER][(ProcedureCode_id_E2setup)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"SetupResponse", "Messages"}});
    peerInfo->counters[OUT_SUCC][BYTES_COUNTER][(ProcedureCode_id_E2setup)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"SetupResponse", "Bytes"}});

    peerInfo->counters[OUT_SUCC][MSG_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"ResetACK", "Messages"}});
    peerInfo->counters[OUT_SUCC][BYTES_COUNTER][(ProcedureCode_id_Reset)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"ResetACK", "Bytes"}});

    peerInfo->counters[OUT_SUCC][MSG_COUNTER][(ProcedureCode_id_RICserviceUpdate)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICserviceUpdateResponse", "Messages"}});
    peerInfo->counters[OUT_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICserviceUpdate)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICserviceUpdateResponse", "Bytes"}});
    //----------------------------------------------------------------------------------------------------------------
    peerInfo->counters[OUT_UN_SUCC][MSG_COUNTER][(ProcedureCode_id_E2setup)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"SetupRequestFailure", "Messages"}});
    peerInfo->counters[OUT_UN_SUCC][BYTES_COUNTER][(ProcedureCode_id_E2setup)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"SetupRequestFailure", "Bytes"}});

    peerInfo->counters[OUT_UN_SUCC][MSG_COUNTER][(ProcedureCode_id_RICserviceUpdate)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICserviceUpdateFailure", "Messages"}});
    peerInfo->counters[OUT_UN_SUCC][BYTES_COUNTER][(ProcedureCode_id_RICserviceUpdate)] = &prometheusFamily->Add({{peerInfo->enodbName, "OUT"}, {"RICserviceUpdateFailure", "Bytes"}});
}

/**
 *
 * @param pdu
 * @param sctpMap
 * @param message
 * @param RANfunctionsAdded_v
 * @return
 */
int collectSetupRequestData(E2AP_PDU_t *pdu,
                                     Sctp_Map_t *sctpMap,
                                     ReportingMessages_t &message /*, vector <string> &RANfunctionsAdded_v*/) {
    memset(message.peerInfo->enodbName, 0 , MAX_ENODB_NAME_SIZE);
    for (auto i = 0; i < pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.count; i++) {
        auto *ie = pdu->choice.initiatingMessage->value.choice.E2setupRequest.protocolIEs.list.array[i];
        if (ie->id == ProtocolIE_ID_id_GlobalE2node_ID) {
            // get the ran name for meid
            if (ie->value.present == E2setupRequestIEs__value_PR_GlobalE2node_ID) {
                if (buildRanName(message.peerInfo->enodbName, ie) < 0) {
                    mdclog_write(MDCLOG_ERR, "Bad param in E2setupRequestIEs GlobalE2node_ID.\n");
                    // no message will be sent
                    return -1;
                }

                memcpy(message.message.enodbName, message.peerInfo->enodbName, strlen(message.peerInfo->enodbName));
                sctpMap->setkey(message.message.enodbName, message.peerInfo);
            }
        } /*else if (ie->id == ProtocolIE_ID_id_RANfunctionsAdded) {
            if (ie->value.present == E2setupRequestIEs__value_PR_RANfunctions_List) {
                if (mdclog_level_get() >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "Run function list have %d entries",
                                 ie->value.choice.RANfunctions_List.list.count);
                }
                if (RAN_Function_list_To_Vector(ie->value.choice.RANfunctions_List, RANfunctionsAdded_v) != 0 ) {
                    return -1;
                }
            }
        } */
    }
//    if (mdclog_level_get() >= MDCLOG_DEBUG) {
//        mdclog_write(MDCLOG_DEBUG, "Run function vector have %ld entries",
//                     RANfunctionsAdded_v.size());
//    }
    return 0;
}

int XML_From_PER(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    E2AP_PDU_t *pdu = nullptr;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "got PER message of size %d is:%s",
                     rmrMessageBuffer.sendMessage->len, rmrMessageBuffer.sendMessage->payload);
    }
    auto rval = asn_decode(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, (void **) &pdu,
                           rmrMessageBuffer.sendMessage->payload, rmrMessageBuffer.sendMessage->len);
    if (rval.code != RC_OK) {
        mdclog_write(MDCLOG_ERR, "Error %d Decoding (unpack) setup response  from E2MGR : %s",
                     rval.code,
                     message.message.enodbName);
        if (pdu != nullptr) {
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        return -1;
    }

    int buff_size = RECEIVE_XAPP_BUFFER_SIZE;
    auto er = asn_encode_to_buffer(nullptr, ATS_BASIC_XER, &asn_DEF_E2AP_PDU, pdu,
                                   rmrMessageBuffer.sendMessage->payload, buff_size);
    if (er.encoded == -1) {
        mdclog_write(MDCLOG_ERR, "encoding of %s failed, %s", asn_DEF_E2AP_PDU.name, strerror(errno));
        if (pdu != nullptr) {
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        return -1;
    } else if (er.encoded > (ssize_t)buff_size) {
        mdclog_write(MDCLOG_ERR, "Buffer of size %d is to small for %s, at %s line %d",
                     (int)rmrMessageBuffer.sendMessage->len,
                     asn_DEF_E2AP_PDU.name,
                     __func__,
                     __LINE__);
        if (pdu != nullptr) {
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        return -1;
    }
    rmrMessageBuffer.sendMessage->len = er.encoded;
    if (pdu != nullptr) {
        ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
        pdu = nullptr;
    }
    return 0;

}

/**
 *
 * @param pdu
 * @param message
 * @param rmrMessageBuffer
 */
void asnInitiatingRequest(E2AP_PDU_t *pdu,
                          Sctp_Map_t *sctpMap,
                          ReportingMessages_t &message,
                          RmrMessagesBuffer_t &rmrMessageBuffer) {
    auto logLevel = mdclog_level_get();
    auto procedureCode = ((InitiatingMessage_t *) pdu->choice.initiatingMessage)->procedureCode;
    if (logLevel >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "Initiating message %ld\n", procedureCode);
    }
    switch (procedureCode) {
        case ProcedureCode_id_E2setup: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got E2setup");
            }

//            vector <string> RANfunctionsAdded_v;
//            vector <string> RANfunctionsModified_v;
//            RANfunctionsAdded_v.clear();
//            RANfunctionsModified_v.clear();
            if (collectSetupRequestData(pdu, sctpMap, message) != 0) {
                break;
            }

            buildPrometheusList(message.peerInfo, message.peerInfo->sctpParams->prometheusFamily);

            string messageName("E2setupRequest");
            string ieName("E2setupRequestIEs");
            message.message.messageType = RIC_E2_SETUP_REQ;
            message.peerInfo->counters[IN_INITI][MSG_COUNTER][ProcedureCode_id_E2setup]->Increment();
            message.peerInfo->counters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_E2setup]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_E2setup]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_E2setup]->Increment((double)message.message.asnLength);

            buildAndSendSetupRequest(message, rmrMessageBuffer, pdu);
            break;
        }
        case ProcedureCode_id_RICserviceUpdate: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICserviceUpdate %s", message.message.enodbName);
            }
//            vector <string> RANfunctionsAdded_v;
//            vector <string> RANfunctionsModified_v;
//            RANfunctionsAdded_v.clear();
//            RANfunctionsModified_v.clear();
//            if (collectServiceUpdate_RequestData(pdu, sctpMap, message,
//                                                 RANfunctionsAdded_v, RANfunctionsModified_v) != 0) {
//                break;
//            }

            string messageName("RICserviceUpdate");
            string ieName("RICserviceUpdateIEs");
            message.message.messageType = RIC_SERVICE_UPDATE;
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_INITI][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment();
            message.peerInfo->counters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment((double)message.message.asnLength);
#endif
            buildAndSendSetupRequest(message, rmrMessageBuffer, pdu);
            break;
        }
        case ProcedureCode_id_ErrorIndication: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got ErrorIndication %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_INITI][MSG_COUNTER][ProcedureCode_id_ErrorIndication]->Increment();
            message.peerInfo->counters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_ErrorIndication]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_ErrorIndication]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_ErrorIndication]->Increment((double)message.message.asnLength);
#endif
            if (sendRequestToXapp(message, RIC_ERROR_INDICATION, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "RIC_ERROR_INDICATION failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_Reset: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got Reset %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_INITI][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->counters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment((double)message.message.asnLength);
#endif
            if (XML_From_PER(message, rmrMessageBuffer) < 0) {
                break;
            }

            if (sendRequestToXapp(message, RIC_E2_RESET_REQ, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "RIC_E2_RESET_REQ message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_RICindication: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICindication %s", message.message.enodbName);
            }
            for (auto i = 0; i < pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.count; i++) {
                auto messageSent = false;
                RICindication_IEs_t *ie = pdu->choice.initiatingMessage->value.choice.RICindication.protocolIEs.list.array[i];
                if (logLevel >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "ie type (ProtocolIE_ID) = %ld", ie->id);
                }
                if (ie->id == ProtocolIE_ID_id_RICrequestID) {
                    if (logLevel >= MDCLOG_DEBUG) {
                        mdclog_write(MDCLOG_DEBUG, "Got RIC requestId entry, ie type (ProtocolIE_ID) = %ld", ie->id);
                    }
                    if (ie->value.present == RICindication_IEs__value_PR_RICrequestID) {
                        static unsigned char tx[32];
                        message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_INDICATION;
                        snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
                        rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, strlen((const char *) tx));
                        rmr_bytes2meid(rmrMessageBuffer.sendMessage,
                                       (unsigned char *)message.message.enodbName,
                                       strlen(message.message.enodbName));
                        rmrMessageBuffer.sendMessage->state = 0;
                        rmrMessageBuffer.sendMessage->sub_id = (int)ie->value.choice.RICrequestID.ricInstanceID;

                        //ie->value.choice.RICrequestID.ricInstanceID;
                        if (mdclog_level_get() >= MDCLOG_DEBUG) {
                            mdclog_write(MDCLOG_DEBUG, "sub id = %d, mtype = %d, ric instance id %ld, requestor id = %ld",
                                         rmrMessageBuffer.sendMessage->sub_id,
                                         rmrMessageBuffer.sendMessage->mtype,
                                         ie->value.choice.RICrequestID.ricInstanceID,
                                         ie->value.choice.RICrequestID.ricRequestorID);
                        }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                        
                        message.peerInfo->counters[IN_INITI][MSG_COUNTER][ProcedureCode_id_RICindication]->Increment();
                        message.peerInfo->counters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_RICindication]->Increment((double)message.message.asnLength);

                        // Update E2T instance level metrics
                        message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_RICindication]->Increment();
                        message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_RICindication]->Increment((double)message.message.asnLength);
#endif
                        sendRmrMessage(rmrMessageBuffer, message);
                        messageSent = true;
                    } else {
                        mdclog_write(MDCLOG_ERR, "RIC request id missing illegal request");
                    }
                }
                if (messageSent) {
                    break;
                }
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
}

/**
 *
 * @param pdu
 * @param message
 * @param rmrMessageBuffer
 */
void asnSuccessfulMsg(E2AP_PDU_t *pdu,
                      Sctp_Map_t *sctpMap,
                      ReportingMessages_t &message,
                      RmrMessagesBuffer_t &rmrMessageBuffer) {
    auto procedureCode = pdu->choice.successfulOutcome->procedureCode;
    auto logLevel = mdclog_level_get();
    if (logLevel >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Successful Outcome %ld", procedureCode);
    }
    switch (procedureCode) {
        case ProcedureCode_id_Reset: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got Reset %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->counters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment((double)message.message.asnLength);
#endif
            if (XML_From_PER(message, rmrMessageBuffer) < 0) {
                break;
            }
            if (sendRequestToXapp(message, RIC_E2_RESET_RESP, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "RIC_E2_RESET_RESP message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_RICcontrol: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICcontrol %s", message.message.enodbName);
            }
            for (auto i = 0;
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
//                        rmrMessageBuffer.sendMessage->sub_id = (int) ie->value.choice.RICrequestID.ricRequestorID;
                        rmrMessageBuffer.sendMessage->sub_id = (int)ie->value.choice.RICrequestID.ricInstanceID;

                        static unsigned char tx[32];
                        snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
                        rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, strlen((const char *) tx));
                        rmr_bytes2meid(rmrMessageBuffer.sendMessage,
                                       (unsigned char *)message.message.enodbName,
                                       strlen(message.message.enodbName));
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                        
                        message.peerInfo->counters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICcontrol]->Increment();
                        message.peerInfo->counters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICcontrol]->Increment((double)message.message.asnLength);

                        // Update E2T instance level metrics
                        message.peerInfo->sctpParams->e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICcontrol]->Increment();
                        message.peerInfo->sctpParams->e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICcontrol]->Increment((double)message.message.asnLength);
#endif
                        sendRmrMessage(rmrMessageBuffer, message);
                        messageSent = true;
                    } else {
                        mdclog_write(MDCLOG_ERR, "RIC request id missing illegal request");
                    }
                }
                if (messageSent) {
                    break;
                }
            }

            break;
        }
        case ProcedureCode_id_RICsubscription: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICsubscription %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscription]->Increment();
            message.peerInfo->counters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscription]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscription]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscription]->Increment((double)message.message.asnLength);
#endif
            if (sendRequestToXapp(message, RIC_SUB_RESP, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription successful message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_RICsubscriptionDelete: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICsubscriptionDelete %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment();
            message.peerInfo->counters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment((double)message.message.asnLength);
#endif
            if (sendRequestToXapp(message, RIC_SUB_DEL_RESP, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription delete successful message failed to send to xAPP");
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
}

/**
 *
 * @param pdu
 * @param message
 * @param rmrMessageBuffer
 */
void asnUnSuccsesfulMsg(E2AP_PDU_t *pdu,
                        Sctp_Map_t *sctpMap,
                        ReportingMessages_t &message,
                        RmrMessagesBuffer_t &rmrMessageBuffer) {
    auto procedureCode = pdu->choice.unsuccessfulOutcome->procedureCode;
    auto logLevel = mdclog_level_get();
    if (logLevel >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "Unsuccessful Outcome %ld", procedureCode);
    }
    switch (procedureCode) {
        case ProcedureCode_id_RICcontrol: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICcontrol %s", message.message.enodbName);
            }
            for (int i = 0;
                 i < pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.count; i++) {
                auto messageSent = false;
                RICcontrolFailure_IEs_t *ie = pdu->choice.unsuccessfulOutcome->value.choice.RICcontrolFailure.protocolIEs.list.array[i];
                if (logLevel >= MDCLOG_DEBUG) {
                    mdclog_write(MDCLOG_DEBUG, "ie type (ProtocolIE_ID) = %ld", ie->id);
                }
                if (ie->id == ProtocolIE_ID_id_RICrequestID) {
                    if (logLevel >= MDCLOG_DEBUG) {
                        mdclog_write(MDCLOG_DEBUG, "Got RIC requestId entry, ie type (ProtocolIE_ID) = %ld", ie->id);
                    }
                    if (ie->value.present == RICcontrolFailure_IEs__value_PR_RICrequestID) {
                        message.message.messageType = rmrMessageBuffer.sendMessage->mtype = RIC_CONTROL_FAILURE;
                        rmrMessageBuffer.sendMessage->state = 0;
//                        rmrMessageBuffer.sendMessage->sub_id = (int)ie->value.choice.RICrequestID.ricRequestorID;
                        rmrMessageBuffer.sendMessage->sub_id = (int)ie->value.choice.RICrequestID.ricInstanceID;
                        static unsigned char tx[32];
                        snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
                        rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, strlen((const char *) tx));
                        rmr_bytes2meid(rmrMessageBuffer.sendMessage, (unsigned char *) message.message.enodbName,
                                       strlen(message.message.enodbName));
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))                        
                        message.peerInfo->counters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICcontrol]->Increment();
                        message.peerInfo->counters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICcontrol]->Increment((double)message.message.asnLength);

                        // Update E2T instance level metrics
                        message.peerInfo->sctpParams->e2tCounters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICcontrol]->Increment();
                        message.peerInfo->sctpParams->e2tCounters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICcontrol]->Increment((double)message.message.asnLength);
#endif
                        sendRmrMessage(rmrMessageBuffer, message);
                        messageSent = true;
                    } else {
                        mdclog_write(MDCLOG_ERR, "RIC request id missing illegal request");
                    }
                }
                if (messageSent) {
                    break;
                }
            }
            break;
        }
        case ProcedureCode_id_RICsubscription: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICsubscription %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscription]->Increment();
            message.peerInfo->counters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscription]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscription]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscription]->Increment((double)message.message.asnLength);
#endif
            if (sendRequestToXapp(message, RIC_SUB_FAILURE, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription unsuccessful message failed to send to xAPP");
            }
            break;
        }
        case ProcedureCode_id_RICsubscriptionDelete: {
            if (logLevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got RICsubscriptionDelete %s", message.message.enodbName);
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment();
            message.peerInfo->counters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment((double)message.message.asnLength);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment((double)message.message.asnLength);
#endif
            if (sendRequestToXapp(message, RIC_SUB_FAILURE, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "Subscription Delete unsuccessful message failed to send to xAPP");
            }
            break;
        }
        default: {
            mdclog_write(MDCLOG_WARN, "Undefined or not supported message = %ld", procedureCode);
            message.message.messageType = 0; // no RMR message type yet
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            buildJsonMessage(message);
#endif            
            break;
        }
    }
}

/**
 *
 * @param message
 * @param requestId
 * @param rmrMmessageBuffer
 * @return
 */
int sendRequestToXapp(ReportingMessages_t &message,
                      int requestId,
                      RmrMessagesBuffer_t &rmrMmessageBuffer) {
    rmr_bytes2meid(rmrMmessageBuffer.sendMessage,
                   (unsigned char *)message.message.enodbName,
                   strlen(message.message.enodbName));
    message.message.messageType = rmrMmessageBuffer.sendMessage->mtype = requestId;
    rmrMmessageBuffer.sendMessage->state = 0;
    static unsigned char tx[32];
    snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
    rmr_bytes2xact(rmrMmessageBuffer.sendMessage, tx, strlen((const char *) tx));

    auto rc = sendRmrMessage(rmrMmessageBuffer, message);
    return rc;
}

/**
 *
 * @param pSctpParams
 */
void getRmrContext(sctp_params_t &pSctpParams) {
    pSctpParams.rmrCtx = nullptr;
    pSctpParams.rmrCtx = rmr_init(pSctpParams.rmrAddress, RECEIVE_XAPP_BUFFER_SIZE, RMRFL_NONE);
    if (pSctpParams.rmrCtx == nullptr) {
        mdclog_write(MDCLOG_ERR, "Failed to initialize RMR");
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
 * @param message
 * @param rmrMessageBuffer
 * @return
 */
int PER_FromXML(ReportingMessages_t &message, RmrMessagesBuffer_t &rmrMessageBuffer) {
    E2AP_PDU_t *pdu = nullptr;

    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "got xml Format  data from xApp of size %d is:%s",
                rmrMessageBuffer.rcvMessage->len, rmrMessageBuffer.rcvMessage->payload);
    }
    auto rval = asn_decode(nullptr, ATS_BASIC_XER, &asn_DEF_E2AP_PDU, (void **) &pdu,
                           rmrMessageBuffer.rcvMessage->payload, rmrMessageBuffer.rcvMessage->len);
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "%s After  decoding the XML to PDU", __func__ );
    }
    if (rval.code != RC_OK) {
#ifdef UNIT_TEST
    return 0;
#endif    
        mdclog_write(MDCLOG_ERR, "Error %d Decoding (unpack) setup response  from E2MGR : %s",
                     rval.code,
                     message.message.enodbName);
        if (pdu != nullptr) {
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        return -1;
    }

    int buff_size = RECEIVE_XAPP_BUFFER_SIZE;
    auto er = asn_encode_to_buffer(nullptr, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, pdu,
                                   rmrMessageBuffer.rcvMessage->payload, buff_size);
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "%s After encoding PDU to PER", __func__ );
    }
    if (er.encoded == -1) {
        mdclog_write(MDCLOG_ERR, "encoding of %s failed, %s", asn_DEF_E2AP_PDU.name, strerror(errno));
        if (pdu != nullptr) {
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        return -1;
    } else if (er.encoded > (ssize_t)buff_size) {
        mdclog_write(MDCLOG_ERR, "Buffer of size %d is to small for %s, at %s line %d",
                     (int)rmrMessageBuffer.rcvMessage->len,
                     asn_DEF_E2AP_PDU.name,
                     __func__,
                     __LINE__);
        if (pdu != nullptr) {
            ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
            pdu = nullptr;
        }
        return -1;
    }
    rmrMessageBuffer.rcvMessage->len = er.encoded;
    if (pdu != nullptr) {
        ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, pdu);
        pdu = nullptr;
    }
    return 0;
}

/**
 *
 * @param sctpMap
 * @param rmrMessageBuffer
 * @param ts
 * @return
 */
int receiveXappMessages(Sctp_Map_t *sctpMap,
                        RmrMessagesBuffer_t &rmrMessageBuffer,
                        struct timespec &ts) {
    int loglevel = mdclog_level_get();
    if (rmrMessageBuffer.rcvMessage == nullptr) {
        //we have error
        mdclog_write(MDCLOG_ERR, "RMR Allocation message, %s", strerror(errno));
        return -1;
    }

//    if (loglevel >= MDCLOG_DEBUG) {
//        mdclog_write(MDCLOG_DEBUG, "Call to rmr_rcv_msg");
//    }
    rmrMessageBuffer.rcvMessage = rmr_rcv_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.rcvMessage);
    if (rmrMessageBuffer.rcvMessage == nullptr) {
        mdclog_write(MDCLOG_ERR, "RMR Receiving message with null pointer, Reallocated rmr message buffer");
        rmrMessageBuffer.rcvMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
        return -2;
    }
    ReportingMessages_t message;
    message.message.direction = 'D';
    message.message.time.tv_nsec = ts.tv_nsec;
    message.message.time.tv_sec = ts.tv_sec;

    // get message payload
    //auto msgData = msg->payload;
#ifdef UNIT_TEST
    rmrMessageBuffer.rcvMessage->state = 0;
#endif
    if (rmrMessageBuffer.rcvMessage->state != 0) {
        mdclog_write(MDCLOG_ERR, "RMR Receiving message with stat = %d", rmrMessageBuffer.rcvMessage->state);
        return -1;
    }
    rmr_get_meid(rmrMessageBuffer.rcvMessage, (unsigned char *)message.message.enodbName);
    message.peerInfo = (ConnectedCU_t *) sctpMap->find(message.message.enodbName);
    if (message.peerInfo == nullptr) {
        auto type = rmrMessageBuffer.rcvMessage->mtype;
        switch (type) {
            case RIC_SCTP_CLEAR_ALL:
            case E2_TERM_KEEP_ALIVE_REQ:
            case RIC_HEALTH_CHECK_REQ:
                break;
            default:
#ifdef UNIT_TEST
    break;
#endif    
                mdclog_write(MDCLOG_ERR, "Failed to send message no CU entry %s", message.message.enodbName);
                return -1;
        }
    }

    if (rmrMessageBuffer.rcvMessage->mtype != RIC_HEALTH_CHECK_REQ) {
        num_of_XAPP_messages.fetch_add(1, std::memory_order_release);

    }
    switch (rmrMessageBuffer.rcvMessage->mtype) {
        case RIC_E2_SETUP_RESP : {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_E2_SETUP_RESP");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_E2setup]->Increment();
            message.peerInfo->counters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_E2setup]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_E2setup]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_E2setup]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_E2_SETUP_RESP");
                return -6;
            }
            break;
        }
        case RIC_E2_SETUP_FAILURE : {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_E2_SETUP_FAILURE");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_UN_SUCC][MSG_COUNTER][ProcedureCode_id_E2setup]->Increment();
            message.peerInfo->counters[OUT_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_E2setup]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_UN_SUCC][MSG_COUNTER][ProcedureCode_id_E2setup]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_E2setup]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_E2_SETUP_FAILURE");
                return -6;
            }
            break;
        }
        case RIC_ERROR_INDICATION: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_ERROR_INDICATION");
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_ErrorIndication]->Increment();
            message.peerInfo->counters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_ErrorIndication]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_ErrorIndication]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_ErrorIndication]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_ERROR_INDICATION");
                return -6;
            }
            break;
        }
        case RIC_SUB_REQ: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_SUB_REQ");
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICsubscription]->Increment();
            message.peerInfo->counters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICsubscription]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICsubscription]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICsubscription]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SUB_REQ");
                return -6;
            }
            break;
        }
        case RIC_SUB_DEL_REQ: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_SUB_DEL_REQ");
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment();
            message.peerInfo->counters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICsubscriptionDelete]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SUB_DEL_REQ");
                return -6;
            }
            break;
        }
        case RIC_CONTROL_REQ: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_CONTROL_REQ");
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICcontrol]->Increment();
            message.peerInfo->counters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICcontrol]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICcontrol]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICcontrol]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_CONTROL_REQ");
                return -6;
            }
            break;
        }
        case RIC_SERVICE_QUERY: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_SERVICE_QUERY");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICserviceQuery]->Increment();
            message.peerInfo->counters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICserviceQuery]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_RICserviceQuery]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_RICserviceQuery]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SERVICE_QUERY");
                return -6;
            }
            break;
        }
        case RIC_SERVICE_UPDATE_ACK: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_SERVICE_UPDATE_ACK");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                mdclog_write(MDCLOG_ERR, "error in PER_FromXML");
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment();
            message.peerInfo->counters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Before sending to CU");
            }
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SERVICE_UPDATE_ACK");
                return -6;
            }
            break;
        }
        case RIC_SERVICE_UPDATE_FAILURE: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_SERVICE_UPDATE_FAILURE");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment();
            message.peerInfo->counters[OUT_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_UN_SUCC][MSG_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_UN_SUCC][BYTES_COUNTER][ProcedureCode_id_RICserviceUpdate]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_SERVICE_UPDATE_FAILURE");
                return -6;
            }
            break;
        }
        case RIC_E2_RESET_REQ: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_E2_RESET_REQ");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_INITI][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->counters[OUT_INITI][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->sctpParams->e2tCounters[IN_INITI][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_E2_RESET");
                return -6;
            }
            break;
        }
        case RIC_E2_RESET_RESP: {
            if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "RIC_E2_RESET_RESP");
            }
            if (PER_FromXML(message, rmrMessageBuffer) != 0) {
                break;
            }
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            message.peerInfo->counters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->counters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment(rmrMessageBuffer.rcvMessage->len);

            // Update E2T instance level metrics
            message.peerInfo->sctpParams->e2tCounters[OUT_SUCC][MSG_COUNTER][ProcedureCode_id_Reset]->Increment();
            message.peerInfo->sctpParams->e2tCounters[OUT_SUCC][BYTES_COUNTER][ProcedureCode_id_Reset]->Increment(rmrMessageBuffer.rcvMessage->len);
#endif            
            if (sendDirectionalSctpMsg(rmrMessageBuffer, message, 0, sctpMap) != 0) {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_E2_RESET_RESP");
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
                    if (sendRequestToXapp(message, RIC_SCTP_CONNECTION_FAILURE, rmrMessageBuffer) != 0) {
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
            rmr_bytes2payload(rmrMessageBuffer.sendMessage,
                              (unsigned char *)rmrMessageBuffer.ka_message,
                              rmrMessageBuffer.ka_message_len);
            rmrMessageBuffer.sendMessage->mtype = E2_TERM_KEEP_ALIVE_RESP;
            rmrMessageBuffer.sendMessage->state = 0;
            static unsigned char tx[32];
            auto txLen = snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
            rmr_bytes2xact(rmrMessageBuffer.sendMessage, tx, txLen);
#if !(defined(UNIT_TEST) || defined(MODULE_TEST))            
            rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);
#endif
            if (rmrMessageBuffer.sendMessage == nullptr) {
                rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
                mdclog_write(MDCLOG_ERR, "Failed to send E2_TERM_KEEP_ALIVE_RESP RMR message returned NULL");
            } else if (rmrMessageBuffer.sendMessage->state != 0)  {
                mdclog_write(MDCLOG_ERR, "Failed to send E2_TERM_KEEP_ALIVE_RESP, on RMR state = %d ( %s)",
                             rmrMessageBuffer.sendMessage->state, translateRmrErrorMessages(rmrMessageBuffer.sendMessage->state).c_str());
            } else if (loglevel >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "Got Keep Alive Request send : %s", rmrMessageBuffer.ka_message);
            }

            break;
        }
        case RIC_HEALTH_CHECK_REQ: {
            static int counter = 0;
            // send message back
            rmr_bytes2payload(rmrMessageBuffer.rcvMessage,
                              (unsigned char *)"OK",
                              2);
            rmrMessageBuffer.rcvMessage->mtype = RIC_HEALTH_CHECK_RESP;
            rmrMessageBuffer.rcvMessage->state = 0;
            static unsigned char tx[32];
            auto txLen = snprintf((char *) tx, sizeof tx, "%15ld", transactionCounter++);
            rmr_bytes2xact(rmrMessageBuffer.rcvMessage, tx, txLen);
            rmrMessageBuffer.rcvMessage = rmr_rts_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.rcvMessage);
            //rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);
            if (rmrMessageBuffer.rcvMessage == nullptr) {
                rmrMessageBuffer.rcvMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_HEALTH_CHECK_RESP RMR message returned NULL");
            } else if (rmrMessageBuffer.rcvMessage->state != 0)  {
                mdclog_write(MDCLOG_ERR, "Failed to send RIC_HEALTH_CHECK_RESP, on RMR state = %d ( %s)",
                             rmrMessageBuffer.rcvMessage->state, translateRmrErrorMessages(rmrMessageBuffer.rcvMessage->state).c_str());
            } else if (loglevel >= MDCLOG_DEBUG && ++counter % 100 == 0) {
                mdclog_write(MDCLOG_DEBUG, "Got %d RIC_HEALTH_CHECK_REQ Request send : OK", counter);
            }

            break;
        }

        default:
            mdclog_write(MDCLOG_WARN, "Message Type : %d is not supported", rmrMessageBuffer.rcvMessage->mtype);
            message.message.asndata = rmrMessageBuffer.rcvMessage->payload;
            message.message.asnLength = rmrMessageBuffer.rcvMessage->len;
            message.message.time.tv_nsec = ts.tv_nsec;
            message.message.time.tv_sec = ts.tv_sec;
            message.message.messageType = rmrMessageBuffer.rcvMessage->mtype;

            buildJsonMessage(message);


            return -7;
    }
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "EXIT OK from %s", __FUNCTION__);
    }
    return 0;
}

/**
 * Send message to the CU that is not expecting for successful or unsuccessful results
 * @param messageBuffer
 * @param message
 * @param failedMsgId
 * @param sctpMap
 * @return
 */
int sendDirectionalSctpMsg(RmrMessagesBuffer_t &messageBuffer,
                           ReportingMessages_t &message,
                           int failedMsgId,
                           Sctp_Map_t *sctpMap) {
    if (mdclog_level_get() >= MDCLOG_DEBUG) {
        mdclog_write(MDCLOG_DEBUG, "send message: %d to %s address", message.message.messageType, message.message.enodbName);
    }

    getRequestMetaData(message, messageBuffer);
    if (mdclog_level_get() >= MDCLOG_INFO) {
        mdclog_write(MDCLOG_INFO, "send message to %s address", message.message.enodbName);
    }

    auto rc = sendMessagetoCu(sctpMap, messageBuffer, message, failedMsgId);
    return rc;
}

/**
 *
 * @param sctpMap
 * @param messageBuffer
 * @param message
 * @param failedMesgId
 * @return
 */
int sendMessagetoCu(Sctp_Map_t *sctpMap,
                    RmrMessagesBuffer_t &messageBuffer,
                    ReportingMessages_t &message,
                    int failedMesgId) {
    // get the FD
    message.message.messageType = messageBuffer.rcvMessage->mtype;
    auto rc = sendSctpMsg(message.peerInfo, message, sctpMap);
    return rc;
}


/**
 *
 * @param epoll_fd
 * @param peerInfo
 * @param events
 * @param sctpMap
 * @param enodbName
 * @param msgType
 * @return
 */
int addToEpoll(int epoll_fd,
               ConnectedCU_t *peerInfo,
               uint32_t events,
               Sctp_Map_t *sctpMap,
               char *enodbName,
               int msgType) {
    // Add to Epol
    struct epoll_event event{};
    event.data.ptr = peerInfo;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, peerInfo->fileDescriptor, &event) < 0) {
#if !(defined(UNIT_TEST) || defined(MODULE_TEST)) 
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "epoll_ctl EPOLL_CTL_ADD (may check not to quit here), %s, %s %d",
                         strerror(errno), __func__, __LINE__);
        }
        close(peerInfo->fileDescriptor);
        if (enodbName != nullptr) {
            cleanHashEntry(peerInfo, sctpMap);
            char key[MAX_ENODB_NAME_SIZE * 2];
            snprintf(key, MAX_ENODB_NAME_SIZE * 2, "msg:%s|%d", enodbName, msgType);
            if (mdclog_level_get() >= MDCLOG_DEBUG) {
                mdclog_write(MDCLOG_DEBUG, "remove key = %s from %s at line %d", key, __FUNCTION__, __LINE__);
            }
            auto tmp = sctpMap->find(key);
            if (tmp) {
                free(tmp);
                sctpMap->erase(key);
            }
        } else {
            peerInfo->enodbName[0] = 0;
        }
        mdclog_write(MDCLOG_ERR, "epoll_ctl EPOLL_CTL_ADD (may check not to quit here)");
        return -1;
#endif
    }
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
 * @return
 */
int modifyToEpoll(int epoll_fd,
                  ConnectedCU_t *peerInfo,
                  uint32_t events,
                  Sctp_Map_t *sctpMap,
                  char *enodbName,
                  int msgType) {
    // Add to Epol
    struct epoll_event event{};
    event.data.ptr = peerInfo;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, peerInfo->fileDescriptor, &event) < 0) {
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "epoll_ctl EPOLL_CTL_MOD (may check not to quit here), %s, %s %d",
                         strerror(errno), __func__, __LINE__);
        }
        close(peerInfo->fileDescriptor);
        cleanHashEntry(peerInfo, sctpMap);
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
        mdclog_write(MDCLOG_ERR, "epoll_ctl EPOLL_CTL_ADD (may check not to quit here)");
        return -1;
    }
    return 0;
}


int sendRmrMessage(RmrMessagesBuffer_t &rmrMessageBuffer, ReportingMessages_t &message) {
    buildJsonMessage(message);
#ifndef UNIT_TEST
    rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);
#else
    rmrMessageBuffer.sendMessage->state = RMR_ERR_RETRY;
#endif
    if (rmrMessageBuffer.sendMessage == nullptr) {
        rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
        mdclog_write(MDCLOG_ERR, "RMR failed send message returned with NULL pointer");
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
#ifndef UNIT_TEST
            rmrMessageBuffer.sendMessage = rmr_send_msg(rmrMessageBuffer.rmrCtx, rmrMessageBuffer.sendMessage);
#endif
            if (rmrMessageBuffer.sendMessage == nullptr) {
                mdclog_write(MDCLOG_ERR, "RMR failed send message returned with NULL pointer");
                rmrMessageBuffer.sendMessage = rmr_alloc_msg(rmrMessageBuffer.rmrCtx, RECEIVE_XAPP_BUFFER_SIZE);
                return -1;
            } else if (rmrMessageBuffer.sendMessage->state != 0) {
                mdclog_write(MDCLOG_ERR,
                             "Message state %s while sending request %d to Xapp from %s after retry of 10 microseconds",
                             translateRmrErrorMessages(rmrMessageBuffer.sendMessage->state).c_str(),
                             rmrMessageBuffer.sendMessage->mtype,
                             rmr_get_meid(rmrMessageBuffer.sendMessage, (unsigned char *)meid));
                auto rc = rmrMessageBuffer.sendMessage->state;
                return rc;
            }
        } else {
            mdclog_write(MDCLOG_ERR, "Message state %s while sending request %d to Xapp from %s",
                         translateRmrErrorMessages(rmrMessageBuffer.sendMessage->state).c_str(),
                         rmrMessageBuffer.sendMessage->mtype,
                         rmr_get_meid(rmrMessageBuffer.sendMessage, (unsigned char *)meid));
            return rmrMessageBuffer.sendMessage->state;
        }
    }
    return 0;
}

void buildJsonMessage(ReportingMessages_t &message) {
#ifdef UNIT_TEST
    jsonTrace = true;
#endif
    if (jsonTrace) {
        message.outLen = sizeof(message.base64Data);
        base64::encode((const unsigned char *) message.message.asndata,
                       (const int) message.message.asnLength,
                       message.base64Data,
                       message.outLen);
        if (mdclog_level_get() >= MDCLOG_DEBUG) {
            mdclog_write(MDCLOG_DEBUG, "Tracing: ASN length = %d, base64 message length = %d ",
                         (int) message.message.asnLength,
                         (int) message.outLen);
        }

        snprintf(message.buffer, sizeof(message.buffer),
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
            str = "RMR_ERR_BADARG - argument passed to function was unusable";
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
