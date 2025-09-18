#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include <posets/concepts.hh>

namespace posets::utils {

  // Forward definition for the operator<<
  template <Vector>
  class sharingtree;

  template <Vector V>
  std::ostream& operator<< (std::ostream& os, const utils::sharingtree<V>& f);

  template <Vector V>
  class sharingtree {
    private:
      template <Vector V2>
      friend std::ostream& operator<< (std::ostream& os, const utils::sharingtree<V2>& f);

      // This is a left-child right-sibling implementation
      // of the sharing tree, so we need nodes and an array
      // of nodes with indices and not pointers to children nodes
      size_t dim;
      int root;
      struct st_node {
          typename V::value_type label;
          int child;
          int sibling;
      };
      st_node* bin_tree;

    public:
      sharingtree () = delete;

      ~sharingforest () {
        delete[] this->bin_tree;
      }

      template <std::ranges::input_range R, class Proj = std::identity>
      sharingtree (R&& elements, Proj proj = {}) : dim (proj (*elements.begin ()).size ()) {
        this->bin_tree = new st_node[dim * elements.size ()];
        this->root = 0;

        // moving the given elements to the internal array in the form of
        // linear trees (just strings) with their roots being siblings
        int idx = 0;
        st_node* prev_root = nullptr;
        for (auto&& e : elements) {
          bool first_comp = true;
          for (auto&& comp : e) {
            cur_node = this->bin_tree[idx];
            cur_node->label = comp;
            cur_node->sibling = -1;
            cur_node->child = -1;
            // if it's the first component of the vector and there is a
            // previous root, then we can update that root so that this new
            // one is its sibling; otherwise we still store the current node
            // as previous root
            if (first_comp) {
              if (prev_root != nullptr)
                prev_root->sibling = idx;
              prev_root = cur_node;
              first_comp = false;
            } else  // if there is a previous component, we make this one its sibling
              prev_node->child = idx;
            prev_node = cur_node;
            idx += 1;
          }
        }

        // now, we make a trie/suffix tree (making sure the children are in
        // decreasing label order)
        // ...
        // finally, we proceed bottom-up to merge language equivalent nodes

      }

      [[nodiscard]]
      std::vector<V> get_all (std::optional<size_t> root = {}) const {
        // FIXME: implement
        return std::vector<V>();
      }
  };

  template <Vector V>
  inline std::ostream& operator<< (std::ostream& os, const sharingforest<V>& f) {
    for (auto&& el : f.get_all ())
      os << el << '\n';

    return os;
  }

}  // namespace posets::utils
