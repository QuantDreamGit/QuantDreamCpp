//
// Created by user on 10/15/25.
//
#include "ibwrapper_test/test.h"
#include "EClient.h"

int main() {
  IBWrapper ib;
  ib.connect("address", 0000, 123);
  return 0;
}