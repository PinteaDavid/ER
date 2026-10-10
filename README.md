# Descompunere celulară aproximativă (Cap. 7.8.2)

Aplicație C++ cu interfață grafică (raylib) care implementează cele două variante ale
metodei descompunerii celulare aproximative pentru planificarea mișcării unui robot mobil:

- **7.8.2.1 Descompunerea omogenă** (metoda grilei omogene)
- **7.8.2.2 Descompunerea neomogenă** (metoda arborelui, arbore 2^m cu m = 2 → quadtree)

## Compilare și rulare (macOS)

Ai nevoie de Xcode Command Line Tools și CMake:

```bash
xcode-select --install      # dacă nu le ai deja
brew install cmake          # opțional: brew install raylib (altfel se descarcă automat)
```

Apoi, din acest folder:

```bash
./build.sh                  # compilează și pornește aplicația
```

sau manual:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/descompunere
```

La prima compilare, dacă raylib nu e instalat, CMake îl descarcă de pe GitHub (raylib 5.5).

## Utilizare

| Control | Ce face |
|---|---|
| **Metodă** | Omogenă (grilă) / Neomogenă (arbore) — tastele `1` / `2` |
| **Latura / lățimea / înălțimea celulei** | Dimensiunea celulelor grilei (pătrate sau dreptunghiulare, fig. 7.74 e) |
| **Înălțimea arborelui** | Rezoluția limită (adâncimea maximă) la metoda arborelui |
| **Reset / Pas / Play / Final** | Rulează algoritmul arborelui iterație cu iterație (`Spațiu` = pas) |
| **Mută / Desenează / Șterge** | Tragi Start, Țintă sau obstacole / desenezi poligoane noi / ștergi obstacole |
| **Scene** | Fig. 7.74, Fig. 7.75, Fig. 7.76 |

Desenare obstacol: click pentru fiecare vârf; închizi cu click pe primul vârf, click dreapta sau `Enter`.
`Backspace` șterge ultimul vârf, `Esc` anulează.

Dacă ții mouse-ul peste o celulă, vezi starea ei (Liberă / Ocupată / Mixtă), dimensiunile și, la
metoda arborelui, eticheta nodului (ex. `114`) și adâncimea lui.

## Cum se leagă codul de curs

| Fișier | Conținut |
|---|---|
| `src/scene.cpp` | `classifyCell` — **Liberă** ⇔ int(K) ∩ SCB = Ø, **Ocupată** ⇔ K ⊆ SCB, **Mixtă** altfel. SCB = regiunea ocupată de obstacole; robotul e considerat punctiform. Plus scenele predefinite. |
| `src/omogen.cpp` | Grila omogenă: tot spațiul de lucru e împărțit în celule identice, clasificate L/O/M. Celulele O și M sunt interzise. Canalul de celule libere adiacente se caută cu A*, iar drumul trece prin mijloacele laturilor comune ale celulelor din canal. |
| `src/neomogen.cpp` | Metoda arborelui: fiecare celulă Mixtă se împarte în 4 celule egale etichetate 1–4 (1 = stânga-sus, 2 = dreapta-sus, 3 = stânga-jos, 4 = dreapta-jos); eticheta fiului = eticheta tatălui + cifra (0 → 1 … 4 → 11 … 14 → 114 …). Implementează exact algoritmul în 8 pași din curs (vezi mai jos). |
| `src/geometry.cpp` | Geometrie: punct în poligon, distanțe segment–segment, celulă în poligon, triangulare. |
| `src/ui.cpp`, `src/main.cpp` | Interfața grafică. |

Algoritmul metodei arborelui (`QuadPlanner::step`):

1. Calculează descompunerea celulară până la o anumită rezoluție (începe cu celula rădăcină 0);
2. Identifică celulele care conțin configurațiile inițială și finală;
3. Caută o secvență de celule Libere/Mixte între cele două celule;
4. Dacă nu există nicio secvență → nu există drum;
5. Dacă există o secvență de celule **Libere** → drum găsit;
6. Dacă s-a atins rezoluția limită (înălțimea arborelui) → eșec; celulele mixte rămase sunt considerate ocupate;
7. Descompune în continuare celulele Mixte;
8. Revine la pasul 2.

Jurnalul din dreapta afișează fiecare pas.

## Ce poți observa

- **Fig. 7.74 / Fig. 7.75**: cu celule mai mari calculul e mai rapid, dar spațiul liber se reduce
  (celulele mixte „expandează” suplimentar obstacolele).
- **Grilă vs. arbore**: compară numărul total de celule din grilă (memorie) cu numărul de noduri din arbore.
