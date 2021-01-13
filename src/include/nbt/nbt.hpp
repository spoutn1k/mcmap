#pragma once
#ifndef NBT_HPP_
#define NBT_HPP_

#include <map>
#include <nbt/iterators.hpp>
#include <nbt/tag_types.hpp>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#define _NTOHS(ptr) (int16_t(((ptr)[0] << 8) + (ptr)[1]))
#define _NTOHI(ptr)                                                            \
  ((uint32_t((ptr)[0]) << 24) + (uint32_t((ptr)[1]) << 16) +                   \
   (uint32_t((ptr)[2]) << 8) + uint32_t((ptr)[3]))
#define _NTOHL(ptr)                                                            \
  ((uint64_t((ptr)[0]) << 56) + (uint64_t((ptr)[1]) << 48) +                   \
   (uint64_t((ptr)[2]) << 40) + (uint64_t((ptr)[3]) << 32) +                   \
   (uint64_t((ptr)[4]) << 24) + (uint64_t((ptr)[5]) << 16) +                   \
   (uint64_t((ptr)[6]) << 8) + uint64_t((ptr)[7]))

namespace nbt {

class NBT {
private:
  template <typename NBTType> friend class nbt::iter;

public:
  using tag_end_t = int8_t;
  using tag_byte_t = int8_t;
  using tag_short_t = int16_t;
  using tag_int_t = int32_t;
  using tag_long_t = int64_t;
  using tag_float_t = float;
  using tag_double_t = double;
  using tag_byte_array_t = std::vector<int8_t>;
  using tag_string_t = std::string;
  using tag_list_t = std::vector<NBT>;
  using tag_compound_t = std::map<std::string, NBT>;
  using tag_int_array_t = std::vector<int32_t>;
  using tag_long_array_t = std::vector<int64_t>;

  using key_type = std::string;
  using value_type = NBT;
  using reference = value_type &;
  using const_reference = const value_type &;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using allocator_type = std::allocator<NBT>;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer =
      typename std::allocator_traits<allocator_type>::const_pointer;
  using iterator = iter<NBT>;
  using const_iterator = iter<const NBT>;

  NBT(const tag_type type, key_type name_ = "")
      : type(type), content(type), name(name_){};

  NBT(std::nullptr_t = nullptr) : NBT(tag_type::tag_end){};

  template <
      typename Integer,
      typename std::enable_if<std::is_integral<Integer>::value, int>::type = 0>
  NBT(const Integer value, key_type name_ = "") : name(name_) {
    switch (std::alignment_of<Integer>()) {
    case 1:
      type = tag_type::tag_byte;
      content = tag_content(tag_byte_t(value));
      break;

    case 2:
      type = tag_type::tag_short;
      content = tag_content(tag_short_t(value));
      break;

    case 3:
    case 4:
      type = tag_type::tag_int;
      content = tag_content(tag_int_t(value));
      break;

    default:
      type = tag_type::tag_long;
      content = tag_content(tag_long_t(value));
      break;
    }
  }

  template <typename Float,
            typename std::enable_if<std::is_floating_point<Float>::value,
                                    int>::type = 0>
  NBT(const Float value, key_type name_ = "") : name(name_) {
    switch (std::alignment_of<Float>()) {
    case 4:
      type = tag_type::tag_float;
      content = tag_content(tag_float_t(value));
      break;

    default:
      type = tag_type::tag_double;
      content = tag_content(tag_double_t(value));
      break;
    }
  }

  NBT(const tag_string_t str, key_type name_ = "")
      : type(tag_type::tag_string), content(str), name(name_){};

  template <
      typename Integer,
      typename std::enable_if<std::is_integral<Integer>::value, int>::type = 0>
  NBT(const std::vector<Integer> &data, key_type name_ = "") : name(name_) {
    switch (std::alignment_of<Integer>()) {
    case 1:
      type = tag_type::tag_byte_array;
      content = tag_content(tag_byte_array_t(data.begin(), data.end()));
      break;

    case 2:
    case 3:
    case 4:
      type = tag_type::tag_int_array;
      content = tag_content(tag_int_array_t(data.begin(), data.end()));
      break;

    default:
      type = tag_type::tag_long_array;
      content = tag_content(tag_long_array_t(data.begin(), data.end()));
      break;
    }
  }

