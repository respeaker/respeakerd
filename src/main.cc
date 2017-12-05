


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <cstring>
#include <memory>
#include <iostream>
#include <csignal>

#include <respeaker.h>
#include <chain_nodes/pulse_collector_node.h>
#include <chain_nodes/vep_aec_bf_node.h>
#include <chain_nodes/vep_doa_kws_node.h>

#include "json.hpp"

// enable asio debugging
// #ifndef NDEBUG
// #define ASIO_ENABLE_BUFFER_DEBUGGING
// #define ASIO_ENABLE_HANDLER_TRACKING
// #endif

using namespace respeaker;
using json = nlohmann::json;

#define SOCKET_FILE    "/tmp/respeakerd.sock"
#define BLOCK_SIZE_MS    8
#define RECV_BUFF_LEN    1024

static bool stop = false;
static char recv_buffer[RECV_BUFF_LEN];

void SignalHandler(int signal){
  std::cerr << "Caught signal " << signal << ", terminating..." << std::endl;
  stop = true;
}

bool blocking_send(int client, std::string data)
{
    // prepare to send response
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
    string request = "";
    // read until we get a newline
    while (request.find("\r\n") == string::npos) {
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
    // a better server would cut off anything after the newline and
    // save it in a cache
    return request;
}

int main(int argc, char *argv[]) 
{
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = SignalHandler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);
    sigaction(SIGTERM, &sig_int_handler, NULL);

    // init librespeaker
    unique_ptr<PulseCollectorNode> collector;
    unique_ptr<VepAecBeamformingNode> vep_bf;
    unique_ptr<VepDoaKwsNode> vep_kws;
    unique_ptr<ReSpeaker> respeaker;

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
    respeaker->RegisterOutputNode(vep_kws.get());

    // init the socket
    int sock, client_sock, rval;
    struct sockaddr_un server;
    char buf[1024];

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "opening stream socket" << std::endl;
        exit(1);
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCKET_FILE);
    if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
        std::cerr << "binding stream socket" << std::endl;
        exit(1);
    }

    // only accept 1 client
    listen(sock, 1);

    

    while(!stop){
        client_sock = accept(sock, 0, 0);
        if (client_sock == -1) {
            std::cerr << "socket accept error" << std::endl;
            continue
        }else{
            if (!respeaker->Start(&stop)) {
                std::cerr << "Can not start the respeaker node chain." << std::endl;
                return -1;
            }

            bool socket_error, detected;
            int dir;
            std::string data;
            int frames;
            size_t num_channels = respeaker->GetNumOutputChannels();
            int rate = respeaker->GetNumOutputRate();

            std::cout << "num channels: " << num_channels << ", rate: " << rate << std::endl;

            socket_error = false;

            while (!stop && !socket_error) {
                detected = respeaker->DetectHotword();

                if (detected) {
                    dir = respeaker->GetDirection();

                    // send the event to client right now
                    json event = {{"type", "event"}, {"data", "hotword"}, {"direction", dir}};
                    std::string event_str = event.dump();
                    event_str += "\r\n";

                    if(!blocking_send(client_sock, event_str))
                        socket_error = true;

                    // now listen the data
                    while (!stop && !socket_error) {
                        // 1st, check stop_capture command
                        // if (stop_capture) break;

                        data = respeaker->Listen(BLOCK_SIZE_MS);
                        // send the data to client

                    }
                }
            } // while
            respeaker->Stop();
            std::cout << "librespeaker cleanup done." << std::endl;
        }
        close(client_sock);
    }

}

