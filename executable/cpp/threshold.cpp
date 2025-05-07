#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>

using namespace std;

// AVL Tree Node structure (same as in create_attendance_avl.cpp)
struct AVLNode {
    int attendance;     // Key by which the tree is balanced
    vector<int> studentIds;  // Student IDs with this attendance
    int height;
    shared_ptr<AVLNode> left;
    shared_ptr<AVLNode> right;

    AVLNode(int attend) 
        : attendance(attend), height(1), left(nullptr), right(nullptr) {}
};

class AVLTree {
private:
    shared_ptr<AVLNode> root;

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

    // Helper function to collect student IDs based on threshold
    void collectStudentIds(const shared_ptr<AVLNode>& node, 
                          int threshold, 
                          int direction, 
                          map<int, vector<int>, greater<int>>& result) {
        if (!node) return;
        
        // Process this node if it meets the condition
        if ((direction > 0 && node->attendance >= threshold) || 
            (direction < 0 && node->attendance <= threshold)) {
            result[node->attendance].insert(result[node->attendance].end(), 
                                           node->studentIds.begin(), 
                                           node->studentIds.end());
        }
        
        // Traverse the tree - in-order traversal
        collectStudentIds(node->left, threshold, direction, result);
        collectStudentIds(node->right, threshold, direction, result);
    }

public:
    AVLTree() : root(nullptr) {}

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

    // Get student IDs above or below a threshold in descending order of attendance
    vector<int> getStudentIdsByThreshold(int threshold, int direction) {
        map<int, vector<int>, greater<int>> attendanceMap; // Uses descending order
        collectStudentIds(root, threshold, direction, attendanceMap);
        
        // Flatten the map into a vector of student IDs
        vector<int> result;
        for (const auto& [attendance, ids] : attendanceMap) {
            result.insert(result.end(), ids.begin(), ids.end());
        }
        
        return result;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <dat_file_name> <threshold> <direction>" << endl;
        cerr << "  direction: 1 for above threshold, -1 for below threshold" << endl;
        return 1;
    }

    const string datFilename = argv[1];
    int threshold;
    int direction;
    
    try {
        threshold = stoi(argv[2]);
        direction = stoi(argv[3]);
        
        if (direction != 1 && direction != -1) {
            cerr << "Direction must be 1 (above) or -1 (below)" << endl;
            return 1;
        }
    } catch (const exception& e) {
        cerr << "Error parsing arguments: " << e.what() << endl;
        return 1;
    }
    
    AVLTree avlTree;
    
    // Check if the file exists
    ifstream fileCheck(datFilename);
    if (!fileCheck) {
        cerr << "File not found: " << datFilename << endl;
        return 1;
    }
    fileCheck.close();
    
    // Deserialize the AVL tree
    if (!avlTree.deserialize(datFilename)) {
        cerr << "Failed to deserialize the AVL tree from " << datFilename << endl;
        return 1;
    }
    
    // Get student IDs based on threshold and direction
    vector<int> studentIds = avlTree.getStudentIdsByThreshold(threshold, direction);
    
    // Output student IDs
    if (studentIds.empty()) {
        cout << "0" << endl; // No students meet the criteria
    } else {
        for (const auto& id : studentIds) {
            cout << id << endl;
        }
    }
    
    return 0;
}