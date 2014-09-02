/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Copyright (C) 2014 Karol 'Kenji Takahashi' Woźniak

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "soulnet.h"

std::map<std::string, std::unique_ptr<newsoul::Message>> newsoul::Message::initMessages() {
    std::map<std::string, std::unique_ptr<Message>> m;
    m["login"] = std::unique_ptr<Message>(new messages::Login());
    return m;
}

const std::map<std::string, std::unique_ptr<newsoul::Message>> newsoul::Message::messages = newsoul::Message::initMessages();

std::unique_ptr<newsoul::Message> newsoul::Message::forge(std::string def) {
    struct json_object *msg_obj = json_tokener_parse(def.c_str());
    struct json_object *method_obj, *params_obj, *id_obj, *jsonrpc_obj;

    bool incorrect = (
        !json_object_object_get_ex(msg_obj, "method", &method_obj) ||
        !json_object_object_get_ex(msg_obj, "params", &params_obj) ||
        !json_object_object_get_ex(msg_obj, "id", &id_obj) ||
        !json_object_object_get_ex(msg_obj, "jsonrpc", &jsonrpc_obj) ||
        std::string(json_object_get_string(jsonrpc_obj)) != "2.0"
    );
    if(incorrect) {
        return std::unique_ptr<Message>(new messages::Error(-1, -32600, "Invalid Request"));
    }

    std::string method(json_object_get_string(method_obj));
    unsigned int id = json_object_get_int(id_obj);
    try {
        return Message::messages.at(method)->make(id, params_obj);
    } catch(std::out_of_range &e) {
        return std::unique_ptr<Message>(new messages::Error(id, -32601, "Method not found"));
    }
}

newsoul::messages::Challenge::Challenge() {
    char map[] = "0123456789abcdef";
    for(int i = 0; i < 64; i++) {
        this->challenge += map[rand() % 16];
    }
}

std::string newsoul::messages::Challenge::stringify() {
    struct json_object *str = json_object_new_object();
    json_object_object_add(str, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(str, "result", json_object_new_string(this->challenge.c_str()));
    json_object_object_add(str, "id", NULL);
    const char *msg = json_object_to_json_string_ext(str, 0);
    return msg + std::string("\n");
}

std::string newsoul::messages::Login::stringify() {
    struct json_object *str = json_object_new_object();
    json_object_object_add(str, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(str, "result", json_object_new_string("success"));
    json_object_object_add(str, "id", json_object_new_int(this->id));
    const char *msg = json_object_to_json_string_ext(str, 0);
    return msg + std::string("\n");
}

std::unique_ptr<newsoul::Message> newsoul::messages::Login::go(Config *config) {
    std::string hash = sha256Digest(this->password);
    if(hash != config->getStr({"listeners", "password"})) {
        return std::unique_ptr<Message>(new Error(this->id, 6000, "Invalid password"));
    }
    // TODO: Set authenticated in Newsoul main go go (somehow)
    return std::unique_ptr<Message>(this);
}

std::unique_ptr<newsoul::Message> newsoul::messages::Login::make(int id, struct json_object *def) {
    // TODO: Versioned API
    struct json_object *password_obj;
    if(!json_object_object_get_ex(def, "password", &password_obj)) {
        return std::unique_ptr<newsoul::Message>(new Error(this->id, 1000, "Invalid params object"));
    }
    std::string password(json_object_get_string(password_obj));
    return std::unique_ptr<newsoul::Message>(new Login(id, password));
}

std::string newsoul::messages::Error::stringify() {
    struct json_object *str = json_object_new_object();
    struct json_object *err = json_object_new_object();
    json_object_object_add(err, "code", json_object_new_int(this->code));
    json_object_object_add(err, "message", json_object_new_string(this->msg.c_str()));
    json_object_object_add(str, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(str, "error", err);
    json_object_object_add(str, "id", json_object_new_int(this->id));
    const char *msg = json_object_to_json_string_ext(str, 0);
    return msg + std::string("\n");
}


void newsoul::Soulnet::ecn(uv_stream_t *server, int status) {
    if(status == -1) {
        return;
    }
    newsoul::Soulnet *this_ = static_cast<newsoul::Soulnet*>(server->data);
    this_->event_connection_new(server, status);
}
void newsoul::Soulnet::event_connection_new(uv_stream_t *server, int status) {
    uv_tcp_t client;
    uv_tcp_init(this->loop, &client);
    if(uv_accept(server, (uv_stream_t*)&client) == 0) {
        messages::Challenge challenge;
        this->write(&client, &challenge);

        uv_read_start((uv_stream_t*)&client, event_client_alloc, ecr);
        LOG(INFO) << "Opened client connection `" << this->get_peername(&client) << "`";
    } else {
        uv_close((uv_handle_t*)&client, NULL);
    }
}

void newsoul::Soulnet::event_client_alloc(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*)malloc(size), size);
}

void newsoul::Soulnet::ecr(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if(nread < 0) {
        return;
    }
    newsoul::Soulnet *this_ = static_cast<newsoul::Soulnet*>(client->data);
    this_->event_client_read(client, nread, buf);
}
void newsoul::Soulnet::event_client_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if(nread <= 0) {
        if(nread == UV_EOF) {
            uv_close((uv_handle_t*)client, NULL);
        }
        //ERROR!
    } else {
        //TODO: get_peername should be cached earlier
        //Client may disconnect before we reach this line
        LOG(INFO) << "Client `" << this->get_peername((uv_tcp_t*)client) << "` <- `" << buf->base << "`";
        Config config;
        std::unique_ptr<Message> msg = Message::forge(std::string(buf->base))->go(&config);
        this->write((uv_tcp_t*)client, msg.get());
        free(buf->base);
    }
}


void newsoul::Soulnet::event_req_free(uv_write_t *req, int status) {
    if(status == -1) {
        //ERROR!
    }
    free(req);
}


void newsoul::Soulnet::write(uv_tcp_t *client, Message *msg) {
    std::string str = msg->stringify();
    char *ch = (char*)malloc((str.length() + 1) * sizeof(char));
    strcpy(ch, str.c_str());

    uv_buf_t buf = uv_buf_init(ch, str.length());

    uv_write_t *req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_write(req, (uv_stream_t*)client, &buf, 1, event_req_free);

    free(ch);
}

std::string newsoul::Soulnet::get_peername(uv_tcp_t *client) {
    struct sockaddr_in addr;
    int len;
    char ip[17];

    uv_tcp_getpeername(client, (sockaddr*)&addr, &len);
    uv_ip4_name(&addr, (char*)ip, sizeof ip);

    return std::string(ip) + ":" + std::to_string(addr.sin_port);
}

newsoul::Soulnet::Soulnet(const char *addr, unsigned int port) {
    this->loop = uv_default_loop();

    uv_tcp_init(this->loop, &this->server);
    this->server.data = this;
    struct sockaddr_in bind_addr;
    uv_ip4_addr(addr, port, &bind_addr);
    uv_tcp_bind(&this->server, (sockaddr*)&bind_addr, 0);

    uv_listen((uv_stream_t*)&this->server, 128, ecn);
}

int newsoul::Soulnet::run() {
    return uv_run(this->loop, UV_RUN_DEFAULT);
}
