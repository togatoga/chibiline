#include "../chibiline.h"
#include <vector>

int main(int argc, char **argv) {
  chibi::App app =
      chibi::App("sample", "Calcualte \"left\" [operation] \"med\" "
                           "[operation] \"right\"");

  app.add_arg<std::string>("op1", "First operation");
  app.add_arg<std::string>("op2", "Second operation");
  app.add_opt<int>("left", 'l', "Left number", 20);
  app.add_opt<int>("med", 'm', "Med number", -100);
  app.add_opt<int>("right", 'r', "Right number", 0);
  app.add_flag("verbose", 'v', "Verbose mode");
  app.parse(argc, argv);

  std::string op1 = app.get_arg<std::string>(0);
  std::string op2 = app.get_arg<std::string>(1);
  int left = app.get_opt<int>("left").value();
  int med = app.get_opt<int>("med").value();
  int right = app.get_opt<int>("right").value();
  bool verbose = app.get_flag("verbose");
  int result = [&]() {
    int result = left;
    if (op1 == "sub") {
      result -= med;
    } else {
      result += med;
    }
    if (op2 == "sub") {
      result -= right;
    } else {
      result += right;
    }
    return result;
  }();
  if (verbose) {
    std::string op1 = " + ";
    std::string op2 = " + ";
    if (op1 == "sub") {
      op1 = " - ";
    }

    if (op2 == "sub") {
      op2 = " - ";
    }

    std::cout << left << op1 << med << op2 << right << " = " << result
              << std::endl;
  } else {
    std::cout << result << std::endl;
  }
}