  NBT(const tag_list_t &data, key_type name_ = "")
      : type(tag_type::tag_list), content(data), name(name_){};

  NBT(const tag_compound_t &data, key_type name_ = "")
      : type(tag_type::tag_compound), content(data), name(name_){};

  NBT(const NBT &other) : type(other.type), name(other.name) {
    switch (type) {
    case tag_type::tag_byte: {
      content = other.content.byte;
      break;
    }
    case tag_type::tag_short: {
      content = other.content.short_n;
      break;
    }
    case tag_type::tag_int: {
      content = other.content.int_n;
      break;
    }
    case tag_type::tag_long: {
      content = other.content.long_n;
      break;
    }
    case tag_type::tag_float: {
      content = other.content.float_n;
      break;
    }
    case tag_type::tag_double: {
      content = other.content.float_n;
      break;
    }
    case tag_type::tag_byte_array: {
      content = *other.content.byte_array;
      break;
    }
    case tag_type::tag_string: {
      content = *other.content.string;
      break;
    }
    case tag_type::tag_list: {
      content = *other.content.list;
      break;
    }
    case tag_type::tag_compound: {
      content = *other.content.compound;
      break;
    }
    case tag_type::tag_int_array: {
      content = *other.content.int_array;
      break;
    }
    case tag_type::tag_long_array: {
      content = *other.content.long_array;
      break;
    }
    default:
      break;
    }
  }

  NBT(NBT &&other)
  noexcept
      : type(other.type), content(std::move(other.content)), name(other.name) {
    other.name = "";
    other.type = tag_type::tag_end;
    other.content = {};
  }

  ~NBT() { content.destroy(type); }

  static NBT parse(uint8_t *data, size_t size) {
    return NBT(data, data + size);
  }

  void assertSize(uint8_t *data, uint8_t *end, size_t length) {
    if (uint64_t(end - data) < length)
      throw(std::domain_error("NBT file ends too soon"));
  }

  void parse(uint8_t *&data, uint8_t *end) {
    switch (type) {
    case tag_type::tag_byte: {
      assertSize(data, end, 1);
      content.byte = int8_t(*data);
      data++;
      break;
    }

    case tag_type::tag_short: {
      assertSize(data, end, 2);
      content.short_n = _NTOHS(data);
      data += 2;
      break;
    }

    case tag_type::tag_int: {
      assertSize(data, end, 4);
      content.int_n = _NTOHI(data);
      data += 4;
      break;
    }

    case tag_type::tag_long: {
      assertSize(data, end, 8);
      content.long_n = _NTOHL(data);
      data += 8;
      break;
    }

    case tag_type::tag_float: {
      assertSize(data, end, 4);
      content.int_n = float(_NTOHI(data));
      data += 4;
      break;
    }

    case tag_type::tag_double: {
      assertSize(data, end, 8);
      content.long_n = double(_NTOHL(data));
      data += 8;
      break;
    }

    case tag_type::tag_byte_array: {
      assertSize(data, end, 4);
      uint32_t len = _NTOHI(data);
      content = tag_content(tag_type::tag_byte_array);

      assertSize(data + 4, end, len);
      for (size_t i = 0; i < len; i++)
        content.byte_array->push_back(*(data + 4 + i));

      data += (len + 4);
      break;
    }

    case tag_type::tag_string: {
      assertSize(data, end, 4);
      uint16_t len = _NTOHS(data);

      assertSize(data + 2, end, len);
      content = tag_string_t((char *)(data + 2), len);

      data += (len + 2);
      break;
    }

    case tag_type::tag_list: {
      assertSize(data, end, 1);
      tag_type chid_type = tag_type(data[0]);

      assertSize(data + 1, end, 4);
      uint32_t len = _NTOHI(data + 1);

      data += 5;
      content = tag_content(tag_type::tag_list);

      for (size_t i = 0; i < len; i++)
        content.list->push_back(NBT(data, end, chid_type));
      break;
    }

    case tag_type::tag_compound: {
      content = tag_content(tag_type::tag_compound);
      while (data[0]) {
        NBT child(data, end);
        content.compound->emplace(std::make_pair(child.name, std::move(child)));
      }
      data++;
      break;
    }

    case tag_type::tag_int_array: {
      assertSize(data, end, 4);
      uint32_t len = _NTOHI(data);
      content = tag_content(tag_type::tag_int_array);

      assertSize(data + 4, end, 4 * len);
      for (size_t i = 0; i < len; i++)
        content.int_array->push_back(_NTOHI(data + 4 * (i + 1)));

      data += (len * 4 + 4);
      break;
    }

    case tag_type::tag_long_array: {
      assertSize(data, end, 4);
      uint32_t len = _NTOHI(data);
      content = tag_content(tag_type::tag_long_array);

      assertSize(data + 4, end, 8 * len);
      for (size_t i = 0; i < len; i++)
        content.long_array->push_back(_NTOHL(data + i * 8 + 4));

      data += (len * 8 + 4);
      break;
    }

    default:
      break;
    }
  }

