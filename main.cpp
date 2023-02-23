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
    bool check = true;
    int x;
    while (check) {
        cout << "Tyep 1 to enter id for lookup";
        cout << "Type 2 to enter number to stop";
        cout << "Type a number: ";
        cin >> x;
        if (x == 1) {

        }
        else if (x == 2) {
            check = false;
        }
    }
    

    return 0;
}
