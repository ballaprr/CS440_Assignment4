#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <cstdlib>
using namespace std;

int int_pow(int base, int exp) {
    int output = 1;
    for (int i = 0; i < exp; i++) {
        output *= base;
    }
    return output;
}

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

class Block {
private:
    const int BLOCK_SIZE = 4096;

public:
    int nextBuffer = 0;     // The first buffer will never be an overflow buffer, and this allows an "if (nextBuffer)" check.
    int numRecords = 0;
    vector<Record> records;

    // Given the filestream and block address, move to and get a given block.
    void fetchFromFile(fstream& indFile, int blockAddr) {
        char* temp_block_charp;
        indFile.seekg(blockAddr);
        indFile.read(temp_block_charp, BLOCK_SIZE);

        std::string temp_block(temp_block_charp);

        std::string numRecordsStr(temp_block, BLOCK_SIZE - 16, 8);
        numRecords = stoi(numRecordsStr);
        for (int i = 0; i < numRecords; i++) {
            int record_offset = i * 716;
            vector<std::string> fields;

            std::string eid(temp_block, record_offset, 8);
            fields.push_back(eid);
            std::string ename(temp_block, record_offset, 200);
            fields.push_back(ename);
            std::string ebio(temp_block, record_offset, 500);
            fields.push_back(ebio);
            std::string emid(temp_block, record_offset, 8);
            fields.push_back(emid);

            Record emp(fields);
            records[i] = emp;
        }

        std::string nextBufferStr(temp_block, BLOCK_SIZE - 8, 8);
        nextBuffer = stoi(nextBufferStr);
    }

    void insertRecord(Record record) {
        records[numRecords] = record;
        numRecords++;
    }

    void clear() {
        records.clear();
        numRecords = 0;
        nextBuffer = 0;
    }

    // Given the filestream and current block, fetch an overflow block.
    void fetchNext(fstream& indFile) {
        if (nextBuffer) {
            fetchFromFile(indFile, nextBuffer);
        }
        else {
            cout << "ERR: Attempted to fetch next file without nextBuffer set!";
        }
    }

