#include <vector>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "opencv2/xfeatures2d.hpp"

#include "../server/server.hpp"

namespace cvasgmt
{

// by Chen Mingjie
cv::Mat GC(const cv::Mat &img, const std::vector<cv::Point>& obj, const std::vector<cv::Point>& bg, int n)
{
    cv::Mat mask;
    mask.create(img.size(), CV_8UC1);
    mask.setTo(cv::GrabCutClasses::GC_PR_BGD);

    for (int i = 0; i < obj.size(); i++)
        cv::circle(mask, obj[i], 2, cv::GrabCutClasses::GC_FGD, -1);
    for (int i = 0; i < bg.size(); i++)
        cv::circle(mask, bg[i], 2, cv::GrabCutClasses::GC_BGD, -1);

    cv::Mat bgdModel, fgdModel;
    cv::grabCut(img, mask, cv::Rect(0, 0, img.cols, img.rows), bgdModel, fgdModel, n, cv::GrabCutModes::GC_INIT_WITH_MASK);

    cv::Mat bin;
    bin.create(mask.size(), CV_8UC1);
    bin = mask & 1;

    cv::Mat res;
    img.copyTo(res, bin);
    return res;
}

void asgmt2(const server::StrParams &insp, const server::ImgParams &inip, server::StrParams &outsp, server::ImgParams &outip)
{
    std::vector<cv::Point> obj;
    std::vector<cv::Point> bg;

    if (inip.size() != 1)
        return;

    for (auto sp : insp)
    {
        if (sp.first.find("in#") != std::string::npos)
        {
            size_t pos = sp.second.find('$');
            std::string x = sp.second.substr(0, pos);
            std::string y = sp.second;
            y.erase(0, pos + 1);
            obj.push_back(cv::Point((int)std::stod(x), (int)std::stod(y)));
        }
        else if (sp.first.find("ex#") != std::string::npos)
        {
            size_t pos = sp.second.find('$');
            std::string x = sp.second.substr(0, pos);
            std::string y = sp.second;
            y.erase(0, pos + 1);
            bg.push_back(cv::Point((int)std::stod(x), (int)std::stod(y)));
        }
    }

    outip[inip.begin()->first + "res"] = GC(inip.begin()->second, obj, bg, 1);
}
};
