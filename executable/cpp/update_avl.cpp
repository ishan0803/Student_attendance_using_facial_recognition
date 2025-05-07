#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

using namespace std;

// AVL Tree Node structure (same as in previous programs)
struct AVLNode {
    int attendance;     // Key by which the tree is balanced
    vector<int> studentIds;  // Student IDs with this attendance
    int height;
    shared_ptr<AVLNode> left;
    shared_ptr<AVLNode> right;

    AVLNode(int attend) 
        : attendance(attend), height(1), left(nullptr), right(nullptr) {}
        
    AVLNode(int attend, int studentId) 
        : attendance(attend), height(1), left(nullptr), right(nullptr) {
        studentIds.push_back(studentId);
    }
};

class AVLTree {
private:
    shared_ptr<AVLNode> root;
    bool studentFound;  // Flag to track if student ID was found during update

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

    // Helper function to deserialize the AVL tree
    shared_ptr<AVLNode> deserializeHelper(ifstream& inFile) {
        // Read attendance value
        int attendance;
        inFile.read(reinterpret_cast<char*>(&attendance), sizeof(int));
        
        // Check for null node marker
        if (attendance == -1) {
            return nullptr;
        }
        
        // Create a new node
        auto node = make_shared<AVLNode>(attendance);
        
        // Read number of student IDs
        size_t numIds;
        inFile.read(reinterpret_cast<char*>(&numIds), sizeof(size_t));
        
        // Read student IDs
        for (size_t i = 0; i < numIds; ++i) {
            int studentId;
            inFile.read(reinterpret_cast<char*>(&studentId), sizeof(int));
            node->studentIds.push_back(studentId);
        }
        
        // Read node height
        inFile.read(reinterpret_cast<char*>(&node->height), sizeof(int));
        
        // Recursively deserialize left and right subtrees
        node->left = deserializeHelper(inFile);
        node->right = deserializeHelper(inFile);
        
        return node;
    }

    // Helper function to serialize the AVL tree
    void serializeHelper(ofstream& outFile, const shared_ptr<AVLNode>& node) {
        if (!node) {
            // Write a marker for null nodes
            int nullMarker = -1;
            outFile.write(reinterpret_cast<const char*>(&nullMarker), sizeof(int));
            return;
        }

        // Write attendance
        outFile.write(reinterpret_cast<const char*>(&node->attendance), sizeof(int));
        
        // Write number of student IDs
        size_t numIds = node->studentIds.size();
        outFile.write(reinterpret_cast<const char*>(&numIds), sizeof(size_t));
        
        // Write student IDs
        for (const auto& id : node->studentIds) {
            outFile.write(reinterpret_cast<const char*>(&id), sizeof(int));
        }
        
        // Write node height
        outFile.write(reinterpret_cast<const char*>(&node->height), sizeof(int));
        
        // Recursively serialize left and right subtrees
        serializeHelper(outFile, node->left);
        serializeHelper(outFile, node->right);
    }

    // Helper function to find and remove a student ID from the tree
    shared_ptr<AVLNode> removeStudentId(shared_ptr<AVLNode> node, int studentId) {
        if (!node) return nullptr;
        
        // Check current node
        auto it = find(node->studentIds.begin(), node->studentIds.end(), studentId);
        if (it != node->studentIds.end()) {
            studentFound = true;
            node->studentIds.erase(it);
            
            // If this node has no more student IDs, remove it from the tree
            if (node->studentIds.empty()) {
                return removeNode(node);
            }
            return node;
        }
        
        // Search both subtrees
        node->left = removeStudentId(node->left, studentId);
        node->right = removeStudentId(node->right, studentId);
        
        // Rebalance the tree if needed
        return balanceNode(node);
    }

    // Remove a node from the tree (used when a node has no more student IDs)
    shared_ptr<AVLNode> removeNode(shared_ptr<AVLNode> node) {
        // Node with only one child or no child
        if (!node->left) return node->right;
        if (!node->right) return node->left;
        
        // Node with two children: Get the inorder successor (smallest in the right subtree)
        shared_ptr<AVLNode> successor = findMin(node->right);
        
        // Copy successor data to this node
        node->attendance = successor->attendance;
        node->studentIds = successor->studentIds;
        
        // Delete the successor
        node->right = removeNode(successor);
        
        // Update height and balance
        return balanceNode(node);
    }

