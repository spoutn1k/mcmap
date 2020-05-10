#pragma once

#include "./tag_types.hpp"
#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace nbt {

class primitive_iterator_t {
private:
  using difference_type = std::ptrdiff_t;
  static constexpr difference_type begin_value = 0;
  static constexpr difference_type end_value = begin_value + 1;
  difference_type m_it = (std::numeric_limits<std::ptrdiff_t>::min)();

public:
  constexpr difference_type get_value() const noexcept { return m_it; }
  void set_begin() noexcept { m_it = begin_value; }
  void set_end() noexcept { m_it = end_value; }
  constexpr bool is_begin() const noexcept { return m_it == begin_value; }
  constexpr bool is_end() const noexcept { return m_it == end_value; }
  friend constexpr bool operator==(primitive_iterator_t lhs,
                                   primitive_iterator_t rhs) noexcept {
    return lhs.m_it == rhs.m_it;
  }

  friend constexpr bool operator<(primitive_iterator_t lhs,
                                  primitive_iterator_t rhs) noexcept {
    return lhs.m_it < rhs.m_it;
  }

  primitive_iterator_t operator+(difference_type n) noexcept {
    auto result = *this;
    result += n;
    return result;
  }

  friend constexpr difference_type
  operator-(primitive_iterator_t lhs, primitive_iterator_t rhs) noexcept {
    return lhs.m_it - rhs.m_it;
  }

  primitive_iterator_t &operator++() noexcept {
    ++m_it;
    return *this;
  }

  primitive_iterator_t const operator++(int) noexcept {
    auto result = *this;
    ++m_it;
    return result;
  }

  primitive_iterator_t &operator--() noexcept {
    --m_it;
    return *this;
  }

  primitive_iterator_t const operator--(int) noexcept {
    auto result = *this;
    --m_it;
    return result;
  }

  primitive_iterator_t &operator+=(difference_type n) noexcept {
    m_it += n;
    return *this;
  }

  primitive_iterator_t &operator-=(difference_type n) noexcept {
    m_it -= n;
    return *this;
  }
};

template <typename NBTType> struct internal_iterator {
  typename NBTType::tag_compound_t::iterator compound_iterator{};
  typename NBTType::tag_list_t::iterator list_iterator{};
  typename NBTType::tag_byte_array_t::iterator byte_iterator{};
  typename NBTType::tag_int_array_t::iterator int_iterator{};
  typename NBTType::tag_long_array_t::iterator long_iterator{};
  primitive_iterator_t primitive_iterator{};
};

template <typename...> struct WhichType;

template <typename NBTType> class iter {
  friend iter<typename std::conditional<
      std::is_const<NBTType>::value, typename std::remove_const<NBTType>::type,
      const NBTType>::type>;

  friend NBTType;

  using compound_t = typename NBTType::tag_compound_t;
  using list_t = typename NBTType::tag_list_t;
  using byte_array_t = typename NBTType::tag_byte_array_t;
  using int_array_t = typename NBTType::tag_int_array_t;
  using long_array_t = typename NBTType::tag_long_array_t;

public:
  using value_type = typename NBTType::value_type;

  using difference_type = typename NBTType::difference_type;

  using pointer = typename std::conditional<std::is_const<NBTType>::value,
                                            typename NBTType::const_pointer,
                                            typename NBTType::pointer>::type;

  using reference =
      typename std::conditional<std::is_const<NBTType>::value,
                                typename NBTType::const_reference,
                                typename NBTType::reference>::type;

  iter() = default;

  explicit iter(pointer object) noexcept : content(object) {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      it.compound_iterator = typename compound_t::iterator();
      break;
    }

    case tag_type::tag_list: {
      it.list_iterator = typename list_t::iterator();
      break;
    }

    case tag_type::tag_byte_array: {
      it.byte_iterator = typename byte_array_t::iterator();
      break;
    }

    case tag_type::tag_int_array: {
      it.int_iterator = typename int_array_t::iterator();
      break;
    }

    case tag_type::tag_long_array: {
      it.long_iterator = typename long_array_t::iterator();
      break;
    }

    default: {
      it.primitive_iterator = primitive_iterator_t();
      break;
    }
    }
  }

  iter(const iter<const NBTType> &other) noexcept
      : content(other.content), it(other.it) {}

  iter &operator=(const iter<const NBTType> &other) noexcept {
    content = other.content;
    it = other.it;
    return *this;
  }

  iter(const iter<typename std::remove_const<NBTType>::type> &other) noexcept
      : content(other.content), it(other.it) {}

  iter &operator=(
      const iter<typename std::remove_const<NBTType>::type> &other) noexcept {
    content = other.content;
    it = other.it;
    return *this;
  }

