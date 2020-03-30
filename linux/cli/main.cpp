#include <iostream>

#include <thread>
#include <mutex>
#include <map>
#include "easywsclient.hpp"
#include <curl/curl.h>
#include "json/json.h"
#include <assert.h>
#include "juice/juice.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
typedef websocketpp::client<websocketpp::config::asio_client> client;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

#define BUFFER_SIZE 4096
//using easywsclient::WebSocket;
typedef int socket_t;
class webrtc_handler;
client c;
websocketpp::connection_hdl hdl;
//WebSocket::pointer ws;
std::mutex mu;
std::string key, username, password, ip, port;
int myid;

std::map<int, webrtc_handler *> Users;
struct sockaddr_in sockaddrdatax;
struct sockaddr_in mainserver;
class webrtc_handler
{

public:
    int id;
    void setremote_sdp(std::string);
    void senddata(std::string);
    void setremote_candidate(std::string);
    static void on_state_changed(juice_agent_t *agent, juice_state_t state, void *user_ptr);
    static void on_candidate(juice_agent_t *agent, const char *sdp, void *user_ptr, int userid);
    static void on_gathering_done(juice_agent_t *agent, void *user_ptr);
    static void on_recv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);
    static void on_recv1(socket_t socket, const char *data, size_t size, void *user_ptr, struct sockaddr *sockaddrdata, int userid);
    void gather_candidate();
    webrtc_handler(int);
    webrtc_handler(int, std::string);
    juice_agent_t *agent;
};

void webrtc_handler::on_state_changed(juice_agent_t *agent, juice_state_t state, void *user_ptr)
{
    if (state == JUICE_STATE_CONNECTED)
    {
        std::cout << "[State: Connected]" << std::endl;
        /*
        if (myid == -1)
        {
            //std::string message = "I Am Admin";
            //juice_send(agent, message.c_str(), strlen(message.c_str()));
        }
        else
        {
            std::string message = "I Am User";
            juice_send(agent, message.c_str(), strlen(message.c_str()));
        }*/
    }
    else if (state == JUICE_STATE_CONNECTING)
    {
        std::cout << "[State: Connecting]" << std::endl;
    }
    else if (state == JUICE_STATE_FAILED)
    {
        std::cout << "[State: Failed]" << std::endl;
    }
}
void webrtc_handler::on_candidate(juice_agent_t *agent, const char *sdp, void *user_ptr, int userid)
{
    std::cout << "[Candidate]\n"
              << sdp << std::endl;

    Json::StyledWriter writer;
    Json::Value json_data;
    json_data["action"] = "send_candidate";
    json_data["payload"]["username"] = username;
    json_data["payload"]["candidate"] = sdp;
    if (myid == -1)
    {
        json_data["payload"]["id"] = userid;
        json_data["payload"]["type"] = "admin";
    }
    else
    {
        json_data["payload"]["id"] = myid;
        json_data["payload"]["type"] = "user";
    }
    websocketpp::lib::error_code ec;
    c.send(hdl, writer.write(json_data), websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    //  ws->send(writer.write(json_data).c_str());
}
void webrtc_handler::on_gathering_done(juice_agent_t *agent, void *user_ptr) {}
void webrtc_handler::on_recv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr)
{
    char buffer[BUFFER_SIZE];
    if (size > BUFFER_SIZE - 1)
        size = BUFFER_SIZE - 1;
    memcpy(buffer, data, size);
    buffer[size] = '\0';
    std::cout << "Message: " << buffer << std::endl;
    socket_t socket = get_agent_socket(agent);

    if (myid == -1)
    {

        sendto(socket, (const char *)buffer, strlen(buffer), 0, (struct sockaddr *)&mainserver, sizeof(mainserver));
    }
    else
    {
        //const struct sockaddr *servaddr = Users.find(-1)->second->sockaddrdata;
        //sendto(socket, (const char *)buffer, strlen(buffer), 0, sockaddrdatax, sizeof(*sockaddrdatax));
        sendto(socket, (const char *)buffer, strlen(buffer), 0, (struct sockaddr *)&sockaddrdatax, sizeof(sockaddrdatax));
        //std::cout << inet_ntoa(sockaddrdatax.sin_addr) << ":" << sockaddrdatax.sin_port << std::endl;
    }
}

