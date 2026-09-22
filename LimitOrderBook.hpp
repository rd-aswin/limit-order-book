#pragma once


#include <map>
#include <deque>
#include <unordered_map>

#include "Order.hpp"
class LimitOrderBook {
 private:
  int lastId = 0;
  std::unordered_map<int, Order> orderBook;
  std::map<int, std::deque<int>> priceLevelBid;
  std::map<int, std::deque<int>> priceLevelAsk;

  int reduceQuantity(int orderId, int quantity);

  int matchOrders(int orderId);


 public:
  
  int placeOrder(int price, int quantity, int side);
   int orderCount() ;
   int getQty(int orderId);
  
  int cancelOrder(int orderId);


  
};
