#if defined(_OPENMP) && defined(_WINDOWS)
#define OMP_FOR_INDEX int
#else
#define OMP_FOR_INDEX std::vector<Terrain::Coordinates>::size_type
#endif

#ifdef _WINDOWS
#define FSEEK fseek
#else
#define FSEEK fseeko
#endif