private:
  void set_begin() noexcept {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      it.compound_iterator = content->content.compound->begin();
      break;
    }

    case tag_type::tag_list: {
      it.list_iterator = content->content.list->begin();
      break;
    }

    case tag_type::tag_byte_array: {
      it.byte_iterator = content->content.byte_array->begin();
      break;
    }

    case tag_type::tag_int_array: {
      it.int_iterator = content->content.int_array->begin();
      break;
    }

    case tag_type::tag_long_array: {
      it.long_iterator = content->content.long_array->begin();
      break;
    }

    case tag_type::tag_end: {
      // set to end so begin()==end() is true: end is empty
      it.primitive_iterator.set_end();
      break;
    }

    default: {
      it.primitive_iterator.set_begin();
      break;
    }
    }
  }

  void set_end() noexcept {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      it.compound_iterator = content->content.compound->end();
      break;
    }

    case tag_type::tag_list: {
      it.list_iterator = content->content.list->end();
      break;
    }

    case tag_type::tag_byte_array: {
      it.byte_iterator = content->content.byte_array->end();
      break;
    }

    case tag_type::tag_int_array: {
      it.int_iterator = content->content.int_array->end();
      break;
    }

    case tag_type::tag_long_array: {
      it.long_iterator = content->content.long_array->end();
      break;
    }

    default: {
      it.primitive_iterator.set_end();
      break;
    }
    }
  }

