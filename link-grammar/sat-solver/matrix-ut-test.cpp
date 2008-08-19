#include <iostream>


#include "matrix-ut.hpp"
using namespace std;

int main() {
  MatrixUpperTriangle<int> m(10, 5);
  for (int i = 0; i < 10; i++)
    for (int j = i+1; j < 10; j++) {
      m.set(i, j, i+j);
    }

  for (int i = 0; i < 10; i++)
    for (int j = i+1; j < 10; j++) {
      cout << m(i, j) << endl;
    }
  return 0;
}
