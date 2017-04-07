#ifndef SERVER_HTTP_HPP
#define SERVER_HTTP_HPP

#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>

#include <map>
#include <unordered_map>
#include <thread>
#include <functional>
#include <iostream>
#include <fstream>
#include <regex>
#include <ctime>
#include <iomanip>

namespace cvasgmt
{
namespace server
{
#ifndef CASE_INSENSITIVE_EQUALS_AND_HASH
#define CASE_INSENSITIVE_EQUALS_AND_HASH
//Based on http://www.boost.org/doc/libs/1_60_0/doc/html/unordered/hash_equality.html

class case_insensitive_equals
{
  public:
    bool operator()(const std::string &key1, const std::string &key2) const
    {
        return boost::algorithm::iequals(key1, key2);
    }
};
class case_insensitive_hash
{
  public:
    size_t operator()(const std::string &key) const
    {
        std::size_t seed = 0;
        for (auto &c : key)
            boost::hash_combine(seed, std::tolower(c));
        return seed;
    }
};
#endif

using Form = std::map<std::string, std::string>;
using HTTP = boost::asio::ip::tcp::socket;

class ServerBase
{
  protected:
    class Content : public std::istream
    {
        friend class ServerBase;

      public:
        size_t size()
        {
            return streambuf.size();
        }

        std::string string()
        {
            std::stringstream ss;
            ss << rdbuf();
            return ss.str();
        }

        Form form()
        {
            Form ret;
            try
            {
                std::string line;
                while (!this->eof())
                {
                    std::getline(*this, line, '&');
                    size_t pos = line.find('=');
                    auto key = line.substr(0, pos);
                    line.erase(0, pos + 1);
                    ret.insert(std::make_pair(key, line));
                }
            }
            catch (...)
            {
                std::cerr << "parse form failed" << std::endl;
            }
            return ret;
        }

      private:
        boost::asio::streambuf &streambuf;
        Content(boost::asio::streambuf &streambuf) : std::istream(&streambuf), streambuf(streambuf) {}
    };

    class Request
    {
        friend class ServerBase;

      public:
        std::string method, path, http_version;

        Content content;

        std::unordered_multimap<std::string, std::string, case_insensitive_hash, case_insensitive_equals> header;

        std::smatch path_match;

        std::string remote_endpoint_address;
        unsigned short remote_endpoint_port;

      private:
        Request(const HTTP &socket) : content(streambuf)
        {
            try
            {
                remote_endpoint_address = socket.lowest_layer().remote_endpoint().address().to_string();
                remote_endpoint_port = socket.lowest_layer().remote_endpoint().port();
            }
            catch (...)
            {
            }
        }

        boost::asio::streambuf streambuf;
    };

    class Response : protected std::ostream
    {
        friend class ServerBase;

      public:
        size_t size()
        {
            return streambuf.size();
        }

        void write_head(std::string key, std::string value)
        {
            header.insert(std::make_pair(key, value));
        }

        void send_form(const Form &form)
        {
            std::stringstream ss;
            ss << "{";
            for (auto it = form.begin(); it != form.end(); ++it)
            {
                if (it != form.begin())
                    ss << ",";
                ss << "\"" <<  it->first  << "\":\"" << it->second << "\"";
            }
            ss << "}";

            dump(ss);
            close_connection_after_response = true;
        }

        void send(std::istream &is)
        {
            dump(is);
            close_connection_after_response = true;
        }

        void send404()
        {
            // status line
            *this << "HTTP/1.1 404 Not Found\r\n\r\n";
            std::ifstream f("views/404.html", std::ios_base::in);
            if (f)
                *this << f.rdbuf();
            else
                *this << "Not Found!";
            close_connection_after_response = true;
        }

      protected:
        void dump(std::istream &is)
        {
            // status line
            *this << "HTTP/1.1 200 OK\r\n";

            // header
            // -- set content-length
            auto it = header.find("Content-Length");
            if (it != header.end())
                header.erase(it);
            std::string payload(std::istreambuf_iterator<char>(is), {});
            write_head("Content-Length", std::to_string(payload.size()));

            for (auto it : header)
            {
                *this << it.first << ":" << it.second << "\r\n";
            }

            *this << "\r\n";

            // content
            *this << payload;
        }

