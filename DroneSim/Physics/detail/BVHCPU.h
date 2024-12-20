#pragma once
#include "Physics/PhysicsStructures.h"
#include <vector>
#include <memory>

class BVH {
private:
    struct Node {
        AABB bounds;
        int bodyIndex = -1;  // -1 for internal nodes
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        Node* parent = nullptr;

        bool isLeaf() const { return bodyIndex != -1; }
    };

public:
    std::unique_ptr<Node> root;
    std::vector<Node*> leafNodes;  // Quick access to body nodes

    void remove(int index) {
        if (index >= (int)leafNodes.size() || !leafNodes[index]) return;
        removeNode(leafNodes[index]);
        leafNodes[index] = nullptr;
    }

    void insert(int index, AABB aabb) {
        if (index >= (int)leafNodes.size()) leafNodes.resize(index + 1, nullptr);
        auto newNode = std::make_unique<Node>();
        newNode->bounds = aabb;
        newNode->bodyIndex = index;

        if (!root) {
            root = std::move(newNode);
            leafNodes[index] = root.get();
            return;
        }

        Node* insertPos = findBestInsertionPoint(root.get(), aabb);
        insertNodeAt(std::move(newNode), insertPos);
        leafNodes[index] = findLeafNode(index);
    }

    std::vector<int> query(const AABB& queryBox) {
        std::vector<int> result;
        if (root) queryNode(root.get(), queryBox, result);
        return result;
    }

    Node* findLeafNode(int index) {
        if (!root) return nullptr;
        return findLeafNodeRecursive(root.get(), index);
    }

    void removeNode(Node* node) {
        if (!node || !node->parent) return;
        
        Node* parent = node->parent;
        Node* sibling = parent->left.get() == node ? 
            parent->right.get() : parent->left.get();

        // Move sibling up to replace parent
        if (!parent->parent) {
            // Parent is root
            if (parent->left.get() == node) {
                root = std::move(parent->right);
            } else {
                root = std::move(parent->left);
            }
            root->parent = nullptr;
        } else {
            Node* grandparent = parent->parent;
            if (grandparent->left.get() == parent) {
                if (parent->left.get() == node) {
                    grandparent->left = std::move(parent->right);
                } else {
                    grandparent->left = std::move(parent->left);
                }
                grandparent->left->parent = grandparent;
            } else {
                if (parent->left.get() == node) {
                    grandparent->right = std::move(parent->right);
                } else {
                    grandparent->right = std::move(parent->left);
                }
                grandparent->right->parent = grandparent;
            }
            refit(grandparent);
        }
    }

    void insertNodeAt(std::unique_ptr<Node> newNode, Node* insertPos) {
        if (!insertPos) return;

        Node* oldParent = insertPos->parent;

        auto newParent = std::make_unique<Node>();
        newParent->bounds = expandAABB(insertPos->bounds, newNode->bounds);
        newParent->parent = oldParent;

        // Before we move anything, store a stable pointer to the node we are inserting at
        Node* insertPosNode = insertPos;

        // Extract insertPos from oldParent or root
        std::unique_ptr<Node> insertPosPtr;
        if (!oldParent) {
            // insertPos was root
            insertPosPtr = std::move(root);
        } else {
            // Identify which child of oldParent is insertPos and move it
            if (oldParent->left.get() == insertPos) {
                insertPosPtr = std::move(oldParent->left);
            } else {
                insertPosPtr = std::move(oldParent->right);
            }
        }

        // Now insertPosPtr is the unique_ptr that owned insertPosNode
        // insertPosNode is valid again via insertPosPtr.get()

        // Set the parents of the nodes under the new parent
        insertPosPtr.get()->parent = newParent.get();
        newNode->parent = newParent.get();

        newParent->left = std::move(insertPosPtr);
        newParent->right = std::move(newNode);

        // Attach newParent back to oldParent or make it root
        if (!oldParent) {
            // We're inserting at the root level
            root = std::move(newParent);
        } else {
            // Attach newParent in place of insertPos in oldParent
            if (oldParent->left.get() == nullptr) {
                oldParent->left = std::move(newParent);
            } else if (oldParent->right.get() == nullptr) {
                oldParent->right = std::move(newParent);
            } else {
                // If oldParent had both children set, make sure the logic to replace
                // the correct one is done above. Usually, you know which child you replaced
                // and reassign that one.
            }
        }

        // Now we call refit up the tree. If the newParent doesn't have a parent (it's root),
        // then there's no need to refit.
        Node* grandparent = (oldParent ? oldParent->parent : nullptr);
        if (grandparent) {
            refit(grandparent);
        }
    }


private:
    void refit(BVH::Node* node) {
        while (node) {
            if (node->left && node->right) {
                node->bounds = expandAABB(node->left->bounds, node->right->bounds);
            }
            node = node->parent;
        }
    }

    void queryNode(Node* node, const AABB& queryBox, std::vector<int>& result) {
        if (!node->bounds.overlaps(queryBox)) return;
        
        if (node->isLeaf()) {
            result.push_back(node->bodyIndex);
        } else {
            if (node->left) queryNode(node->left.get(), queryBox, result);
            if (node->right) queryNode(node->right.get(), queryBox, result);
        }
    }

    Node* findBestInsertionPoint(Node* node, const AABB& newBounds) {
        if (node->isLeaf()) return node;

        AABB combinedLeft = node->left ? expandAABB(node->left->bounds, newBounds) : newBounds;
        AABB combinedRight = node->right ? expandAABB(node->right->bounds, newBounds) : newBounds;

        float costLeft = surfaceArea(combinedLeft);
        float costRight = surfaceArea(combinedRight);

        if (costLeft < costRight) {
            return findBestInsertionPoint(node->left.get(), newBounds);
        } else {
            return findBestInsertionPoint(node->right.get(), newBounds);
        }
    }

    Node* findLeafNodeRecursive(Node* node, int index) {
        if (!node) return nullptr;
        if (node->isLeaf()) {
            return node->bodyIndex == index ? node : nullptr;
        }
        
        Node* found = findLeafNodeRecursive(node->left.get(), index);
        if (found) return found;
        return findLeafNodeRecursive(node->right.get(), index);
    }

    friend void refit(Node* node);
};