#!/bin/bash

# =========================================
# COMPILATION
# =========================================

echo "========== COMPILATION =========="

g++ -std=c++17 -O2 -Iinclude \
src/main.cpp \
src/sequential/SequentialCompressor.cpp \
src/openmp/OpenMPCompressor.cpp \
src/Decompressor.cpp \
-o compression_prog \
$(pkg-config --cflags --libs opencv4) \
-fopenmp

if [ $? -ne 0 ]; then
    echo "Erreur compilation !"
    exit 1
fi

echo "Compilation reussie !"

# =========================================
# MODE IMAGE PERSONNALISEE
# =========================================

if [ "$1" = "--single" ]; then

    IMAGE="$2"
    THREADS="${3:-4}"

    if [ -z "$IMAGE" ]; then
        echo "Image manquante !"
        exit 1
    fi

    if ! [[ "$THREADS" =~ ^[1-9][0-9]*$ ]]; then
        echo "Nombre de threads invalide !"
        exit 1
    fi

    echo ""
    echo "========== IMAGE PERSONNALISEE =========="

    ./compression_prog --single "$IMAGE" "$THREADS"

    exit $?
fi

# =========================================
# MODE TEST NORMAL
# =========================================

THREADS=${1:-4}

if ! [[ "$THREADS" =~ ^[1-9][0-9]*$ ]]; then
    echo "Nombre de threads invalide !"
    echo "Utilisation : ./run.sh 4"
    exit 1
fi

echo ""
echo "========== EXECUTION =========="

./compression_prog "$THREADS"