  void set_name(const std::string &name_) { name = name_; };
  std::string get_name() const { return name; };

  nbt::tag_type get_type() const { return type; };

  constexpr bool is_end() const noexcept { return type == tag_type::tag_end; }
  constexpr bool is_byte() const noexcept { return type == tag_type::tag_byte; }
  constexpr bool is_short() const noexcept {
    return type == tag_type::tag_short;
  }
  constexpr bool is_int() const noexcept { return type == tag_type::tag_int; }
  constexpr bool is_long() const noexcept { return type == tag_type::tag_long; }
  constexpr bool is_float() const noexcept {
    return type == tag_type::tag_float;
  }
  constexpr bool is_double() const noexcept {
    return type == tag_type::tag_double;
  }
  constexpr bool is_byte_array() const noexcept {
    return type == tag_type::tag_byte_array;
  }
  constexpr bool is_string() const noexcept {
    return type == tag_type::tag_string;
  }
  constexpr bool is_list() const noexcept { return type == tag_type::tag_list; }
  constexpr bool is_compound() const noexcept {
    return type == tag_type::tag_compound;
  }
  constexpr bool is_int_array() const noexcept {
    return type == tag_type::tag_int_array;
  }
  constexpr bool is_long_array() const noexcept {
    return type == tag_type::tag_long_array;
  }

  reference at(size_type index) {
    if (is_list()) {
      return content.list->at(index);
    } else {
      throw(std::domain_error("Cannot use at() with " +
                              std::string(type_name())));
    }
  }

  const_reference at(size_type index) const {
    if (is_list()) {
      return content.list->at(index);
    } else {
      throw(std::domain_error("Cannot use at() with " +
                              std::string(type_name())));
    }
  }

  reference at(const std::string &key) {
    if (is_compound()) {
      // try {
      return content.compound->at(key);
      //} catch (std::out_of_range &) {
    }
    throw(std::domain_error("Invalid type"));
  }

  const_reference at(const std::string &key) const {
    if (is_compound()) {
      // try {
      return content.compound->at(key);
      //} catch (std::out_of_range &) {
    }
    throw(std::domain_error("Invalid type"));
  }

  reference operator[](size_type index) {
    if (is_list()) {
      return content.list->operator[](index);
    }
    throw(std::domain_error(
        "Cannot use operator[] with a numeric argument on tag of type " +
        std::string(type_name())));
  }

  const_reference operator[](size_type index) const {
    if (is_list()) {
      return content.list->operator[](index);
    }
    throw(std::domain_error(
        "Cannot use operator[] with a numeric argument on tag of type " +
        std::string(type_name())));
  }

  reference operator[](const std::string &key) {
    if (is_compound()) {
      return content.compound->operator[](key);
    }
    throw(std::domain_error(
        "Cannot use operator[] with a string argument on tag of type " +
        std::string(type_name())));
  }

  const_reference operator[](const std::string &key) const {
    if (is_compound()) {
      if (content.compound->find(key) != content.compound->end()) {
        return content.compound->find(key)->second;
      } else
        throw(std::out_of_range("Key " + key + " not found"));
    }
    throw(std::domain_error(
        "Cannot use operator[] with a string argument on tag of type " +
        std::string(type_name())));
  }

