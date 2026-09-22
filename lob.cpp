
#include <cassert>

#include "LimitOrderBook.hpp"
using namespace std;

void test_case() {
  LimitOrderBook book;
  book.placeOrder(99, 10, 1);
  book.placeOrder(77, 5, 1);
  assert(book.orderCount() == 2);
  book.placeOrder(85, 3, 2);
  assert(book.getQty(1) == 7);
  book.placeOrder(100, 7, 2);
  assert(book.orderCount() == 3);
  book.placeOrder(85, 15, 2);
  assert(book.getQty(5) == 8);
}
int main() {
  test_case();
  return 0;
}
