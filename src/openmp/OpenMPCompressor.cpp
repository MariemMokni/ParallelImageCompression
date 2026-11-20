#include "../../include/Compressor.h"

#include <fstream>
#include <chrono>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <utility>
#include <cstdint>
#include <omp.h>

using namespace std;
using namespace cv;

struct EncodedBlock {
    uint8_t mode; // 0 = Raw, 1 = RLE, 2 = Bit Packing
    vector<Run> runs;
    vector<uchar> raw;
    vector<uchar> packed;
};

// Calcule le nombre de bits necessaires pour une valeur
static int getBitsPerValue(int quantizationStep) {

    int levels = 255 / quantizationStep + 1;
    int bits = 1;

    while ((1 << bits) < levels) {
        bits++;
    }

    return bits;
}

// Compacte les valeurs quantifiees
static vector<uchar> packValues(
    const vector<uchar>& values,
    int quantizationStep,
    int bits
) {

    size_t totalBits = values.size() * bits;

    vector<uchar> packed((totalBits + 7) / 8, 0);

    for (size_t i = 0; i < values.size(); i++) {

        uint32_t index = values[i] / quantizationStep;
        size_t position = i * bits;

        for (int b = 0; b < bits; b++) {

            if (index & (1u << b)) {

                packed[(position + b) / 8] |=
                    static_cast<uchar>(
                        1u << ((position + b) % 8)
                    );
            }
        }
    }

    return packed;
}