  std::pair<tag_compound_t::iterator, bool>
  insert(const tag_compound_t::value_type &value) {
    if (is_compound())
      return content.compound->insert(value);
    throw(std::domain_error("Cannot use insert on tag of type " +
                            std::string(type_name())));
  }

  std::pair<tag_compound_t::iterator, bool>
  insert(tag_compound_t::value_type &&value) {
    if (is_compound())
      return content.compound->insert(
          std::forward<tag_compound_t::value_type>(value));
    throw(std::domain_error("Cannot use insert on tag of type " +
                            std::string(type_name())));
  }

  size_type erase(const key_type &key) {
    if (is_compound())
      return content.compound->erase(key);
    throw(std::domain_error(
        "Cannot use erase with a string argument on tag of type " +
        std::string(type_name())));
  }

  template <typename T> reference operator[](T *key) {
    if (is_end()) {
      type = tag_type::tag_compound;
      content = tag_type::tag_compound;
    }

    if (is_compound()) {
      return content.compound->operator[](key);
    }

    throw(std::domain_error("Cannot use operator[] with type" +
                            std::string(type_name())));
  }

  NBT &operator=(const NBT &other) noexcept {
    type = other.type;
    name = other.name;
    content = other.content;

    return *this;
  }

  NBT &operator=(NBT &&other) noexcept {
    std::swap(type, other.type);
    std::swap(name, other.name);
    std::swap(content, other.content);

    return *this;
  }

  const char *type_name() const noexcept {
    switch (type) {
    case tag_type::tag_byte:
      return "byte";
    case tag_type::tag_short:
      return "short";
    case tag_type::tag_int:
      return "int";
    case tag_type::tag_long:
      return "long";
    case tag_type::tag_float:
      return "float";
    case tag_type::tag_double:
      return "double";
    case tag_type::tag_byte_array:
      return "byte_array";
    case tag_type::tag_string:
      return "string";
    case tag_type::tag_list:
      return "list";
    case tag_type::tag_compound:
      return "compound";
    case tag_type::tag_int_array:
      return "int_array";
    case tag_type::tag_long_array:
      return "long_array";
    default:
      return "end";
    }
  }

  reference front() { return *begin(); }
  const_reference front() const { return *cbegin(); }
  reference back() {
    auto tmp = end();
    --tmp;
    return *tmp;
  }
  const_reference back() const {
    auto tmp = cend();
    --tmp;
    return *tmp;
  }

  iterator begin() noexcept {
    iterator result(this);
    result.set_begin();
    return result;
  }

  const_iterator begin() const noexcept { return cbegin(); }

  const_iterator cbegin() const noexcept {
    const_iterator result(this);
    result.set_begin();
    return result;
  }

  iterator end() noexcept {
    iterator result(this);
    result.set_end();
    return result;
  }

  const_iterator end() const noexcept { return cend(); }

  const_iterator cend() const noexcept {
    const_iterator result(this);
    result.set_end();
    return result;
  }

  iterator find(std::string &&key) {
    auto result = end();

    if (is_compound()) {
      result.it.compound_iterator =
          content.compound->find(std::forward<std::string>(key));
    }

    return result;
  }

  const_iterator find(std::string &&key) const {
    auto result = cend();

    if (is_compound()) {
      result.it.compound_iterator =
          content.compound->find(std::forward<std::string>(key));
    }

    return result;
  }

  size_type count(std::string &&key) const {
    return is_compound()
               ? content.compound->count(std::forward<std::string>(key))
               : 0;
  }

  bool contains(std::string &&key) const {
    return is_compound() and
           content.compound->find(std::forward<std::string>(key)) !=
               content.compound->end();
  }

  bool empty() const noexcept {
    switch (type) {
    case tag_type::tag_end:
      return true;
    case tag_type::tag_byte_array:
      return content.byte_array->empty();
    case tag_type::tag_list:
      return content.list->empty();
    case tag_type::tag_int_array:
      return content.int_array->empty();
    case tag_type::tag_long_array:
      return content.long_array->empty();
    default:
      return false;
    }
  }

