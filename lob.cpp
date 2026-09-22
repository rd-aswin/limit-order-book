#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <unordered_map>
using namespace std;

struct Order {
  int price;
  int quantity;
  int side;
};

class LimitOrderBook {
 private:
  int lastId = 0;
  unordered_map<int, Order> orderBook;
  map<int, deque<int>> priceLevelBid;
  map<int, deque<int>> priceLevelAsk;

 public:
  int orderCount() { return orderBook.size(); }
  int getQty(int orderId) { return orderBook[orderId].quantity; }
  int cancelOrder(int orderId) {
    Order order = orderBook[orderId];
    if (order.side == 1) {
      priceLevelBid[order.price].erase(
          remove(priceLevelBid[order.price].begin(),
                 priceLevelBid[order.price].end(), orderId),
          priceLevelBid[order.price].end());
      if (priceLevelBid[order.price].empty()) priceLevelBid.erase(order.price);
    } else {
      priceLevelAsk[order.price].erase(
          remove(priceLevelAsk[order.price].begin(),
                 priceLevelAsk[order.price].end(), orderId),
          priceLevelAsk[order.price].end());
      if (priceLevelAsk[order.price].empty()) priceLevelAsk.erase(order.price);
    }

    orderBook.erase(orderId);

    cout << "Order with id:" << orderId << " is cancelled" << endl;
    return 0;
  }

  int reduceQuantity(int orderId, int quantity) {
    orderBook[orderId].quantity -= quantity;
    if (orderBook[orderId].quantity == 0) cancelOrder(orderId);
    cout << "reduced order:" << orderId << endl;
    return 0;
  }

  int matchOrders(int orderId) {
    Order placedOrder = orderBook[orderId];
    while (placedOrder.quantity > 0) {
      if (placedOrder.side == 1) {
        auto it = priceLevelAsk.begin();
        if (it != priceLevelAsk.end() && it->first <= placedOrder.price) {
          int matchedId = it->second.front();
          cout << "order: " << orderId << " got match with order: " << matchedId
               << endl;
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
          cout << "put to rest in order book." << endl;
          return 0;
        }
      } else if (placedOrder.side == 2) {
        if (!priceLevelBid.empty()) {
          auto it = --priceLevelBid.end();

          if (it->first >= placedOrder.price) {
            int matchedId = it->second.front();
            cout << "order: " << orderId
                 << " got match with order: " << matchedId << endl;
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
            cout << "put to rest in order book." << endl;
            return 0;
          }
        } else {
          cout << "put to rest in order book." << endl;
          return 0;
        }
      }
    }

    return 0;
  }

  int placeOrder(int price, int quantity, int side) {
    int orderId = ++lastId;
    orderBook[orderId] = {price, quantity, side};
    if (side == 1)
      priceLevelBid[price].push_back(orderId);
    else
      priceLevelAsk[price].push_back(orderId);
    matchOrders(orderId);
    return 0;
  }
  int viewOrders() {
    auto it = orderBook.begin();
    cout << "| ID | PRICE | QUANTITY | SIDE" << endl;
    while (it != orderBook.end()) {
      cout << "   " << it->first << "    " << it->second.price << "       "
           << it->second.quantity << "       "
           << ((it->second.side == 1) ? "BUY" : "SELL") << endl;
      ++it;
    }
    return 0;
  }
};

void test_case() {
  LimitOrderBook book;
  book.placeOrder(99, 10, 1);
  book.placeOrder(77, 5, 1);
  assert(book.orderCount() == 2);
  book.placeOrder(85, 3, 2);
  assert(book.getQty(1)==7);
  book.placeOrder(100, 7, 2);
  assert(book.orderCount() == 3);
  book.placeOrder(85, 15, 2);
  assert(book.getQty(5)==8);
}
int main() {
  test_case();
  return 0;
}
/*int main() {
  while (true) {
    int choice;
    cout << "1.Add order\n2.Update order\n3.Cancel order\n4.Exit\nEnter your "
            "choice: ";
    cin >> choice;
    switch (choice) {
      case 1:
        placeOrder();
        break;
      case 2:
        break;
      case 3:
        int id;
        cout << "Enter the order id to cancel: ";
        cin >> id;
        cancelOrder(id);
        break;
      case 4:
        return 0;
        break;
      default:
        cout << "Idiot!!";
    }
  }

  return 0;
}
*/