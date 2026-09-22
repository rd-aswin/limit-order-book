#include "LimitOrderBook.hpp"

#include <algorithm>
#include <iostream>

int LimitOrderBook::cancelOrder(int orderId) {
  Order order = orderBook[orderId];
  if (order.side == 1) {
    priceLevelBid[order.price].erase(
        std::remove(priceLevelBid[order.price].begin(),
                    priceLevelBid[order.price].end(), orderId),
        priceLevelBid[order.price].end());
    if (priceLevelBid[order.price].empty()) priceLevelBid.erase(order.price);
  } else {
    priceLevelAsk[order.price].erase(
        std::remove(priceLevelAsk[order.price].begin(),
                    priceLevelAsk[order.price].end(), orderId),
        priceLevelAsk[order.price].end());
    if (priceLevelAsk[order.price].empty()) priceLevelAsk.erase(order.price);
  }

  orderBook.erase(orderId);

  std::cout << "Order with id:" << orderId << " is cancelled" << std::endl;
  return 0;
}

int LimitOrderBook::reduceQuantity(int orderId, int quantity) {
  orderBook[orderId].quantity -= quantity;
  if (orderBook[orderId].quantity == 0) cancelOrder(orderId);
  std::cout << "reduced order:" << orderId << std::endl;
  return 0;
}

int LimitOrderBook::matchOrders(int orderId) {
  Order placedOrder = orderBook[orderId];
  while (placedOrder.quantity > 0) {
    if (placedOrder.side == 1) {
      auto it = priceLevelAsk.begin();
      if (it != priceLevelAsk.end() && it->first <= placedOrder.price) {
        int matchedId = it->second.front();
        std::cout << "order: " << orderId
                  << " got match with order: " << matchedId << std::endl;
        Order matchedOrder = orderBook[matchedId];
        if (placedOrder.quantity >= matchedOrder.quantity) {
          reduceQuantity(orderId, matchedOrder.quantity);
          placedOrder.quantity -= matchedOrder.quantity;
          cancelOrder(matchedId);
        } else {
          reduceQuantity(matchedId, placedOrder.quantity);
          cancelOrder(orderId);
          return 0;
        }
      } else {
        std::cout << "put to rest in order book." << std::endl;
        return 0;
      }
    } else if (placedOrder.side == 2) {
      if (!priceLevelBid.empty()) {
        auto it = --priceLevelBid.end();

        if (it->first >= placedOrder.price) {
          int matchedId = it->second.front();
          std::cout << "order: " << orderId
                    << " got match with order: " << matchedId << std::endl;
          Order matchedOrder = orderBook[matchedId];
          if (placedOrder.quantity >= matchedOrder.quantity) {
            reduceQuantity(orderId, matchedOrder.quantity);
            placedOrder.quantity -= matchedOrder.quantity;
            cancelOrder(matchedId);
          } else {
            reduceQuantity(matchedId, placedOrder.quantity);
            placedOrder.quantity = 0;
            cancelOrder(orderId);
            return 0;
          }
        } else {
          std::cout << "put to rest in order book." << std::endl;
          return 0;
        }
      } else {
        std::cout << "put to rest in order book." << std::endl;
        return 0;
      }
    }
  }

  return 0;
}

int LimitOrderBook::placeOrder(int price, int quantity, int side) {
  int orderId = ++lastId;
  orderBook[orderId] = {price, quantity, side};
  if (side == 1)
    priceLevelBid[price].push_back(orderId);
  else
    priceLevelAsk[price].push_back(orderId);
  matchOrders(orderId);
  return 0;
}

int LimitOrderBook::orderCount() { return orderBook.size(); }
int LimitOrderBook::getQty(int orderId) { return orderBook[orderId].quantity; }