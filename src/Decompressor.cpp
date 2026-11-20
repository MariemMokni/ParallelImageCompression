#include "../include/Compressor.h"

#include <climits>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <algorithm>

using namespace std;
using namespace cv;

// Calcule le nombre de bits necessaires
static int getBitsPerValue(int quantizationStep) {

    int levels = 255 / quantizationStep + 1;
    int bits = 1;

    while ((1 << bits) < levels) {
        bits++;
    }

    return bits;
}

// Decompresse les valeurs Bit Packing
static vector<uchar> unpackValues(
    const vector<uchar>& packed,
    size_t valueCount,
    int quantizationStep,
    int bits
) {

    size_t expectedSize = (valueCount * bits + 7) / 8;

    if (packed.size() != expectedSize) {
        throw runtime_error("taille bp invalide");
    }

    vector<uchar> values;
    values.reserve(valueCount);

    for (size_t i = 0; i < valueCount; i++) {

        uint32_t index = 0;
        size_t position = i * bits;

        for (int b = 0; b < bits; b++) {

            if (packed[(position + b) / 8] &
                (1u << ((position + b) % 8))) {

                index |= (1u << b);
            }
        }

        values.push_back(
            static_cast<uchar>(index * quantizationStep)
        );
    }

    return values;
}

// Decompression du fichier PIC
Mat decompressImage(const string& inputPath) {

    ifstream file(inputPath, ios::binary);

    if (!file) {
        throw runtime_error("impo d'ouvrir le fichier PIC");
    }

    // 1. Lecture du magic number
    char magic[4];

    file.read(magic, 4);

    if (!file || string(magic, 4) != "PIC3") {
        throw runtime_error("format PIC invalide");
    }

    // 2. Lecture du header
    uint32_t width, height, channels;
    uint32_t step, blockSize, blocksX, blocksY;

    file.read(reinterpret_cast<char*>(&width), sizeof(width));
    file.read(reinterpret_cast<char*>(&height), sizeof(height));
    file.read(reinterpret_cast<char*>(&channels), sizeof(channels));
    file.read(reinterpret_cast<char*>(&step), sizeof(step));
    file.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
    file.read(reinterpret_cast<char*>(&blocksX), sizeof(blocksX));
    file.read(reinterpret_cast<char*>(&blocksY), sizeof(blocksY));

    if (!file || width == 0 || height == 0 ||
        channels != 3 || step == 0 || step > 256 ||
        blockSize == 0) {

        throw runtime_error("Header PIC invalide");
    }

    if (width > INT_MAX || height > INT_MAX) {
        throw runtime_error("Dimensions de l'image invalides");
    }

    if (blocksX != (width + blockSize - 1) / blockSize ||
        blocksY != (height + blockSize - 1) / blockSize) {

        throw runtime_error("Dimensions des blocs invalides");
    }

    // 3. Creation de l'image reconstruite
    Mat image(
        static_cast<int>(height),
        static_cast<int>(width),
        CV_8UC3
    );

    // 4. Lecture et decompression des blocs
    for (uint32_t block = 0; block < blocksX * blocksY; block++) {

        uint32_t bx = block % blocksX;
        uint32_t by = block / blocksX;

        uint32_t startX = bx * blockSize;
        uint32_t startY = by * blockSize;

        uint32_t bw = min(blockSize, width - startX);
        uint32_t bh = min(blockSize, height - startY);

        size_t expected =
            static_cast<size_t>(bw) * bh * channels;

        uint8_t mode;

        file.read(reinterpret_cast<char*>(&mode), sizeof(mode));

        if (!file || mode > 2) {
            throw runtime_error("mode de compression invalide");
        }

        uint32_t size;

        file.read(reinterpret_cast<char*>(&size), sizeof(size));

        if (!file) {
            throw runtime_error("fichier PIC incomplet");
        }

        vector<uchar> decoded;

        // MODE 1 : RLE
        if (mode == 1) {

            uint32_t runCount = size;

            for (uint32_t i = 0; i < runCount; i++) {

                uint32_t count;
                uint8_t value;

                file.read(
                    reinterpret_cast<char*>(&count),
                    sizeof(count)
                );

                file.read(
                    reinterpret_cast<char*>(&value),
                    sizeof(value)
                );

                if (!file || count == 0 ||
                    count > expected - decoded.size()) {

                    throw runtime_error("donnees RLE invalides");
                }

                decoded.insert(decoded.end(), count, value);
            }
        }

        // MODE 2 : BIT PACKING
        else if (mode == 2) {

            vector<uchar> data(size);

            file.read(
                reinterpret_cast<char*>(data.data()),
                size
            );

            if (!file) {
                throw runtime_error("Donnees Bit Packing incompletes");
            }

            decoded = unpackValues(
                data,
                expected,
                step,
                getBitsPerValue(step)
            );
        }

        // MODE 0 : RAW
        else {

            if (size != expected) {
                throw runtime_error("Taille Raw invalide");
            }

            decoded.resize(size);

            file.read(
                reinterpret_cast<char*>(decoded.data()),
                size
            );

            if (!file) {
                throw runtime_error("Donnees Raw incompletes");
            }
        }

        // 5. Verification du bloc reconstruit
        if (decoded.size() != expected) {
            throw runtime_error("Taille du bloc reconstruite invalide");
        }

        // 6. Copie des pixels dans l'image finale
        size_t position = 0;

        for (uint32_t y = 0; y < bh; y++) {

            uchar* output = image.ptr<uchar>(startY + y);

            for (uint32_t x = 0; x < bw; x++) {

                for (uint32_t c = 0; c < channels; c++) {

                    output[(startX + x) * channels + c] =
                        decoded[position++];
                }
            }
        }
    }

    return image;
}
