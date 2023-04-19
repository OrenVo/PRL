/***************************/
/* Author: Vojtěch Ulej    */
/* Login: xulejv00         */
/* Year: 22/23             */
/* Assignment: PRL k-means */
/***************************/


#include<iostream>
#include<fstream>
#include<mpi.h>
#include<cstdint>
#include<sstream>
#include<vector>
#include<array>

#define ifname "numbers"
#define MAX_ITERATIONS 1000
#define THRESHOLD 1e-6

namespace prl {
    // Load numbers from binary file
    std::vector<uint8_t> loadFile(std::string filename=ifname){
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

    
    
    inline float distance(uint8_t value, float mean){
        return std::abs(float(value) - mean);
    }
    
    enum Cluster {
        A=0, B, C, D
    };

    void updateClusters(std::array<float,4> &medians, uint8_t val, prl::Cluster cluster){
        //TODO MPI_AllReduce or MPI_Reduce to calculate new center for each cluster and store it in medians.
        //
    }
    
    Cluster assignCluster(float dis1, float dis2, float dis3, float dis4){
        float min = dis1;
        Cluster cluster = Cluster::A;
        if (dis2 < min) {
            min = dis2;
            cluster = Cluster::B;
        }
        if (dis3 < min) {
            min = dis3;
          cluster = Cluster::C;
        }
        if (dis4 < min) {
            min = dis4;
            cluster = Cluster::D;
        }
        return cluster;
    }

    void log(std::stringstream &message){
        std::cerr << message.str() << std::endl;
    }
    void log(std::string &message){
        std::cerr << message << std::endl;
    }
    void log(std::string message){
        std::cerr << message << std::endl;
    }
}

int main(int argc, char *argv[]) {
    int rank, world_size;
    std::vector<uint8_t> numbers;

    const int root = 0;
    // MPI initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    std::array<float, 4> medians = { 0., 0., 0., 0. };
    if (rank == root) {
        numbers = prl::loadFile(ifname);
        if (numbers.empty()) {
            std::cerr << "Error: empty numbers!" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        
        medians[prl::Cluster::A] = float(numbers[0]);
        medians[prl::Cluster::B] = float(numbers[1]);
        medians[prl::Cluster::C] = float(numbers[2]);
        medians[prl::Cluster::D] = float(numbers[3]);
        std::stringstream ss;
        ss << "[ " << medians[prl::Cluster::A] << ", ";
        ss << medians[prl::Cluster::B] << ", ";
        ss << medians[prl::Cluster::C] << ", ";
        ss << medians[prl::Cluster::D] << "]" << std::endl;
        prl::log("Medians: " + ss.str());
    }
    // broadcast medians to all processes
    MPI_Bcast(medians.data(), 4, MPI_FLOAT, root, MPI_COMM_WORLD);
    // scatter numbers to processes (each process gets one number)
    uint8_t number;
    MPI_Scatter(numbers.data(), 1, MPI_UINT8_T, &number, 1, MPI_UINT8_T, root, MPI_COMM_WORLD);
    for (size_t i = 0; i < MAX_ITERATIONS; i++){
        // calculate distance from each number to each median
        prl::Cluster cluster = prl::assignCluster(
            prl::distance(number, medians[prl::Cluster::A]),
            prl::distance(number, medians[prl::Cluster::B]),
            prl::distance(number, medians[prl::Cluster::C]),
            prl::distance(number, medians[prl::Cluster::D])
            );
        prl::log("Process " + std::to_string(rank) + " got number: " + std::to_string(number) + " and add it to cluster: " + std::to_string(cluster) + " median: " + std::to_string(medians[cluster]));
        //update clusters for all nodes and check if end conditions are met
        prl::updateClusters(medians, number, cluster);
        // TODO what next?
    }
    
    // clean up because OpenMPI cannot work with C++ STL containers
    if (rank == root) {
        
    }
    
    MPI_Finalize();
    return 0;
}