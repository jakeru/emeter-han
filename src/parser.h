#pragma once

#include <string>

class ParserCallbacks {
public:
  virtual void frame_start(const std::string &header) {}
  virtual void reading(const std::string &reg, const std::string &value,
                       const std::string &unit) {}
  virtual void frame_end(bool crc_ok) {}
};

class Parser {
  Parser(ParserCallbacks *callbacks);
  void feed(char c);
  bool parse_reg_line(const std::string &line);
  uint16_t calculate_crc(const std::string &data);

  enum State {
    WaitingForFrame,
    ReceivingHeader,
    WaitingForRegOrChecksum,
    ReceivingReg,
    ReceivingChecksum
  };

protected:
  ParserCallbacks *callbacks_;
  State state_;
  std::string frame_;
  std::string line_;
};
