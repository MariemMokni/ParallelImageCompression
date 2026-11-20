#include <iostream>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <omp.h>

#include "../include/ImageLoader.h"
#include "../include/Compressor.h"
#include "../include/Metrics.h"

using namespace std;
using namespace cv;

namespace fs = std::filesystem;

// =============================================
// TRAITEMENT D'UNE IMAGE
// =============================================

bool processImage(
    const string& path,
    int threads,
    const string& outputFolder
) {
    const int QUANTIZATION_STEP = 16;

    Mat image = loadImage(path);

    if (image.empty()) {
        cerr << "Erreur : image impossible a charger : "
             << path << endl;
        return false;
    }

    string name = fs::path(path).stem().string();

    size_t originalSize = image.total() * image.elemSize();

    string seqFolder = outputFolder + "/sequential/";
    string ompFolder = outputFolder + "/openmp/";

    fs::create_directories(seqFolder);
    fs::create_directories(ompFolder);

    cout << "\n====================================\n";
    cout << "IMAGE : " << path << endl;
    cout << "Resolution : " << image.cols
         << " x " << image.rows << endl;
    cout << "Taille originale : "
         << originalSize << " octets\n";

    // =========================================
    // COMPRESSION SEQUENTIELLE
    // =========================================

    CompressionResult seq = compressSequential(
        image,
        seqFolder + name + ".pic",
        QUANTIZATION_STEP
    );

    // =========================================
    // COMPRESSION OPENMP
    // =========================================

    CompressionResult par = compressOpenMP(
        image,
        ompFolder + name + ".pic",
        QUANTIZATION_STEP
    );

    // =========================================
    // DECOMPRESSION
    // =========================================

    Mat seqReconstructed = decompressImage(
        seqFolder + name + ".pic"
    );

    Mat parReconstructed = decompressImage(
        ompFolder + name + ".pic"
    );

    if (seqReconstructed.empty() ||
        parReconstructed.empty()) {

        cerr << "Erreur de decompression !\n";
        return false;
    }

    // =========================================
    // METRIQUES
    // =========================================

    double seqPSNR = calculatePSNR(
        image, seqReconstructed
    );

    double seqSSIM = calculateSSIM(
        image, seqReconstructed
    );

    double parPSNR = calculatePSNR(
        image, parReconstructed
    );

    double parSSIM = calculateSSIM(
        image, parReconstructed
    );

    double seqRate =
        (1.0 - (double)seq.compressedSize / originalSize)
        * 100.0;

    double parRate =
        (1.0 - (double)par.compressedSize / originalSize)
        * 100.0;

    double speedup = 0.0;

    if (par.executionTime > 0) {
        speedup = seq.executionTime / par.executionTime;
    }

    // =========================================
    // AFFICHAGE
    // =========================================

    cout << fixed << setprecision(4);

    cout << "\n--- SEQUENTIAL ---\n";
    cout << "Temps : " << seq.executionTime << " s\n";
    cout << "Taille compressee : "
         << seq.compressedSize << " octets\n";
    cout << "Taux de compression : "
         << seqRate << " %\n";
    cout << "PSNR : " << seqPSNR << " dB\n";
    cout << "SSIM : " << seqSSIM << endl;

    cout << "\n--- OPENMP ---\n";
    cout << "Temps : " << par.executionTime << " s\n";
    cout << "Taille compressee : "
         << par.compressedSize << " octets\n";
    cout << "Taux de compression : "
         << parRate << " %\n";
    cout << "PSNR : " << parPSNR << " dB\n";
    cout << "SSIM : " << parSSIM << endl;

    cout << "\n--- COMPARAISON ---\n";
    cout << "Speedup : " << speedup << "x\n";

    // =========================================
    // SAUVEGARDE IMAGES RECONSTRUITES
    // =========================================

    imwrite(
        seqFolder + name + "_reconstructed.png",
        seqReconstructed
    );

    imwrite(
        ompFolder + name + "_reconstructed.png",
        parReconstructed
    );

    cout << "\nImages reconstruites sauvegardees.\n";

    return true;
}

// =============================================
// MAIN
// =============================================

int main(int argc, char* argv[]) {

    int threads = 4;

    omp_set_dynamic(0);

    // =========================================
    // MODE IMAGE PERSONNALISEE
    // =========================================

    if (argc >= 2 &&
        string(argv[1]) == "--single") {

        if (argc < 3) {
            cerr << "Usage : ./compression_prog --single image [threads]\n";
            return 1;
        }

        string imagePath = argv[2];

        if (argc >= 4) {
            try {
                threads = stoi(argv[3]);
            }
            catch (...) {
                cerr << "Nombre de threads invalide !\n";
                return 1;
            }
        }

        if (threads < 1) {
            cerr << "Nombre de threads invalide !\n";
            return 1;
        }

        omp_set_num_threads(threads);

        cout << "Mode image personnalisee\n";
        cout << "Threads OpenMP : " << threads << endl;

        return processImage(
            imagePath,
            threads,
            "results/gui"
        ) ? 0 : 1;
    }

    // =========================================
    // MODE TEST DES 5 IMAGES
    // =========================================

    if (argc >= 2) {
        try {
            threads = stoi(argv[1]);
        }
        catch (...) {
            cerr << "Nombre de threads invalide !\n";
            return 1;
        }
    }

    if (threads < 1) {
        cerr << "Nombre de threads invalide !\n";
        return 1;
    }

    omp_set_num_threads(threads);

    vector<string> images = {
        "images/small.jpg",
        "images/medium.jpg",
        "images/large.jpg",
        "images/nature.png",
        "images/test.bmp"
    };

    cout << "\nTests des 5 images\n";
    cout << "Threads OpenMP : " << threads << endl;

    for (const string& path : images) {

        if (!processImage(
                path,
                threads,
                "results"
            )) {

            cerr << "Erreur pour : " << path << endl;
        }
    }

    cout << "\nFin des tests.\n";

    return 0;
}
