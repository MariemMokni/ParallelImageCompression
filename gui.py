import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk
import subprocess
import threading
import os

# ==========================================
# CONFIGURATION
# ==========================================

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))

RESULTS_DIR = os.path.join(PROJECT_DIR, "results", "gui")

SEQ_DIR = os.path.join(RESULTS_DIR, "sequential")
OMP_DIR = os.path.join(RESULTS_DIR, "openmp")

# ==========================================
# INTERFACE
# ==========================================

class CompressionGUI:

    def __init__(self, root):

        self.root = root

        self.root.title("Parallel Image Compression")
        self.root.geometry("1150x800")
        self.root.configure(bg="#f0f2f5")

        self.image_path = ""
        self.photos = []

        self.create_interface()

    # ======================================
    # INTERFACE
    # ======================================

    def create_interface(self):

        title = tk.Label(
            self.root,
            text="PARALLEL IMAGE COMPRESSION",
            font=("Arial", 22, "bold"),
            bg="#f0f2f5"
        )

        title.pack(pady=15)

        # ==================================
        # PARAMETRES
        # ==================================

        settings = ttk.LabelFrame(
            self.root,
            text="Paramètres",
            padding=10
        )

        settings.pack(
            fill="x",
            padx=15,
            pady=5
        )

        ttk.Label(
            settings,
            text="Threads OpenMP :"
        ).pack(side="left", padx=5)

        self.threads_var = tk.StringVar(value="4")

        ttk.Combobox(
            settings,
            textvariable=self.threads_var,
            values=["1", "2", "4", "8"],
            width=8,
            state="readonly"
        ).pack(side="left", padx=5)

        ttk.Button(
            settings,
            text="Choisir une image",
            command=self.choose_image
        ).pack(side="left", padx=15)

        self.run_button = ttk.Button(
            settings,
            text="Lancer compression",
            command=self.run_compression
        )

        self.run_button.pack(side="left", padx=5)

        self.status_label = ttk.Label(
            settings,
            text="Choisissez une image"
        )

        self.status_label.pack(side="left", padx=15)

        # ==================================
        # IMAGE CHOISIE
        # ==================================

        self.path_label = ttk.Label(
            self.root,
            text="Aucune image sélectionnée",
            font=("Arial", 10)
        )

        self.path_label.pack(pady=5)

        # ==================================
        # AFFICHAGE IMAGES
        # ==================================

        images_frame = ttk.Frame(self.root)

        images_frame.pack(
            fill="both",
            expand=True,
            padx=15,
            pady=10
        )

        self.original_panel = self.create_panel(
            images_frame,
            "Image originale"
        )

        self.seq_panel = self.create_panel(
            images_frame,
            "Sequential"
        )

        self.omp_panel = self.create_panel(
            images_frame,
            "OpenMP"
        )

        self.original_panel.pack(
            side="left",
            fill="both",
            expand=True,
            padx=5
        )

        self.seq_panel.pack(
            side="left",
            fill="both",
            expand=True,
            padx=5
        )

        self.omp_panel.pack(
            side="left",
            fill="both",
            expand=True,
            padx=5
        )

        # ==================================
        # RESULTATS
        # ==================================

        result_frame = ttk.LabelFrame(
            self.root,
            text="Résultats",
            padding=5
        )

        result_frame.pack(
            fill="both",
            expand=True,
            padx=15,
            pady=10
        )

        self.results = tk.Text(
            result_frame,
            height=13,
            font=("Courier", 10)
        )

        self.results.pack(
            side="left",
            fill="both",
            expand=True
        )

        scrollbar = ttk.Scrollbar(
            result_frame,
            command=self.results.yview
        )

        scrollbar.pack(side="right", fill="y")

        self.results.config(
            yscrollcommand=scrollbar.set
        )

    # ======================================
    # IMAGE PANEL
    # ======================================

    def create_panel(self, parent, title):

        panel = ttk.LabelFrame(
            parent,
            text=title,
            padding=5
        )

        panel.image_label = tk.Label(
            panel,
            text="Image non disponible",
            bg="white"
        )

        panel.image_label.pack(
            fill="both",
            expand=True
        )

        return panel

    # ======================================
    # CHOISIR IMAGE
    # ======================================

    def choose_image(self):

        path = filedialog.askopenfilename(
            title="Choisir une image",
            filetypes=[
                ("Images", "*.jpg *.jpeg *.png *.bmp"),
                ("Tous les fichiers", "*.*")
            ]
        )

        if not path:
            return

        self.image_path = path

        self.path_label.config(
            text=path
        )

        self.show_image(
            self.original_panel,
            path
        )

        # Effacer les anciens résultats
        self.clear_panel(self.seq_panel)
        self.clear_panel(self.omp_panel)

        self.results.delete(
            "1.0",
            tk.END
        )

    # ======================================
    # AFFICHER IMAGE
    # ======================================

    def show_image(self, panel, path):

        if not os.path.exists(path):
            return

        image = Image.open(path)

        image.thumbnail((340, 260))

        photo = ImageTk.PhotoImage(image)

        panel.image_label.config(
            image=photo,
            text=""
        )

        panel.image_label.image = photo

        self.photos.append(photo)

    # ======================================
    # EFFACER IMAGE
    # ======================================

    def clear_panel(self, panel):

        panel.image_label.config(
            image="",
            text="Image non disponible"
        )

        panel.image_label.image = None

    # ======================================
    # LANCER COMPRESSION
    # ======================================

    def run_compression(self):

        if not self.image_path:

            messagebox.showwarning(
                "Attention",
                "Choisissez d'abord une image."
            )

            return

        threads = self.threads_var.get()

        self.run_button.config(
            state="disabled"
        )

        self.status_label.config(
            text="Compression en cours..."
        )

        self.results.delete(
            "1.0",
            tk.END
        )

        self.results.insert(
            tk.END,
            "Compilation et compression en cours...\n"
            "Veuillez patienter.\n"
        )

        # Thread pour ne pas bloquer l'interface

        threading.Thread(
            target=self.execute,
            args=(threads,),
            daemon=True
        ).start()

    # ======================================
    # EXECUTION C++
    # ======================================

    def execute(self, threads):

        try:

            process = subprocess.run(
                [
                    "bash",
                    "run.sh",
                    "--single",
                    self.image_path,
                    threads
                ],
                cwd=PROJECT_DIR,
                capture_output=True,
                text=True
            )

            output = process.stdout

            if process.stderr:
                output += "\n" + process.stderr

            self.root.after(
                0,
                self.finish,
                process.returncode,
                output
            )

        except Exception as error:

            self.root.after(
                0,
                self.finish,
                1,
                str(error)
            )

    # ======================================
    # FIN EXECUTION
    # ======================================

    def finish(self, code, output):

        self.run_button.config(
            state="normal"
        )

        self.results.delete(
            "1.0",
            tk.END
        )

        self.results.insert(
            tk.END,
            output
        )

        if code == 0:

            self.status_label.config(
                text="Terminé"
            )

            name = os.path.splitext(
                os.path.basename(self.image_path)
            )[0]

            seq_path = os.path.join(
                SEQ_DIR,
                name + "_reconstructed.png"
            )

            omp_path = os.path.join(
                OMP_DIR,
                name + "_reconstructed.png"
            )

            self.show_image(
                self.seq_panel,
                seq_path
            )

            self.show_image(
                self.omp_panel,
                omp_path
            )

        else:

            self.status_label.config(
                text="Erreur"
            )

            messagebox.showerror(
                "Erreur",
                "Erreur pendant le traitement.\n"
                "Consultez les résultats."
            )


# ==========================================
# LANCEMENT
# ==========================================

if __name__ == "__main__":

    root = tk.Tk()

    app = CompressionGUI(root)
