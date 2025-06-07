#ifndef SIMPLE_CALCULATOR_HPP
#define SIMPLE_CALCULATOR_HPP

namespace simple {

class Calculator {
  public:
    int add(int a, int b);
    int subtract(int a, int b);
    int multiply(int a, int b);
    double divide(int a, int b);
};

} // namespace simple

#endif // SIMPLE_CALCULATOR_HPP
