#include <fstream>
#include <iostream>
#include <string>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include "server/server.hpp"
#include "asgmt/asgmt1.hpp"
#include "asgmt/asgmt2.hpp"

int main(int argc, char *argv[])
{
    unsigned short port = 8888;

    if (argc == 2)
    {
        port = std::stoi(argv[1]);
    }

    cvasgmt::server::Server server(port);

    // register asgmts
    server.route_proc("/asgmt/1/", cvasgmt::asgmt1);
    server.route_proc("/asgmt/2/", cvasgmt::asgmt2);

    server.start();
}