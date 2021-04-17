#include "parser.h"

using namespace std;

bool is_cr_or_lf(char c) { return c == '\r' || c == '\n'; }

std::ostream &operator<<(std::ostream &os, const RegValue &rv) {
  return os << rv.reg << " " << rv.value << " " << rv.unit;
}

Parser::Parser() : state_(WaitingForFrame) {}

std::experimental::optional<RegValue>
Parser::parse_reg_line(const std::string &line) {
  const size_t left_paren = line.find('(');
  const size_t right_paren = line.find(')');
  if (left_paren == string::npos || right_paren == string::npos ||
      left_paren > right_paren || left_paren == 0 ||
      right_paren != line.size() - 1) {
    return {};
  }
  const string reg = line.substr(0, left_paren);
  const string value_unit =
      line.substr(left_paren + 1, right_paren - left_paren - 1);
  const size_t asterisk = value_unit.find('*');
  const string value =
      asterisk != string::npos ? value_unit.substr(0, asterisk) : value_unit;
  const string unit =
      asterisk != string::npos ? value_unit.substr(asterisk + 1) : "";
  return RegValue(reg, value, unit);
}

void Parser::feed(const std::string &str) {
  for (char c : str) {
    feed(c);
  }
}

void Parser::feed(char c) {
  switch (state_) {
  case WaitingForFrame:
    if (c == '/') {
      state_ = ReceivingHeader;
      frame_.clear();
      line_.clear();
    }
    break;
  case ReceivingHeader:
    frame_.push_back(c);
    if (is_cr_or_lf(c)) {
      if (frame_start_cb_) {
        frame_start_cb_(line_);
      }
      line_.clear();
      state_ = WaitingForRegOrChecksum;
    } else {
      line_.push_back(c);
    }
    break;
  case WaitingForRegOrChecksum:
    frame_.push_back(c);
    if (c == '!') {
      state_ = ReceivingChecksum;
    } else if (!is_cr_or_lf(c)) {
      line_.push_back(c);
      state_ = ReceivingReg;
    }
    break;
  case ReceivingReg:
    frame_.push_back(c);
    if (is_cr_or_lf(c)) {
      if (auto reg_value = parse_reg_line(line_)) {
        if (reg_value_cb_) {
          reg_value_cb_(*reg_value);
        }
      }
      line_.clear();
      state_ = WaitingForRegOrChecksum;
    } else {
      line_.push_back(c);
    }
    break;
  case ReceivingChecksum:
    if (!isxdigit(c)) {
      state_ = WaitingForFrame;
    } else {
      line_.push_back(c);
      if (line_.size() == 4) {
        /*
        uint16_t crc = calculate_crc(frame_);
        uint16_t expected_crc = stol(line_, nullptr, 16);
        if (frame_end_cb_) {
          frame_end_cb_(crc == expected_crc);
        }
        */
        if (frame_end_cb_) {
          frame_end_cb_();
        }
        state_ = WaitingForFrame;
      }
    }
    break;
  }
}
