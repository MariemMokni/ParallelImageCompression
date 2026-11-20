#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <cstdint>

//utilisée pour la méthode RLE

struct Run {
    uint32_t count;
    uint8_t value;
};

//récupérer les résultats de la compression
struct CompressionResult {
    double executionTime;
    size_t compressedSize;
    cv::Mat reconstructed;
};

CompressionResult compressSequential(
    const cv::Mat& image,
    const std::string& outputPath,
    int quantizationStep
);

CompressionResult compressOpenMP(
    const cv::Mat& image,
    const std::string& outputPath,
    int quantizationStep
);

// Decompression depuis le fichier .pic
cv::Mat decompressImage(const std::string& inputPath);

#endif