      protected:
        boost::asio::streambuf streambuf;

        std::shared_ptr<HTTP> socket;

        Response(const std::shared_ptr<HTTP> &socket) : std::ostream(&streambuf), socket(socket) {}

        std::unordered_multimap<std::string, std::string, case_insensitive_hash, case_insensitive_equals> header;
        /// If true, force server to close the connection after the response have been sent.
        ///
        /// This is useful when implementing a HTTP/1.0-server sending content
        /// without specifying the content length.
        bool close_connection_after_response = false;
    };

  public:
    ServerBase(unsigned short port = 80, size_t thread_pool_size = 1, long timeout_request = 5, long timeout_content = 300)
    {
        config.port = port;
        config.thread_pool_size = thread_pool_size;
        config.timeout_request = timeout_request;
        config.timeout_content = timeout_content;
    }

    virtual ~ServerBase() {}

  public:
    class Config
    {
        friend class ServerBase;

      public:
        /// Port number to use. Defaults to 80 for HTTP and 443 for HTTPS.
        unsigned short port = 80;
        /// Number of threads that the server will use when start() is called. Defaults to 1 thread.
        size_t thread_pool_size = 1;
        /// Timeout on request handling. Defaults to 5 seconds.
        size_t timeout_request = 5;
        /// Timeout on content handling. Defaults to 300 seconds.
        size_t timeout_content = 300;
        /// IPv4 address in dotted decimal form or IPv6 address in hexadecimal notation.
        /// If empty, the address will be any address.
        std::string address;
        /// Set to false to avoid binding the socket to an address that is already in use. Defaults to true.
        bool reuse_address = true;
    };
    ///Set before calling start().
    Config config;

  private:
    class regex_orderable : public std::regex
    {
        std::string str;

      public:
        regex_orderable(const char *regex_cstr) : std::regex(regex_cstr), str(regex_cstr) {}
        regex_orderable(const std::string &regex_str) : std::regex(regex_str), str(regex_str) {}
        bool operator<(const regex_orderable &rhs) const
        {
            return str < rhs.str;
        }
    };

  public:
    /// Warning: do not add or remove routes after start() is called
    std::map<regex_orderable, std::map<std::string,
                                       std::function<void(std::shared_ptr<typename ServerBase::Response>, std::shared_ptr<typename ServerBase::Request>)>>>
        route;

    std::map<std::string,
             std::function<void(std::shared_ptr<typename ServerBase::Response>, std::shared_ptr<typename ServerBase::Request>)>>
        default_route;

    std::function<void(std::shared_ptr<typename ServerBase::Request>, const boost::system::error_code &)> on_error;

    std::function<void(std::shared_ptr<HTTP> socket, std::shared_ptr<typename ServerBase::Request>)> on_upgrade;

    virtual void start()
    {
        if (!io_service)
            io_service = std::make_shared<boost::asio::io_service>();

        if (io_service->stopped())
            io_service->reset();

        boost::asio::ip::tcp::endpoint endpoint;
        if (config.address.size() > 0)
            endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config.address), config.port);
        else
            endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port);

        if (!acceptor)
            acceptor = std::unique_ptr<boost::asio::ip::tcp::acceptor>(new boost::asio::ip::tcp::acceptor(*io_service));
        acceptor->open(endpoint.protocol());
        acceptor->set_option(boost::asio::socket_base::reuse_address(config.reuse_address));
        acceptor->bind(endpoint);
        acceptor->listen();

        accept();

        //If thread_pool_size>1, start m_io_service.run() in (thread_pool_size-1) threads for thread-pooling
        threads.clear();
        for (size_t c = 1; c < config.thread_pool_size; c++)
        {
            threads.emplace_back([this]() {
                io_service->run();
            });
        }

        std::cout << "listening on " << config.port << std::endl;

        //Main thread
        if (config.thread_pool_size > 0)
            io_service->run();

