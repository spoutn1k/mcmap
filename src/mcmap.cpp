#include "./mcmap.h"
#include <counter.hpp>

#ifdef _OPENMP
#define THREADS omp_get_max_threads()
#else
#define THREADS 1
#endif

namespace mcmap {

bool writeMapInfo(fs::path outFile, const Canvas &finalImage,
                  const uint32_t tileSize) {
  json data({{"imageDimensions", {finalImage.width(), finalImage.height()}},
             {"layerLocation", outFile.string()},
             {"tileSize", tileSize}});

  fs::path infoFile = outFile / "mapinfo.json";
  std::ofstream infoStream;

  try {
    infoStream.open(infoFile.string());
  } catch (const std::exception &err) {
    logger::error("Failed to open {} for writing: {}", infoFile.string(),
                  err.what());
    return false;
  }

  infoStream << data.dump();
  infoStream.close();

  return true;
}

int render(const Settings::WorldOptions &options, const Colors::Palette &colors,
           Progress::Callback cb) {
  logger::debug("Rendering {} with {}\n", options.save.name,
                options.boundaries.to_string());

  // Divide terrain according to fragment size
  std::vector<World::Coordinates> fragment_coordinates;
  options.boundaries.fragment(fragment_coordinates, options.fragment_size);

  std::vector<Canvas> fragments(fragment_coordinates.size());

  Progress::Status s =
      Progress::Status(fragment_coordinates.size(), cb, Progress::RENDERING);

  // This value represents the amount of canvasses that can fit in memory at
  // once to avoid going over the limit of RAM
  Counter<size_t> capacity =
      memory_capacity(options.mem_limit, fragment_coordinates[0].footprint(),
                      fragment_coordinates.size(), THREADS);

  if (!capacity)
    return false;

  logger::debug("Memory capacity: {} fragments - {} fragments scheduled\n",
                size_t(capacity), fragment_coordinates.size());

  // If caching is needed, ensure the cache directory is available
  if (capacity < fragments.size())
    if (!prepare_cache(getTempDir()))
      return false;

  auto begin = std::chrono::high_resolution_clock::now();
#ifdef _OPENMP
#pragma omp parallel shared(fragments, capacity)
#endif
  {
#ifdef _OPENMP
#pragma omp for ordered schedule(dynamic)
#endif
    for (OMP_FOR_INDEX i = 0; i < fragment_coordinates.size(); i++) {
      logger::debug("Rendering {}\n", fragment_coordinates[i].to_string());
      IsometricCanvas canvas;
      canvas.setMap(fragment_coordinates[i]);
      canvas.setColors(colors);

      // Load the minecraft terrain to render
      Terrain::Data world(fragment_coordinates[i], options.regionDir(), colors);

      // Draw the terrain fragment
      canvas.shading = options.shading;
      canvas.lighting = options.lighting;
      canvas.setMarkers(options.totalMarkers, options.markers);
      canvas.renderTerrain(world);

      if (!canvas.empty()) {
        if (i >= capacity) {
          fs::path temporary = getTempDir() / canvas.map.to_string();
          canvas.save(temporary);

          fragments[i] = std::move(ImageCanvas(canvas.map, temporary));
        } else
          fragments[i] = std::move(canvas);
      } else {
        // If the canvas was empty, increase the capacity to reflect the free
        // space
        if (i < capacity)
          ++capacity;
      }

#ifdef _OPENMP
#pragma omp critical
#endif
      { s.increment(); }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();

  logger::debug(
      "Rendered in {}ms\n",
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count());

  CompositeCanvas merged(std::move(fragments));
  logger::debug("{}\n", merged.to_string());

  if (merged.empty()) {
    logger::error("Canvas is empty !\n");
    return false;
  }

  begin = std::chrono::high_resolution_clock::now();

  bool save_status;

  if (options.tile_size &&
      writeMapInfo(options.outFile, merged, options.tile_size)) {
    save_status = merged.tile(options.outFile, options.tile_size, cb);
  } else {
    save_status = merged.save(options.outFile, options.padding, cb);
  }

  end = std::chrono::high_resolution_clock::now();

  if (!save_status)
    return false;

  logger::debug(
      "Drawn PNG in {}ms\n",
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count());

  return true;
}

} // namespace mcmap
