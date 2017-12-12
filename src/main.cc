#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <cstring>
#include <memory>
#include <iostream>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <chrono>

extern "C"
{
#include <sndfile.h>
}

#include <respeaker.h>
#include <chain_nodes/pulse_collector_node.h>
#include <chain_nodes/vep_aec_bf_node.h>
#include <chain_nodes/vep_doa_kws_node.h>

#include "json.hpp"
#include "cppcodec/base64_default_rfc4648.hpp"

// enable asio debugging
// #ifndef NDEBUG
// #define ASIO_ENABLE_BUFFER_DEBUGGING
// #define ASIO_ENABLE_HANDLER_TRACKING
// #endif
using namespace respeaker;
using json = nlohmann::json;
using base64 = cppcodec::base64_rfc4648;
using SteadyClock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<SteadyClock>;

#define SOCKET_FILE    "/tmp/respeakerd.sock"
#define BLOCK_SIZE_MS    8
#define RECV_BUFF_LEN    1024
#define STOP_CAPTURE_TIMEOUT    30000    //millisecond

static bool stop = false;
static char recv_buffer[RECV_BUFF_LEN];

void SignalHandler(int signal){
    if (signal == SIGPIPE){
        std::cerr << "Caught signal SIGPIPE" << std::endl;
        return;
    }
    std::cerr << "Caught signal " << signal << ", terminating..." << std::endl;
    stop = true;
}

bool blocking_send(int client, std::string data)
{
    // prepare to send response
    std::string response = data;
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                std::cerr << "socket write error" << std::endl;
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return true;
}

