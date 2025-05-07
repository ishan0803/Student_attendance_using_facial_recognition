#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

using namespace std;

// Trie node structure (same as in create_trie.cpp)
struct TrieNode {
    bool isEndOfName;
    unordered_map<char, shared_ptr<TrieNode>> children;
    vector<string> studentIds;

    TrieNode() : isEndOfName(false) {}
};

class Trie {
private:
    shared_ptr<TrieNode> root;

    // Helper function to deserialize the trie
    shared_ptr<TrieNode> deserializeHelper(ifstream& inFile) {
        auto node = make_shared<TrieNode>();
        
        // Read if this node marks the end of a name
        inFile.read(reinterpret_cast<char*>(&node->isEndOfName), sizeof(bool));
        
        // Read number of student IDs at this node
        size_t numIds;
        inFile.read(reinterpret_cast<char*>(&numIds), sizeof(size_t));
        
        // Read each student ID
        for (size_t i = 0; i < numIds; ++i) {
            size_t idLength;
            inFile.read(reinterpret_cast<char*>(&idLength), sizeof(size_t));
            
            string id(idLength, '\0');
            inFile.read(&id[0], idLength);
            node->studentIds.push_back(id);
        }
        
        // Read number of children
        size_t numChildren;
        inFile.read(reinterpret_cast<char*>(&numChildren), sizeof(size_t));
        
        // Read each child
        for (size_t i = 0; i < numChildren; ++i) {
            char ch;
            inFile.read(&ch, sizeof(char));
            
            // Recursively deserialize the child node
            node->children[ch] = deserializeHelper(inFile);
        }
        
        return node;
    }

    // Helper function to find all student IDs with a given prefix
    void findStudentIdsWithPrefix(const shared_ptr<TrieNode>& node, 
                                  const string& prefix, 
                                  string currentPrefix,
                                  vector<pair<string, vector<string>>>& results) {
        if (node->isEndOfName) {
            results.push_back({currentPrefix, node->studentIds});
        }
        
        for (const auto& [ch, childNode] : node->children) {
            findStudentIdsWithPrefix(childNode, prefix, currentPrefix + ch, results);
        }
    }

public:
    Trie() {
        root = make_shared<TrieNode>();
    }

    // Deserialize the trie from a binary file
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

    // Search for student IDs with a given name prefix
    vector<pair<string, vector<string>>> searchByPrefix(const string& prefix) {
        vector<pair<string, vector<string>>> results;
        
        // Navigate to the node corresponding to the prefix
        shared_ptr<TrieNode> current = root;
        for (char c : prefix) {
            if (current->children.find(c) == current->children.end()) {
                // Prefix not found
                return results;
            }
            current = current->children[c];
        }
        
        // Collect all student IDs with the given prefix
        findStudentIdsWithPrefix(current, prefix, prefix, results);
        return results;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <name_prefix>" << endl;
        return 1;
    }

    // Using fixed filename for the serialized trie
    const string trieFilename = "executable/serialized/name.dat";
    const string prefix = argv[1];
    
    // Check if the file exists and is not empty
    ifstream fileCheck(trieFilename, ios::binary | ios::ate);
    if (!fileCheck || fileCheck.tellg() == 0) {
        cout << "-1" << endl; // File doesn't exist or is empty
        return 1;
    }
    fileCheck.close();
    
    Trie trie;
    
    // Deserialize the trie
    if (!trie.deserialize(trieFilename)) {
        cout << "-1" << endl; // Failed to deserialize
        return 1;
    }
    
    // Search for student IDs with the given prefix
    auto results = trie.searchByPrefix(prefix);
    if (results.empty()) {
        cout << "0" << endl; // No students found with the given prefix
    } else {
        // Print only the student IDs
        for (const auto& [name, ids] : results) {
            for (const auto& id : ids) {
                cout << id << endl;
            }
        }
    }
    
    return 0;
}