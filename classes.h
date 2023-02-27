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
        char temp_block_charp[BLOCK_SIZE];
        indFile.seekg(blockAddr);
        indFile.read(temp_block_charp, BLOCK_SIZE);
        indFile.seekg(blockAddr);

        std::string numRecordsStr(&temp_block_charp[BLOCK_SIZE - 16], 8);
        numRecords = stoi(numRecordsStr);
        for (int i = 0; i < numRecords; i++) {
            int record_offset = i * 716;
            vector<std::string> fields;

            std::string eid(&temp_block_charp[record_offset], 8);
            fields.push_back(eid);
            std::string ename(&temp_block_charp[record_offset + 8], 200);
            fields.push_back(ename);
            std::string ebio(&temp_block_charp[record_offset + 208], 500);
            fields.push_back(ebio);
            std::string emid(&temp_block_charp[record_offset + 708], 8);
            fields.push_back(emid);

            Record emp(fields);
            records.push_back(emp);
        }

        std::string nextBufferStr(&temp_block_charp[BLOCK_SIZE - 8], 8);
        nextBuffer = stoi(nextBufferStr);
    }

    void insertRecord(Record record) {
        records.push_back(record);
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
            fetchFromFile(indFile, nextBuffer * BLOCK_SIZE);
        }
        else {
            cout << "ERR: Attempted to fetch next file without nextBuffer set!";
        }
    }

    // Format the current info as a char array of size BLOCK_SIZE and write to
    // the passed position in the file.
    void writeBlock(fstream& indFile) {
        // Initialise variables
        char writeBuffer[BLOCK_SIZE + 1];
        memset(writeBuffer, '\0', BLOCK_SIZE);
        int i;
        char temp[9];
        // Copy all records into the string
        for (i = 0; i < numRecords; i++) {
            //cout << "Writing first record...\n";
            int j;

            // Copy employee id
            //cout << "Employee ID:\t";
            snprintf(temp, 9, "%d", records[i].id);
            for (j = 0; j < 8 - strlen(temp); j++) {
                writeBuffer[i * 716 + j] = '0';
                //cout << '0';
            }
            strncpy(&writeBuffer[i * 716 + j], temp, strlen(temp));
            //cout << temp << endl;
            
            // Copy employee name
            records[i].name.copy(&writeBuffer[i * 716 + 8], 200);
            //cout << "Name:\t" << records[i].name << endl;

            // Copy employee bio
            records[i].bio.copy(&writeBuffer[i * 716 + 208], 500);
            //cout << "Bio:\t" << records[i].bio << endl;

            // Copy manager id
            //cout << "Manager ID:\n";
            snprintf(temp, 9, "%d", records[i].manager_id);
            for (j = 0; j < 8 - strlen(temp); j++) {
                writeBuffer[i * 716 + j] = '0';
                //cout << '0';
            }
            strncpy(&writeBuffer[i * 716 + 708 + j], temp, strlen(temp));
            //cout << temp << endl;
        }

        snprintf(temp, 9, "%d", numRecords);
        for (i = 0; i < 8 - strlen(temp); i++) {
            writeBuffer[BLOCK_SIZE - 16 + i] = '0';
        }
        strncpy(&writeBuffer[BLOCK_SIZE - 16 + i], temp, strlen(temp));
        //cout << "numRecords:\t" << numRecords << endl;

        snprintf(temp, 9, "%d", nextBuffer);
        for (i = 0; i < 8 - strlen(temp); i++) {
            writeBuffer[BLOCK_SIZE - 8 + i] = '0';
        }
        strncpy(&writeBuffer[BLOCK_SIZE - 8 + i], temp, strlen(temp));
        //cout << "nextBuffer:\t" << nextBuffer << endl;

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

        /*
        // No records written to index yet
        // This loop uses 2 blocks (one of class Block, one temporary string to write Block)
        if (numRecords == 0) {
            // Initialise blockDirectory to hold up to 256 elements
            blockDirectory.reserve(256);

            // Initialize index with first blocks (start with 4)
            for (int iter = 0; iter < 4; iter++) {
                blockDirectory[iter] = nextFreeBlock;
                nextFreeBlock ++;

                // Place an empty block in the designated spot.
                Block emptyBlock;
                indexedFile.seekg(blockDirectory[iter] * BLOCK_SIZE);
                emptyBlock.writeBlock(indexedFile);
            }
            n = 4;
            i = 2;
        }
        */

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
        //cout << "Inserting into bucket " << sigbits << endl;
        int targetBlockAddr = blockDirectory[sigbits] * BLOCK_SIZE;
        Block tempBlock;

        tempBlock.fetchFromFile(indexedFile, targetBlockAddr);
        // Get a block with at least one free space
        while (tempBlock.numRecords == 5) {
            if (tempBlock.nextBuffer) {
                tempBlock.fetchNext(indexedFile);
            }
            else {
                // Set the current block's nextBuffer to nextFreeBlock.
                tempBlock.nextBuffer = nextFreeBlock;
                tempBlock.writeBlock(indexedFile);

                // Go to the next block and increment nextFreeBlock.
                indexedFile.seekg(nextFreeBlock * BLOCK_SIZE);
                nextFreeBlock++;

                // Clear tempBlock's data.
                tempBlock.clear();
            }
        }
        tempBlock.insertRecord(record);
        tempBlock.writeBlock(indexedFile);

        // GOOD TO HERE!!!
        // REMOVE THIS

        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        numRecords++;
        if (numRecords >= 0.7 * 5 * n) {
            int blockTracker;
            //cout << "Increasing n to " << n + 1 << endl;
            // Increase n; if n is greater than 2^i, increase i
            if (++n > int_pow(2, i)) {
                //cout << "Increasing i to " << i + 1 << endl;
                i++;
            }

            blockDirectory[n - 1] = nextFreeBlock;
            nextFreeBlock++;

            // Place an empty block in the designated spot.
            tempBlock.clear();
            indexedFile.seekg(blockDirectory[n - 1] * BLOCK_SIZE);
            tempBlock.writeBlock(indexedFile);

            // Get the bucket that is the new largest bucket with the most significant bit turned off
            int bucketToCheck = (n - 1) % int_pow(2, i - 1);
            blockTracker = blockDirectory[bucketToCheck];
            tempBlock.fetchFromFile(indexedFile, blockTracker * BLOCK_SIZE);
            int iter = 0;
            while (iter < tempBlock.numRecords || tempBlock.nextBuffer != 0) {
                if (iter == tempBlock.numRecords) {
                    blockTracker = tempBlock.nextBuffer;
                    tempBlock.fetchNext(indexedFile);
                    iter = 0;
                }
                else {
                    // Grab a record from the bucket being checked
                    Record recordChecked = tempBlock.records[iter];
                    // If that record should go in the new one ...
                    if (recordChecked.id % int_pow(2, i) == n - 1) {
                        // Erase it from the current bucket
                        tempBlock.records.erase(tempBlock.records.begin() + iter);
                        tempBlock.numRecords--;
                        tempBlock.writeBlock(indexedFile);
                        // Decrement numRecords (it will be increased in the next line and we don't want
                        // to double-count moved records)
                        numRecords--;
                        // Insert the record. This will use at most 2 blocks at any given time, plus the
                        // 1 block (tempBlock) at the current depth. n should never be increased more than
                        // once per insertion, so no more than 3 blocks should be in memory at any given
                        // time (1 at depth 0, 2 at depth 1)
                        //cout << "Placing record with ID " << recordChecked.id << " in new spot...";
                        indexedFile.close();
                        insertRecord(recordChecked);
                        //cout << "Done!\n";
                        indexedFile.open(fName, ios::in | ios::out);
                        indexedFile.seekg(blockTracker * BLOCK_SIZE);
                    }
                    else {
                        iter++;
                    }

                }
            }
            indexedFile.close();
        }
    }

