#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <iostream>

inline cv::Mat loadImage(const std::string& path) {

    cv::Mat image = cv::imread(
        path,
        cv::IMREAD_COLOR //l'image charge en couleur
    );

    if (image.empty()) {
        std::cerr << "erreur : impossible de charger l'image : "
                  << path << std::endl;
    }

    return image;
}

#endif

