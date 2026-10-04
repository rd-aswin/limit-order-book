#include "LimitOrderBook.hpp"

#include <algorithm>
#include <iostream>

int LimitOrderBook::cancelOrder(int orderId) {
  // Order order = orderPool[orderId];
  int nextId = orderPool[orderId].nextOrderId;
  int prevId = orderPool[orderId].prevOrderId;
  if (prevId == 0) {
    orderPool[nextId].prevOrderId = 0;
  } else if (nextId == 0) {
    orderPool[prevId].nextOrderId = 0;
  } else {
    orderPool[prevId].nextOrderId = nextId;
    orderPool[nextId].prevOrderId = prevId;
  }
  if (orderPool[orderId].side == 1 &&
      priceLevelBid[orderPool[orderId].price].headOrderId == orderId)
    priceLevelBid[orderPool[orderId].price].headOrderId = nextId;
  else if (orderPool[orderId].side == 2 &&
           priceLevelAsk[orderPool[orderId].price].headOrderId == orderId)
    priceLevelAsk[orderPool[orderId].price].headOrderId = nextId;

  orderPool[orderId].nextOrderId = 0;
  orderPool[orderId].prevOrderId = 0;

  // if (order.side == 1) {
  //   priceLevelBid[order.price].erase(
  //       std::remove(priceLevelBid[order.price].begin(),
  //                   priceLevelBid[order.price].end(), orderId),
  //       priceLevelBid[order.price].end());
  //   if (priceLevelBid[order.price].empty()) priceLevelBid.erase(order.price);
  // } else {
  //   priceLevelAsk[order.price].erase(
  //       std::remove(priceLevelAsk[order.price].begin(),
  //                   priceLevelAsk[order.price].end(), orderId),
  //       priceLevelAsk[order.price].end());
  //   if (priceLevelAsk[order.price].empty()) priceLevelAsk.erase(order.price);
  // }
  activeOrders--;
  orderPool[orderId].isActive = false;

  return 0;
}

int LimitOrderBook::reduceQuantity(int orderId, int quantity) {
  orderPool[orderId].quantity -= quantity;
  if (orderPool[orderId].quantity == 0) cancelOrder(orderId);
  return 0;
}

int LimitOrderBook::matchOrders(int orderId) {
  Order& placedOrder = orderPool[orderId];
  while (placedOrder.quantity > 0) {
    if (placedOrder.side == 1) {
      if (placedOrder.price >= bestAsk) {
        int matchedId = priceLevelAsk[bestAsk].headOrderId;
        Order& matchedOrder = orderPool[matchedId];
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
        return 0;
      }
    } else if (placedOrder.side == 2) {
      if (placedOrder.price <= bestBid) {
        int matchedId = priceLevelBid[bestBid].headOrderId;

        Order& matchedOrder = orderPool[matchedId];
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
        return 0;
      }
    }
  }

  return 0;
}

int LimitOrderBook::placeOrder(int price, int quantity, uint8_t side) {
  int orderId = ++lastId;
  orderPool[orderId] = {price, quantity, side, true};
  if (side == 1) {
    if (price > bestBid) bestBid = price;
    addOrder(priceLevelBid[price], orderId);
  } else {
    if (price < bestAsk) bestAsk = price;
    addOrder(priceLevelAsk[price], orderId);
  }
  activeOrders++;
  matchOrders(orderId);
  return 0;
}

int LimitOrderBook::orderCount() { return activeOrders; }
int LimitOrderBook::getQty(int orderId) { return orderPool[orderId].quantity; }

int LimitOrderBook::addOrder(PriceLevel& level, int orderId) {
  if (level.headOrderId == 0) {
    level.headOrderId = orderId;
    level.tailOrderId = orderId;

  } else {
    orderPool[level.tailOrderId].nextOrderId = orderId;
    orderPool[orderId].prevOrderId = level.tailOrderId;
    level.tailOrderId = orderId;
  }
  return 0;
}