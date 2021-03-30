#pragma once

#include <sstream>
#include <string>
#include <vector>

struct RegisterValue {
  std::string obis;
  std::string value;
  std::string unit;
};

class MessageBuffer {
  std::string buf;

public:
  void append(std::string str) { buf.append(str); }
  void append(char c) { buf.push_back(c); }
  const std::string &str() const { return buf; }
  void clear() { buf.clear(); }
};
