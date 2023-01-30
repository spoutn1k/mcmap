#include "./marker.h"

void Markers::from_json(const json &data, Marker &m) {
  data.at("x").get_to(m.x);
  data.at("y").get_to(m.y);
  data.at("z").get_to(m.z);

  data.at("title").get_to(m.text);
}

void Markers::to_json(json &j, const Marker &m) {
  j = json({{"coordinates", {m.map_x, m.map_y}}, {"title", m.text}});
}

bool Markers::load(std::vector<Marker> &markers, const fs::path &file) {

  std::ifstream markerStream;
  markerStream.open(file.string());

  json data = json::parse(markerStream);

  try {
    markers = data.get<std::vector<Markers::Marker>>();
  } catch (const nlohmann::detail::parse_error &err) {
    logger::error("Parsing JSON data failed: {}", err.what());
    return false;
  } catch (const std::invalid_argument &err) {
    logger::error("Parsing JSON data failed: {}", err.what());
    return false;
  }

  return true;
}
