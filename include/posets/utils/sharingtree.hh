#pragma once

#include <cassert>
#include <iostream>
#include <vector>
#include <stack>
#include <tuple>

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
          int son;
          int bro;
      };
      st_node* bin_tree;

    public:
      sharingtree () = delete;

      ~sharingforest () { delete[] this->bin_tree; }

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
            cur_node->bro = -1;
            cur_node->son = -1;
            // if it's the first component of the vector and there is a
            // previous root, then we can update that root so that this new
            // one is its sibling; otherwise we still store the current node
            // as previous root
            if (first_comp) {
              if (prev_root != nullptr)
                prev_root->bro = idx;
              prev_root = cur_node;
              first_comp = false;
            }
            else  // if there is a previous component, we make this one its sibling
              prev_node->son = idx;
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
        // A stack of node indices and directions (0 down, 1 right)
        std::stack<std::tuple<int, bool>> to_visit;
        to_visit.emplace (this->root, 0);
        std::vector<V> res;
        std::vector<typename V::value_type> temp;
        
        while (not to_visit.empty ()) {
          assert (to_visit.size () <= this->dim);
          const auto [idx, going_down] = to_visit.top ();
          to_visit.pop ();
          st_node* cur = this->bin_tree[idx];
          temp.push_back (cur.label);

          // base case: reached the bottom layer
          if (cur.son == -1) {
            assert (to_visit.size () == this->dim - 1);
            std::vector<typename V::value_type> cpy {temp};
            res.push_back (V (std::move (cpy)));
            // is there a sibling with another label?
            temp.pop_back ();
            if (cur.bro > -1)
              to_visit.emplace (cur.bro, 0);
          }
          // recursive case: we need to push something into the stack
          else {
            assert (to_visit.size () < this->dim - 1);
            // either this is the first time we've seen the node, and we need
            // to go down pushing a reminder of the next time not being the
            // first, and its sibling...
            if (going_down) {
              assert (cur.son > -1);
              to_visit.emplace (idx, 1);
              to_visit.emplace (cur.son, 0);
            }
            // or we're already going right and we need to push its
            // sibling (and start by going down from there)
            else {
              temp.pop_back ();
              if (cur.bro > - 1)
                to_visit.emplace (cur.bro, 0);
            }
          }
        }

        return res;
      }
  };

  template <Vector V>
  inline std::ostream& operator<< (std::ostream& os, const sharingtree<V>& f) {
    for (auto&& el : f.get_all ())
      os << el << '\n';

    return os;
  }

}  // namespace posets::utils
