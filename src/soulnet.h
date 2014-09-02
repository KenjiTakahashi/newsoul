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

#ifndef __NEWSOUL_SOULNET_H__
#define __NEWSOUL_SOULNET_H__

#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <json-c/json.h>
#include <uv.h>

#include "config.h"
#include "utils/cipher.h"

namespace newsoul {
    class Message {
        static const std::map<std::string, std::unique_ptr<Message>> messages;
        static std::map<std::string, std::unique_ptr<Message>> initMessages();

    protected:
        int id = -1;
        virtual std::unique_ptr<Message> make(int id, struct json_object *def) { return nullptr; }

    public:
        static std::unique_ptr<Message> forge(std::string def);
        virtual std::string stringify() { return ""; }
        virtual std::unique_ptr<Message> go(Config *config) { return std::unique_ptr<Message>(this); }
    };

    namespace messages {
        class Challenge : public Message {
            std::string challenge;

        public:
            Challenge();
            std::string stringify();
        };

        class Login : public Message {
            std::string password;

            Login(int id, std::string &password) : password(password) { this->id = id; }

        protected:
            std::unique_ptr<Message> make(int id, struct json_object *def);

        public:
            Login() { }
            std::string stringify();
            std::unique_ptr<Message> go(Config *config);
        };

        class Error : public Message {
            int code;
            std::string msg;

        public:
            Error(int id, int code, std::string msg) : code(code), msg(msg) { this->id = id; }
            std::string stringify();
        };
    }

    class Soulnet {
        uv_loop_t *loop;
        uv_tcp_t server;

        static void ecn(uv_stream_t *server, int status);
        void event_connection_new(uv_stream_t *server, int status);

        static void event_client_alloc(uv_handle_t *handle, size_t size, uv_buf_t *buf);

        static void ecr(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
        void event_client_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);


        static void event_req_free(uv_write_t *req, int status);


        void write(uv_tcp_t *client, Message *msg);

        std::string get_peername(uv_tcp_t *client);

    public:
        Soulnet(const char *addr, unsigned int port);

        int run();
    };
}

#endif // __NEWSOUL_SOULNET_H__
