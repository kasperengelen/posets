# Downset Manipulation Library
This header-only C++ library implements several data structures that can be
used to store and manipulate downward closed sets (a.k.a. downsets).

The implementations include:
* Vector-based data structures
* kd-tree-based data structures
* Sharing-tree data structures (much like binary decision diagrams)
* Sharing-trie data structures (like sharing-tree, which is actually a DFA, but kept as a tree)

## Applications
The downset data structures have been optimized for the following
applications:
* Parity-game solving
* Antichain-based temporal synthesis (from LTL specifications)

## Compilation
Due to our use of `std::experimental` we prefer `libstdc++` thus `g++`. Even on OSX, we
recommend installing `g++` say via Homebrew. Set `CXX` to your newly installed `g++`
before setting up the build with `meson`.
