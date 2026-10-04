#pragma once

#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

#include "Order.hpp"

struct PriceLevel {
  int headOrderId = 0;
  int tailOrderId = 0;
  int nextPriceLevel = 0;
  int prevPriceLevel = 0;
};

class LimitOrderBook {
  static constexpr int maxOrders = 1000000;
  static constexpr int maxPrice = 10000;

 private:
  // std::unordered_map<int, Order> orderBook;
  std::vector<Order> orderPool;
  std::vector<PriceLevel> priceLevelBid;
  std::vector<PriceLevel> priceLevelAsk;

  int reduceQuantity(int orderId, int quantity);

  int matchOrders(int orderId);
  int lastId = 0, activeOrders = 0;
  int bestBid = 0;  // highest
  int bestAsk = 10001;  // lowest

  int addOrder(PriceLevel& level, int orderId);

 public:
  LimitOrderBook() {
    orderPool.resize(maxOrders);
    priceLevelBid.resize(maxPrice);
    priceLevelAsk.resize(maxPrice);
  }
  int placeOrder(int price, int quantity, uint8_t side);
  int orderCount();
  int getQty(int orderId);

  int cancelOrder(int orderId);
};
