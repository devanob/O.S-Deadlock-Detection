#include "graphcycle.h"

GraphCycle::GraphCycle(int V)
{
    this->V = V;
    adj = new std::list<int>[V];
}

void GraphCycle::addEdge(int v, int w)
{
    adj[v].push_back(w); // Add w to v’s list.
}

bool GraphCycle::isCyclicUtil(int v, bool visited[], bool *recStack, std::vector<int> &path) const {
        if (visited[v] == false) {
            // Mark the current node as visited as you need 
            visited[v] = true; //marked 
            recStack[v] = true; //marked 

            // Recur for all the vertices adjacent to this vertex
            std::list<int>::iterator i; //created a list interator to keep track of states graph transisiton and movement 
            for (i = adj[v].begin(); i != adj[v].end(); ++i) {
                if (!visited[*i] && isCyclicUtil(*i, visited, recStack, path)){
                    path.push_back(*i);
                    return true;}
                else if (recStack[*i]){
                    path.push_back(*i);
                    return true;}
            }
        }
        recStack[v] = false; // remove the vertex from the stack 
        path.pop_back();
        return false;
    }

bool GraphCycle::cycle(std::vector<int> &path) const {
        // Mark all the vertices as not visited and not part of the current stack state 
        bool *visited = new bool[V];
        bool *recStack = new bool[V];
        for (int i = 0; i < V; i++) {
            visited[i] = false;
            recStack[i] = false;
        }
        // Call the recursive helper function to detect cycle in different tree states 
        for (int i = 0; i < V; i++){
            path.push_back(i);
            if (isCyclicUtil(i, visited, recStack, path)) {
                std::reverse(path.begin(),path.end());
                path.pop_back();
                return true;
            }
            path.clear();
        }
        path.clear();
        return false;
}
