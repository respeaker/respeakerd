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
#include <csignal>
#include <cstdlib>
#include <cstdio>

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

#define SOCKET_FILE        "/tmp/respeakerd.sock"
#define BLOCK_SIZE_MS      8
#define RECV_BUFF_LEN      1024
#define cmd_silence_gap_ms 10000

static bool stop = false;
static bool signal_pipe = false;
static char recv_buffer[RECV_BUFF_LEN];

void SignalHandler(int signal){
    std::cerr << "Caught signal " << signal << ", terminating..." << std::endl;
    stop = true;
}

bool blocking_send(int client, std::string data)
{
    // prepare to send response
    std::string response = data;
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten, nfds;                
    fd_set  writefds;                    
    struct  timeval tv;                      
    tv.tv_sec = 0;                      
    tv.tv_usec = 1000;
    // loop to be sure it is all sent
    while (nleft) {
        FD_ZERO(&writefds);                      
        FD_SET(client, &writefds);                       
        nfds = select(client+1, NULL, &writefds, NULL, &tv); 
        if(nfds < 0){
            std::cerr << "blocking_send() function select error" << std::endl;                          
            return false;               
        }else if(nfds == 0){
            //std::cout << "blocking_send() select timeout" << std::endl;
            continue;
        }else{
            if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
                if (errno == EINTR) {
                    // the socket call was interrupted -- try again
                    continue;
                } else {
                    // an error occurred, so break out
                    std::cerr << "blocking_send() function send error" << std::endl;
                    return false;
                }
            } else if (nwritten == 0) {
                // the socket is closed
                std::cout << "blocking_send() function send judge socket is closed" <<  std::endl;
                return false;
            }else{
                nleft -= nwritten;
                ptr += nwritten;
                ////std::cout << "Successfully send the data to the client" <<  std::endl;
            }
        }       
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
    memset(recv_buffer, 0, RECV_BUFF_LEN);
    // read until we get a newline
    while (request.find("\r\n") == std::string::npos) {
        FD_ZERO(&readfds);                      
        FD_SET(client, &readfds);                       
        nfds = select(client+1, &readfds, NULL, NULL, &tv); 
        if(nfds < 0){
            std::cerr << "cut_line() function select error" << std::endl;                          
            return "error";               
        }else if(nfds == 0){
            //std::cout << "cut line select timeout  error" << std::endl;
            return "";
        }else{
            int nread = recv(client, recv_buffer, RECV_BUFF_LEN, 0);
            if (nread < 0) {
                if (errno == EINTR)
                    // the socket call was interrupted -- try again
                    continue;
                else
                    // an error occurred, so break out
                    return "error";
            } else if (nread == 0) {
                // the socket is closed
                std::cerr << "cut_line() function recv error, the socket is closed" << std::endl;
                return "error";
            }
            // be sure to use append in case we have binary data
            request.append(recv_buffer, nread);
        }  
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    return request;
}
//if have a client connect, this server always judge the socket status
bool judge_socket_status(int client)
{
    int     nfds;                      
    fd_set  readfds;                       
    struct  timeval tv;                      
    tv.tv_sec = 0;                      
    tv.tv_usec = 1000;
    FD_ZERO(&readfds);                      
    FD_SET(client, &readfds);    
    //std::cout << "11111111" <<  std::endl;                   
    nfds = select(client+1, &readfds, NULL, NULL, &tv); 
    if(nfds < 0){
        std::cerr << "judge_socket_status1 function select error" << std::endl;                          
        return false;               
    }else if(nfds == 0){
        //std::cout << "judge_socket_status1 select timeout  error" << std::endl;
        return true;
    }else{
        int nread = recv(client, recv_buffer, RECV_BUFF_LEN, 0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                return true;
            else
                // an error occurred, so break out
                return false;
        } else if (nread == 0) {
            // the socket is closed
            return false;
        }
    }
    memset(recv_buffer, 0, RECV_BUFF_LEN);
    return true;
}

int main(int argc, char *argv[]) 
{
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = SignalHandler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);
    sigaction(SIGTERM, &sig_int_handler, NULL);
    //signal(SIGPIPE, SIG_IGN);

    std::string source = "default";
    // init librespeaker
    std::unique_ptr<PulseCollectorNode> collector;
    std::unique_ptr<VepAecBeamformingNode> vep_bf;
    std::unique_ptr<VepDoaKwsNode> vep_kws;
    std::unique_ptr<ReSpeaker> respeaker;

    collector.reset(PulseCollectorNode::Create(source, 16000, BLOCK_SIZE_MS));
    vep_bf.reset(VepAecBeamformingNode::Create(6));
    // TODO: user gflags to pass in the resource path on command line
    vep_kws.reset(VepDoaKwsNode::Create("./resources/common.res",
                                        "./resources/alexa/alexa_02092017.umdl",
                                        "0.5"));
    //vep_kws->DisableAutoStateTransfer();
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
    // init the socket
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "opening stream socket error" << std::endl;
        exit(1);
    }
    // init bind
    memset(&server, 0, sizeof(server));
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCKET_FILE);
    unlink(SOCKET_FILE);
    if ((bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) < 0){
        std::cerr << "binding stream socket error" << std::endl;
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
        // init accept, wait client connect
        un_size = sizeof(struct sockaddr_un);
        client_sock = accept(sock, (struct sockaddr *)&new_addr, &un_size);
        if (client_sock == -1) {
            //std::cerr << "socket accept error" << std::endl;
            continue;
        }else{
            std::cout << "Have a client connected" << std::endl;
            if (!respeaker->Start(&stop)) {
                std::cerr << "Can not start the respeaker node chain." << std::endl;
                exit(1);
            }

            bool socket_error, detected;
            int dir, frames;
            std::string data, client_data;
            size_t base64_len;
            size_t num_channels = respeaker->GetNumOutputChannels();
            int rate = respeaker->GetNumOutputRate();
            std::cout << "num channels: " << num_channels << ", rate: " << rate << std::endl;

            TimePoint silence_time;
            bool cmd_ended = false;
            socket_error = false;
            memset(recv_buffer, 0, RECV_BUFF_LEN);
            while (!stop && !socket_error) {            
            /*    if(judge_socket_status(client_sock) == false){
                    socket_error = true;       
                    std::cout << "judge client socket is disconnect" << std::endl;  
                }
            */
                //if have a client connected,this respeaker always detect hotword,if there are hotword,send event and audio.
                detected = respeaker->DetectHotword(); 
                
                if (detected) {
                    std::cout << "detect Hotword" <<  std::endl;
                    dir = respeaker->GetDirection(); 
                    silence_time = SteadyClock::now(); 

                    // send the event to client right now
                    json event = {{"type", "event"}, {"data", "hotword"}, {"direction", dir}};
                    std::string event_str = event.dump();
                    event_str += "\r\n";
                    cmd_ended = false;

                    if(!blocking_send(client_sock, event_str))
                        socket_error = true;

                    // now listen the data
                    while (!stop && !socket_error) {
                        // 1st, check stop_capture command
                        client_data = cut_line(client_sock);
                        if(client_data != ""){
                            if(client_data.find("error") != std::string::npos){
                                std::cout << "receive client data error" <<  std::endl;
                                socket_error = true;
                                break;
                            }
                            if(client_data.find("stop_capture") != std::string::npos){
                                std::cout << "receive stop capture cmd" <<  std::endl;
                                cmd_ended = true;
                                break;
                            }     
                        }
                        if(!cmd_ended){
                            if(SteadyClock::now() - silence_time > std::chrono::milliseconds(cmd_silence_gap_ms)){
                                std::cout << "timeout not receive stop capture" <<  std::endl;
                                cmd_ended = false;
                                break;
                            }
                        }
                        data = respeaker->Listen(BLOCK_SIZE_MS);
                        // send the data to client
                        //base64_len = strlen(data.c_str());
                        json event = {{"type", "audio"}, {"data", base64::encode(data)}, {"direction", dir}};
                        std::string event_str = event.dump();
                        event_str += "\r\n";

                        if(!blocking_send(client_sock, event_str))
                            socket_error = true;
                    }
                    respeaker->SetChainState(WAIT_TRIGGER_WITH_BGM);
                }
            } // while
            respeaker->Stop();
            std::cout << "librespeaker cleanup done." << std::endl;
            close(client_sock);
        }
        std::cout << "close a client connect" << std::endl;
    }
    close(sock);
    std::cout << "server main finish." << std::endl;
    return 0;
}