void webrtc_handler::on_recv1(socket_t socket, const char *data, size_t size, void *user_ptr, struct sockaddr *sockaddrdata, int userid)
{

    char buffer[BUFFER_SIZE];
    if (size > BUFFER_SIZE - 1)
        size = BUFFER_SIZE - 1;
    memcpy(buffer, data, size);
    buffer[size] = '\0';

    std::cout << "Message1: " << buffer << std::endl;
    if (myid == -1)
    {

        juice_send(Users.find(userid)->second->agent, buffer, strlen(buffer));
    }
    else
    {
        struct sockaddr_in *b = (struct sockaddr_in *)sockaddrdata;
        memset(&sockaddrdatax, 0, sizeof(sockaddrdatax));
        sockaddrdatax.sin_addr = b->sin_addr;
        sockaddrdatax.sin_family = b->sin_family;
        sockaddrdatax.sin_port = b->sin_port;

        //std::cout << inet_ntoa(b->sin_addr) << ":" << b->sin_port << std::endl;
        juice_send(Users.find(-1)->second->agent, buffer, strlen(buffer));
    }
}
void webrtc_handler::gather_candidate()
{
    juice_gather_candidates(agent);
}

void webrtc_handler::setremote_sdp(std::string sdp)
{
    juice_set_remote_description(agent, sdp.c_str());
    Json::StyledWriter writer;
    Json::Value json_data;
    json_data["action"] = "answer_set";
    json_data["payload"]["username"] = username;
    json_data["payload"]["id"] = id;
    websocketpp::lib::error_code ec;
    c.send(hdl, writer.write(json_data), websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    // ws->send(writer.write(json_data).c_str());
}

void webrtc_handler::senddata(std::string data)
{
}

void webrtc_handler::setremote_candidate(std::string candidate)
{
    juice_add_remote_candidate(agent, candidate.c_str());
}

webrtc_handler::webrtc_handler(int userid)
{
    id = userid;

    juice_config_t config;
    memset(&config, 0, sizeof(config));
    config.stun_server_host = "stun.l.google.com";
    config.stun_server_port = 19302;
    config.cb_state_changed = on_state_changed;
    config.cb_candidate = on_candidate;
    config.cb_gathering_done = on_gathering_done;
    config.cb_recv = on_recv;
    config.user_ptr = NULL;
    config.cb_recv1 = on_recv1;

    agent = juice_create(&config, userid);

    char sdp[JUICE_MAX_SDP_STRING_LEN];
    juice_get_local_description(agent, sdp, JUICE_MAX_SDP_STRING_LEN);
    std::cout << "[Local Description:]\n"
              << sdp << std::endl;

    Json::StyledWriter writer;
    Json::Value json_data;
    json_data["action"] = "send_offer";
    json_data["payload"]["username"] = username;
    json_data["payload"]["sdp"] = sdp;
    json_data["payload"]["id"] = id;
    websocketpp::lib::error_code ec;
    c.send(hdl, writer.write(json_data), websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    // ws->send(writer.write(json_data).c_str());
}

webrtc_handler::webrtc_handler(int userid, std::string remote_sdp)
{
    id = userid;
    juice_config_t config;
    memset(&config, 0, sizeof(config));
    config.stun_server_host = "stun.l.google.com";
    config.stun_server_port = 19302;
    config.cb_state_changed = on_state_changed;
    config.cb_candidate = on_candidate;
    config.cb_gathering_done = on_gathering_done;
    config.cb_recv = on_recv;
    config.user_ptr = NULL;
    config.cb_recv1 = on_recv1;

    agent = juice_create(&config, userid);

    juice_set_remote_description(agent, remote_sdp.c_str());
    char sdp[JUICE_MAX_SDP_STRING_LEN];
    juice_get_local_description(agent, sdp, JUICE_MAX_SDP_STRING_LEN);
    std::cout << "[Local Description:]\n"
              << sdp << std::endl;

    Json::StyledWriter writer;
    Json::Value json_data;
    json_data["action"] = "send_answer";
    json_data["payload"]["username"] = username;
    json_data["payload"]["sdp"] = sdp;
    json_data["payload"]["id"] = myid;
    websocketpp::lib::error_code ec;
    c.send(hdl, writer.write(json_data), websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    //  ws->send(writer.write(json_data).c_str());
}
void on_message(client *c, websocketpp::connection_hdl hdl, message_ptr msg)
{
    /* std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;*/
    Json::Reader reader;
    Json::Value response_json;
    bool parse_status = reader.parse(msg->get_payload(), response_json);
    if (!parse_status)
    {
        std::cerr << "parse() failed: " << reader.getFormattedErrorMessages() << std::endl;
        exit(0);
    }
    else
    {
        std::string action = response_json["action"].asString();
        if (action.compare("join_admin_response") == 0)
        {
            myid = -1;
            std::cout << response_json["message"] << std::endl;
            if (!response_json["status"].asBool())
            {
                exit(0);
            }
        }
        else if (action.compare("join_response") == 0)
        {

            std::cout << response_json["message"] << std::endl;
            myid = response_json["id"].asInt();
            if (!response_json["status"].asBool())
            {
                exit(0);
            }
        }
        else if (action.compare("create_offer") == 0)
        {
            int id = response_json["id"].asInt();
            webrtc_handler *h = new webrtc_handler(id);
            Users.insert({id, h});
        }
        else if (action.compare("accept_offer") == 0)
        {

            std::string sdp = response_json["payload"]["sdp"].asString();
            webrtc_handler *h = new webrtc_handler(-1, sdp);
            Users.insert({-1, h});
        }
        else if (action.compare("accept_answer") == 0)
        {
            std::string sdp = response_json["payload"]["sdp"].asString();
            int id = response_json["payload"]["id"].asInt();
            Users.find(id)->second->setremote_sdp(sdp);
        }
        else if (action.compare("accept_candidate") == 0)
        {
            std::string candidate = response_json["payload"]["candidate"].asString();
            if (myid == -1)
            {
                int id = response_json["payload"]["id"].asInt();
                Users.find(id)->second->setremote_candidate(candidate);
            }
            else
            {
                Users.find(-1)->second->setremote_candidate(candidate);
            }
        }
        else if (action.compare("error") == 0)
        {
            std::cout << response_json["message"] << std::endl;
        }
        else if (action.compare("gather_candidate") == 0)
        {
            if (myid == -1)
            {
                int id = response_json["id"].asInt();
                Users.find(id)->second->gather_candidate();
            }
            else
            {
                Users.find(-1)->second->gather_candidate();
            }
        }
    }
}
void websocket_handler()
{
    std::string uri = "ws://p2pgaming.eu-gb.cf.appdomain.cloud/ws";

    try
    {
        // Set logging to be pretty verbose (everything except message payloads)
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize ASIO
        c.init_asio();

        // Register our message handler
        c.set_message_handler(bind(&on_message, &c, ::_1, ::_2));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        hdl = con->get_handle();
        if (ec)
        {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            exit(0);
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        c.connect(con);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run()
        // will exit when this connection is closed.
        c.run();
    }
    catch (websocketpp::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
}
/*
void websocket_handler()
{
    ws = WebSocket::from_url("ws://p2pgaming.herokuapp.com/ws");
    assert(ws);
    while (ws->getReadyState() != WebSocket::CLOSED)
    {
        mu.lock();

        ws->poll();
        ws->dispatch([](std::string msg) {
            Json::Reader reader;
            Json::Value response_json;
            bool parse_status = reader.parse(msg, response_json);
            if (!parse_status)
            {
                std::cerr << "parse() failed: " << reader.getFormattedErrorMessages() << std::endl;
                exit(0);
            }
            else
            {
                std::string action = response_json["action"].asString();
                if (action.compare("join_admin_response") == 0)
                {
                    myid = -1;
                    std::cout << response_json["message"] << std::endl;
                    if (!response_json["status"].asBool())
                    {
                        exit(0);
                    }
                }
                else if (action.compare("join_response") == 0)
                {

                    std::cout << response_json["message"] << std::endl;
                    myid = response_json["id"].asInt();
                    if (!response_json["status"].asBool())
                    {
                        exit(0);
                    }
                }
                else if (action.compare("create_offer") == 0)
                {
                    int id = response_json["id"].asInt();
                    webrtc_handler *h = new webrtc_handler(id);
                    Users.insert({id, h});
                }
                else if (action.compare("accept_offer") == 0)
                {

                    std::string sdp = response_json["payload"]["sdp"].asString();
                    webrtc_handler *h = new webrtc_handler(-1, sdp);
                    Users.insert({-1, h});
                }
                else if (action.compare("accept_answer") == 0)
                {
                    std::string sdp = response_json["payload"]["sdp"].asString();
                    int id = response_json["payload"]["id"].asInt();
                    Users.find(id)->second->setremote_sdp(sdp);
                }
                else if (action.compare("accept_candidate") == 0)
                {
                    std::string candidate = response_json["payload"]["candidate"].asString();
                    if (myid == -1)
                    {
                        int id = response_json["payload"]["id"].asInt();
                        Users.find(id)->second->setremote_candidate(candidate);
                    }
                    else
                    {
                        Users.find(-1)->second->setremote_candidate(candidate);
                    }
                }
                else if (action.compare("error") == 0)
                {
                    std::cout << response_json["message"] << std::endl;
                }
                else if (action.compare("gather_candidate") == 0)
                {
                    if (myid == -1)
                    {
                        int id = response_json["id"].asInt();
                        Users.find(id)->second->gather_candidate();
                    }
                    else
                    {
                        Users.find(-1)->second->gather_candidate();
                    }
                }
            }
        });
        mu.unlock();
    }

    delete ws;
}*/
std::size_t write_callback(char *in, size_t size, size_t nmemb, std::string *out)
{
    std::size_t total_size = size * nmemb;
    if (total_size)
    {
        out->append(in, total_size);
        return total_size;
    }
    return 0;
}

void ping()
{

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {

        curl_easy_setopt(curl, CURLOPT_URL, "https://p2pgaming.eu-gb.cf.appdomain.cloud/");

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cout << "Error occured!" << std::endl;
            exit(0);
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void create_room()
{
    int sucess = 0;
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        Json::Value json_data;
        json_data["username"] = username;
        json_data["password"] = password;

        Json::StyledWriter writer;

        curl_easy_setopt(curl, CURLOPT_URL, "https://p2pgaming.eu-gb.cf.appdomain.cloud/create_game");

        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, writer.write(json_data).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cout << "Error occured!" << std::endl;
            exit(0);
        }
        else
        {

            Json::Reader reader;
            Json::Value response_json;
            bool parse_status = reader.parse(readBuffer.c_str(), response_json);
            if (!parse_status)
            {
                std::cerr << "parse() failed: " << reader.getFormattedErrorMessages() << std::endl;
                exit(0);
            }
            else
            {
                bool status = response_json["status"].asBool();
                if (status)
                {
                    key = response_json["key"].asString();
                    std::cout << response_json["message"].asString() << std::endl;
                }
                else
                {
                    std::cout << response_json["message"].asString() << std::endl;
                    exit(0);
                }
            }
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

void joinop()
{

    mu.lock();
    ping();
    Json::StyledWriter writer;
    Json::Value json_data;
    json_data["action"] = "join";
    json_data["payload"]["username"] = username;
    json_data["payload"]["password"] = password;
    /* while (ws->getReadyState() != WebSocket::OPEN)
    {
    }
    ws->send(writer.write(json_data).c_str());*/
    websocketpp::lib::error_code ec;
    c.send(hdl, writer.write(json_data), websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    mu.unlock();
}

void createop()
{

    mu.lock();
    create_room();

    Json::StyledWriter writer;
    Json::Value json_data;
    json_data["action"] = "join_admin";
    json_data["payload"]["username"] = username;
    json_data["payload"]["password"] = password;
    json_data["payload"]["key"] = key;
    /* while (ws->getReadyState() != WebSocket::OPEN)
    {
    }
    ws->send(writer.write(json_data).c_str());*/
    websocketpp::lib::error_code ec;
    c.send(hdl, writer.write(json_data), websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    mu.unlock();
}

void help()
{
    std::cout << " Syntax: ./p2p-gaming [Operation] [Username] [Password] [Game Server [PORT] optional(required for room creator)] \n\n"
              << " Options: \n\n"
              << " [Operation]: (join,create) join :- to join room created by admin. create :- to create room as admin \n\n"
              << " [Server IP:PORT]: If you room creator then you need to provide ip address and port of your game lan server \n\n"
              << " [Username]: Username for your room or Set username if you room creator \n\n"
              << " [Password]: Password for your room or Set Password if you room creator \n\n"
              << std::endl;
}
int main(int argc, char **argv)
{
    if (!(argc > 3))
    {
        help();
        return 0;
    }
    else
    {
        std::thread t2(websocket_handler);

        std::string operation = argv[1];
        username = argv[2];
        password = argv[3];
        if (operation.compare("join") == 0)
        {
            std::thread t1(joinop);
            t2.join();
            t1.join();
        }
        else if (operation.compare("create") == 0)
        {
            if (argc != 5)
            {
                help();
                return 0;
            }

            port = argv[4];
            memset(&mainserver, 0, sizeof(mainserver));
            mainserver.sin_family = AF_INET;
            mainserver.sin_port = htons(atoi(port.c_str()));
            mainserver.sin_addr.s_addr = INADDR_ANY;
            std::thread t1(createop);
            t2.join();
            t1.join();
        }
        else
        {
            help();
            return 0;
        }
    }
    int pause;
    std::cin >> pause;
}