#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
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
#include "gflags/gflags.h"

using namespace respeaker;
using json = nlohmann::json;
using base64 = cppcodec::base64_rfc4648;
using SteadyClock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<SteadyClock>;

#define SOCKET_FILE    "/tmp/respeakerd.sock"
#define BLOCK_SIZE_MS    8
#define RECV_BUFF_LEN    1024
#define STOP_CAPTURE_TIMEOUT    15000    //millisecond
#define WAIT_READY_TIMEOUT      30000    //millisecond
#define SKIP_KWS_TIME_ON_SPEAK  2000     //millisecond

DEFINE_string(snowboy_res_path, "./resources/common.res", "the path to snowboay's resource file");
DEFINE_string(snowboy_model_path, "./resources/alexa.umdl", "the path to snowboay's model file");
DEFINE_string(snowboy_sensitivity, "0.5", "the sensitivity of snowboay");
DEFINE_string(source, "default", "the source of pulseaudio");
DEFINE_int32(agc_level, -10, "dBFS for AGC, the range is [-31, 0]");
DEFINE_bool(debug, false, "print more message");
DEFINE_bool(enable_wav_log, false, "enable VEP to log its input and output into wav files");

static bool stop = false;
static char recv_buffer[RECV_BUFF_LEN];
std::string recv_string;

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

std::string cut_line(int client, bool &alive)
{
    int     nfds;
    fd_set  readfds;
    struct  timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    size_t idx;
    std::string line;

    while (true) {
        // read until we get a newline
        while ((idx = recv_string.find("\r\n")) == std::string::npos) {
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
                    alive = false;
                    return "";
                }
                // be sure to use append in case we have binary data
                recv_string.append(recv_buffer, nread);
                //std::cout << recv_string << std::endl;
            }
        }

        line = recv_string.substr(0, idx + 2);
        recv_string.erase(0, idx + 2);

        if (line.find("[0]") != std::string::npos) continue;
        else break;
    }

    return line;
}

