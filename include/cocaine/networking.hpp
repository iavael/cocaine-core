#ifndef COCAINE_NETWORKING_HPP
#define COCAINE_NETWORKING_HPP

#include <zmq.hpp>

#if ZMQ_VERSION < 20107
    #error ZeroMQ version 2.1.7+ required!
#endif

#include <msgpack.hpp>

#include "cocaine/common.hpp"

namespace cocaine { namespace lines {

using namespace boost::tuples;

class socket_t: 
    public boost::noncopyable,
    public helpers::birth_control_t<socket_t>
{
    public:
        socket_t(zmq::context_t& context, int type):
            m_socket(context, type)
        {}

        inline bool send(zmq::message_t& message, int flags = 0) {
            try {
                return m_socket.send(message, flags);
            } catch(const zmq::error_t& e) {
                syslog(LOG_ERR, "net: [%s()] %s", __func__, e.what());
                return false;
            }
        }

        inline bool recv(zmq::message_t* message, int flags = 0) {
            try {
                return m_socket.recv(message, flags);
            } catch(const zmq::error_t& e) {
                syslog(LOG_ERR, "net: [%s()] %s", __func__, e.what());
                return false;
            }
        }

        inline void getsockopt(int name, void* value, size_t* length) {
            try {
                m_socket.getsockopt(name, value, length);
            } catch(const zmq::error_t& e) {
                syslog(LOG_ERR, "net: [%s()] %s", __func__, e.what());
            }
        }

        inline void setsockopt(int name, const void* value, size_t length) {
            m_socket.setsockopt(name, value, length);
        }

        inline void bind(const std::string& endpoint) {
            m_socket.bind(endpoint.c_str());
        }

        inline void connect(const std::string& endpoint) {
            m_socket.connect(endpoint.c_str());
        }
       
    public: 
        int fd();
        bool pending(int event = ZMQ_POLLIN);
        bool has_more();

    private:
        zmq::socket_t m_socket;
};

#define PUSH      1 /* engine pushes a task to an overseer */
#define DROP      2 /* engine drops a task from an overseer */
#define TERMINATE 3 /* engine terminates an overseer */
#define FUTURE    4 /* overseer fulfills an engine's request */
#define SUICIDE   5 /* overseer performs a suicide */
#define EVENT     6 /* driver sends the invocation results to the core */
#define HEARTBEAT 7 /* overseer is reporting that it's still alive */

template<class T> class raw;

template<> class raw<const std::string> {
    public:
        raw(const std::string& object):
            m_object(object)
        { }

        void pack(zmq::message_t& message) const {
            message.rebuild(m_object.length());
            memcpy(message.data(), m_object.data(), m_object.length());
        }

    private:
        const std::string& m_object;
};

template<> class raw<std::string> {
    public:
        raw(std::string& object):
            m_object(object)
        { }

        void pack(zmq::message_t& message) const {
            message.rebuild(m_object.length());
            memcpy(message.data(), m_object.data(), m_object.length());
        }

        bool unpack(/* const */ zmq::message_t& message) {
            m_object.assign(
                static_cast<const char*>(message.data()),
                message.size());
            return true;
        }

    private:
        std::string& m_object;
};

template<class T>
static raw<T> protect(T& object) {
    return raw<T>(object);
}

class channel_t:
    public socket_t
{
    public:
        channel_t(zmq::context_t& context, int type):
            socket_t(context, type)
        {}

        // Bring original methods into the scope
        using socket_t::send;
        using socket_t::recv;

        // Packs and sends a single object
        template<class T>
        bool send(const T& value, int flags = 0) {
            zmq::message_t message;
            
            msgpack::sbuffer buffer;
            msgpack::pack(buffer, value);
            
            message.rebuild(buffer.size());
            memcpy(message.data(), buffer.data(), buffer.size());
            
            return send(message, flags);
        }

        template<class T>
        bool send(const raw<T>& object, int flags) {
            zmq::message_t message;
            object.pack(message);
            return send(message, flags);
        }

        // Packs and sends a tuple
        bool send_multi(const null_type&, int flags = 0) {
            return true;
        }

        template<class Head>
        bool send_multi(const cons<Head, null_type>& o, int flags = 0) {
            return send(o.get_head(), flags);
        }

        template<class Head, class Tail>
        bool send_multi(const cons<Head, Tail>& o, int flags = 0) {
            return (send(o.get_head(), ZMQ_SNDMORE | flags) 
                    && send_multi(o.get_tail(), flags));
        }

        // Receives and unpacks a single object
        template<class T>
        bool recv(T& result, int flags = 0) {
            zmq::message_t message;
            msgpack::unpacked unpacked;

            if(!recv(&message, flags)) {
                return false;
            }
           
            try { 
                msgpack::unpack(&unpacked,
                    static_cast<const char*>(message.data()),
                    message.size());
                msgpack::object object = unpacked.get();
                object.convert(&result);
            } catch(const std::exception& e) {
                syslog(LOG_ERR, "net: [%s()] corrupted object - %s", __func__, e.what());
                return false;
            }

            return true;
        }
      
        template<class T>
        bool recv(raw<T>& result, int flags) {
            zmq::message_t message;

            if(!recv(&message, flags)) {
                return false;
            }

            return result.unpack(message);
        }

        // Receives and unpacks a tuple
        bool recv_multi(const null_type&, int flags = 0) {
            return true;
        }

        template<class Head, class Tail>
        bool recv_multi(cons<Head, Tail>& o, int flags = 0) {
            return (recv(o.get_head(), flags)
                    && recv_multi(o.get_tail(), flags));
        }
};

}}

namespace msgpack {
    template<class Stream>
    packer<Stream>& operator<<(packer<Stream>& o, const Json::Value& v) {
        Json::FastWriter writer;
        std::string json(writer.write(v));

        o.pack_raw(json.size());
        o.pack_raw_body(json.data(), json.size());

        return o;
    }

    template<>
    Json::Value& operator>>(msgpack::object o, Json::Value& v);
}

#endif