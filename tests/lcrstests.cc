#include <vector>

#include <posets/utils/sharingtree.hh>
#include <posets/vectors.hh>

namespace utils = posets::utils;

using VType = posets::vectors::vector_backed<char>;

std::vector<VType> vvtovv (const std::vector<std::vector<char>>& vv) {
  std::vector<VType> out;
  for (size_t i = 0; i < vv.size (); ++i)
    out.emplace_back (VType (std::move (vv[i])));
  return out;
}

int main (int argc, const char* argv[]) {
  std::vector<std::vector<char>> data {{6, 3, 2}, {5, 5, 4}, {2, 6, 2}};
  utils::sharingtree<VType> f1 (std::move (vvtovv (data)));
  std::cout << f1 << std::endl;
  assert (f1.get_all ().size () == 3);

  // Test adding a second tree to the forest
  data = {{7, 4, 3}, {4, 8, 4}, {2, 5, 6}, {1, 9, 9}};
  utils::sharingtree<VType> f2 (std::move (vvtovv (data)));
  std::cout << f2 << std::endl;
  assert (f2.get_all ().size () == 4);

  // And yet another tree, this time with a shared suffix
  // and with dimension 4
  data = {{3, 2, 2, 2}, {4, 1, 2, 1}, {5, 0, 2, 1}};
  utils::sharingtree<VType> f3 (std::move (vvtovv (data)));
  std::cout << f3 << std::endl;
  assert (f3.get_all ().size () == 3);

  return 0;
}
