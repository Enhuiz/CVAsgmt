#ifndef SERVER_HPP
#define SERVER_HPP

#include <fstream>
#include <sstream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "base64.hpp"
#include "server_base.hpp"
#include "../utils.hpp"

namespace cvasgmt
{
namespace server
{

// string param
using StrParams = std::map<std::string, std::string>;
// image param
using ImgParams = std::map<std::string, cv::Mat>;

class Server : public ServerBase
{
  public:
    Server(unsigned short port = 80, size_t thread_pool_size = 1, long timeout_request = 10, long timeout_content = 30)
        : ServerBase(port, thread_pool_size, timeout_request, timeout_content)
    {
        route_public();
        route_404();
    }

    void route_proc(const std::string &path,
                    std::function<void(const StrParams &, const ImgParams &, StrParams &, ImgParams &)> proc)
    {
        route["^/*" + utils::trim(path, '/') + "/*$"]["POST"] = [proc](std::shared_ptr<Response> response, std::shared_ptr<Request> request) {
            StrParams insp;
            ImgParams inip;
            StrParams outsp;
            ImgParams outip;

            auto raw_form = request->content.form();
            for (auto kv : raw_form)
            {
                if (kv.first.find("img#") != std::string::npos)
                {
                    auto ibuf = base64_decode(kv.second);
                    std::vector<char> data(ibuf.begin(), ibuf.end());
                    std::string id = kv.first;
                    id.erase(0, id.find("#") + 1);
                    inip.insert(std::make_pair(id, cv::imdecode(cv::Mat(data), cv::IMREAD_COLOR)));
                }
                else
                {
                    insp.insert(std::make_pair(kv.first, kv.second));
                }
            }

            try
            {
                // process the data
                proc(insp, inip, outsp, outip);
            }
            catch (...)
            {
                std::cerr << "process failed!" << std::endl;
            }

            Form form;

            for (auto kv : outsp)
            {
                form.insert(kv);
            }

            for (auto kv : outip)
            {
                std::vector<uchar> data;
                cv::imencode(".png", kv.second, data);
                std::string ibuf(data.begin(), data.end());
                form.insert(std::make_pair("img#" + kv.first, base64_encode(ibuf)));
            }

            response->send_form(form);
        };
    }

  private:
    void route_public()
    {
        default_route["GET"] = [](std::shared_ptr<Response> response, std::shared_ptr<Request> request) {
            if (request->path.find(".") == std::string::npos)
                request->path += "/index.html";
            std::ifstream f("public/" + request->path, std::ios_base::in | std::ios_base::binary);
            if (f)
                response->send(f);
            else
                response->send404();
        };
    }

    void route_404()
    {
        default_route["POST"] = [](std::shared_ptr<Response> response, std::shared_ptr<Request> request) {
            response->send404();
        };
    }
};
}
}

#endif