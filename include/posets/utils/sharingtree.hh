#pragma once

#include <boost/functional/hash.hpp>
#include <cassert>
#include <iostream>
#include <limits>
#include <map>
#include <stack>
#include <tuple>
#include <unordered_map>
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

      // This is a left-child right-sibling implementation of the sharing
      // tree, so we need nodes (with indices of a son and a brother)
      // and a single array of nodes to store them all. We also keep a color,
      // this will be used to define equivalence classes (as we DO NOT go all
      // the way down to a DFA/DAG, instead storing the intermediate trie).
      size_t dim;
      int root;
      struct st_node {
          typename V::value_type label;
          int color;
          int son;
          int bro;
      };
      st_node* bin_tree;

      // We will need to compare nodes based on their label
      class gt_wrt_label {
          st_node* t;

        public:
          gt_wrt_label (st_node* that) : t {that} {}
          bool operator() (int lhs, int rhs) const {
            assert (lhs > -1 and rhs > -1);
            return this->t[lhs].label > this->t[rhs].label;
          }
      };
      // We will also need to compare subtrees (assuming the trie construction
      // has been applied)
      struct intvec_hash {
        std::size_t operator() (std::vector<int> const& v) const {
          return boost::hash_range(v.begin (), v.end ());
        }
      };

      // We change the sibling pointers/indices of children of the given nodes
      // so that they form a single set of siblings
      void string_children (const std::vector<int>& nodes) {
        // we fetch the first sibling, if there is one
        st_node* cur = this->bin_tree + nodes[0];
        if (cur->son == -1)
          return;
        st_node* last = this->bin_tree + cur->son;
        // now, for all remaining nodes we
        // (1) get to the last sibling
        // (2) link to the next first sibling and
        for (size_t i = 1; i < nodes.size (); i++) {
          while (last->bro > -1)
            last = this->bin_tree + last->bro;
          cur = this->bin_tree + nodes[i];
          assert (cur->son > -1);
          last->bro = cur->son;
        }
      }

      void to_trie () {
        // A stack of node indices and
        // modes (0 reorder siblings, 1 down, 2 right)
        std::stack<std::tuple<int, short>> to_visit;
        to_visit.emplace (this->root, 0);
        std::vector<V> res;
        std::vector<typename V::value_type> temp;
        // We reset the root since it may move
        this->root = std::numeric_limits<int>::min ();

        while (not to_visit.empty ()) {
          assert (to_visit.size () <= this->dim);
          auto [idx, mode] = to_visit.top ();
          to_visit.pop ();

          if (mode == 1) {
            // we are going down, so look for a son and push it into the
            // stack; before that, we also push back the current node but in
            // mode=right so that when we come back we move to its sibling
            st_node* cur = this->bin_tree + idx;
            if (cur->son > -1) {
              to_visit.emplace (idx, 2);
              to_visit.emplace (cur->son, 0);
            }
          }
          else if (mode == 2) {
            // we are going right, so look for a sibling and push it into the
            // stack in mode=down
            st_node* cur = this->bin_tree + idx;
            if (cur->bro > -1)
              to_visit.emplace (cur->bro, 1);
          }
          else if (mode == 0) {
            // order nodes in label-decreasing order
            std::map<int, std::vector<int>, gt_wrt_label> buckets (gt_wrt_label (this->bin_tree));
            while (idx > -1) {
              st_node* cur = this->bin_tree + idx;
              buckets[cur->label].push_back (idx);
              idx = cur->bro;
            }
            // traverse the map in order to construct a set of children with the
            // first node per label only (in decreasing order)
            int head = -1;
            st_node* prev;
            for (auto& [label, nodes] : buckets) {
              string_children (nodes);
              if (head == -1)
                head = nodes[0];
              else
                prev->bro = nodes[0];
              prev = this->bin_tree + nodes[0];
            }
            prev->bro = -1;
            // now, we either repair the root, or the top-of-stack node
            if (to_visit.size () > 0) {
              const auto [idx_parent, mode_parent] = to_visit.top ();
              assert (mode_parent == 2);  // went down already, next time right
              st_node* parent = this->bin_tree + idx_parent;
              parent->son = head;
            }
            else
              this->root = head;
            // finally we put the head back in the stack in mode=down
            to_visit.emplace (head, 1);
          }
          else
            assert (false);
        }
      }

      void color_as_dfa () {
        std::vector<int> layer[dim];

        // We collect the indices of nodes per layer via a DFS. For this, we
        // use a stack of node indices and directions (0 down, 1 right)
        std::stack<std::tuple<int, short>> to_visit;
        to_visit.emplace (this->root, 0);

        while (not to_visit.empty ()) {
          assert (to_visit.size () <= this->dim);
          const auto [idx, direction] = to_visit.top ();
          to_visit.pop ();
          st_node* cur = this->bin_tree + idx;

          // base case: reached the bottom layer
          if (cur->son == -1) {
            assert (to_visit.size () == this->dim - 1);
            assert (direction == 0);  // leaves only reached going down
            layer[to_visit.size ()].push_back (idx);
            // is there a sibling?
            if (cur->bro > -1)
              to_visit.emplace (cur->bro, 0);
          }
          // recursive case: we need to push something into the stack
          else {
            assert (to_visit.size () < this->dim - 1);
            // either this is the first time we've seen the node, and we need
            // to go down pushing a reminder of the next time not being the
            // first, and its sibling...
            if (direction == 0) {
              assert (cur->son > -1);
              layer[to_visit.size ()].push_back (idx);
              to_visit.emplace (idx, 1);
              to_visit.emplace (cur->son, 0);
            }
            // or we're already going right and we need to push its
            // sibling (and start by going down from there)
            else if (direction == 1) {
              if (cur->bro > -1)
                to_visit.emplace (cur->bro, 0);
            }
            else
              assert (false);
          }
        }
        
        // Now, per layer (in bottom-up fashion) we use a hash table to assign
        // "colors" to the nodes based on their label and the colors of their
        // children.
        int nxt_color = 0;
        for (int i = this->dim - 1; i >= 0; i--) {
          std::unordered_map<std::vector<int>, std::vector<int>, intvec_hash> colors2indices;
          for (const int idx : layer[i]) {
            st_node* cur = this->bin_tree + idx;
            std::vector<int> k;
            k.push_back (cur->label);
            int son_idx = cur->son;
            while (son_idx > -1) {
              cur = this->bin_tree + son_idx;
              k.push_back (cur->color);
              son_idx = cur->bro;
            }
            colors2indices[k].push_back (idx);
          }
          // Every node in this layer is ready to get its equivalence-class
          // color
          for (const auto& [key, idces] : colors2indices) {
            for (const int idx : idces) {
              st_node* cur = this->bin_tree + idx;
              cur->color = nxt_color;
#ifndef NDEBUG
              std::cout << "layer=" << i << ", label="
                        << (int) cur->label << ", color="
                        << cur->color << '\n';
#endif
            }
            nxt_color += 1;
          }
        }
      }

    public:
      sharingtree () = delete;

      ~sharingtree () { delete[] this->bin_tree; }

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
          st_node* prev_node;
          for (auto&& comp : e) {
            st_node* cur_node = this->bin_tree + idx;
            cur_node->label = (int) comp;
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
            else  // there is a previous component, so we make this one its sibling
              prev_node->son = idx;
            prev_node = cur_node;
            idx += 1;
          }
        }

        // now, we make a trie/suffix tree (making sure the children are in
        // decreasing label order)
        this->to_trie ();

        // finally, we proceed bottom-up to merge language equivalent nodes
        this->color_as_dfa ();
      }

      [[nodiscard]] std::vector<V> get_all () const {
        // A stack of node indices and directions (0 down, 1 right)
        std::stack<std::tuple<int, short>> to_visit;
        to_visit.emplace (this->root, 0);
        std::vector<V> res;
        std::vector<typename V::value_type> temp;

        while (not to_visit.empty ()) {
          assert (to_visit.size () <= this->dim);
          const auto [idx, direction] = to_visit.top ();
          to_visit.pop ();
          st_node* cur = this->bin_tree + idx;

          // base case: reached the bottom layer
          if (cur->son == -1) {
            assert (to_visit.size () == this->dim - 1);
            temp.push_back (cur->label);
            std::vector<typename V::value_type> cpy {temp};
            res.push_back (V (std::move (cpy)));
            temp.pop_back ();
            // is there a sibling with another label?
            if (cur->bro > -1)
              to_visit.emplace (cur->bro, 0);
          }
          // recursive case: we need to push something into the stack
          else {
            assert (to_visit.size () < this->dim - 1);
            // either this is the first time we've seen the node, and we need
            // to go down pushing a reminder of the next time not being the
            // first, and its sibling...
            if (direction == 0) {
              assert (cur->son > -1);
              to_visit.emplace (idx, 1);
              temp.push_back (cur->label);
              to_visit.emplace (cur->son, 0);
            }
            // or we're already going right and we need to push its
            // sibling (and start by going down from there)
            else if (direction == 1) {
              temp.pop_back ();
              if (cur->bro > -1)
                to_visit.emplace (cur->bro, 0);
            }
            else
              assert (false);
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
