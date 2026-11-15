#pragma once

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nbt {

// A map keeping its elements in insertion order, with the subset of the `std::map` interface used on compounds. NBT does not order the keys of a compound, but keeping them as they were read makes the data written back identical to its source. Lookups are linear, which suits compounds as they hold a handful of keys.
//
// As elements are stored in a vector, inserting can invalidate references and iterators to the other elements of the map.
template <typename Key, typename T> class ordered_map {
public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key, T>;
  using container_type = std::vector<value_type>;
  using size_type = typename container_type::size_type;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  ordered_map() = default;

  // Like `std::map`, the first occurrence of a duplicated key wins
  ordered_map(std::initializer_list<value_type> init) {
    for (const auto &element : init)
      insert(element);
  }

  iterator begin() noexcept { return elements.begin(); }
  const_iterator begin() const noexcept { return elements.begin(); }
  const_iterator cbegin() const noexcept { return elements.cbegin(); }
  iterator end() noexcept { return elements.end(); }
  const_iterator end() const noexcept { return elements.end(); }
  const_iterator cend() const noexcept { return elements.cend(); }

  size_type size() const noexcept { return elements.size(); }
  bool empty() const noexcept { return elements.empty(); }
  void clear() noexcept { elements.clear(); }

  iterator find(const Key &key) {
    return std::find_if(begin(), end(), [&key](const value_type &element) {
      return element.first == key;
    });
  }

  const_iterator find(const Key &key) const {
    return std::find_if(begin(), end(), [&key](const value_type &element) {
      return element.first == key;
    });
  }

  size_type count(const Key &key) const { return find(key) != end(); }

  T &at(const Key &key) {
    auto it = find(key);
    if (it == end())
      throw std::out_of_range("ordered_map::at: key not found");
    return it->second;
  }

  const T &at(const Key &key) const {
    auto it = find(key);
    if (it == end())
      throw std::out_of_range("ordered_map::at: key not found");
    return it->second;
  }

  T &operator[](const Key &key) {
    auto it = find(key);
    if (it != end())
      return it->second;

    elements.emplace_back(key, T());
    return elements.back().second;
  }

  std::pair<iterator, bool> insert(const value_type &value) {
    auto it = find(value.first);
    if (it != end())
      return {it, false};

    elements.push_back(value);
    return {std::prev(end()), true};
  }

  std::pair<iterator, bool> insert(value_type &&value) {
    auto it = find(value.first);
    if (it != end())
      return {it, false};

    elements.push_back(std::move(value));
    return {std::prev(end()), true};
  }

  size_type erase(const Key &key) {
    auto it = find(key);
    if (it == end())
      return 0;

    elements.erase(it);
    return 1;
  }

  iterator erase(const_iterator position) { return elements.erase(position); }

private:
  container_type elements;
};

} // namespace nbt