bool is_client_alive(int client_sock)
{
    int nfds;
    fd_set  testfds;
    struct  timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    FD_ZERO(&testfds);
    FD_SET(client_sock, &testfds);
    nfds = select(client_sock + 1, &testfds, NULL, NULL, &tv);
    if (nfds > 0)
    {
        if (recv(client_sock, recv_buffer, RECV_BUFF_LEN, 0) <= 0) {
            if (errno != EINTR) {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // signal process
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = SignalHandler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);
    sigaction(SIGTERM, &sig_int_handler, NULL);
    sigaction(SIGPIPE, &sig_int_handler, NULL);

    std::cout << "source: " << FLAGS_source << std::endl;
    std::cout << "enable_wav_log: " << FLAGS_enable_wav_log << std::endl;
    std::cout << "snowboy_res_path: " << FLAGS_snowboy_res_path << std::endl;
    std::cout << "snowboy_model_path: " << FLAGS_snowboy_model_path << std::endl;
    std::cout << "snowboy_sensitivity: " << FLAGS_snowboy_sensitivity << std::endl;
    std::cout << "agc_level: " << FLAGS_agc_level << std::endl;

    // init librespeaker
    std::unique_ptr<PulseCollectorNode> collector;
    std::unique_ptr<VepAecBeamformingNode> vep_bf;
    std::unique_ptr<VepDoaKwsNode> vep_kws;
    std::unique_ptr<ReSpeaker> respeaker;

    collector.reset(PulseCollectorNode::Create(FLAGS_source, 16000, BLOCK_SIZE_MS));
    vep_bf.reset(VepAecBeamformingNode::Create(6, FLAGS_enable_wav_log));
    vep_kws.reset(VepDoaKwsNode::Create(FLAGS_snowboy_res_path,
                                        FLAGS_snowboy_model_path,
                                        FLAGS_snowboy_sensitivity,
                                        10,
                                        true));
    vep_kws->DisableAutoStateTransfer();
    vep_kws->SetTriggerPostConfirmThresholdTime(160);
    vep_kws->SetAgcTargetLevelDbfs((int)std::abs(FLAGS_agc_level));
    //collector->BindToCore(0);
    //vep_bf->BindToCore(1);
    vep_kws->BindToCore(2);

    vep_bf->Uplink(collector.get());
    vep_kws->Uplink(vep_bf.get());

    if (FLAGS_debug) {
        respeaker.reset(ReSpeaker::Create(DEBUG_LOG_LEVEL));
    } else {
        respeaker.reset(ReSpeaker::Create(INFO_LOG_LEVEL));
    }
    respeaker->RegisterChainByHead(collector.get());
    respeaker->RegisterDirectionReporterNode(vep_kws.get());
    respeaker->RegisterHotwordDetectionNode(vep_kws.get());
    respeaker->RegisterOutputNode(vep_kws.get());



    int sock, client_sock, rval, un_size ;
    struct sockaddr_un server, new_addr;
    char buf[1024];
    bool socket_error, detected, cloud_ready;
    int dir;
    std::string one_block, one_line;
    std::string event_pkt_str, audio_pkt_str;
    int frames;
    size_t base64_len;

    int counter;
    uint16_t tick;

    TimePoint on_detected, cur_time, on_speak;

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
    if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0){
        std::cerr << "error when binding stream socket" << std::endl;
        close(sock);
        exit(1);
    }
    if (chmod(SOCKET_FILE, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
        std::cerr << "error when changing the permission of the socket file" << std::endl;
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

            respeaker->Pause();
            std::cout << "waiting for ready signal..." << std::endl;

            cur_time = SteadyClock::now();
            socket_error = false;
            cloud_ready = false;

            // wait the cloud to be ready
            while (!stop && !socket_error)
            {
                // check if the client socket is still alive
                //if (!is_client_alive(client_sock)) {
                //    std::cerr << "client socket is closed, drop it" << std::endl;
                //    socket_error = true;
                //    break;
                //}

                // wait for the ready signal
                bool alive = true;
                one_line = cut_line(client_sock, alive);
                if (!alive) {
                    std::cerr << "client socket is closed, drop it" << std::endl;
                    socket_error = true;
                    break;
                }
                if (one_line != "") {
                    //std::cout << one_line << std::endl;
                    if(one_line.find("ready") != std::string::npos) {
                        cloud_ready = true;
                        std::cout << "cloud ready" << std::endl;
                        break;
                    }
                }
                if (SteadyClock::now() - cur_time > std::chrono::milliseconds(WAIT_READY_TIMEOUT)) {
                    std::cout << "wait ready timeout" << std::endl;
                    socket_error = true;
                    break;
                }
            }

            if (!socket_error) respeaker->Resume();

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

            counter = 0;
            tick = 0;
            while (!stop && !socket_error)
            {
                // check if the client socket is still alive
                //if (!is_client_alive(client_sock)) {
                //    std::cerr << "client socket is closed, drop it" << std::endl;
                //    socket_error = true;
                //    break;
                //}

                // check cloud ready status
                bool alive = true;
                do {
                    one_line = cut_line(client_sock, alive);
                    if (!alive) {
                        std::cerr << "client socket is closed, drop it" << std::endl;
                        socket_error = true;
                        break;
                    }
                    if (one_line != "") {
                        //std::cout << one_line << std::endl;
                        if (one_line.find("ready") != std::string::npos) {
                            cloud_ready = true;
                            //std::cout << "cloud ready" << std::endl;
                        } else if (one_line.find("connecting") != std::string::npos) {
                            cloud_ready = false;
                            std::cout << "cloud is reconnecting..." << std::endl;
                        } else if (one_line.find("on_speak") != std::string::npos) {
                            on_speak = SteadyClock::now();
                            if (FLAGS_debug) std::cout << "on_speak..." << std::endl;
                        }
                    }
                } while (one_line != "");

                if (FLAGS_debug && tick++ % 12 == 0) {
                    std::cout << "collector: " << collector->GetQueueDeepth() << ", vep1: " <<
                    vep_bf->GetQueueDeepth() << ", vep2: " << vep_kws->GetQueueDeepth() << std::endl;
                }

                //if have a client connected,this respeaker always detect hotword,if there are hotword,send event and audio.
                one_block = respeaker->DetectHotword(detected);

                frames = one_block.length() / (sizeof(int16_t) * num_channels);
                sf_writef_short(file, (const int16_t *)(one_block.data()), frames);

                if (FLAGS_debug && detected && (SteadyClock::now() - on_speak) <= std::chrono::milliseconds(SKIP_KWS_TIME_ON_SPEAK)) {
                    std::cout << "detected, but skipped!" << std::endl;
                }

                if (detected && cloud_ready && (SteadyClock::now() - on_speak) > std::chrono::milliseconds(SKIP_KWS_TIME_ON_SPEAK)) {
                    dir = respeaker->GetDirection();

                    on_detected = SteadyClock::now();

                    // send the event to client right now
                    json event = {{"type", "event"}, {"data", "hotword"}, {"direction", dir}};
                    event_pkt_str = event.dump();
                    event_pkt_str += "\r\n";

                    if (!blocking_send(client_sock, event_pkt_str)) {
                        socket_error = true;
                    }
                }
                if (cloud_ready) {
                    json audio = {{"type", "audio"}, {"data", base64::encode(one_block)}, {"direction", dir}};
                    audio_pkt_str = audio.dump();
                    audio_pkt_str += "\r\n";

                    if (!blocking_send(client_sock, audio_pkt_str)) {
                        socket_error = true;
                    }
                }
            } // while
            respeaker->Stop();
            std::cout << "librespeaker cleanup done." << std::endl;
            sf_close (file);
        }
        close(client_sock);
    }
    close(sock);

    return 0;
}
