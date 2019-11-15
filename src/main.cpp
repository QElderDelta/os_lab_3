#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <tbb/concurrent_queue.h>

using Graph_t = std::vector<std::vector<int>>;

struct Work {
    int vertex;
    std::string type;
};

struct Result {
    bool found;
    std::string type;
};

Graph_t graph;
std::vector<bool> used;
std::vector<int> parent;
tbb::concurrent_bounded_queue<Work> tasks;
tbb::concurrent_queue<Result> results;
int numberOfThreads = 1;
int threadsFinished = 0;
pthread_mutex_t lock;
bool found = false;


void* threadPool(void* params) {
    bool done = false;
    Work currentTask;
    while(!done) {
        pthread_mutex_lock(&lock);
        ++threadsFinished;
        int currentlyFinished = threadsFinished;
        pthread_mutex_unlock(&lock);
        if(currentlyFinished + 1 == numberOfThreads) {
            results.push({false, "Done"});
        }
        tasks.pop(currentTask);
        pthread_mutex_lock(&lock);
        --threadsFinished;
        pthread_mutex_unlock(&lock);
        if(currentTask.type == "Done") {
            done = true;
            continue;
        }
        for(const auto& vertex : graph[currentTask.vertex]) {
            pthread_mutex_lock(&lock);
            if(!used[vertex]) {
                used[vertex] = true;
                parent[vertex] = currentTask.vertex;
                tasks.push({vertex,  "Continue"});
            } else if(parent[currentTask.vertex] != vertex) {
                //results.push({1, "Continue"});
                found = true;
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);
        }
        results.push({false, "Continue"});
    }    
    pthread_exit(NULL);
}


void multithreadedBFS(int startingVertex) {
    pthread_t threads[numberOfThreads];
    for(int i = 0; i < numberOfThreads; ++i) {
        pthread_create(&threads[i], NULL, threadPool, (void*)0);
    }
    tasks.push({startingVertex, "Continue"});
    bool done = false;
    while(!done) {
        Result r;
        results.try_pop(r);
        //if(r.found) {
           // std::cout << "1" << std::endl;
       // }
       // found |= r.found;
        if(r.type == "Done") {
            done = true;
        }
    }
    for(int i = 0; i < numberOfThreads; ++i) {
        tasks.push({startingVertex, "Done"});
    }
    for(int i = 0; i < numberOfThreads; ++i) {
        pthread_join(threads[i], NULL);
    }
    std::cout << "Mutlithreaded: ";
    if(found) {
        std::cout << "Cycle found" << std::endl;
    } else {
        std::cout << "Cycle not found" << std::endl;
    }
}

bool singlethreadedBFS(const Graph_t& graph, int startingVertex) {
    std::queue<int> q;
    q.push(startingVertex);
    std::vector<bool> used(graph.size());
    std::vector<int> parent(graph.size(), -1);
    used[startingVertex] = true;
    while(!q.empty()) {
        int currentVertex = q.front();
        q.pop();
        for(const auto& v : graph[currentVertex]) {
            if(!used[v]) {
                used[v] = true;
                q.push(v);
                parent[v] = currentVertex;
            } else if (parent[currentVertex] != v) {
                return true;
            }
        }
    }
    return false;
}

int main() {
    int n, m;
    std::cin >> n >> m;
    graph.resize(n);
    for(int i = 0; i < m; ++i) {
        int u, v;
        std::cin >> u >> v;
        u -= 1;
        v -= 1;
        graph[u].push_back(v);
        graph[v].push_back(u);
    }
    int s;
    std::cin >> s;
    if(s < 0 || s > n) {
        std::cout << "Node with such ID doesn't exist" << std::endl;
        return -1;
    }
    s -= 1;
    std::cout << "Singlethreaded:";
    if(singlethreadedBFS(graph, s)) {
        std::cout << "Cycle found" << std::endl;
    } else {
        std::cout << "Cycle not found" << std::endl;
    }
    std::cin >> numberOfThreads;
    if(numberOfThreads < 1) {
        std::cout << "Number of threads must be at least 1" << std::endl;
        return -1;
    }
    if(numberOfThreads > n - 1) {
        std::cout << "Number of threads can't be greater than number of nodes - 1" << std::endl;
        return -1;
    }
    std::vector<pthread_t> threads(numberOfThreads);
    used.assign(graph.size(), false);
    parent.assign(graph.size(), -1);
    pthread_mutex_init(&lock, NULL);
    multithreadedBFS(s);
    pthread_mutex_destroy(&lock);
    return 0;
}