    // Format the current info as a char array of size BLOCK_SIZE and write to
    // the passed position in the file.
    void writeBlock(fstream& indFile) {
        // Initialise variables
        char writeBuffer[BLOCK_SIZE];
        int i;
        char* temp;

        // Copy all records into the string
        for (i = 0; i < numRecords; i++) {
            int j;

            // Copy employee id
            snprintf(temp, 9, "%d", records[i].id);
            for (j = 0; j < 8 - strlen(temp); j++) {
                writeBuffer[i * BLOCK_SIZE + j] = '0';
            }
            strncpy(&writeBuffer[i * BLOCK_SIZE + j], temp, strlen(temp));
            
            // Copy employee name
            records[i].name.copy(&writeBuffer[i * BLOCK_SIZE + 8], 200);

            // Copy employee bio
            records[i].bio.copy(&writeBuffer[i * BLOCK_SIZE + 208], 500);

            // Copy manager id
            snprintf(temp, 9, "%d", records[i].manager_id);
            for (j = 0; j < 8 - strlen(temp); j++) {
                writeBuffer[i * BLOCK_SIZE + j] = '0';
            }
            strncpy(&writeBuffer[i * BLOCK_SIZE + 708 + j], temp, strlen(temp));
        }

        snprintf(temp, 9, "%d", numRecords);
        for (i = 0; i < 8 - strlen(temp); i++) {
            writeBuffer[BLOCK_SIZE - 16 + i] = '0';
        }
        strncpy(&writeBuffer[BLOCK_SIZE - 16 + i], temp, strlen(temp));

        snprintf(temp, 9, "%d", nextBuffer);
        for (i = 0; i < 8 - strlen(temp); i++) {
            writeBuffer[BLOCK_SIZE - 8 + i] = '0';
        }
        strncpy(&writeBuffer[BLOCK_SIZE - 8 + i], temp, strlen(temp));

        indFile.write(writeBuffer, BLOCK_SIZE);
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
        fstream indexedFile;
        indexedFile.open(fName, ios::out | ios::in);

        // No records written to index yet
        // This loop uses 2 blocks (one of class Block, one temporary string to write Block)
        if (numRecords == 0) {
            // Initialise blockDirectory to hold up to 256 elements
            blockDirectory.reserve(256);

            // Initialize index with first blocks (start with 4)
            for (int iter = 0; iter < 4; iter++) {
                blockDirectory[iter] = nextFreeBlock;
                nextFreeBlock += BLOCK_SIZE;

                // Place an empty block in the designated spot.
                Block emptyBlock;
                indexedFile.seekg(blockDirectory[iter]);
                emptyBlock.writeBlock(indexedFile);
            }
            n = 4;
            i = 2;
        }

        // Add record to the index in the correct block, creating an overflow block if necessary
        // Adding a record to the index uses at most 2 blocks (one of class Block, one temporary string
        // to write Block) - this section may be entered by the next section's recursive call for a
        // total of 3 blocks in use.
        //
        // Get bits relevant to block address
        int hash = record.id % 256;
        int sigbits = hash % int_pow(2, i);
        // Bitflip if necessary.
        if (sigbits >= n) {
            sigbits = sigbits % int_pow(2, i - 1);
        }
        int targetBlockAddr = blockDirectory[sigbits];
        Block tempBlock;
        tempBlock.fetchFromFile(indexedFile, targetBlockAddr);
        // Get a block with at least one free space
        // 
        while (tempBlock.numRecords == 5) {
            if (tempBlock.nextBuffer) {
                tempBlock.fetchNext(indexedFile);
            }
            else {
                // Set the current block's nextBuffer to nextFreeBlock.
                tempBlock.nextBuffer = nextFreeBlock;
                tempBlock.writeBlock(indexedFile);

                // Go to the next block and increment nextFreeBlock.
                indexedFile.seekg(nextFreeBlock);
                nextFreeBlock += BLOCK_SIZE;

                // Clear tempBlock's data.
                tempBlock.clear();
            }
        }
        tempBlock.insertRecord(record);
        tempBlock.writeBlock(indexedFile);

        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        if (++numRecords >= 0.7 * 5 * n) {
            // Increase n; if n is greater than 2^i, increase i
            if (++n > int_pow(2, i)) {
                i++;
            }

            blockDirectory[n - 1] = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;

            // Place an empty block in the designated spot.
            tempBlock.clear();
            indexedFile.seekg(blockDirectory[n - 1]);
            tempBlock.writeBlock(indexedFile);

            // Get the bucket that is the new largest bucket with the most significant bit turned off
            int bucketToCheck = (n - 1) % int_pow(2, i - 1);
            tempBlock.fetchFromFile(indexedFile, blockDirectory[bucketToCheck]);
            int iter = 0;
            while (iter < tempBlock.numRecords) {
                // Grab a record from the bucket being checked
                Record recordChecked = tempBlock.records[iter];
                // If that record should go in the new one ...
                if (recordChecked.id % int_pow(2, i) == n - 1) {
                    // Erase it from the current bucket
                    tempBlock.records.erase(tempBlock.records.begin() + iter);
                    // Decrement numRecords (it will be increased in the next line and we don't want
                    // to double-count moved records)
                    numRecords--;
                    // Insert the record. This will use at most 2 blocks at any given time, plus the
                    // 1 block (tempBlock) at the current depth. n should never be increased more than
                    // once per insertion, so no more than 3 blocks should be in memory at any given
                    // time (1 at depth 0, 2 at depth 1)
                    cout << "Placing record in new spot...";
                    insertRecord(recordChecked);
                    cout << "Done!";
                }
                // Increment iter; if there are no more records in this bucket and an overflow bucket
                // exists, go to that and set iter to 0
                if (++iter == tempBlock.numRecords && tempBlock.nextBuffer != 0) {
                    tempBlock.fetchNext(indexedFile);
                    iter = 0;
                }
            }
            indexedFile.close();
        }
    }

public:
    LinearHashIndex(string indexedFileName) {
        cout << "0";
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexedFileName;

        cout << "1";

        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
        fstream indexedFile;
        indexedFile.open(fName, ios::out | ios::in | ios::trunc);
        for (int iter = 0; iter < 4; iter++) {
            blockDirectory[iter] = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;

            // Place an empty block in the designated spot.
            Block emptyBlock;
            indexedFile.seekg(blockDirectory[iter]);
            emptyBlock.writeBlock(indexedFile);
        }

        cout << "2";
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        // Open csv file
        fstream csv_file;
        csv_file.open(csvFName, ios::in);

        cout << "3";

        // For each entry in the csv file, create and insert a record into the index
        std::string line, word;
        while (getline(csv_file, line, '\n')) {
            vector<std::string> fields;
            stringstream s(line);
            getline(s, word, ',');
            fields.push_back(word);
            getline(s, word, ',');
            fields.push_back(word);
            getline(s, word, ',');
            fields.push_back(word);
            getline(s, word, ',');
            fields.push_back(word);

            Record emp(fields);

            cout << "4";

            insertRecord(emp);
        }

        cout << "5";

        csv_file.close();
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        fstream indexedFile;
        indexedFile.open(fName, ios::in);

        // Determine bucket based off i least significant bits
        int bucketToCheck = id % int_pow(2, i);
        // Bitflip leading bit if necessary
        if (bucketToCheck >= n) {
            bucketToCheck = id % int_pow(2, i - 1);
        }

        Block tempBlock;
        tempBlock.fetchFromFile(indexedFile, blockDirectory[bucketToCheck]);
        int iter = 0;
        int found_record = 0;
        while (iter < tempBlock.numRecords) {
            // Grab a record from the bucket being checked
            Record recordChecked = tempBlock.records[iter];
            // If that record's id matches, print it and stop looking.
            if (recordChecked.id == id) {
                recordChecked.print();
                found_record = 1;
            }
            // Increment iter; if there are no more records in this bucket and an overflow bucket
            // exists, go to that and set iter to 0
            if (++iter == tempBlock.numRecords && tempBlock.nextBuffer != 0) {
                tempBlock.fetchNext(indexedFile);
                iter = 0;
            }
        }
        if (found_record == 0) {
            cout << "No matching record found!";
        }
        indexedFile.close();
    }
};
