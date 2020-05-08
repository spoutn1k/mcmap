#pragma once
#include "./iterators.hpp"
#include "./tag_types.hpp"
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define NTOHUS(ptr) (uint16_t(((ptr)[0] << 8) + (ptr)[1]))
#define NTOHS(ptr) (int16_t(((ptr)[0] << 8) + (ptr)[1]))
#define NTOHI(ptr)                                                             \
  ((int32_t(ptr[0]) << 24) + (int32_t(ptr[1]) << 16) +                         \
   (int32_t(ptr[2]) << 8) + int32_t(ptr[2]))
#define NTOHL(ptr)                                                             \
  ((int64_t(ptr[0]) << 56) + (int64_t(ptr[1]) << 48) +                         \
   (int64_t(ptr[2]) << 40) + (int64_t(ptr[3]) << 32) +                         \
   (int64_t(ptr[4]) << 24) + (int64_t(ptr[5]) << 16) +                         \
   (int64_t(ptr[6]) << 8) + int64_t(ptr[7]))

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
  using tag_compound_t = std::unordered_map<std::string, NBT>;

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

  NBT(const tag_type type) : type(type), content(type){};
  NBT(std::nullptr_t = nullptr) : NBT(tag_type::tag_end){};

  NBT(const NBT &other);
  NBT(NBT &&other) noexcept;

  NBT(uint8_t *&data, uint8_t *end);
  ~NBT();

  std::string get_name() { return name; };
  nbt::tag_type get_type() { return type; };

  constexpr bool is_end() const noexcept { return type == tag_type::tag_end; }
  constexpr bool is_byte() const noexcept { return type == tag_type::tag_byte; }
  constexpr bool is_compound() const noexcept {
    return type == tag_type::tag_compound;
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

  NBT &operator=(NBT other) noexcept {
    using std::swap;
    swap(type, other.type);
    swap(content, other.content);

    return *this;
  }

  const char *type_name() const noexcept {
    switch (type) {
    case tag_type::tag_byte:
      return "byte";
    case tag_type::tag_compound:
      return "compound";
    default:
      return "end";
    }
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

private:
  union tag_content {
    tag_end_t end;
    tag_byte_t byte;
    tag_short_t short_n;
    tag_int_t int_n;
    tag_long_t long_n;
    tag_compound_t *compound;

    tag_content() = default;
    tag_content(tag_byte_t v) : byte(v){};
    tag_content(tag_short_t v) : short_n(v){};
    tag_content(tag_int_t v) : int_n(v){};
    tag_content(tag_long_t v) : long_n(v){};
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
      case tag_type::tag_compound: {
        compound = new tag_compound_t();
        break;
      }
      }
    }

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
      case tag_type::tag_compound: {
        delete compound;
        break;
      }
      default:
        break;
      }
    }
  };

  tag_type type = tag_type::tag_end;
  tag_content content = {};
  std::string name = "";
};

NBT::NBT(const NBT &other) : type(other.type), name(other.name) {
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
  case tag_type::tag_compound: {
    content = *other.content.compound;
    break;
  }
  default:
    break;
  }
}

NBT::NBT(NBT &&other) noexcept
    : type(other.type), content(std::move(other.content)), name(other.name) {
  other.name = "";
  other.type = tag_type::tag_end;
  other.content = {};
}

NBT::NBT(uint8_t *&data, uint8_t *end) {
  type = tag_type(data[0]);
  if (type == tag_type::tag_end)
    return;

  name = std::string((char *)(data + 3), *(int16_t *)(data + 1));
  data += NTOHUS(data + 1) + 3;

  switch (type) {
  case tag_type::tag_byte: {
    content = tag_content(int8_t(*data));
    data++;
    break;
  }
  case tag_type::tag_short: {
    content.short_n = NTOHS(data);
    data += 2;
    break;
  }
  case tag_type::tag_int: {
    content.int_n = NTOHI(data);
    data += 4;
    break;
  }
  case tag_type::tag_long: {
    content.long_n = NTOHL(data);
    data += 8;
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
  default:
    break;
  }
}

NBT::~NBT() { content.destroy(type); }

} // namespace nbt
