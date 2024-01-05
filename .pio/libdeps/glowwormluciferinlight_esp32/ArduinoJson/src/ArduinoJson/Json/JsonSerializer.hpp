// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2024, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Json/TextFormatter.hpp>
#include <ArduinoJson/Serialization/measure.hpp>
#include <ArduinoJson/Serialization/serialize.hpp>
#include <ArduinoJson/Variant/VariantDataVisitor.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename TWriter>
class JsonSerializer : public VariantDataVisitor<size_t> {
 public:
  static const bool producesText = true;

  JsonSerializer(TWriter writer, const ResourceManager* resources)
      : formatter_(writer), resources_(resources) {}

  FORCE_INLINE size_t visit(const ArrayData& array) {
    write('[');

    auto it = array.createIterator(resources_);

    while (!it.done()) {
      it->accept(*this);

      it.next(resources_);
      if (it.done())
        break;

      write(',');
    }

    write(']');
    return bytesWritten();
  }

  size_t visit(const ObjectData& object) {
    write('{');

    auto it = object.createIterator(resources_);

    while (!it.done()) {
      formatter_.writeString(it.key());
      write(':');
      it->accept(*this);

      it.next(resources_);
      if (it.done())
        break;

      write(',');
    }

    write('}');
    return bytesWritten();
  }

  size_t visit(JsonFloat value) {
    formatter_.writeFloat(value);
    return bytesWritten();
  }

  size_t visit(const char* value) {
    formatter_.writeString(value);
    return bytesWritten();
  }

  size_t visit(JsonString value) {
    formatter_.writeString(value.c_str(), value.size());
    return bytesWritten();
  }

  size_t visit(RawString value) {
    formatter_.writeRaw(value.data(), value.size());
    return bytesWritten();
  }

  size_t visit(JsonInteger value) {
    formatter_.writeInteger(value);
    return bytesWritten();
  }

  size_t visit(JsonUInt value) {
    formatter_.writeInteger(value);
    return bytesWritten();
  }

  size_t visit(bool value) {
    formatter_.writeBoolean(value);
    return bytesWritten();
  }

  size_t visit(nullptr_t) {
    formatter_.writeRaw("null");
    return bytesWritten();
  }

 protected:
  size_t bytesWritten() const {
    return formatter_.bytesWritten();
  }

  void write(char c) {
    formatter_.writeRaw(c);
  }

  void write(const char* s) {
    formatter_.writeRaw(s);
  }

 private:
  TextFormatter<TWriter> formatter_;

 protected:
  const ResourceManager* resources_;
};

ARDUINOJSON_END_PRIVATE_NAMESPACE

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

// Produces a minified JSON document.
// https://arduinojson.org/v7/api/json/serializejson/
template <typename TDestination>
size_t serializeJson(JsonVariantConst source, TDestination& destination) {
  using namespace detail;
  return serialize<JsonSerializer>(source, destination);
}

// Produces a minified JSON document.
// https://arduinojson.org/v7/api/json/serializejson/
inline size_t serializeJson(JsonVariantConst source, void* buffer,
                            size_t bufferSize) {
  using namespace detail;
  return serialize<JsonSerializer>(source, buffer, bufferSize);
}

// Computes the length of the document that serializeJson() produces.
// https://arduinojson.org/v7/api/json/measurejson/
inline size_t measureJson(JsonVariantConst source) {
  using namespace detail;
  return measure<JsonSerializer>(source);
}

#if ARDUINOJSON_ENABLE_STD_STREAM
template <typename T>
inline typename detail::enable_if<
    detail::is_convertible<T, JsonVariantConst>::value, std::ostream&>::type
operator<<(std::ostream& os, const T& source) {
  serializeJson(source, os);
  return os;
}
#endif

ARDUINOJSON_END_PUBLIC_NAMESPACE
