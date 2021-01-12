#pragma once
#ifndef NBT_TO_JSON_HPP_
#define NBT_TO_JSON_HPP_

#include <json.hpp>
#include <nbt/nbt.hpp>

using nlohmann::json;

namespace nbt {

void to_json(json &j, const NBT &nbt) {

  switch (nbt.get_type()) {
  case tag_type::tag_byte:
  case tag_type::tag_short:
  case tag_type::tag_int:
  case tag_type::tag_long:
    j = json({{nbt.get_name(), nbt.get<long>()}});
    break;
  case tag_type::tag_float:
  case tag_type::tag_double:
    j = json({{nbt.get_name(), nbt.get<double>()}});
    break;
  case tag_type::tag_byte_array:
    j = json({{nbt.get_name(), *nbt.get<const std::vector<int8_t> *>()}});
    break;
  case tag_type::tag_string:
    j = json({{nbt.get_name(), nbt.get<std::string>()}});
    break;
  case tag_type::tag_list: {
    std::vector<json> data;
    const std::vector<NBT> *subs = nbt.get<const std::vector<NBT> *>();

    for (auto el : *subs)
      data.emplace_back(json(el));

    j = json({{nbt.get_name(), data}});
    break;
  }
  case tag_type::tag_compound: {
    json contents;

    const std::map<std::string, NBT> *subs =
        nbt.get<const std::map<std::string, NBT> *>();

    for (auto el : *subs)
      contents.update(json(el.second));

    j = json({{nbt.get_name(), contents}});
    break;
  }
  case tag_type::tag_int_array:
    j = json({{nbt.get_name(), *nbt.get<const std::vector<int> *>()}});
    break;
  case tag_type::tag_long_array:
    j = json({{nbt.get_name(), *nbt.get<const std::vector<long> *>()}});
    break;
  case tag_type::tag_end:
  default:
    j = json({});
    break;
  }
}

} // namespace nbt

#endif
