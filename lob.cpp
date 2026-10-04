
#include <cassert>
#include <chrono>
#include <iostream>

#include "LimitOrderBook.hpp"
using namespace std;
void benchmarkPlaceOrder(){
  LimitOrderBook book;
  auto start =  chrono::high_resolution_clock::now();
  for (int i=0 ; i<+100000 ;i++){
    book.placeOrder(99 +i,i%100,i%2 + 1);
  }
  auto end =  chrono::high_resolution_clock::now();

  auto avgLatency = chrono::duration_cast<chrono::nanoseconds>(end-start).count();
  cout << "Benchmark Latency: " << avgLatency/100000 << endl;
}
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
  benchmarkPlaceOrder();
  return 0;
}
