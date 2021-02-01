#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/numeric/conversion/cast.hpp>

#include <utils/meta.hpp>

#include <cache/dump/operations.hpp>

namespace cache::dump {

namespace impl {

template <typename T>
void WriteRaw(Writer& writer, T value) {
  static_assert(std::is_trivially_copyable_v<T>);
  writer.WriteRaw(
      std::string_view{reinterpret_cast<const char*>(&value), sizeof(value)});
}

template <typename T>
T ReadRaw(Reader& reader, To<T>) {
  static_assert(std::is_trivially_copyable_v<T>);
  T value;
  reader.ReadRaw(sizeof(T)).copy(reinterpret_cast<char*>(&value), sizeof(T));
  return value;
}

void WriteInteger(Writer& writer, std::uint64_t value);

std::uint64_t ReadInteger(Reader& reader);

}  // namespace impl

/// @{
/// @brief `std::string_view` support
/// @warning The `string_view` will be invalidated on the next `Read` operation
void Write(Writer& writer, std::string_view value);

std::string_view Read(Reader& reader, To<std::string_view>);
/// @}

/// @{
/// `std::string` support
void Write(Writer& writer, const std::string& value);

std::string Read(Reader& reader, To<std::string>);
/// @}

/// Allows writing string literals
void Write(Writer& writer, const char* value);

/// @{
/// Integer support
template <typename T>
std::enable_if_t<meta::kIsInteger<T>> Write(Writer& writer, T value) {
  if constexpr (sizeof(T) == 1) {
    impl::WriteRaw(writer, value);
  } else {
    impl::WriteInteger(writer, static_cast<std::uint64_t>(value));
  }
}

template <typename T>
std::enable_if_t<meta::kIsInteger<T>, T> Read(Reader& reader, To<T> to) {
  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (sizeof(T) == 1) {
    return impl::ReadRaw(reader, to);
  }

  const auto raw = impl::ReadInteger(reader);

  if constexpr (std::is_signed_v<T>) {
    return boost::numeric_cast<T>(static_cast<std::int64_t>(raw));
  } else {
    return boost::numeric_cast<T>(raw);
  }
}
/// @}

/// @{
/// Floating-point support
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>> Write(Writer& writer, T value) {
  impl::WriteRaw(writer, value);
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> Read(Reader& reader,
                                                      To<T> to) {
  return impl::ReadRaw(reader, to);
}
/// @}

/// @{
/// `bool` support
void Write(Writer& writer, bool value);

bool Read(Reader& reader, To<bool>);
/// @}

}  // namespace cache::dump