/***********************************/
/* First project for course PRL    */
/* Author: Vojtěch Ulej (xulejv00) */
/* Year: 2022/2023                 */
/***********************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <sstream>
#include <numeric>
#include "mpi.h"

#define ifname "numbers"
// Load numbers from binary file
std::vector<uint8_t> loadFile(std::string filename){
    std::vector<uint8_t> numbers;
    std::ifstream infile(filename, std::ios::binary);
        if (infile) {
            while (!infile.eof()) {
                uint8_t number;
                infile.read((char*)&number, sizeof(number));
                numbers.push_back(number);
            }
            infile.close();
            return numbers;
        } else {
            infile.close();
            std::cerr << "Error opening file: " << filename << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return numbers;
        }
}

void printOutput(std::vector<uint8_t> lower, std::vector<uint8_t> equal, std::vector<uint8_t> greater){
    std::stringstream ss;
    // print lower numbers than median
    ss << "L: ";
    for (auto i : lower) {
        ss << (int)i << " ";
    }
    ss << std::endl;
    // print numbers equal to median
    ss << "E: ";
    for (auto i : equal) {
        ss << (int)i << " ";
    }
    ss << std::endl;
    // print greater numbers than median
    ss << "G: ";
    for (auto i : greater) {
        ss << (int)i << " ";
    }
    ss << std::endl;
    std::cout << ss.str();
}

// calculate cumulative sum of recvCounts and store it to displs
void calculateDisplacements(int *recvCounts, int *displs, int world_size){
    displs[0] = 0;
    for (int i = 1; i < world_size; i++) {
        displs[i] = displs[i-1] + recvCounts[i-1];
    }
}

int main(int argc, char *argv[]) {
    int rank, world_size;
    uint8_t median;
    std::vector<uint8_t> numbers;
    uint8_t *numbersSplitted;
    std::vector<uint8_t> lower;
    std::vector<uint8_t> equal;
    std::vector<uint8_t> greater;
    size_t splitArrSize;
    
    const int root = 0;
    // MPI initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // load file, find median and size of processes local array
    if (rank == root) {
        numbers = loadFile(ifname);
        if (numbers.empty()) {
            std::cerr << "Error: empty numbers!" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }   
        median = numbers.size() % 2 ? numbers[numbers.size()/2-1] : numbers[numbers.size()/2];
        splitArrSize = numbers.size()/world_size;
    }

    // broadcast median and size of local array to all processes
    MPI_Bcast(&median, 1, MPI_UINT8_T, 0, MPI_COMM_WORLD);
    MPI_Bcast(&splitArrSize, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

    // scatter numbers across all processes
    numbersSplitted = new uint8_t[splitArrSize];
    MPI_Scatter(numbers.data(), splitArrSize, MPI_UINT8_T, numbersSplitted, splitArrSize, MPI_UINT8_T, 0, MPI_COMM_WORLD);

    // split numbers to lower, equal and greater
    for (size_t i = 0; i < splitArrSize; i++) {
        uint8_t number = numbersSplitted[i];
        if (number < median) {
            lower.push_back(number);
        } else if (number == median) {
            equal.push_back(number);
        } else {
            greater.push_back(number);
        }
    }

    // Gather size of vectors
    int *recvCountsL, *recvCountsE, *recvCountsG = nullptr;
    if (rank == root) {
        recvCountsL = new int[world_size];
        recvCountsE = new int[world_size];
        recvCountsG = new int[world_size];
    }
    int lSize = lower.size();
    int eSize = equal.size();
    int gSize = greater.size();

    MPI_Gather(&lSize, 1, MPI_INT, recvCountsL, 1, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Gather(&eSize, 1, MPI_INT, recvCountsE, 1, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Gather(&gSize, 1, MPI_INT, recvCountsG, 1, MPI_INT, root, MPI_COMM_WORLD);

    // compute length and displacements for gatherv operation for each: lower, equal, greater
    size_t lowerLength, equalLength, greaterLength = 0;
    int *lowerDisplacement, *equalDisplacement, *greaterDisplacement = nullptr;
    uint8_t *lowerGathered, *equalGathered, *greaterGathered = nullptr;

    if (rank == root) {
        lowerLength = std::accumulate(recvCountsL, recvCountsL + world_size, 0);
        equalLength = std::accumulate(recvCountsE, recvCountsE + world_size, 0);
        greaterLength = std::accumulate(recvCountsG, recvCountsG + world_size, 0);

        lowerDisplacement   = new int[world_size];
        equalDisplacement   = new int[world_size];
        greaterDisplacement = new int[world_size];

        calculateDisplacements(recvCountsL, lowerDisplacement, world_size);
        calculateDisplacements(recvCountsE, equalDisplacement, world_size);
        calculateDisplacements(recvCountsG, greaterDisplacement, world_size);

        lowerGathered   = new uint8_t[lowerLength];
        equalGathered   = new uint8_t[equalLength];
        greaterGathered = new uint8_t[greaterLength];
    }
    
    // gather results from all processes
    MPI_Gatherv(lower.data(), lower.size(), MPI_UINT8_T, lowerGathered, recvCountsL, lowerDisplacement, MPI_UINT8_T, root, MPI_COMM_WORLD);
    MPI_Gatherv(equal.data(), equal.size(), MPI_UINT8_T, equalGathered, recvCountsE, equalDisplacement, MPI_UINT8_T, root, MPI_COMM_WORLD);
    MPI_Gatherv(greater.data(), greater.size(), MPI_UINT8_T, greaterGathered, recvCountsG, greaterDisplacement, MPI_UINT8_T, root, MPI_COMM_WORLD);


    if (rank == root) { // master print numbers
        printOutput(std::vector<uint8_t>(lowerGathered, lowerGathered+lowerLength), 
                    std::vector<uint8_t>(equalGathered, equalGathered+equalLength),
                    std::vector<uint8_t>(greaterGathered, greaterGathered+greaterLength)
                   );    
    }

    // clean up because OpenMPI cannot work with C++ STL containers
    if (rank == root){
        delete[] recvCountsL;
        delete[] recvCountsE;
        delete[] recvCountsG;
        delete[] lowerDisplacement;
        delete[] equalDisplacement;
        delete[] greaterDisplacement;
        delete[] lowerGathered;
        delete[] equalGathered;
        delete[] greaterGathered;
    }
    delete[] numbersSplitted;
    MPI_Finalize();    
    return 0;
}
