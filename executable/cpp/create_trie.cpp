#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

using namespace std;

// Trie node structure
struct TrieNode {
    bool isEndOfName;
    unordered_map<char, shared_ptr<TrieNode>> children;
    vector<string> studentIds; // Store IDs at the end of each name

    TrieNode() : isEndOfName(false) {}
};

class Trie {
private:
    shared_ptr<TrieNode> root;

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
        current->studentIds.push_back(studentId);
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
};

// Function to parse CSV data
vector<string> parseCSVLine(const string& line) {
    vector<string> result;
    string current;
    bool inQuotes = false;
    
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    
    // Add the last field
    result.push_back(current);
    return result;
}

int main() {
    // Using fixed filenames
    const string csvFilename = "executable/data/students.csv";
    const string trieFilename = "executable/serialized/name.dat";
    
    // Open CSV file
    ifstream csvFile(csvFilename);
    if (!csvFile) {
        cerr << "Error opening CSV file: " << csvFilename << endl;
        return 1;
    }
    
    Trie trie;
    string line;
    bool isFirstLine = true; // To skip header if present
    
    while (getline(csvFile, line)) {
        if (isFirstLine) {
            isFirstLine = false;
            // Uncomment the following line if the CSV has a header row
            // continue;
        }
        
        auto fields = parseCSVLine(line);
        if (fields.size() >= 3) {
            const string& name = fields[1];
            const string& studentId = fields[0];
            trie.insert(name, studentId);
        }
    }
    
    // Serialize the trie
    if (trie.serialize(trieFilename)) {
        cout << "Trie has been successfully serialized to " << trieFilename << endl;
        return 0;
    } else {
        cerr << "Failed to serialize the trie" << endl;
        return 1;
    }
}