        //Wait for the rest of the threads, if any, to finish as well
        for (auto &t : threads)
        {
            t.join();
        }
    }

    void stop()
    {
        acceptor->close();
        if (config.thread_pool_size > 0)
            io_service->stop();
    }

    ///Use this function if you need to recursively send parts of a longer message
    void send(const std::shared_ptr<Response> &response, const std::function<void(const boost::system::error_code &)> &callback = nullptr) const
    {
        boost::asio::async_write(*response->socket, response->streambuf, [this, response, callback](const boost::system::error_code &ec, size_t /*bytes_transferred*/) {
            if (callback)
                callback(ec);
        });
    }

    /// If you have your own boost::asio::io_service, store its pointer here before running start().
    /// You might also want to set config.thread_pool_size to 0.
    std::shared_ptr<boost::asio::io_service> io_service;

  protected:
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    std::vector<std::thread> threads;

    void accept()
    {
        //Create new socket for this connection
        //Shared_ptr is used to pass temporary objects to the asynchronous functions
        auto socket = std::make_shared<HTTP>(*io_service);

        acceptor->async_accept(*socket, [this, socket](const boost::system::error_code &ec) {
            //Immediately start accepting a new connection (if io_service hasn't been stopped)
            if (ec != boost::asio::error::operation_aborted)
                accept();

            if (!ec)
            {
                boost::asio::ip::tcp::no_delay option(true);
                socket->set_option(option);

                this->read_request_and_content(socket);
            }
            else if (on_error)
                on_error(std::shared_ptr<Request>(new Request(*socket)), ec);
        });
    }

    std::shared_ptr<boost::asio::deadline_timer> get_timeout_timer(const std::shared_ptr<HTTP> &socket, long seconds)
    {
        if (seconds == 0)
            return nullptr;

        auto timer = std::make_shared<boost::asio::deadline_timer>(*io_service);
        timer->expires_from_now(boost::posix_time::seconds(seconds));
        timer->async_wait([socket](const boost::system::error_code &ec) {
            if (!ec)
            {
                boost::system::error_code ec;
                socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                socket->lowest_layer().close();
            }
        });
        return timer;
    }

    void read_request_and_content(const std::shared_ptr<HTTP> &socket)
    {
        //Create new streambuf (Request::streambuf) for async_read_until()
        //shared_ptr is used to pass temporary objects to the asynchronous functions
        std::shared_ptr<Request> request(new Request(*socket));

        //Set timeout on the following boost::asio::async-read or write function
        auto timer = this->get_timeout_timer(socket, config.timeout_request);

        boost::asio::async_read_until(*socket, request->streambuf, "\r\n\r\n",
                                      [this, socket, request, timer](const boost::system::error_code &ec, size_t bytes_transferred) {
                                          if (timer)
                                              timer->cancel();
                                          if (!ec)
                                          {
                                              //request->streambuf.size() is not necessarily the same as bytes_transferred, from Boost-docs:
                                              //"After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
                                              //The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
                                              //streambuf (maybe some bytes of the content) is appended to in the async_read-function below (for retrieving content).
                                              size_t num_additional_bytes = request->streambuf.size() - bytes_transferred;

                                              if (!this->parse_request(request))
                                                  return;

                                              // log
                                              auto t = std::time(nullptr);
                                              auto tm = *std::localtime(&t);
                                              std::cout << std::put_time(&tm, "%b %d %T") << " "
                                                        << request->method
                                                        << " " << request->path << " from "
                                                        << request->remote_endpoint_address << ":"
                                                        << request->remote_endpoint_port << std::endl;

                                              //If content, read that as well
                                              auto it = request->header.find("Content-Length");
                                              if (it != request->header.end())
                                              {
                                                  unsigned long long content_length;
                                                  try
                                                  {
                                                      content_length = stoull(it->second);
                                                  }
                                                  catch (const std::exception &e)
                                                  {
                                                      if (on_error)
                                                          on_error(request, boost::system::error_code(boost::system::errc::protocol_error, boost::system::generic_category()));
                                                      return;
                                                  }
                                                  if (content_length > num_additional_bytes)
                                                  {
                                                      //Set timeout on the following boost::asio::async-read or write function
                                                      auto timer = this->get_timeout_timer(socket, config.timeout_content);
                                                      boost::asio::async_read(*socket, request->streambuf,
                                                                              boost::asio::transfer_exactly(content_length - num_additional_bytes),
                                                                              [this, socket, request, timer](const boost::system::error_code &ec, size_t /*bytes_transferred*/) {
                                                                                  if (timer)
                                                                                      timer->cancel();
                                                                                  if (!ec)
                                                                                      this->find_route(socket, request);
                                                                                  else if (on_error)
                                                                                      on_error(request, ec);
                                                                              });
                                                  }
                                                  else
                                                      this->find_route(socket, request);
                                              }
                                              else
                                                  this->find_route(socket, request);
                                          }
                                          else if (on_error)
                                              on_error(request, ec);
                                      });
    }

    bool parse_request(const std::shared_ptr<Request> &request) const
    {
        std::string line;
        getline(request->content, line);
        size_t method_end;
        if ((method_end = line.find(' ')) != std::string::npos)
        {
            size_t path_end;
            if ((path_end = line.find(' ', method_end + 1)) != std::string::npos)
            {
                request->method = line.substr(0, method_end);
                request->path = line.substr(method_end + 1, path_end - method_end - 1);

                size_t protocol_end;
                if ((protocol_end = line.find('/', path_end + 1)) != std::string::npos)
                {
                    if (line.compare(path_end + 1, protocol_end - path_end - 1, "HTTP") != 0)
                        return false;
                    request->http_version = line.substr(protocol_end + 1, line.size() - protocol_end - 2);
                }
                else
                    return false;

                getline(request->content, line);
                size_t param_end;
                while ((param_end = line.find(':')) != std::string::npos)
                {
                    size_t value_start = param_end + 1;
                    if ((value_start) < line.size())
                    {
                        if (line[value_start] == ' ')
                            value_start++;
                        if (value_start < line.size())
                            request->header.emplace(line.substr(0, param_end), line.substr(value_start, line.size() - value_start - 1));
                    }

                    getline(request->content, line);
                }
            }
            else
                return false;
        }
        else
            return false;
        return true;
    }

    void find_route(const std::shared_ptr<HTTP> &socket, const std::shared_ptr<Request> &request)
    {
        //Upgrade connection
        if (on_upgrade)
        {
            auto it = request->header.find("Upgrade");
            if (it != request->header.end())
            {
                on_upgrade(socket, request);
                return;
            }
        }
        //Find path- and method-match, and call write_response
        for (auto &regex_method : route)
        {
            auto it = regex_method.second.find(request->method);
            if (it != regex_method.second.end())
            {
                std::smatch sm_res;
                if (std::regex_match(request->path, sm_res, regex_method.first))
                {
                    request->path_match = std::move(sm_res);
                    write_response(socket, request, it->second);
                    return;
                }
            }
        }
        auto it = default_route.find(request->method);
        if (it != default_route.end())
        {
            write_response(socket, request, it->second);
        }
    }

    void write_response(const std::shared_ptr<HTTP> &socket, const std::shared_ptr<Request> &request,
                        std::function<void(std::shared_ptr<typename ServerBase::Response>,
                                           std::shared_ptr<typename ServerBase::Request>)> &route_function)
    {
        //Set timeout on the following boost::asio::async-read or write function
        auto timer = this->get_timeout_timer(socket, config.timeout_content);

        auto response = std::shared_ptr<Response>(new Response(socket), [this, request, timer](Response *response_ptr) {
            auto response = std::shared_ptr<Response>(response_ptr);
            this->send(response, [this, response, request, timer](const boost::system::error_code &ec) {
                if (timer)
                    timer->cancel();
                if (!ec)
                {
                    if (response->close_connection_after_response)
                        return;

                    auto range = request->header.equal_range("Connection");
                    for (auto it = range.first; it != range.second; it++)
                    {
                        if (boost::iequals(it->second, "close"))
                            return;
                    }
                    if (request->http_version >= "1.1")
                        this->read_request_and_content(response->socket);
                }
                else if (on_error)
                    on_error(request, ec);
            });
        });

        try
        {
            route_function(response, request);
        }
        catch (const std::exception &e)
        {
            if (on_error)
                on_error(request, boost::system::error_code(boost::system::errc::operation_canceled, boost::system::generic_category()));
            return;
        }
    }
};
}
}

#endif /* SERVER_HTTP_HPP */
