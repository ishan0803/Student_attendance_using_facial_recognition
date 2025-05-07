#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <queue>

using namespace std;

// AVL Tree Node structure
struct AVLNode {
    int attendance;     // Key by which we will balance the tree
    vector<int> studentIds;  // Student IDs with this attendance
    int height;
    shared_ptr<AVLNode> left;
    shared_ptr<AVLNode> right;

    AVLNode(int attend, int studentId) 
        : attendance(attend), height(1), left(nullptr), right(nullptr) {
        studentIds.push_back(studentId);
    }
};

class AVLTree {
private:
    shared_ptr<AVLNode> root;

    // Get height of a node
    int getHeight(const shared_ptr<AVLNode>& node) {
        if (!node) return 0;
        return node->height;
    }

    // Get balance factor of a node
    int getBalanceFactor(const shared_ptr<AVLNode>& node) {
        if (!node) return 0;
        return getHeight(node->left) - getHeight(node->right);
    }

    // Update height of a node
    void updateHeight(shared_ptr<AVLNode>& node) {
        if (!node) return;
        node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    }

    // Right rotation
    shared_ptr<AVLNode> rightRotate(shared_ptr<AVLNode> y) {
        shared_ptr<AVLNode> x = y->left;
        shared_ptr<AVLNode> T2 = x->right;

        // Perform rotation
        x->right = y;
        y->left = T2;

        // Update heights
        updateHeight(y);
        updateHeight(x);

        return x;
    }

    // Left rotation
    shared_ptr<AVLNode> leftRotate(shared_ptr<AVLNode> x) {
        shared_ptr<AVLNode> y = x->right;
        shared_ptr<AVLNode> T2 = y->left;

        // Perform rotation
        y->left = x;
        x->right = T2;

        // Update heights
        updateHeight(x);
        updateHeight(y);

        return y;
    }

    // Insert a node into the AVL tree
    shared_ptr<AVLNode> insertNode(shared_ptr<AVLNode> node, int attendance, int studentId) {
        // Standard BST insertion
        if (!node) {
            return make_shared<AVLNode>(attendance, studentId);
        }

        if (attendance < node->attendance) {
            node->left = insertNode(node->left, attendance, studentId);
        } else if (attendance > node->attendance) {
            node->right = insertNode(node->right, attendance, studentId);
        } else {
            // If attendance already exists, add student ID to the list
            // Check if student ID already exists to avoid duplicates
            if (find(node->studentIds.begin(), node->studentIds.end(), studentId) == node->studentIds.end()) {
                node->studentIds.push_back(studentId);
            }
            return node;
        }

        // Update height of current node
        updateHeight(node);

        // Get balance factor to check if this node became unbalanced
        int balance = getBalanceFactor(node);

        // Left Left Case
        if (balance > 1 && attendance < node->left->attendance) {
            return rightRotate(node);
        }

        // Right Right Case
        if (balance < -1 && attendance > node->right->attendance) {
            return leftRotate(node);
        }

        // Left Right Case
        if (balance > 1 && attendance > node->left->attendance) {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }

        // Right Left Case
        if (balance < -1 && attendance < node->right->attendance) {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }

        // Return the unchanged node pointer
        return node;
    }

    // Helper function to serialize the AVL tree using level order traversal
    void serializeHelper(ofstream& outFile, const shared_ptr<AVLNode>& root) {
        if (!root) {
            // Write a marker for null nodes
            int nullMarker = -1;
            outFile.write(reinterpret_cast<const char*>(&nullMarker), sizeof(int));
            return;
        }

        // Write attendance
        outFile.write(reinterpret_cast<const char*>(&root->attendance), sizeof(int));
        
        // Write number of student IDs
        size_t numIds = root->studentIds.size();
        outFile.write(reinterpret_cast<const char*>(&numIds), sizeof(size_t));
        
        // Write student IDs
        for (const auto& id : root->studentIds) {
            outFile.write(reinterpret_cast<const char*>(&id), sizeof(int));
        }
        
        // Write node height
        outFile.write(reinterpret_cast<const char*>(&root->height), sizeof(int));
        
        // Recursively serialize left and right subtrees
        serializeHelper(outFile, root->left);
        serializeHelper(outFile, root->right);
    }

public:
    AVLTree() : root(nullptr) {}

    // Insert a new entry into the AVL tree
    void insert(int attendance, int studentId) {
        root = insertNode(root, attendance, studentId);
    }

    // Serialize the AVL tree to a binary file
    bool serialize(const string& filename) {
        ofstream outFile(filename, ios::binary);
        if (!outFile) {
            cerr << "Error opening file for writing: " << filename << endl;
            return false;
        }

        serializeHelper(outFile, root);
        outFile.close();
        
        return true;
    }

    // Print the AVL tree (in-order traversal) - for debugging purposes
    void printInOrder(const shared_ptr<AVLNode>& node) {
        if (!node) return;
        
        printInOrder(node->left);
        
        cout << "Attendance: " << node->attendance << ", Student IDs: ";
        for (size_t i = 0; i < node->studentIds.size(); ++i) {
            cout << node->studentIds[i];
            if (i < node->studentIds.size() - 1) {
                cout << ", ";
            }
        }
        cout << endl;
        
        printInOrder(node->right);
    }

    void printTree() {
        cout << "AVL Tree (In-order traversal):" << endl;
        printInOrder(root);
    }
};

// Function to read student attendance data from stdin and build the AVL tree
AVLTree buildAVLTree() {
    AVLTree tree;
    int attendance, studentId;
    
    cout << "Enter attendance and student ID pairs (Ctrl+Z or Ctrl+D to end):" << endl;
    cout << "Format: <attendance> <student_id>" << endl;
    
    while (cin >> attendance >> studentId) {
        if (attendance < 0 || attendance > 100) {
            cerr << "Warning: Attendance should be between 0 and 100. Skipping entry." << endl;
            continue;
        }
        tree.insert(attendance, studentId);
    }
    
    // Clear the EOF flag in case we need to use cin again
    cin.clear();
    
    return tree;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <output_dat_file>" << endl;
        return 1;
    }

    const string outFilename = argv[1];
    
    // Build the AVL tree
    AVLTree avlTree = buildAVLTree();
    
    // Print the tree (optional, for debugging)
    avlTree.printTree();
    
    // Serialize the AVL tree
    if (avlTree.serialize(outFilename)) {
        cout << "AVL tree successfully serialized to " << outFilename << endl;
        return 0;
    } else {
        cerr << "Failed to serialize the AVL tree" << endl;
        return 1;
    }
}