// Decompacte les valeurs
static vector<uchar> unpackValues(
    const vector<uchar>& packed,
    size_t valueCount,
    int quantizationStep,
    int bits
) {

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

CompressionResult compressOpenMP(
    const Mat& image,
    const string& outputPath,
    int quantizationStep
) {

    if (image.empty() || quantizationStep < 1 ||
        quantizationStep > 256) {

        throw runtime_error("parametres de compression invalides");
    }

    const int BLOCK_SIZE = 64;

    int width = image.cols;
    int height = image.rows;
    int channels = image.channels();

    int blocksX = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int blocksY = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;

    int totalBlocks = blocksX * blocksY;

    vector<EncodedBlock> encodedBlocks(totalBlocks);

    auto start = chrono::high_resolution_clock::now();

    // 1. Traitement parallele des blocs
    #pragma omp parallel for schedule(static)
    for (int block = 0; block < totalBlocks; block++) {

        int bx = block % blocksX;
        int by = block / blocksX;

        int startX = bx * BLOCK_SIZE;
        int startY = by * BLOCK_SIZE;

        int blockWidth = min(BLOCK_SIZE, width - startX);
        int blockHeight = min(BLOCK_SIZE, height - startY);

        vector<uchar> quantized;

        quantized.reserve(
            static_cast<size_t>(blockWidth) *
            blockHeight * channels
        );

        // 2. Quantification
        for (int y = 0; y < blockHeight; y++) {

            const uchar* input = image.ptr<uchar>(startY + y);

            for (int x = 0; x < blockWidth; x++) {

                for (int c = 0; c < channels; c++) {

                    uchar value =
                        input[(startX + x) * channels + c];

                    uchar qValue =
                        (value / quantizationStep) *
                        quantizationStep;

                    quantized.push_back(qValue);
                }
            }
        }

        // 3. Encodage RLE
        vector<Run> runs;

        for (size_t i = 0; i < quantized.size();) {

            uchar value = quantized[i];
            uint32_t count = 1;

            while (i + count < quantized.size() &&
                   quantized[i + count] == value) {

                count++;
            }

            runs.push_back({count, value});
            i += count;
        }

        // 4. Encodage Bit Packing
        int bits = getBitsPerValue(quantizationStep);

        vector<uchar> packed = packValues(
            quantized,
            quantizationStep,
            bits
        );

        // 5. Comparaison des trois methodes
        size_t rleSize =
            sizeof(uint32_t) +
            runs.size() *
            (sizeof(uint32_t) + sizeof(uint8_t));

        size_t rawSize =
            sizeof(uint32_t) + quantized.size();

        size_t packedSize =
            sizeof(uint32_t) + packed.size();

        if (rleSize <= rawSize && rleSize <= packedSize) {

            encodedBlocks[block].mode = 1;
            encodedBlocks[block].runs = move(runs);
        }

        else if (packedSize <= rawSize) {

            encodedBlocks[block].mode = 2;
            encodedBlocks[block].packed = move(packed);
        }

        else {

            encodedBlocks[block].mode = 0;
            encodedBlocks[block].raw = move(quantized);
        }
    }

    // 6. Enregistrement du fichier compresse
    ofstream file(outputPath, ios::binary);

    if (!file) {
        throw runtime_error(
            "impo de creer le fichier compresse"
        );
    }

    const char magic[4] = {'P', 'I', 'C', '3'};

    uint32_t w = width;
    uint32_t h = height;
    uint32_t ch = channels;
    uint32_t step = quantizationStep;
    uint32_t blockSize = BLOCK_SIZE;
    uint32_t bxCount = blocksX;
    uint32_t byCount = blocksY;

    file.write(magic, 4);

    file.write(reinterpret_cast<const char*>(&w), sizeof(w));
    file.write(reinterpret_cast<const char*>(&h), sizeof(h));
    file.write(reinterpret_cast<const char*>(&ch), sizeof(ch));
    file.write(reinterpret_cast<const char*>(&step), sizeof(step));
    file.write(reinterpret_cast<const char*>(&blockSize), sizeof(blockSize));
    file.write(reinterpret_cast<const char*>(&bxCount), sizeof(bxCount));
    file.write(reinterpret_cast<const char*>(&byCount), sizeof(byCount));

    for (const EncodedBlock& block : encodedBlocks) {

        file.write(
            reinterpret_cast<const char*>(&block.mode),
            sizeof(block.mode)
        );

        if (block.mode == 1) {

            // RLE
            uint32_t runCount =
                static_cast<uint32_t>(block.runs.size());

            file.write(
                reinterpret_cast<const char*>(&runCount),
                sizeof(runCount)
            );

            for (const Run& run : block.runs) {

                file.write(
                    reinterpret_cast<const char*>(&run.count),
                    sizeof(run.count)
                );

                file.write(
                    reinterpret_cast<const char*>(&run.value),
                    sizeof(run.value)
                );
            }
        }

        else if (block.mode == 2) {

            // Bit Packing
            uint32_t size =
                static_cast<uint32_t>(block.packed.size());

            file.write(
                reinterpret_cast<const char*>(&size),
                sizeof(size)
            );

            file.write(
                reinterpret_cast<const char*>(block.packed.data()),
                size
            );
        }

        else {

            // Raw
            uint32_t size =
                static_cast<uint32_t>(block.raw.size());

            file.write(
                reinterpret_cast<const char*>(&size),
                sizeof(size)
            );

            file.write(
                reinterpret_cast<const char*>(block.raw.data()),
                size
            );
        }
    }

    file.close();

    if (!file) {
        throw runtime_error("erreur pendant l'ecriture du fichier");
    }

    auto end = chrono::high_resolution_clock::now();

    double elapsed =
        chrono::duration<double>(end - start).count();

    // 7. Reconstruction parallele de l'image
    Mat reconstructed(image.size(), image.type());

    vector<int> errors(totalBlocks, 0);

    #pragma omp parallel for schedule(static)
    for (int block = 0; block < totalBlocks; block++) {

        int bx = block % blocksX;
        int by = block / blocksX;

        int startX = bx * BLOCK_SIZE;
        int startY = by * BLOCK_SIZE;

        int blockWidth = min(BLOCK_SIZE, width - startX);
        int blockHeight = min(BLOCK_SIZE, height - startY);

        size_t expectedSize =
            static_cast<size_t>(blockWidth) *
            blockHeight * channels;

        vector<uchar> decoded;

        const EncodedBlock& encoded = encodedBlocks[block];

        if (encoded.mode == 1) {

            // Decodage RLE
            for (const Run& run : encoded.runs) {

                for (uint32_t i = 0; i < run.count; i++) {
                    decoded.push_back(run.value);
                }
            }
        }

        else if (encoded.mode == 2) {

            // Decodage Bit Packing
            decoded = unpackValues(
                encoded.packed,
                expectedSize,
                quantizationStep,
                getBitsPerValue(quantizationStep)
            );
        }

        else {

            // Decodage Raw
            decoded = encoded.raw;
        }

        if (decoded.size() != expectedSize) {
            errors[block] = 1;
            continue;
        }

        size_t position = 0;

        for (int y = 0; y < blockHeight; y++) {

            uchar* output = reconstructed.ptr<uchar>(startY + y);

            for (int x = 0; x < blockWidth; x++) {

                for (int c = 0; c < channels; c++) {

                    output[(startX + x) * channels + c] =
                        decoded[position++];
                }
            }
        }
    }

    // Verification apres le traitement parallele
    for (int block = 0; block < totalBlocks; block++) {

        if (errors[block] != 0) {

            throw runtime_error(
                "erreur lors de la reconstruction d'un bloc"
            );
        }
    }

    return {
        elapsed,
        static_cast<size_t>(filesystem::file_size(outputPath)),
        reconstructed
    };
}