  size_type size() const noexcept {
    switch (type) {
    case tag_type::tag_end: {
      return 0;
    }

    case tag_type::tag_byte_array: {
      return content.byte_array->size();
    }

    case tag_type::tag_list: {
      return content.list->size();
    }

    case tag_type::tag_compound: {
      return content.compound->size();
    }

    case tag_type::tag_int_array: {
      return content.int_array->size();
    }

    case tag_type::tag_long_array: {
      return content.long_array->size();
    }

    default: {
      // all other types have size 1
      return 1;
    }
    }
  }

private:
  union tag_content {
    tag_end_t end;
    tag_byte_t byte;
    tag_short_t short_n;
    tag_int_t int_n;
    tag_long_t long_n;
    tag_float_t float_n;
    tag_double_t double_n;
    tag_byte_array_t *byte_array;
    tag_string_t *string;
    tag_list_t *list;
    tag_compound_t *compound;
    tag_int_array_t *int_array;
    tag_long_array_t *long_array;

    tag_content() = default;
    tag_content(tag_byte_t v) : byte(v){};
    tag_content(tag_short_t v) : short_n(v){};
    tag_content(tag_int_t v) : int_n(v){};
    tag_content(tag_long_t v) : long_n(v){};
    tag_content(tag_float_t v) : float_n(v){};
    tag_content(tag_double_t v) : double_n(v){};
    tag_content(tag_type t) {
      switch (t) {
      case tag_type::tag_end: {
        end = tag_end_t(0);
        break;
      }
      case tag_type::tag_byte: {
        byte = tag_byte_t(0);
        break;
      }
      case tag_type::tag_short: {
        short_n = tag_short_t(0);
        break;
      }
      case tag_type::tag_int: {
        int_n = tag_int_t(0);
        break;
      }
      case tag_type::tag_long: {
        long_n = tag_long_t(0);
        break;
      }
      case tag_type::tag_float: {
        float_n = tag_float_t(0);
        break;
      }
      case tag_type::tag_double: {
        double_n = tag_double_t(0);
        break;
      }
      case tag_type::tag_byte_array: {
        byte_array = new tag_byte_array_t{};
        break;
      }
      case tag_type::tag_string: {
        string = new tag_string_t("");
        break;
      }
      case tag_type::tag_list: {
        list = new tag_list_t();
        break;
      }
      case tag_type::tag_compound: {
        compound = new tag_compound_t();
        break;
      }
      case tag_type::tag_int_array: {
        int_array = new tag_int_array_t();
        break;
      }
      case tag_type::tag_long_array: {
        long_array = new tag_long_array_t();
        break;
      }
      }
    }

    tag_content(const tag_byte_array_t &value) {
      byte_array = new tag_byte_array_t(value);
    }

    tag_content(tag_byte_array_t &&value) {
      byte_array = new tag_byte_array_t(std::move(value));
    }

    tag_content(const tag_int_array_t &value) {
      int_array = new tag_int_array_t(value);
    }

    tag_content(tag_int_array_t &&value) {
      int_array = new tag_int_array_t(std::move(value));
    }

    tag_content(const tag_long_array_t &value) {
      long_array = new tag_long_array_t(value);
    }

    tag_content(tag_long_array_t &&value) {
      long_array = new tag_long_array_t(std::move(value));
    }

    tag_content(const tag_string_t &value) : string(new tag_string_t(value)) {}

    tag_content(tag_string_t &&value)
        : string(new tag_string_t(std::move(value))) {}

    tag_content(const tag_list_t &value) : list(new tag_list_t(value)) {}

    tag_content(tag_list_t &&value) : list(new tag_list_t(std::move(value))) {}

    tag_content(const tag_compound_t &value)
        : compound(new tag_compound_t(value)) {}

    tag_content(tag_compound_t &&value)
        : compound(new tag_compound_t(std::move(value))) {}

