#include <vector>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "opencv2/xfeatures2d.hpp"

#include "../server/server.hpp"

namespace cvasgmt
{

void echo(const server::StrParams &insp, const server::ImgParams &inip, server::StrParams &outsp, server::ImgParams &outip)
{
    for (auto kv : inip)
    {
        outip[kv.first] = kv.second;
    }
};


cv::Ptr<cv::Feature2D> get_f2d(std::string method)
{
    if (method == "sift")
    {
        return cv::xfeatures2d::SIFT::create();
    }
    else if (method == "surf")
    {
        return cv::xfeatures2d::SURF::create();
    }
    else if (method == "orb")
    {
        return cv::ORB::create();
    }
    return cv::Ptr<cv::Feature2D>();
}

void asgmt1(const server::StrParams &insp, const server::ImgParams &inip, server::StrParams &outsp, server::ImgParams &outip)
{
    if (inip.size() == 0)
        return;

    if (insp.count("extract"))
    {
        std::vector<cv::Mat> imgs;
        for (auto kv : inip)
        {
            imgs.push_back(kv.second);
        }

        auto f2d = get_f2d(insp.at("extract"));
        std::vector<cv::KeyPoint> kp1;
        f2d->detect(imgs[0], kp1);

        if (imgs.size() == 1)
        {
            cv::drawKeypoints(imgs[0], kp1, outip["res"]);
        }
        else
        {
            std::vector<cv::KeyPoint> kp2;
            f2d->detect(imgs[1], kp2);

            cv::Mat d1, d2;
            f2d->compute(imgs[0], kp1, d1);
            f2d->compute(imgs[1], kp2, d2);

            cv::BFMatcher matcher;
            std::vector<cv::DMatch> matches;
            matcher.match(d1, d2, matches);

            std::nth_element(matches.begin(), matches.begin() + 20, matches.end());
            matches.erase(matches.begin() + 21, matches.end());        

            cv::Mat res;
            cv::drawMatches(imgs[0], kp1, imgs[1], kp2, matches, res);

            outip["res"] = res;
        }
    }
    else
    {
        echo(insp, inip, outsp, outip);
    }
}
};
