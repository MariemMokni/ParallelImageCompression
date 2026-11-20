# 🖼️ Compression d'Images – Version Séquentielle et Parallèle

## 📌 Présentation

Ce projet consiste à développer une application de compression d'images en C++ proposant deux approches :

- **Version séquentielle** : traitement des blocs les uns après les autres.
- **Version parallèle** : traitement simultané des blocs avec OpenMP.

L'objectif est de comparer les performances des deux versions en termes de temps d'exécution, de taux de compression et de qualité des images reconstruites.

## 🎯 Objectifs

- Implémenter un algorithme de compression séquentielle.
- Développer une version parallèle avec OpenMP.
- Diviser les images en blocs de 64 × 64 pixels.
- Comparer les performances selon le nombre de threads.
- Mesurer le taux de compression et la qualité visuelle.
- Analyser le Speedup, le PSNR et le SSIM.

## ⚙️ Méthode de compression

La méthode proposée repose sur les étapes suivantes :

1. Découpage de l'image en blocs de 64 × 64 pixels.
2. Quantification des couleurs avec un pas de 16.
3. Encodage de chaque bloc avec trois méthodes :
   - **RLE (Run-Length Encoding)**
   - **Bit Packing**
   - **Raw**
4. Sélection automatique de la représentation la plus compacte.
5. Enregistrement dans un fichier compressé `.pic`.

## 🛠️ Technologies utilisées

| Technologie | Utilisation |
|---|---|
| C++ | Algorithmes de compression |
| OpenMP | Programmation parallèle |
| OpenCV | Lecture des images et métriques |
| Python 3 | Interface graphique |
| Tkinter | Interface utilisateur |
| G++ | Compilation |
| Ubuntu | Environnement de développement |

## 🚀 Fonctionnalités

- Sélection d'une image.
- Compression séquentielle.
- Compression parallèle avec OpenMP.
- Choix du nombre de threads.
- Décompression et reconstruction des images.
- Comparaison des deux versions.
- Calcul du temps d'exécution.
- Calcul du taux de compression.
- Évaluation de la qualité avec PSNR et SSIM.
- Calcul du Speedup.
- Visualisation des résultats.

## 📊 Résultats expérimentaux

Les tests ont été réalisés sur cinq images avec 4 threads.

| Image | Taux de compression | PSNR (dB) | SSIM | Speedup |
|---|---:|---:|---:|---:|
| small.jpg | 49,92 % | 29,4651 | 0,8658 | 2,12x |
| medium.jpg | 51,24 % | 29,4800 | 0,8672 | 2,54x |
| large.jpg | 54,01 % | 29,4944 | 0,8791 | 2,61x |
| nature.png | 49,95 % | 29,0223 | 0,9108 | 2,76x |
| test.bmp | 54,88 % | 31,4844 | 0,9453 | 1,81x |

**Résultat principal :** la version parallèle est plus rapide sur les cinq images testées. Le meilleur Speedup obtenu est de **2,76x** pour nature.png.

Les deux versions produisent les mêmes tailles de fichiers compressés et les mêmes valeurs PSNR et SSIM.

## 🖥️ Interface graphique

Une interface graphique développée avec Python et Tkinter permet de :

- Sélectionner une image.
- Choisir le nombre de threads.
- Lancer les compressions séquentielle et parallèle.
- Visualiser les images originales et reconstruites.
- Consulter les métriques et les résultats.

## 📂 Structure du projet

<img width="857" height="787" alt="image" src="https://github.com/user-attachments/assets/c039612c-aba5-4dff-b26f-95d821f8582e" />


## 💻 Installation et exécution

### 1. Cloner le repository

```bash
git clone https://github.com/USERNAME/ParallelImageCompression.git
cd ParallelImageCompression
```

### 2. Installer les dépendances

```bash
sudo apt update
sudo apt install g++ build-essential libopencv-dev python3 python3-tk
```

### 3. Compiler et exécuter

```bash
chmod +x run.sh
./run.sh
```

### 4. Lancer l'interface graphique

```bash
python3 gui.py
```



**Formation :** Master CCDAD 2.2

**Année universitaire :** 2026–2027

---

⭐ Projet académique – Compression parallèle d'images avec C++ et OpenMP.
