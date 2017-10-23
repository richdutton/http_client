// https://w9f19o6faj.execute-api.us-east-1.amazonaws.com/api/

/**
 * OSX BUILD Instructions
 * 
 * OPENSSL_ROOT_DIR=/usr/local/opt/openssl/ cmake ..
 */
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class Uri {
public:
    Uri(const std::string &url_s) {
        const std::string prot_end("://");
        std::string::const_iterator prot_i = std::search(url_s.begin(), url_s.end(),
        prot_end.begin(), prot_end.end());

        protocol_.reserve(distance(url_s.begin(), prot_i));
        std::transform(url_s.begin(), prot_i,
                  back_inserter(protocol_),
                  std::ptr_fun<int,int>(tolower)); // protocol is icase
        if (prot_i == url_s.end()) {
            return;
        }

        if (protocol_ == "https") {
            port_ = "443";
        }

        std::advance(prot_i, prot_end.length());
        
        std::string::const_iterator path_i = std::find(prot_i, url_s.end(), '/');
        host_.reserve(std::distance(prot_i, path_i));

        std::transform(prot_i, path_i,
                  std::back_inserter(host_),
                  std::ptr_fun<int,int>(tolower)); // host is icase

        std::string::const_iterator query_i = std::find(path_i, url_s.end(), '?');
        path_.assign(path_i, query_i);
        if (query_i != url_s.end()){
            ++query_i;
        }
        query_.assign(query_i, url_s.end());
    }

    ~Uri() = default;

    const std::string & protocol() const {
        return protocol_;
    }

    const std::string & host() const {
        return host_;
    }

    const std::string & path() const {
        return path_;
    }

    const std::string & query() const {
        return query_;
    }

    const std::string & port() const {
        return port_;
    }

    bool secure() {
        return protocol_ == "https";
    }

private:
    std::string protocol_;
    std::string host_;
    std::string path_;
    std::string query_;
    std::string port_{"80"};
};

class HTTPClientInterface {
public:
    virtual ~HTTPClientInterface() = default;
    virtual void connect(boost::asio::ip::tcp::resolver::iterator lookup) = 0;
    virtual void write(http::request<http::string_body> &req) = 0;
    virtual std::unique_ptr<http::response<http::dynamic_body>> read() = 0;
    virtual void close() = 0;
};

class HTTPClient : public HTTPClientInterface {
public:

    HTTPClient(boost::asio::io_service &ios) : socket_(ios) {}
    ~HTTPClient() = default;

    void connect(boost::asio::ip::tcp::resolver::iterator lookup) {
        boost::asio::connect(socket_, lookup);
    }

    void write(http::request<http::string_body> &req) {
        http::write(socket_, req);
    }

    std::unique_ptr<http::response<http::dynamic_body>> read() {
        auto res = std::make_unique<http::response<http::dynamic_body>>();
        boost::beast::flat_buffer buffer;
        http::read(socket_, buffer, *res);

        return res;
    }

    void close() {
        // Gracefully close the socket
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        if(ec && ec != boost::system::errc::not_connected) {
            throw boost::system::system_error{ec};
        }
    }

private:
    tcp::socket socket_;
};

class HTTPSClient : public HTTPClientInterface {
public:
    HTTPSClient(boost::asio::io_service &ios, ssl::context &ctx) : stream_(ios, ctx) {}
    ~HTTPSClient() = default;

    void connect(boost::asio::ip::tcp::resolver::iterator lookup) {
        boost::asio::connect(stream_.next_layer(), lookup);
        SSL_set_tlsext_host_name(stream_.native_handle(), lookup->host_name().c_str());
        stream_.handshake(ssl::stream_base::client);
    }

    void write(http::request<http::string_body> &req) {
        http::write(stream_, req);
    }

    std::unique_ptr<http::response<http::dynamic_body>> read() {
        auto res = std::make_unique<http::response<http::dynamic_body>>();
        boost::beast::flat_buffer buffer;
        http::read(stream_, buffer, *res);

        return res;
    }

    void close() {
        boost::system::error_code ec;
        stream_.shutdown(ec);
        if (ec == boost::asio::error::eof) {
            ec.assign(0, ec.category());
        }

        if (ec) {
            throw boost::system::system_error{ec};
        }
    }

private:
    ssl::stream<tcp::socket> stream_;
};

class Requests {
public:
    Requests() {
        ctx_.set_verify_mode(boost::asio::ssl::verify_none);
        ctx_.set_options(boost::asio::ssl::context::default_workarounds);
    }
    ~Requests() {}

    std::unique_ptr<http::response<http::dynamic_body>> Request(const std::string &host, const std::string &port, const std::string &path, bool secure) {
        std::unique_ptr<HTTPClientInterface> client;

        if (secure) {
            client = std::make_unique<HTTPSClient>(ios_, ctx_);
        } else {
            client = std::make_unique<HTTPClient>(ios_);
        }

        // Look up the domain name
        auto const lookup = resolver_.resolve({host, port});
        client->connect(lookup);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, path, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "Richard-Agent");

        client->write(req);
        auto res = client->read();

        // Write the message to standard out
        std::cout << *res << std::endl;
        client->close();

        return res;
    }

    std::unique_ptr<http::response<http::dynamic_body>> Get(const std::string &url) {
        
        Uri u(url);
        std::cout << u.host() << " " << u.port() << " " << u.path() << '\n';
        return Request(u.host(), u.port(), u.path(), u.secure());
    }

private:
    boost::asio::io_service ios_;
    tcp::resolver resolver_{ios_};
    ssl::context ctx_{ssl::context::sslv23_client};
};

int 
main(int argc, char* argv[])
{
    if (argc != 2){
        std::cerr << "Usage: <url>\n";
        return EXIT_FAILURE;
    }

    try {
        Requests r;
        auto const url = argv[1];
        auto res = r.Get(url);
        std::cout << *res << '\n';
  
    } catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