std::string cut_line(int client)
{
    int     nfds;                      
    fd_set  readfds;                       
    struct  timeval tv;                      
    tv.tv_sec = 0;                      
    tv.tv_usec = 1000;
    std::string request = "";

    // read until we get a newline
    while (request.find("\r\n") == std::string::npos) {
        FD_ZERO(&readfds);                      
        FD_SET(client, &readfds);                       
        nfds = select(client+1, &readfds, NULL, NULL, &tv);
        if(nfds < 0){
            std::cerr << "select  error" << std::endl;
            return "";               
        }else if(nfds == 0){
            //std::cout << "timeout  error" << std::endl;
            return "";
        }else{
            int nread = recv(client, recv_buffer, RECV_BUFF_LEN, 0);
            if (nread < 0) {
                if (errno == EINTR)
                    // the socket call was interrupted -- try again
                    continue;
                else
                    // an error occurred, so break out
                    return "";
            } else if (nread == 0) {
                // the socket is closed
                return "";
            }
            // be sure to use append in case we have binary data
            request.append(recv_buffer, nread);
        }  
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    return request;
}

int main(int argc, char *argv[]) 
{
    // signal process
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = SignalHandler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);
    sigaction(SIGTERM, &sig_int_handler, NULL);
    sigaction(SIGPIPE, &sig_int_handler, NULL);

    ////
    std::string source = "default";
    bool enable_wav_log = true;

    // init librespeaker
    std::unique_ptr<PulseCollectorNode> collector;
    std::unique_ptr<VepAecBeamformingNode> vep_bf;
    std::unique_ptr<VepDoaKwsNode> vep_kws;
    std::unique_ptr<ReSpeaker> respeaker;

    collector.reset(PulseCollectorNode::Create(source, 16000, BLOCK_SIZE_MS));
    vep_bf.reset(VepAecBeamformingNode::Create(6, enable_wav_log));
    // TODO: user gflags to pass in the resource path on command line
    // "./resources/snowboy.umdl"
    vep_kws.reset(VepDoaKwsNode::Create("./resources/common.res",
                                        "./resources/alexa/alexa_02092017.umdl",
                                        "0.2"));
    //vep_kws->DisableAutoStateTransfer();
    //collector->BindToCore(0);
    //vep_bf->BindToCore(1);
    vep_kws->BindToCore(2);

    vep_bf->Uplink(collector.get());
    vep_kws->Uplink(vep_bf.get());

    respeaker.reset(ReSpeaker::Create());
    respeaker->RegisterChainByHead(collector.get());
    respeaker->RegisterDirectionReporterNode(vep_kws.get());
    respeaker->RegisterHotwordDetectionNode(vep_kws.get());
    respeaker->RegisterOutputNode(vep_kws.get());

    
    int sock, client_sock, rval, un_size ;
    struct sockaddr_un server, new_addr;
    char buf[1024];
    bool socket_error, detected;
    int dir;
    std::string one_block, one_line;
    std::string event_pkt_str, audio_pkt_str;
    int frames;
    size_t base64_len;
    int nfds;
    fd_set  testfds;
    struct  timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    int counter;
    uint16_t tick;

    TimePoint on_detected;

    SNDFILE	*file ;
    SF_INFO	sfinfo ;

    // init the socket
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "error when opening stream socket" << std::endl;
        exit(1);
    }
    // init bind
    memset(&server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCKET_FILE);
    unlink(SOCKET_FILE);
    if ((bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) < 0){
        std::cerr << "error when binding stream socket" << std::endl;
        close(sock);
        exit(1);
    }
    // only accept 1 client
    if ((listen(sock, 1)) < 0){
        std::cerr << "cannot listen the client connect request" << std::endl;
        close(sock);
        exit(1);
    }


    while(!stop){
        un_size = sizeof(struct sockaddr_un);
        client_sock = accept(sock, (struct sockaddr *)&new_addr, &un_size);
        if (client_sock == -1) {
            std::cerr << "socket accept error" << std::endl;
            continue;
        }else{
            std::cout << "accepted socket client " << client_sock << std::endl;

            if (!respeaker->Start(&stop)) {
                std::cerr << "Can not start the respeaker node chain." << std::endl;
                return -1;
            }

            size_t num_channels = respeaker->GetNumOutputChannels();
            int rate = respeaker->GetNumOutputRate();
            std::cout << "respeakerd output: num channels: " << num_channels << ", rate: " << rate << std::endl;

            // init libsndfile
            memset (&sfinfo, 0, sizeof (sfinfo));
            sfinfo.samplerate	= rate ;
            sfinfo.channels		= num_channels ;
            sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ;
            std::ostringstream   file_name;
            file_name << "record_respeakerd_" << client_sock << ".wav";
            if (! (file = sf_open (file_name.str().c_str(), SFM_WRITE, &sfinfo)))
            {
                std::cerr << "Error : Not able to open output file." << std::endl;
                return -1 ;
            }

            socket_error = false;
            counter = 0;
            tick = 0;
            while (!stop && !socket_error)
            {
                // check if the client socket is still alive
                FD_ZERO(&testfds);                      
                FD_SET(client_sock, &testfds);                       
                nfds = select(client_sock + 1, &testfds, NULL, NULL, &tv);
                //std::cout << "-" << std::flush;
                //std::cout << nfds << std::endl;
                if (nfds > 0)
                {
                    if (recv(client_sock, recv_buffer, RECV_BUFF_LEN, 0) <= 0) {
                        if (errno != EINTR) {
                            std::cerr << "client socket is closed, drop it" << std::endl;
                            socket_error = true;
                            break;
                        }
                    }
                }

                // if (counter++ > 100) {
                //     counter = 0;
                //     if (!blocking_send(client_sock, "\r\n")) {
                //         break;
                //     }
                // }

                if (tick++ % 100 == 0) {
                    std::cout << "collector: " << collector->GetQueueDeepth() << ", vep1: " <<
                    vep_bf->GetQueueDeepth() << ", vep2: " << vep_kws->GetQueueDeepth() << std::endl;
                }

                //if have a client connected,this respeaker always detect hotword,if there are hotword,send event and audio.
                one_block = respeaker->DetectHotword(detected);

                frames = one_block.length() / (sizeof(int16_t) * num_channels);
                sf_writef_short(file, (const int16_t *)(one_block.data()), frames);

                //std::cout << "+" << std::endl;

                if (detected) {
                    dir = respeaker->GetDirection();  

                    on_detected = SteadyClock::now();

                    // send the event to client right now
                    json event = {{"type", "event"}, {"data", "hotword"}, {"direction", dir}};
                    event_pkt_str = event.dump();
                    event_pkt_str += "\r\n";

                    if(!blocking_send(client_sock, event_pkt_str))
                        socket_error = true;

                    // now listen the data
                    while (!stop && !socket_error) {
                        // 1st, check stop_capture command
                        one_line = cut_line(client_sock);
                        if(one_line != ""){
                            if(one_line.find("stop_capture") != std::string::npos) {
                                std::cout << "stop_capture" << std::endl;
                                break;
                            }
                                
                        }
                        if (SteadyClock::now() - on_detected > std::chrono::milliseconds(STOP_CAPTURE_TIMEOUT)) {
                            std::cout << "stop_capture by timeout" << std::endl;
                            break;
                        }

                        if (tick++ % 100 == 0) {
                            std::cout << "collector: " << collector->GetQueueDeepth() << ", vep1: " <<
                            vep_bf->GetQueueDeepth() << ", vep2: " << vep_kws->GetQueueDeepth() << std::endl;
                        }

                        one_block = respeaker->Listen(BLOCK_SIZE_MS);
                        //std::cout << "*" << std::flush;
                        frames = one_block.length() / (sizeof(int16_t) * num_channels);
                        sf_writef_short(file, (const int16_t *)(one_block.data()), frames);

                        // send the data to client
                        // base64_len = one_block.length();
                        // std::cout << base64_len << std::endl;
                        json audio = {{"type", "audio"}, {"data", base64::encode(one_block)}, {"direction", dir}};
                        audio_pkt_str = audio.dump();
                        audio_pkt_str += "\r\n";

                        if(!blocking_send(client_sock, audio_pkt_str))
                            socket_error = true;
                    }
                    //std::this_thread::sleep_for(std::chrono::seconds(2));
                    respeaker->SetChainState(WAIT_TRIGGER_WITH_BGM);
                }
            } // while
            respeaker->Stop();
            std::cout << "librespeaker cleanup done." << std::endl;
            sf_close (file);
        }
        close(client_sock);
    }
    close(sock);
    unlink(SOCKET_FILE);

    return 0;
}