    void destroy(tag_type t) {
      std::vector<NBT> stack;

      if (t == tag_type::tag_compound) {
        stack.reserve(compound->size());
        for (auto &&it : *compound)
          stack.push_back(std::move(it.second));
      }

      if (t == tag_type::tag_list) {
        stack.reserve(list->size());
        for (auto &&it : *list)
          stack.push_back(std::move(it));
      }

      while (!stack.empty()) {
        NBT current(std::move(stack.back()));
        stack.pop_back();

        if (current.type == tag_type::tag_compound) {
          for (auto &&it : *current.content.compound)
            stack.push_back(std::move(it.second));
          current.content.compound->clear();
        }
      }

      switch (t) {
      case tag_type::tag_byte_array: {
        delete byte_array;
        break;
      }
      case tag_type::tag_string: {
        delete string;
        break;
      }
      case tag_type::tag_list: {
        delete list;
        break;
      }
      case tag_type::tag_compound: {
        delete compound;
        break;
      }
      case tag_type::tag_int_array: {
        delete int_array;
        break;
      }
      case tag_type::tag_long_array: {
        delete long_array;
        break;
      }
      default:
        break;
      }
    }
  };

  NBT(uint8_t *&data, uint8_t *end) : NBT() {
    assertSize(data, end, 1);
    type = tag_type(data[0]);
    if (type == tag_type::tag_end)
      return;

    assertSize(data + 1, end, 2);
    uint16_t len = _NTOHS(data + 1);

    assertSize(data + 3, end, len);
    name = std::string((char *)(data + 3), len);
    data += len + 3;

    this->parse(data, end);
  }

  NBT(uint8_t *&data, uint8_t *end, tag_type t) {
    type = t;
    this->parse(data, end);
  }

  tag_type type = tag_type::tag_end;
  tag_content content = {};
  std::string name = "";

  //            _
  //  __ _  ___| |_
  // / _` |/ _ \ __|
  //| (_| |  __/ |_
  // \__, |\___|\__|
  // |___/

  // Byte pointers
  tag_byte_t *get_impl_ptr(tag_byte_t *) noexcept {
    return is_byte() ? &content.byte : nullptr;
  }

  constexpr const tag_byte_t *get_impl_ptr(const tag_byte_t *) const noexcept {
    return is_byte() ? &content.byte : nullptr;
  }

  // Short pointers
  tag_short_t *get_impl_ptr(tag_short_t *) noexcept {
    return is_short() ? &content.short_n : nullptr;
  }

  constexpr const tag_short_t *
  get_impl_ptr(const tag_short_t *) const noexcept {
    return is_short() ? &content.short_n : nullptr;
  }

  // Int pointers
  tag_int_t *get_impl_ptr(tag_int_t *) noexcept {
    return is_int() ? &content.int_n : nullptr;
  }

  constexpr const tag_int_t *get_impl_ptr(const tag_int_t *) const noexcept {
    return is_int() ? &content.int_n : nullptr;
  }

  // Long pointers
  tag_long_t *get_impl_ptr(tag_long_t *) noexcept {
    return is_long() ? &content.long_n : nullptr;
  }

  constexpr const tag_long_t *get_impl_ptr(const tag_long_t *) const noexcept {
    return is_long() ? &content.long_n : nullptr;
  }

  // Float pointers
  tag_float_t *get_impl_ptr(tag_float_t *) noexcept {
    return is_float() ? &content.float_n : nullptr;
  }

  constexpr const tag_float_t *
  get_impl_ptr(const tag_float_t *) const noexcept {
    return is_float() ? &content.float_n : nullptr;
  }

  // Double pointers
  tag_double_t *get_impl_ptr(tag_double_t *) noexcept {
    return is_double() ? &content.double_n : nullptr;
  }

  constexpr const tag_double_t *
  get_impl_ptr(const tag_double_t *) const noexcept {
    return is_double() ? &content.double_n : nullptr;
  }

  // Byte Array pointers
  tag_byte_array_t *get_impl_ptr(tag_byte_array_t *) noexcept {
    return is_byte_array() ? content.byte_array : nullptr;
  }

  constexpr const tag_byte_array_t *
  get_impl_ptr(const tag_byte_array_t *) const noexcept {
    return is_byte_array() ? content.byte_array : nullptr;
  }

