/*
    Copyright (c) 2011-2012 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2012 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/

#ifndef COCAINE_RPC_HPP
#define COCAINE_RPC_HPP

#include "cocaine/io.hpp"

namespace cocaine {

namespace rpc {

struct heartbeat;
struct terminate;
struct invoke;
struct chunk;
struct error;
struct choke;

typedef boost::mpl::list<
    heartbeat, terminate, invoke, chunk, error, choke
>::type domain;

}

namespace io {

// Specific packers
// ----------------

template<>
struct command<rpc::domain, rpc::invoke>:
    public boost::tuple<const std::string&, zmq::message_t&>
{
    typedef boost::tuple<const std::string&, zmq::message_t&> tuple_type;

    command(const std::string& event, const void * data, size_t size):
        tuple_type(event, message),
        message(size)
    {
        memcpy(
            message.data(),
            data,
            size
        );
    }

private:
    zmq::message_t message;
};

template<>
struct command<rpc::domain, rpc::chunk>:
    public boost::tuple<zmq::message_t&>
{
    typedef boost::tuple<zmq::message_t&> tuple_type;

    command(const void * data, size_t size):
        tuple_type(message),
        message(size)
    {
        memcpy(
            message.data(),
            data,
            size
        );
    }

private:
    zmq::message_t message;
};

template<>
struct command<rpc::domain, rpc::error>:
    // NOTE: Not a string reference to allow literal error messages.
    public boost::tuple<int, std::string>
{
    typedef boost::tuple<int, std::string> tuple_type;

    command(int code, const std::string& message):
        tuple_type(code, message)
    { }
};
    
}}

#endif
