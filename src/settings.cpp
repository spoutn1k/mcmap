#include "./settings.h"
#include "logger.hpp"

void Settings::to_json(json &j, const Settings::WorldOptions &o) {
  j["mode"] = o.mode;
  j["output"] = o.outFile.string();
  j["colors"] = o.colorFile.string();

  j["save"] = o.save;
  j["dimension"] = o.dim;

  j["coordinates"] = o.boundaries.to_string();
  j["padding"] = o.padding;

  j["hideWater"] = o.hideWater;
  j["hideBeacons"] = o.hideBeacons;
  j["shading"] = o.shading;
  j["lighting"] = o.lighting;

  j["memory"] = o.mem_limit;
  j["tile"] = o.tile_size;
}
