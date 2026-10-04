#pragma once
#include <cstdint>

struct Order {
  int price;
  int quantity;
  uint8_t side;
  bool isActive = false;
  int prevOrderId = 0;
  int nextOrderId=0;
};