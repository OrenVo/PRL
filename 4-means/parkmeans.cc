/***************************/
/* Author: Vojtěch Ulej    */
/* Login: xulejv00         */
/* Year: 22/23             */
/* Assignment: PRL k-means */
/***************************/


#include<cstddef>
#include<iostream>
#include<fstream>
#include<cstdint>
#include<sstream>
#include <string>
#include<vector>
#include<array>

#include<mpi.h>

#define ifname "numbers"
#define MAX_ITERATIONS 1000
#define THRESHOLD 1e-6

namespace prl {
    // Logging
    void log(std::stringstream &message){
      std::cerr << message.str() << std::endl;
    }
    void log(std::string &message){
      std::cerr << message << std::endl;
    }
    void log(std::string message){
        std::cerr << message << std::endl;
      }

    // Load numbers from binary file
    std::vector<uint8_t> loadFile(std::string filename=ifname){
        std::vector<uint8_t> numbers;
        std::ifstream infile(filename, std::ios::binary);
        if (infile) {
          // Get the file size
          infile.seekg(0, std::ios::end);
          size_t file_size = infile.tellg();
          infile.seekg(0, std::ios::beg);

          // Read the contents of the file into a buffer
          std::vector<uint8_t> numbers(file_size);
          infile.read(reinterpret_cast<char*>(numbers.data()), file_size);

          infile.close();
          return numbers;
        } else {
          infile.close();
          std::cerr << "Error opening file: " << filename << std::endl;
          MPI_Abort(MPI_COMM_WORLD, 1);
          return numbers;
        }
    }
    // Calculate distance of value from mean of cluster
    inline float distance(uint8_t value, float mean){
        return std::abs(float(value) - mean);
    }
    
    enum Cluster : uint8_t {
        A=0, B, C, D
    };

    void updateClusters(std::array<float,4> &medians, uint8_t val, prl::Cluster cluster, int rank){
      // Calculate counts of elements in each clusters
      size_t sizeA, sizeB, sizeC, sizeD;
      size_t clusA = cluster == Cluster::A ? 1 : 0;
      size_t clusB = cluster == Cluster::B ? 1 : 0;
      size_t clusC = cluster == Cluster::C ? 1 : 0;
      size_t clusD = cluster == Cluster::D ? 1 : 0;
      MPI_Allreduce(&clusA, &sizeA, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&clusB, &sizeB, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&clusC, &sizeC, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&clusD, &sizeD, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);

      // Calculate average for each cluster and use it as mean in next iteration.
      uint64_t sumA, sumB, sumC, sumD;
      uint64_t valA = cluster == Cluster::A ? uint64_t(val) : 0;
      uint64_t valB = cluster == Cluster::B ? uint64_t(val) : 0;
      uint64_t valC = cluster == Cluster::C ? uint64_t(val) : 0;
      uint64_t valD = cluster == Cluster::D ? uint64_t(val) : 0;
      // Calculate sums of each cluster
      MPI_Allreduce(&valA, &sumA, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&valB, &sumB, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&valC, &sumC, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&valD, &sumD, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);

      std::array<size_t, 4> sizes = {sizeA, sizeB, sizeC, sizeD};
      std::array<uint64_t , 4> sums = {sumA, sumB, sumC, sumD};
      for (size_t i = 0; i < 4; i++){
        size_t size = sizes[i];
        uint64_t sum = sums[i];
        if (size > 0)
            medians[i] = float(sum)/float(size);
      }
      //log("Process: " + std::to_string(rank) + ": got averages: [ " + std::to_string(medians[0]) + ", " + std::to_string(medians[1]) + ", " + std::to_string(medians[2]) + ", " + std::to_string(medians[3]) + " ]" );
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
    }
    // broadcast medians to all processes
    MPI_Bcast(medians.data(), 4, MPI_FLOAT, root, MPI_COMM_WORLD);
    // scatter numbers to processes (each process gets one number)
    uint8_t number;
    prl::Cluster cluster;
    MPI_Scatter(numbers.data(), 1, MPI_UINT8_T, &number, 1, MPI_UINT8_T, root, MPI_COMM_WORLD);
    for (size_t i = 0; i < MAX_ITERATIONS; i++){
        // calculate distance from each number to each median
        cluster = prl::assignCluster(
            prl::distance(number, medians[prl::Cluster::A]),
            prl::distance(number, medians[prl::Cluster::B]),
            prl::distance(number, medians[prl::Cluster::C]),
            prl::distance(number, medians[prl::Cluster::D])
            );
        //update clusters for all nodes and check if end conditions are met
        std::array<float, 4> oldMedians = medians;
        prl::updateClusters(medians, number, cluster, rank);
        if (medians == oldMedians){ // no medians were changed
          break;
        }
    }

    uint8_t *clusters;
    if (rank == root){
      clusters = new uint8_t[numbers.size()];
      for (int i = 0; i < numbers.size(); i++){
        clusters[i] = 0;
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(&cluster, 1, MPI_UINT8_T, clusters, 1, MPI_UINT8_T, root, MPI_COMM_WORLD);
    if (rank == root){
      std::array<std::vector<uint8_t>,4> clustersValues;

      for (size_t i = 0; i < numbers.size(); i++){
        clustersValues[clusters[i]].push_back(numbers[i]);
      }
      std::stringstream ss;
      for (size_t i = 0; i < 4; i++){
        ss << "[" << std::to_string(medians[i]) << "]";
        for (auto val : clustersValues[i]) {
          ss << " " << std::to_string(val) << ",";
        }
        ss.seekp(-1, std::ios_base::end);
        ss << std::endl;
      }
      std::cout << ss.str();
    }
    // clean up because OpenMPI cannot work with C++ STL containers
    if (rank == root) {
      delete[] clusters;
    }
    
    MPI_Finalize();
    return 0;
}