    // Find the node with minimum attendance in a subtree
    shared_ptr<AVLNode> findMin(shared_ptr<AVLNode> node) {
        if (!node) return nullptr;
        while (node->left) {
            node = node->left;
        }
        return node;
    }

    // Balance a node after insertion or deletion
    shared_ptr<AVLNode> balanceNode(shared_ptr<AVLNode> node) {
        if (!node) return nullptr;
        
        // Update height
        updateHeight(node);
        
        // Get balance factor
        int balance = getBalanceFactor(node);
        
        // Left Left Case
        if (balance > 1 && getBalanceFactor(node->left) >= 0) {
            return rightRotate(node);
        }
        
        // Left Right Case
        if (balance > 1 && getBalanceFactor(node->left) < 0) {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }
        
        // Right Right Case
        if (balance < -1 && getBalanceFactor(node->right) <= 0) {
            return leftRotate(node);
        }
        
        // Right Left Case
        if (balance < -1 && getBalanceFactor(node->right) > 0) {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }
        
        return node;
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

        // Balance the tree
        return balanceNode(node);
    }

public:
    AVLTree() : root(nullptr), studentFound(false) {}

    // Deserialize the AVL tree from a binary file
    bool deserialize(const string& filename) {
        ifstream inFile(filename, ios::binary);
        if (!inFile) {
            cerr << "Error opening file for reading: " << filename << endl;
            return false;
        }

        root = deserializeHelper(inFile);
        inFile.close();
        
        return true;
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

    // Update the attendance for a student ID
    bool updateAttendance(int studentId, int newAttendance) {
        studentFound = false;
        
        // First, remove the student ID from its current location
        root = removeStudentId(root, studentId);
        
        // Insert the student ID with the new attendance
        root = insertNode(root, newAttendance, studentId);
        
        return studentFound;
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

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <dat_file_name> <new_attendance> <student_id>" << endl;
        return 1;
    }

    const string datFilename = argv[1];
    int newAttendance, studentId;
    
    try {
        newAttendance = stoi(argv[2]);
        studentId = stoi(argv[3]);
        
        if (newAttendance < 0 || newAttendance > 100) {
            cerr << "Attendance should be between 0 and 100" << endl;
            return 1;
        }
    } catch (const exception& e) {
        cerr << "Error parsing arguments: " << e.what() << endl;
        return 1;
    }
    
    AVLTree avlTree;
    
    // Check if the file exists
    ifstream fileCheck(datFilename);
    bool fileExists = fileCheck.good();
    fileCheck.close();
    
    if (fileExists) {
        // Deserialize the AVL tree
        if (!avlTree.deserialize(datFilename)) {
            cerr << "Failed to deserialize the AVL tree from " << datFilename << endl;
            return 1;
        }
        
        // Update the attendance for the student ID
        bool studentUpdated = avlTree.updateAttendance(studentId, newAttendance);
        
        if (studentUpdated) {
            cout << "Updated attendance for student ID " << studentId << " to " << newAttendance << endl;
        } else {
            cout << "Student ID " << studentId << " not found. Inserted new entry with attendance " << newAttendance << endl;
        }
    } else {
        // File doesn't exist, create a new tree and insert the student
        cout << "File not found. Creating new AVL tree." << endl;
        avlTree.updateAttendance(studentId, newAttendance);
        cout << "Inserted student ID " << studentId << " with attendance " << newAttendance << endl;
    }
    
    // Serialize the updated AVL tree
    if (avlTree.serialize(datFilename)) {
        cout << "AVL tree successfully serialized to " << datFilename << endl;
    } else {
        cerr << "Failed to serialize the updated AVL tree" << endl;
        return 1;
    }
    
    // Uncomment for debugging
    avlTree.printTree();
    
    return 0;
}