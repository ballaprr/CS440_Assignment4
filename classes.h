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

class Block {
private:
    const int BLOCK_SIZE = 4096;

public:
    int nextBuffer = 0;     // The first buffer will never be an overflow buffer, and this allows an "if (nextBuffer)" check.
    int numRecords = 0;
    vector<Record> records;

    // Given the filestream and block address, move to and get a given block.
    void fetchFromFile(fstream& indFile, int blockAddr) {
        string temp_block;
        indFile.seekg(blockAddr)
        indFile.read(temp_block, BLOCK_SIZE);

        std::string numRecordsStr(temp_block, BLOCK_SIZE - 16, 8);
        numRecords = stoi(numRecordsStr);
        for (i = 0; i < numRecords; i++) {
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
            cout << "ERR: Attempted to fetch next file without nextBuffer set!"
        }
    }

    // Format the current info as a char array of size BLOCK_SIZE and write to
    // the passed position in the file.
    void writeBlock(fstream& indFile) {
        // Initialise variables
        char writeBuffer[BLOCK_SIZE];
        int i;

        // Copy all records into the string
        for (i = 0; i < numRecords; i++) {
            int j;

            // Copy employee id
            char* temp = itoa(records[i].id);
            for (j = 0; j < 8 - strlen(temp); j++) {
                writeBuffer[i * BLOCK_SIZE + j] = '0';
            }
            strncpy(&writeBuffer[i * BLOCK_SIZE + j], temp, strlen(temp));
            
            // Copy employee name
            strncpy(&writeBuffer[i * BLOCK_SIZE + 8], records[i].name, 200);

            // Copy employee bio
            strncpy(&writeBuffer[i * BLOCK_SIZE + 208], records[i].bio, 500);

            // Copy manager id
            char* temp = itoa(records[i].manager_id);
            for (j = 0; j < 8 - strlen(temp); j++) {
                writeBuffer[i * BLOCK_SIZE + j] = '0';
            }
            strncpy(&writeBuffer[i * BLOCK_SIZE + 708 + j], temp, strlen(temp));
        }

        char* temp = itoa(numRecords);
        for (i = 0; i < 8 - strlen(temp); i++) {
            writeBuffer[BLOCK_SIZE - 16 + i] = '0';
        }
        strncpy(&writeBuffer[BLOCK_SIZE - 16 + i], temp, strlen(temp));

        temp = itoa(nextBuffer);
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
        fstream indexFile;
        indexFile.open(ios::write | ios::read);

        // No records written to index yet
        if (numRecords == 0) {
            // Initialise blockDirectory to hold up to 256 elements
            blockDirectory.reserve(256);

            // Initialize index with first blocks (start with 4)
            for (int iter = 0; iter < 4; iter++) {
                blockDirectory[iter] = nextFreeBlock;
                nextFreeBlock += BLOCK_SIZE;

                // Place an empty block in the designated spot.
                Block emptyBlock;
                indexFile.seekg(blockDirectory[iter]);
                emptyBlock.writeBlock(fstream &indexFile);
            }
        }

        // Add record to the index in the correct block, creating an overflow block if necessary

        // Get bits relevant to block address
        int hash = record.id % 256;
        int sigbits = hash % pow(2, i);
        // Bitflip if necessary.
        if (sigbits >= n) {
            sigbits = sigbits % pow(2, i - 1);
        }
        int targetBlockAddr = blockDirectory[sigbits];
        Block tempBlock;
        tempBlock.fetchFromFile(indexFile, targetBlockAddr);
        // Get a block with at least one free space
        while (tempBlock.numRecords == 5) {
            if (tempBlock.nextBuffer) {
                tempBlock.fetchNext;
            }
            else {
                // Set the current block's nextBuffer to nextFreeBlock.
                tempBlock.nextBuffer = nextFreeBlock;
                tempBlock.writeBlock(fstream &indexFile);

                // Go to the next block and increment nextFreeBlock.
                indexFile.seekg(nextFreeBlock);
                nextFreeBlock += BLOCK_SIZE;

                // Clear tempBlock's data.
                tempBlock.clear();
            }
        }

        tempBlock.insertRecord(record);

        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        if (++numRecords >= 0.7 * 5 * n) {
            ; // HERE is where I'm currently working.
        }
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
      
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        // Open index file, truncating (as this should be creating a new file)
        fstream index_file;
        index_file.open(self.fName, ios::out | ios::trunc);
        index_file.close();

        // Open csv file
        fstream csv_file;
        csv_file.open(csvFName, ios::in);

        // For each entry in the csv file, create and insert a record into the index
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

            Record(fields) emp;

            self.insertRecord(emp);

            // Free the record
            free(emp);
        }

        csv_file.close();
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        
    }
};
