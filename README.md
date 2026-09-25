gnss-tool/
├── CMakeLists.txt              # top-level, enable-language(CXX Fortran)
├── cmake/
│   └── FortranCInterop.cmake   # helper modul, kalau perlu
├── src/
│   ├── core/                   # Fortran: numerik inti
│   │   ├── linalg/             # LS solver, matrix ops
│   │   ├── estimator/          # LS/EKF interface (modul 3-5 nanti)
│   │   └── CMakeLists.txt
│   ├── io/                     # C++: RINEX parser, SP3/CLK parser (nanti)
│   │   ├── rinex/
│   │   │   ├── obs/            # observation file parser
│   │   │   └── nav/            # navigation message parser
│   │   ├── CMakeLists.txt
│   ├── cli/                    # C++: CLI entry point, arg parsing, orchestrasi
│   │   ├── main.cpp
│   │   └── CMakeLists.txt
│   └── bindings/                # ISO-C-BINDING interface layer (C++ ↔ Fortran)
│       └── CMakeLists.txt
├── include/                    # header C++ publik (kalau dipisah dari src)
├── tests/
│   ├── unit/                   # per-modul (parser, LS solver, dll)
│   ├── data/                   # sample RINEX kecil untuk test (2.11/3.x/4.x masing-masing 1 contoh)
│   └── CMakeLists.txt
├── docs/
├── .gitignore
└── README.md
