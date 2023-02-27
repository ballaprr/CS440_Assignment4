/*
Skeleton code for linear hash indexing
*/
#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;


int main(int argc, char* const argv[]) {
    // Create the index
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit
    std::string user_input;
    while (1) {
        cout << "Enter ID to look up (or <exit> to exit):" << endl;
        cin >> user_input;
        std::cin.clear();
        if (user_input.compare("exit") == 0) {
            break;
        }
        else {
            int id = stoi(user_input);
            Record recFound = emp_index.findRecordById(id);
            recFound.print();
        }
    }

    return 0;
}