  // String pointers
  tag_string_t *get_impl_ptr(tag_string_t *) noexcept {
    return is_string() ? content.string : nullptr;
  }

  constexpr const tag_string_t *
  get_impl_ptr(const tag_string_t *) const noexcept {
    return is_string() ? content.string : nullptr;
  }

  // List pointers
  tag_list_t *get_impl_ptr(tag_list_t *) noexcept {
    return is_list() ? content.list : nullptr;
  }

  constexpr const tag_list_t *get_impl_ptr(const tag_list_t *) const noexcept {
    return is_list() ? content.list : nullptr;
  }

  // Compound pointers
  tag_compound_t *get_impl_ptr(tag_compound_t *) noexcept {
    return is_compound() ? content.compound : nullptr;
  }

  constexpr const tag_compound_t *
  get_impl_ptr(const tag_compound_t *) const noexcept {
    return is_compound() ? content.compound : nullptr;
  }

  // Int Array pointers
  tag_int_array_t *get_impl_ptr(tag_int_array_t *) noexcept {
    return is_int_array() ? content.int_array : nullptr;
  }

  constexpr const tag_int_array_t *
  get_impl_ptr(const tag_int_array_t *) const noexcept {
    return is_int_array() ? content.int_array : nullptr;
  }

  // Long Array pointers
  tag_long_array_t *get_impl_ptr(tag_long_array_t *) noexcept {
    return is_long_array() ? content.long_array : nullptr;
  }

  constexpr const tag_long_array_t *
  get_impl_ptr(const tag_long_array_t *) const noexcept {
    return is_long_array() ? content.long_array : nullptr;
  }

public:
  template <typename PointerType,
            typename std::enable_if<std::is_pointer<PointerType>::value,
                                    int>::type = 0>
  PointerType get_ptr() noexcept {
    return get_impl_ptr(static_cast<PointerType>(nullptr));
  }

  template <
      typename PointerType,
      typename std::enable_if<std::is_pointer<PointerType>::value and
                                  std::is_const<typename std::remove_pointer<
                                      PointerType>::type>::value,
                              int>::type = 0>
  constexpr PointerType get_ptr() const noexcept {
    return get_impl_ptr(static_cast<PointerType>(nullptr));
  }

  template <typename PointerType,
            typename std::enable_if<std::is_pointer<PointerType>::value,
                                    int>::type = 0>
  auto get() noexcept -> decltype(get_ptr<PointerType>()) {
    return get_ptr<PointerType>();
  }

  template <typename PointerType,
            typename std::enable_if<std::is_pointer<PointerType>::value,
                                    int>::type = 0>
  constexpr auto get() const noexcept -> decltype(get_ptr<PointerType>()) {
    return get_ptr<PointerType>();
  }

  template <typename ArithmeticType,
            typename std::enable_if<std::is_arithmetic<ArithmeticType>::value,
                                    int>::type = 0>
  ArithmeticType get() const {
    switch (get_type()) {
    case tag_type::tag_byte:
      return static_cast<ArithmeticType>(*get_ptr<const tag_byte_t *>());

    case tag_type::tag_short:
      return static_cast<ArithmeticType>(*get_ptr<const tag_short_t *>());

    case tag_type::tag_int:
      return static_cast<ArithmeticType>(*get_ptr<const tag_int_t *>());

    case tag_type::tag_long:
      return static_cast<ArithmeticType>(*get_ptr<const tag_long_t *>());

    case tag_type::tag_float:
      return static_cast<ArithmeticType>(*get_ptr<const tag_float_t *>());

    case tag_type::tag_double:
      return static_cast<ArithmeticType>(*get_ptr<const tag_double_t *>());

    default:
      throw(std::invalid_argument("Not available for " +
                                  std::string(type_name())));
    }
  }

  template <typename StringType,
            typename std::enable_if<
                std::is_same<StringType, tag_string_t>::value, int>::type = 0>
  StringType get() const {
    if (get_type() == tag_type::tag_string)
      return static_cast<StringType>(*get_ptr<const tag_string_t *>());

    throw(
        std::invalid_argument("Not available for " + std::string(type_name())));
  }
};

} // namespace nbt

#endif
