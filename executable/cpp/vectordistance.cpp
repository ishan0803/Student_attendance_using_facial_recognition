#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <string>
#include <unordered_map>
#include <limits>

using namespace std;

float smallestDistance = numeric_limits<float>::max(); 

// Euclidean distance
double calculateDistance(const vector<double>& v1, const vector<double>& v2) {
    double sum = 0.0;
    for (size_t i = 0; i < v1.size(); ++i){
        double diff = v1[i] - v2[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

// Hashmap
unordered_map<double, string> computeDistances(const vector<double>& inputVector, const vector<vector<double>>& faceVectors, const vector<string>& studentIds) {
    unordered_map<double, string> distancesMap;
    int i = 0;
    float n;
    for (const auto& vec : faceVectors) {
        if (vec.size() != inputVector.size()) {
            distancesMap.insert({-1, "Error"}); 
        } else {
            n = calculateDistance(inputVector, vec);
            if (n < smallestDistance) {
                smallestDistance = n;
            }
            distancesMap.insert({n, studentIds[i]});
        }
        i++;
    }
    return distancesMap;
}

int main(int argc, char* argv[]) {
    if (argc != 129) {
        cerr << "Usage: " << argv[0] << " <128 double values>\n";
        return 1;
    }

    ifstream file("executable/data/students.csv");
    if (!file.is_open()) {
        cerr << "Could not open students.csv file.\n";
        return 1;
    }

    string line;
    vector<string> studentIds;
    vector<vector<double>> faceVectors;

    getline(file, line); 

    while (getline(file, line)) {
        stringstream ss(line);
        string name, rn, student_id, vector_str;
        
        getline(ss, student_id, ',');
        getline(ss, name, ',');
        getline(ss, rn, ',');
        getline(ss, vector_str);

        if (!vector_str.empty() && vector_str.front() == '"') vector_str.erase(0, 1);
        if (!vector_str.empty() && vector_str.back() == '"') vector_str.pop_back();

        vector<double> vec;
        stringstream vectorStream(vector_str);
        string val;
        while (getline(vectorStream, val, ',')) {
            vec.push_back(stod(val));
        }

        studentIds.push_back(student_id);
        faceVectors.push_back(vec);
    }

    file.close();

    vector<double> inputVector;
    for (int i = 1; i <= 128; ++i) {
        try {
            inputVector.push_back(stod(argv[i]));
        } catch (invalid_argument& e) {
            cerr << "Invalid number at position " << i << ": " << argv[i] << "\n";
            return 1;
        }
    }

    // distance function
    unordered_map<double, string> distances = computeDistances(inputVector, faceVectors, studentIds);

    // Print results
    if(smallestDistance < 0.6){
        cout << distances[smallestDistance];
    }
    else{
        cout << "-1";
    }
    return 0;
}
