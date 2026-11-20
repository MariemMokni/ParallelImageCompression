#ifndef METRICS_H
#define METRICS_H

#include <opencv2/opencv.hpp>
#include <cmath>
#include <cstddef>
#include <vector>

// PSNR : mesure la difference entre les deux images
inline double calculatePSNR(
    const cv::Mat& original,
    const cv::Mat& reconstructed
) {
    return cv::PSNR(original, reconstructed);
}

// SSIM : mesure la similarite visuelle entre les deux images
inline double calculateSSIM(
    const cv::Mat& original,
    const cv::Mat& reconstructed
) {
    CV_Assert(original.size() == reconstructed.size());
    CV_Assert(original.type() == reconstructed.type());

    cv::Mat img1, img2;

    original.convertTo(img1, CV_64F); //SSIM utilise des op math (pixel->double)
    reconstructed.convertTo(img2, CV_64F);

    std::vector<cv::Mat> channels1, channels2;

    cv::split(img1, channels1);
    cv::split(img2, channels2); //separe 3 canaux RGB

    const double C1 = 6.5025;
    const double C2 = 58.5225;

    double totalSSIM = 0.0;

    for (size_t i = 0; i < channels1.size(); i++) {

        cv::Mat mu1, mu2;

        cv::GaussianBlur(
            channels1[i], mu1, cv::Size(11, 11), 1.5
        );

        cv::GaussianBlur(
            channels2[i], mu2, cv::Size(11, 11), 1.5
        );

        cv::Mat mu1Sq = mu1.mul(mu1);
        cv::Mat mu2Sq = mu2.mul(mu2);
        cv::Mat mu1mu2 = mu1.mul(mu2);

        cv::Mat img1Sq = channels1[i].mul(channels1[i]);
        cv::Mat img2Sq = channels2[i].mul(channels2[i]);
        cv::Mat img1img2 = channels1[i].mul(channels2[i]);

        cv::Mat sigma1Sq, sigma2Sq, sigma12;

        cv::GaussianBlur(
            img1Sq, sigma1Sq, cv::Size(11, 11), 1.5
        );

        cv::GaussianBlur(
            img2Sq, sigma2Sq, cv::Size(11, 11), 1.5
        );

        cv::GaussianBlur(
            img1img2, sigma12, cv::Size(11, 11), 1.5
        );

        sigma1Sq -= mu1Sq;
        sigma2Sq -= mu2Sq;
        sigma12 -= mu1mu2;

        cv::Mat numerator =
            (2.0 * mu1mu2 + C1).mul(2.0 * sigma12 + C2);

        cv::Mat denominator =
            (mu1Sq + mu2Sq + C1).mul(
                sigma1Sq + sigma2Sq + C2
            );

        cv::Mat ssimMap;

        cv::divide(numerator, denominator, ssimMap);

        totalSSIM += cv::mean(ssimMap)[0];
    }

    return totalSSIM / channels1.size();
}

// Compression ratio : taille originale / taille compressee
inline double calculateCompressionRatio(
    size_t originalSize,
    size_t compressedSize
) {
    if (compressedSize == 0) return 0.0;

    return static_cast<double>(originalSize) /
           static_cast<double>(compressedSize);
}

#endif