public:
  reference operator*() const {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      assert(it.compound_iterator != content->content.compound->end());
      return it.compound_iterator->second;
    }
    case tag_type::tag_list: {
      assert(it.list_iterator != content->content.list->end());
      return *it.list_iterator;
    }

      /*
          case tag_type::tag_byte_array: {
            assert(it.byte_iterator != content->content.byte_array->end());
            return *it.byte_iterator;
          }

          case tag_type::tag_int_array: {
            assert(it.int_iterator != content->content.int_array->end());
            return *it.int_iterator;
          }

          case tag_type::tag_long_array: {
            assert(it.long_iterator != content->content.long_array->end());
            return *it.long_iterator;
          }
      */
    case tag_type::tag_end:
      throw(std::range_error("No values in end tag"));

    default: {
      if (it.primitive_iterator.is_begin()) {
        return *content;
      }

      throw(std::range_error("Cannot get value"));
    }
    }
  }

  pointer operator->() const {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      assert(it.compound_iterator != content->content.compound->end());
      return &(it.compound_iterator->second);
    }

    case tag_type::tag_list: {
      assert(it.list_iterator != content->content.list->end());
      return &*it.list_iterator;
    }

    case tag_type::tag_end:
      throw(std::range_error("No values in end tag"));

    default: {
      if (it.primitive_iterator.is_begin()) {
        return content;
      }

      throw(std::range_error("Cannot get value"));
    }
    }
  }

  iter const operator++(int) {
    auto result = *this;
    ++(*this);
    return result;
  }

  iter &operator++() {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      std::advance(it.compound_iterator, 1);
      break;
    }

    case tag_type::tag_list: {
      std::advance(it.list_iterator, 1);
      break;
    }

    case tag_type::tag_byte_array: {
      std::advance(it.byte_iterator, 1);
      break;
    }

    case tag_type::tag_int_array: {
      std::advance(it.int_iterator, 1);
      break;
    }

    case tag_type::tag_long_array: {
      std::advance(it.long_iterator, 1);
      break;
    }

    default: {
      ++it.primitive_iterator;
      break;
    }
    }

    return *this;
  }

  iter const operator--(int) {
    auto result = *this;
    --(*this);
    return result;
  }

  iter &operator--() {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound: {
      std::advance(it.compound_iterator, -1);
      break;
    }

    case tag_type::tag_list: {
      std::advance(it.list_iterator, -1);
      break;
    }

    case tag_type::tag_byte_array: {
      std::advance(it.byte_iterator, -1);
      break;
    }

    case tag_type::tag_int_array: {
      std::advance(it.int_iterator, -1);
      break;
    }

    case tag_type::tag_long_array: {
      std::advance(it.long_iterator, -1);
      break;
    }

    default: {
      ++it.primitive_iterator;
      break;
    }
    }

    return *this;
  }

  bool operator==(const iter &other) const {
    if (content != other.content) {
      throw(std::domain_error("Bad comparison between iterators of type " +
                              std::string(content->type_name()) + " and type " +
                              std::string(content->type_name())));
    }

    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound:
      return (it.compound_iterator == other.it.compound_iterator);

    case tag_type::tag_list:
      return (it.list_iterator == other.it.list_iterator);

    case tag_type::tag_byte_array:
      return (it.byte_iterator == other.it.byte_iterator);

    case tag_type::tag_int_array:
      return (it.int_iterator == other.it.int_iterator);

    case tag_type::tag_long_array:
      return (it.long_iterator == other.it.long_iterator);

    default:
      return (it.primitive_iterator == other.it.primitive_iterator);
    }
  }

  bool operator!=(const iter &other) const { return not operator==(other); }

  bool operator<(const iter &other) const {
    if (content != other.content) {
      throw(std::domain_error("Bad comparison between iterators of type " +
                              std::string(content->type_name()) + " and type " +
                              std::string(content->type_name())));
    }

    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound:
      throw(std::domain_error("Cannot compare compound types"));
    case tag_type::tag_list:
      return (it.list_iterator < other.it.list_iterator);
    case tag_type::tag_byte_array:
      return (it.byte_iterator < other.it.byte_iterator);
    case tag_type::tag_int_array:
      return (it.int_iterator < other.it.int_iterator);
    case tag_type::tag_long_array:
      return (it.long_iterator < other.it.long_iterator);
    default:
      return (it.primitive_iterator < other.it.primitive_iterator);
    }
  }

  bool operator<=(const iter &other) const {
    return not other.operator<(*this);
  }

  bool operator>(const iter &other) const { return not operator<=(other); }
  bool operator>=(const iter &other) const { return not operator<(other); }

  iter &operator+=(difference_type i) {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound:
      throw(std::domain_error("cannot use offsets with compound iterators"));
    case tag_type::tag_list:
      std::advance(it.list_iterator, i);
      break;
    case tag_type::tag_byte_array:
      std::advance(it.byte_iterator, i);
      break;
    case tag_type::tag_int_array:
      std::advance(it.int_iterator, i);
      break;
    case tag_type::tag_long_array:
      std::advance(it.long_iterator, i);
      break;

    default: {
      it.primitive_iterator += i;
      break;
    }
    }

    return *this;
  }

  iter &operator-=(difference_type i) { return operator+=(-i); }

  iter operator+(difference_type i) const {
    auto result = *this;
    result += i;
    return result;
  }

  friend iter operator+(difference_type i, const iter &arg_it) {
    auto result = arg_it;
    result += i;
    return result;
  }

  iter operator-(difference_type i) const {
    auto result = *this;
    result -= i;
    return result;
  }

  difference_type operator-(const iter &other) const {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound:
      throw(std::domain_error("cannot use offsets with compound iterators"));
    case tag_type::tag_list:
      return (it.list_iterator - other.it.list_iterator);
    case tag_type::tag_byte_array:
      return (it.byte_iterator - other.it.byte_iterator);
    case tag_type::tag_int_array:
      return (it.int_iterator - other.it.int_iterator);
    case tag_type::tag_long_array:
      return (it.long_iterator - other.it.long_iterator);

    default:
      return it.primitive_iterator - other.it.primitive_iterator;
    }
  }

  reference operator[](difference_type n) const {
    assert(content != nullptr);

    switch (content->type) {
    case tag_type::tag_compound:
      throw(std::domain_error("cannot use operator[] with compound iterators"));

    case tag_type::tag_list:
      return *std::next(it.list_iterator, n);
    case tag_type::tag_byte_array:
      return *std::next(it.byte_iterator, n);
    case tag_type::tag_int_array:
      return *std::next(it.int_iterator, n);
    case tag_type::tag_long_array:
      return *std::next(it.long_iterator, n);

    case tag_type::tag_end:
      throw(std::domain_error("cannot get value from end tag"));

    default: {
      if (it.primitive_iterator.get_value() == -n) {
        return *content;
      }
      throw(std::domain_error("cannot get value from end tag"));
    }
    }
  }

  const std::string &key() const {
    assert(content != nullptr);
    if (content->is_compound()) {
      return it.compound_iterator->first;
    }
    throw(std::domain_error(
        "cannot use operator key with non-compound iterators"));
  }

  reference value() const { return operator*(); }

private:
  pointer content = nullptr;
  internal_iterator<typename std::remove_const<NBTType>::type> it{};
};

} // namespace nbt
