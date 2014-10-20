/*
    Copyright (c) 2011-2014 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2014 Other contributors as noted in the AUTHORS file.

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

#ifndef COCAINE_NODE_SERVICE_INTERFACE_HPP
#define COCAINE_NODE_SERVICE_INTERFACE_HPP

#include "cocaine/dynamic.hpp"

#include "cocaine/rpc/protocol.hpp"

#include "cocaine/traits.hpp"

namespace cocaine { namespace io {

struct app_tag;

// App invocation service interface

struct app {

struct enqueue {
    typedef app_tag tag;

    typedef stream_of<
     /* Allow clients to stream data into the apps in chunks. */
        msgpack::object
    >::tag dispatch_type;

    static const char* alias() {
        return "enqueue";
    }

    typedef boost::mpl::list<
     /* Event name. This name is intentionally dynamic so that the underlying application can
        do whatever it wants using these event names, for example handle every possible one. */
        std::string,
     /* Tag. Event can be enqueued to a specific worker with some user-defined name. */
        optional<std::string>
    > argument_type;

    typedef stream_of<
     /* Allow apps to stream data back to the client in chunks. */
        msgpack::object
    >::tag upstream_type;
};

struct info {
    typedef app_tag tag;

    static const char* alias() {
        return "info";
    }

    typedef option_of<
     /* Various runtime information about the running app. */
        dynamic_t
    >::tag upstream_type;
};

}; // struct app

template<>
struct protocol<app_tag> {
    typedef boost::mpl::int_<
        1
    >::type version;

    typedef boost::mpl::list<
        app::enqueue,
        app::info
    > messages;

    typedef app scope;
};

// Node service interface

struct node_tag;

struct node {

struct start_app {
    typedef node_tag tag;

    static const char* alias() {
        return "start_app";
    }

    typedef boost::mpl::list<
        std::string,
        std::string
    > argument_type;
};

struct pause_app {
    typedef node_tag tag;

    static const char* alias() {
        return "pause_app";
    }

    typedef boost::mpl::list<
     /* Name of the app to susped. */
        std::string
    > argument_type;
};

struct list {
    typedef node_tag tag;

    static const char* alias() {
        return "list";
    }

    typedef option_of<
     /* A list of running app names. */
        dynamic_t
    >::tag upstream_type;
};

}; // struct node

template<>
struct protocol<node_tag> {
    typedef boost::mpl::int_<
        1
    >::type version;

    typedef boost::mpl::list<
        node::start_app,
        node::pause_app,
        node::list
    > messages;

    typedef node scope;
};

}} // namespace cocaine::io

#endif
