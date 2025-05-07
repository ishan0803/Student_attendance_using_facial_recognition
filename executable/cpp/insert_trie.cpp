#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

using namespace std;  

// Trie node structure (consistent with the other programs)
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

    // Helper function to serialize the trie
    void serializeHelper(ofstream& outFile, const shared_ptr<TrieNode>& node) {
        // Write if this node marks the end of a name
        outFile.write(reinterpret_cast<const char*>(&node->isEndOfName), sizeof(bool));
        
        // Write number of student IDs at this node
        size_t numIds = node->studentIds.size();
        outFile.write(reinterpret_cast<const char*>(&numIds), sizeof(size_t));
        
        // Write each student ID
        for (const auto& id : node->studentIds) {
            size_t idLength = id.length();
            outFile.write(reinterpret_cast<const char*>(&idLength), sizeof(size_t));
            outFile.write(id.c_str(), idLength);
        }
        
        // Write number of children
        size_t numChildren = node->children.size();
        outFile.write(reinterpret_cast<const char*>(&numChildren), sizeof(size_t));
        
        // Write each child
        for (const auto& [ch, childNode] : node->children) {
            // Write the character
            outFile.write(&ch, sizeof(char));
            
            // Recursively serialize the child node
            serializeHelper(outFile, childNode);
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

    // Serialize the trie to a binary file
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

    // Insert a name and its student ID into the trie
    void insert(const string& name, const string& studentId) {
        shared_ptr<TrieNode> current = root;
        
        for (char c : name) {
            if (current->children.find(c) == current->children.end()) {
                current->children[c] = make_shared<TrieNode>();
            }
            current = current->children[c];
        }
        
        current->isEndOfName = true;
        
        // Check if this student ID already exists to avoid duplicates
        bool idExists = false;
        for (const auto& id : current->studentIds) {
            if (id == studentId) {
                idExists = true;
                break;
            }
        }
        
        if (!idExists) {
            current->studentIds.push_back(studentId);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <student_name> <student_id>" << endl;
        return 1;
    }

    const string name = argv[1];
    const string studentId = argv[2];
    const string trieFilename = "executable/serialized/name.dat";
    
    Trie trie;
    
    // Check if the file exists and is not empty
    ifstream fileCheck(trieFilename, ios::binary | ios::ate);
    if (!fileCheck || fileCheck.tellg() == 0) {
        // Create a new trie if the file doesn't exist or is empty
        fileCheck.close();
        
        // Create the serialized directory if it doesn't exist
        string dir = "serialized";
        #ifdef _WIN32
            system(("mkdir " + dir + " 2>nul").c_str());
        #else
            system(("mkdir -p " + dir).c_str());
        #endif
        
        cout << "Creating new trie..." << endl;
    } else {
        fileCheck.close();
        
        // Load the existing trie
        if (!trie.deserialize(trieFilename)) {
            cerr << "Failed to deserialize the trie from " << trieFilename << endl;
            return 1;
        }
        cout << "Existing trie loaded successfully." << endl;
    }
    
    // Insert the new name and student ID
    trie.insert(name, studentId);
    cout << "Inserted name: " << name << " with student ID: " << studentId << endl;
    
    // Serialize the updated trie
    if (trie.serialize(trieFilename)) {
        cout << "Trie has been successfully updated and serialized." << endl;
        return 0;
    } else {
        cerr << "Failed to serialize the updated trie" << endl;
        return 1;
    }
}