#pragma once

#include <experimental/optional>
#include <functional>
#include <iostream>
#include <string>

struct RegValue {
  std::string reg;
  std::string value;
  std::string unit;

  RegValue(std::string reg, std::string value, std::string unit)
      : reg(reg), value(value), unit(unit) {}

  bool operator==(const RegValue &other) const {
    return reg == other.reg && value == other.value && unit == other.unit;
  }
};

std::ostream &operator<<(std::ostream &os, const RegValue &rv);

class Parser {
public:
  Parser();
  void feed(const std::string &str);
  void feed(char c);

  static std::experimental::optional<RegValue>
  parse_reg_line(const std::string &line);
  static uint16_t calculate_crc(const std::string &data);

  std::function<void(const RegValue &rv)> reg_value_cb_;
  std::function<void(const std::string &header)> frame_start_cb_;
  std::function<void()> frame_end_cb_;

  const std::string &frame() const { return frame_; }

protected:
  enum State {
    WaitingForFrame,
    ReceivingHeader,
    WaitingForRegOrChecksum,
    ReceivingReg,
    ReceivingChecksum
  };

  State state_;
  std::string frame_;
  std::string line_;
};
