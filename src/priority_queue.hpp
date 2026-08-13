#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue
 * should be restored to its original state before the operation began.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
    struct Node {
        T val;
        Node *left;
        Node *right;
        int npl; // null path length

        Node(const T &v) : val(v), left(nullptr), right(nullptr), npl(0) {}
    };

    Node *root;
    size_t size_;
    Compare cmp;

    /**
     * @brief recursively delete all nodes in the tree
     */
    static void deleteTree(Node *node) {
        if (!node) return;
        deleteTree(node->left);
        deleteTree(node->right);
        delete node;
    }

    /**
     * @brief recursively deep-copy a tree
     * This does not use cmp, so no exception from cmp can occur.
     */
    static Node* copyTree(Node *other) {
        if (!other) return nullptr;
        Node *node = new Node(other->val);
        node->left = copyTree(other->left);
        node->right = copyTree(other->right);
        node->npl = other->npl;
        return node;
    }

    /**
     * @brief merge two leftist heaps and return the new root.
     * The merge is naturally exception-safe: cmp is called before any
     * pointer modifications at each recursion level. If cmp throws,
     * the C++ exception mechanism ensures that no assignments from
     * the current or deeper levels have taken effect.
     */
    Node* merge(Node *a, Node *b) {
        if (!a) return b;
        if (!b) return a;
        // cmp may throw here; if it does, no modifications have been made
        if (cmp(a->val, b->val)) {
            Node *tmp = a;
            a = b;
            b = tmp;
        }
        // If cmp above threw, we never reach here.
        // Now a is the larger element (max-heap property).
        a->right = merge(a->right, b);
        // If merge threw, a->right is unchanged (assignment never happened).
        if (!a->left || a->left->npl < a->right->npl) {
            Node *tmp = a->left;
            a->left = a->right;
            a->right = tmp;
        }
        a->npl = a->right ? a->right->npl + 1 : 0;
        return a;
    }

public:
    /**
     * @brief default constructor
     */
    priority_queue() : root(nullptr), size_(0), cmp() {}

    /**
     * @brief copy constructor
     * @param other the priority_queue to be copied
     */
    priority_queue(const priority_queue &other)
        : root(nullptr), size_(0), cmp(other.cmp) {
        root = copyTree(other.root);
        size_ = other.size_;
    }

    /**
     * @brief deconstructor
     */
    ~priority_queue() {
        deleteTree(root);
    }

    /**
     * @brief Assignment operator
     * @param other the priority_queue to be assigned from
     * @return a reference to this priority_queue after assignment
     */
    priority_queue &operator=(const priority_queue &other) {
        if (this == &other) return *this;
        // copy-and-swap idiom for exception safety
        Node *newRoot = copyTree(other.root);
        // copyTree does not use cmp, so it won't throw from cmp.
        // If copyTree throws (e.g., bad_alloc), newRoot is nullptr
        // and we haven't modified *this yet.
        deleteTree(root);
        root = newRoot;
        size_ = other.size_;
        cmp = other.cmp;
        return *this;
    }

    /**
     * @brief get the top element of the priority queue.
     * @return a reference of the top element.
     * @throws container_is_empty if empty() returns true
     */
    const T & top() const {
        if (empty()) throw container_is_empty();
        return root->val;
    }

    /**
     * @brief push new element to the priority queue.
     * @param e the element to be pushed
     */
    void push(const T &e) {
        Node *newNode = new Node(e);
        try {
            root = merge(root, newNode);
        } catch (...) {
            delete newNode;
            throw runtime_error();
        }
        ++size_;
    }

    /**
     * @brief delete the top element from the priority queue.
     * @throws container_is_empty if empty() returns true
     */
    void pop() {
        if (empty()) throw container_is_empty();
        Node *oldRoot = root;
        Node *leftChild = root->left;
        Node *rightChild = root->right;
        try {
            root = merge(leftChild, rightChild);
        } catch (...) {
            // root was not modified (assignment never happened)
            throw runtime_error();
        }
        delete oldRoot;
        --size_;
    }

    /**
     * @brief return the number of elements in the priority queue.
     * @return the number of elements.
     */
    size_t size() const {
        return size_;
    }

    /**
     * @brief check if the container is empty.
     * @return true if it is empty, false otherwise.
     */
    bool empty() const {
        return size_ == 0;
    }

    /**
     * @brief merge another priority_queue into this one.
     * The other priority_queue will be cleared after merging.
     * The complexity is at most O(logn).
     * @param other the priority_queue to be merged.
     */
    void merge(priority_queue &other) {
        // Save old state for potential recovery
        Node *oldMyRoot = root;
        Node *oldOtherRoot = other.root;
        size_t oldMySize = size_;
        size_t oldOtherSize = other.size_;

        try {
            root = merge(root, other.root);
        } catch (...) {
            // Both heaps are intact (merge is exception-safe)
            throw runtime_error();
        }

        size_ = oldMySize + oldOtherSize;
        other.root = nullptr;
        other.size_ = 0;
    }
};

}

#endif
