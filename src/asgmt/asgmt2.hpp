#include <vector>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "opencv2/xfeatures2d.hpp"

#include "../server/server.hpp"

namespace cvasgmt
{
void asgmt2(const server::StrParams &insp, const server::ImgParams &inip, server::StrParams &outsp, server::ImgParams &outip)
{
    for (auto sp : insp)
    {
        std::cerr << sp.first << ": " << sp.second << std::endl;
    }
    
    outip = inip;
}
};
