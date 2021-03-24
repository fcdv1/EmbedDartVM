// Copyright (c) 2010, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.
//
// The original file can be found at:
// https://github.com/v8/v8/blob/master/src/splay-tree-inl.h

#ifndef RUNTIME_PLATFORM_SPLAY_TREE_INL_H_
#define RUNTIME_PLATFORM_SPLAY_TREE_INL_H_

#include <vector>

#include "platform/splay-tree.h"

namespace dart {

template <typename Config, class B, class Allocator>
SplayTree<Config, B, Allocator>::~SplayTree() {
  NodeDeleter deleter;
  ForEachNode(&deleter);
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::Insert(const Key& key, Locator* locator) {
  if (is_empty()) {
    // If the tree is empty, insert the new node.
    root_ = new (allocator_) Node(key, Config::NoValue());
  } else {
    // Splay on the key to move the last node on the search path
    // for the key to the root of the tree.
    Splay(key);
    // Ignore repeated insertions with the same key.
    int cmp = Config::Compare(key, root_->key_);
    if (cmp == 0) {
      locator->bind(root_);
      return false;
    }
    // Insert the new node.
    Node* node = new (allocator_) Node(key, Config::NoValue());
    InsertInternal(cmp, node);
  }
  locator->bind(root_);
  return true;
}

template <typename Config, class B, class Allocator>
void SplayTree<Config, B, Allocator>::InsertInternal(int cmp, Node* node) {
  if (cmp > 0) {
    node->left_ = root_;
    node->right_ = root_->right_;
    root_->right_ = nullptr;
  } else {
    node->right_ = root_;
    node->left_ = root_->left_;
    root_->left_ = nullptr;
  }
  root_ = node;
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::FindInternal(const Key& key) {
  if (is_empty()) return false;
  Splay(key);
  return Config::Compare(key, root_->key_) == 0;
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::Contains(const Key& key) {
  return FindInternal(key);
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::Find(const Key& key, Locator* locator) {
  if (FindInternal(key)) {
    locator->bind(root_);
    return true;
  } else {
    return false;
  }
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::FindGreatestLessThan(const Key& key,
                                                           Locator* locator) {
  if (is_empty()) return false;
  // Splay on the key to move the node with the given key or the last
  // node on the search path to the top of the tree.
  Splay(key);
  // Now the result is either the root node or the greatest node in
  // the left subtree.
  int cmp = Config::Compare(root_->key_, key);
  if (cmp <= 0) {
    locator->bind(root_);
    return true;
  } else {
    Node* temp = root_;
    root_ = root_->left_;
    bool result = FindGreatest(locator);
    root_ = temp;
    return result;
  }
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::FindLeastGreaterThan(const Key& key,
                                                           Locator* locator) {
  if (is_empty()) return false;
  // Splay on the key to move the node with the given key or the last
  // node on the search path to the top of the tree.
  Splay(key);
  // Now the result is either the root node or the least node in
  // the right subtree.
  int cmp = Config::Compare(root_->key_, key);
  if (cmp >= 0) {
    locator->bind(root_);
    return true;
  } else {
    Node* temp = root_;
    root_ = root_->right_;
    bool result = FindLeast(locator);
    root_ = temp;
    return result;
  }
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::FindGreatest(Locator* locator) {
  if (is_empty()) return false;
  Node* current = root_;
  while (current->right_ != nullptr)
    current = current->right_;
  locator->bind(current);
  return true;
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::FindLeast(Locator* locator) {
  if (is_empty()) return false;
  Node* current = root_;
  while (current->left_ != nullptr)
    current = current->left_;
  locator->bind(current);
  return true;
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::Move(const Key& old_key,
                                           const Key& new_key) {
  if (!FindInternal(old_key)) return false;
  Node* node_to_move = root_;
  RemoveRootNode(old_key);
  Splay(new_key);
  int cmp = Config::Compare(new_key, root_->key_);
  if (cmp == 0) {
    // A node with the target key already exists.
    delete node_to_move;
    return false;
  }
  node_to_move->key_ = new_key;
  InsertInternal(cmp, node_to_move);
  return true;
}

template <typename Config, class B, class Allocator>
bool SplayTree<Config, B, Allocator>::Remove(const Key& key) {
  if (!FindInternal(key)) return false;
  Node* node_to_remove = root_;
  RemoveRootNode(key);
  delete node_to_remove;
  return true;
}

template <typename Config, class B, class Allocator>
void SplayTree<Config, B, Allocator>::RemoveRootNode(const Key& key) {
  if (root_->left_ == nullptr) {
    // No left child, so the new tree is just the right child.
    root_ = root_->right_;
  } else {
    // Left child exists.
    Node* right = root_->right_;
    // Make the original left child the new root.
    root_ = root_->left_;
    // Splay to make sure that the new root has an empty right child.
    Splay(key);
    // Insert the original right child as the right child of the new
    // root.
    root_->right_ = right;
  }
}

template <typename Config, class B, class Allocator>
void SplayTree<Config, B, Allocator>::Splay(const Key& key) {
  if (is_empty()) return;
  Node dummy_node(Config::kNoKey, Config::NoValue());
  // Create a dummy node.  The use of the dummy node is a bit
  // counter-intuitive: The right child of the dummy node will hold
  // the L tree of the algorithm.  The left child of the dummy node
  // will hold the R tree of the algorithm.  Using a dummy node, left
  // and right will always be nodes and we avoid special cases.
  Node* dummy = &dummy_node;
  Node* left = dummy;
  Node* right = dummy;
  Node* current = root_;
  while (true) {
    int cmp = Config::Compare(key, current->key_);
    if (cmp < 0) {
      if (current->left_ == nullptr) break;
      if (Config::Compare(key, current->left_->key_) < 0) {
        // Rotate right.
        Node* temp = current->left_;
        current->left_ = temp->right_;
        temp->right_ = current;
        current = temp;
        if (current->left_ == nullptr) break;
      }
      // Link right.
      right->left_ = current;
      right = current;
      current = current->left_;
    } else if (cmp > 0) {
      if (current->right_ == nullptr) break;
      if (Config::Compare(key, current->right_->key_) > 0) {
        // Rotate left.
        Node* temp = current->right_;
        current->right_ = temp->left_;
        temp->left_ = current;
        current = temp;
        if (current->right_ == nullptr) break;
      }
      // Link left.
      left->right_ = current;
      left = current;
      current = current->right_;
    } else {
      break;
    }
  }
  // Assemble.
  left->right_ = current->left_;
  right->left_ = current->right_;
  current->left_ = dummy->right_;
  current->right_ = dummy->left_;
  root_ = current;
}

template <typename Config, class B, class Allocator>
template <class Callback>
void SplayTree<Config, B, Allocator>::ForEach(Callback* callback) {
  NodeToPairAdaptor<Callback> callback_adaptor(callback);
  ForEachNode(&callback_adaptor);
}

template <typename Config, class B, class Allocator>
template <class Callback>
void SplayTree<Config, B, Allocator>::ForEachNode(Callback* callback) {
  if (root_ == nullptr) return;
  // Pre-allocate some space for tiny trees.
  std::vector<Node*> nodes_to_visit;
  nodes_to_visit.push_back(root_);
  size_t pos = 0;
  while (pos < nodes_to_visit.size()) {
    Node* node = nodes_to_visit[pos++];
    if (node->left() != nullptr) nodes_to_visit.push_back(node->left());
    if (node->right() != nullptr) nodes_to_visit.push_back(node->right());
    callback->Call(node);
  }
}

}  // namespace dart

#endif  // RUNTIME_PLATFORM_SPLAY_TREE_INL_H_