public:
    LinearHashIndex(string indexedFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexedFileName;

        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
        fstream indexedFile;
        indexedFile.open(fName, ios::out | ios::in | ios::trunc);
        blockDirectory.reserve(256);
        for (int iter = 0; iter < 4; iter++) {
            blockDirectory[iter] = nextFreeBlock;
            nextFreeBlock++;
            // Place an empty block in the designated spot.
            Block emptyBlock;
            indexedFile.seekg(blockDirectory[iter] * BLOCK_SIZE);
            emptyBlock.writeBlock(indexedFile);
            //cout << "blockDirectory[" << iter << "] = " << blockDirectory[iter] << endl;
        }
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        // Open csv file
        fstream csv_file;
        csv_file.open(csvFName, ios::in);

        // For each entry in the csv file, create and insert a record into the index
        std::string line, word;
        int loop_counter = 1;
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

            //cout << loop_counter << endl;
            loop_counter++;
            insertRecord(emp);
        }

        //cout << "Out of insert loop\n";

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
        // CHANGE THIS: THIS FETCHES FROM CSV FILE, NOT OUR FORMAT
        tempBlock.fetchFromFile(indexedFile, blockDirectory[bucketToCheck] * BLOCK_SIZE);
        int iter = 0;
        int found_record = 0;
        while (iter < tempBlock.numRecords) {
            // Grab a record from the bucket being checked
            Record recordChecked = tempBlock.records[iter];
            // If that record's id matches, print it and stop looking.
            if (recordChecked.id == id) {
                indexedFile.close();
                return(recordChecked);
            }
            // Increment iter; if there are no more records in this bucket and an overflow bucket
            // exists, go to that and set iter to 0
            if (++iter == tempBlock.numRecords && tempBlock.nextBuffer != 0) {
                tempBlock.fetchNext(indexedFile);
                iter = 0;
            }
        }
        vector<std::string> fields;
        fields.push_back("-1\0");
        fields.push_back("Employee Not Found\0");
        fields.push_back("Bio Not Found\0");
        fields.push_back("-1\0");
        Record emptyRecord(fields);
        return(emptyRecord);
    }
};
