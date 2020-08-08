#pragma once
#ifndef NBT_TO_JSON_HPP_
#define NBT_TO_JSON_HPP_

#include <json.hpp>
#include <nbt/nbt.hpp>
#include <nbt/tag_types.hpp>

using nlohmann::json;

namespace nbt {

void to_json(json &j, const NBT &nbt) {

  switch (nbt.get_type()) {
  case tag_type::tag_byte:
    j = json({nbt.get_name(), nbt.get<int8_t>()});
    break;
  case tag_type::tag_short:
    j = json({nbt.get_name(), nbt.get<short>()});
    break;
  case tag_type::tag_int:
    j = json({nbt.get_name(), nbt.get<int>()});
    break;
  case tag_type::tag_long:
    j = json({nbt.get_name(), nbt.get<long>()});
    break;
  case tag_type::tag_float:
    j = json({nbt.get_name(), nbt.get<float>()});
    break;
  case tag_type::tag_double:
    j = json({nbt.get_name(), nbt.get<double>()});
    break;
  case tag_type::tag_byte_array:
    j = json(*nbt.get<const std::vector<int8_t> *>());
    break;
  case tag_type::tag_string:
    j = json({nbt.get_name(), nbt.get<std::string>()});
    break;
  case tag_type::tag_list: {
    std::vector<json> data;
    const std::vector<NBT> *subs = nbt.get<const std::vector<NBT> *>();

    for (auto el : *subs)
      data.emplace_back(json(el));

    j = json(data);
    break;
  }
  case tag_type::tag_compound: {
    std::map<std::string, json> data;

    const std::map<std::string, NBT> *subs =
        nbt.get<const std::map<std::string, NBT> *>();

    for (auto el : *subs)
      data[el.first] = json(el.second);

    j = json(data);
    break;
  }
  case tag_type::tag_int_array:
    j = json(*nbt.get<const std::vector<int> *>());
    break;
  case tag_type::tag_long_array:
    j = json(*nbt.get<const std::vector<long> *>());
    break;
  case tag_type::tag_end:
  default:
    j = json({});
    break;
  }
};

} // namespace nbt

#endif