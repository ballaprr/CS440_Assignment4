#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
using namespace std;

class Record {
public:
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }
};


class LinearHashIndex {

private:
    const int BLOCK_SIZE = 4096;

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;    // Records currently in index. Used to test whether to increase n
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file

    // Insert new record into index
    void insertRecord(Record record) {

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)
            n = 4;

        }

        bitset<i> binary_number = bitset<i>(record.id % 256);

        if (n <= binary_number.to_ullong()) {
            binary_number.flip(i-1);
        }
        int j = binary_number.to_ullong();
        int offset = j * BLOCK_SIZE;
        // Add record to the index in the correct block, creating a overflow block if necessary
        char block[BLOCK_SIZE];
        ifstream inputFile(fName, ios::binary);
        inputFile.seekg(offset);
        inputFile.read(block, BLOCK_SIZE);
        inputFile.close();

        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip


    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexFileName;

        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately

        n = nextFreeBlock + n;
        ofstream outputFile(indexFileName, ios::binary);
        for (int j = 0; j < n; j++) {
            char block[BLOCK_SIZE] = {0};
            outputFile.write(block, BLOCK_SIZE);
            nextFreeBlock += BLOCK_SIZE;
        }
        outputFile.close();
      
    }

    // https://www.youtube.com/watch?v=NFvxA-57LLA&t=626s
    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        ifstream inputFile;
        inputFile.open(csvFName);

        string line = "";
        Record record;
        while (getline(inputFile, line)) {
            vector<string> cols;
            stringstream inputString(line);
            string col;
            while(getline(inputString, col, ',')) {
                cols.push_back(col);
            }
            record = Record(cols);
            insertRecord(record);

        }
        file.close();
        
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        
    }
};
