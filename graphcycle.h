#ifndef GRAPHCYCLE_H
#define GRAPHCYCLE_H
#include <list>
#include <vector>
#include <algorithm>

class GraphCycle{
    private:
        int V;			// No. of vertices
        std::list<int> *adj; // Pointer to array containing adjacency lists
        bool isCyclicUtil(int v, bool visited[], bool *recStack, std::vector<int> &path) const;
     public:
        GraphCycle(int V);   // Constructor
        void addEdge(int v, int w);   // to add an edge to graph
        bool isCyclic();
        bool cycle(std::vector<int> &path) const;
};
#endif // GRAPHCYCLE_H
