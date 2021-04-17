#include "unity.h"

#include "parser.h"

using namespace std;

static void test_parse_reg_line() {
  TEST_ASSERT_TRUE(Parser::parse_reg_line("1-0:24.7.0(0000.000*kvar)") ==
                   RegValue("1-0:24.7.0", "0000.000", "kvar"));
  TEST_ASSERT_FALSE(Parser::parse_reg_line("(1)"));
  TEST_ASSERT_TRUE(Parser::parse_reg_line("1(2)") == RegValue("1", "2", ""));
  TEST_ASSERT_FALSE(Parser::parse_reg_line("1(1)."));
}

static void test_parser() {
  optional<RegValue> last_reg_value;
  optional<string> last_header;
  optional<bool> frame_ended;

  Parser parser;
  parser.reg_value_cb_ = [&last_reg_value](const RegValue &rv) {
    last_reg_value = rv;
  };
  parser.frame_start_cb_ = [&last_header](const string &h) { last_header = h; };
  parser.frame_end_cb_ = [&frame_ended]() { frame_ended = true; };
  parser.feed("/header\r\n");
  TEST_ASSERT_TRUE(*last_header == "header");
  parser.feed("1(2*u)\r\n");
  TEST_ASSERT_TRUE(*last_reg_value == RegValue("1", "2", "u"));
  parser.feed("3(4)\r\n");
  TEST_ASSERT_TRUE(*last_reg_value == RegValue("3", "4", ""));
  parser.feed("!1234\r\n");
  TEST_ASSERT_TRUE(*frame_ended);

  parser.feed("\r\n");
  parser.feed("/header2\r\n");
  TEST_ASSERT_TRUE(last_header == "header2");
}

static void test_parse_real() {
  const char *frame = R"(/ELL5\253833635_A

0-0:1.0.0(210407111627W)
1-0:1.8.0(00004788.631*kWh)
1-0:2.8.0(00000000.000*kWh)
1-0:3.8.0(00001203.621*kvarh)
1-0:4.8.0(00000150.940*kvarh)
1-0:1.7.0(0004.905*kW)
1-0:2.7.0(0000.000*kW)
1-0:3.7.0(0001.625*kvar)
1-0:4.7.0(0000.000*kvar)
1-0:21.7.0(0000.976*kW)
1-0:41.7.0(0001.882*kW)
1-0:61.7.0(0002.045*kW)
1-0:22.7.0(0000.000*kW)
1-0:42.7.0(0000.000*kW)
1-0:62.7.0(0000.000*kW)
1-0:23.7.0(0000.574*kvar)
1-0:43.7.0(0000.553*kvar)
1-0:63.7.0(0000.498*kvar)
1-0:24.7.0(0000.000*kvar)
1-0:44.7.0(0000.000*kvar)
1-0:64.7.0(0000.000*kvar)
1-0:32.7.0(233.6*V)
1-0:52.7.0(234.7*V)
1-0:72.7.0(233.7*V)
1-0:31.7.0(004.8*A)
1-0:51.7.0(008.3*A)
1-0:71.7.0(009.0*A)
!57B6)";

  Parser parser;
  parser.reg_value_cb_ = [](const RegValue &rv) {
    cout << "reg " << rv << endl;
  };
  parser.frame_start_cb_ = [](const string &h) {
    cout << "Frame start: " << h << endl;
  };
  parser.frame_end_cb_ = []() { cout << "Frame end" << endl; };
  parser.feed(frame);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_reg_line);
  RUN_TEST(test_parser);
  RUN_TEST(test_parse_real);
  UNITY_END();
  return 0;
}
