#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>

using Graph_t = std::vector<std::vector<int>>;

Graph_t graph;
std::vector<bool> used;
std::vector<int> parent;
std::vector<std::queue<int>> queues;
std::queue<int> mainQueue;
bool found = false;
bool alive = true;
int numberOfThreads;
pthread_mutex_t lock;
pthread_cond_t cond;
int threadsFinished;

void sendSignal() {
    pthread_mutex_lock(&lock);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

void* threadPool(void* params) {
    int id = *(int*)params;
    while(alive) {
        pthread_mutex_lock(&lock);
        while(1) {
            pthread_cond_wait(&cond, &lock);
            break;
        }
        pthread_mutex_unlock(&lock);
        while(!queues[id].empty()) {
            int currentVertex = queues[id].front();
            queues[id].pop();
            for(const auto& v : graph[currentVertex]) {
                pthread_mutex_lock(&lock);
                if(!used[v]) {
                    used[v] = true;
                    mainQueue.push(v);
                    parent[v] = currentVertex;
                } else if(parent[currentVertex] != v) {
                    found = true;
                }
                pthread_mutex_unlock(&lock);
            }
        }
        pthread_mutex_lock(&lock);
        threadsFinished++;
        pthread_mutex_unlock(&lock);
    }
    pthread_exit(NULL);
}

bool multithreadedBFS(int startingVertex) {
    used.assign(graph.size(), false);
    parent.assign(graph.size(), 0);
    mainQueue.push(startingVertex);
    used[startingVertex] = true;
    queues.resize(numberOfThreads);
    int threadCount = 0;
    while(1) {
        threadsFinished = 0;
        while(!mainQueue.empty()) {
            int temp = mainQueue.front();
            mainQueue.pop();
            queues[threadCount%(numberOfThreads - 1)].push(temp);
            threadCount++;
        }
        threadCount %= (numberOfThreads - 1);
        sendSignal();
        while(threadsFinished != numberOfThreads - 1);
        if(mainQueue.empty()) {
            break;
        }
    }
    if(found) {
        return true;
    }
    return false;
}

bool singlethreadedBFS(const Graph_t& graph, int startingVertex) {
    std::queue<int> q;
    q.push(startingVertex);
    std::vector<bool> used(graph.size());
    std::vector<int> parent(graph.size());
    used[startingVertex] = true;
    while(!q.empty()) {
        int currentVertex = q.front();
        q.pop();
        for(const auto& v : graph[currentVertex]) {
            if(!used[v]) {
                used[v] = true;
                q.push(v);
                parent[v] = currentVertex;
            } else if(parent[currentVertex] != v) {
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
    s -= 1;
    if(singlethreadedBFS(graph, s)) {
        std::cout << "Cycle found" << std::endl;
    } else {
        std::cout << "Cycle not found" << std::endl;
    }
    if(s < 0 || s > n) {
        std::cout << "Node with such ID doesn't exist" << std::endl;
        return -1;
    }
    std::cin >> numberOfThreads;
    std::vector<pthread_t> threads(numberOfThreads);
    if(numberOfThreads < 1) {
        std::cout << "Number of threads must be positive number" << std::endl;
        return -1;
    }
    if(numberOfThreads > n) {
        std::cout << "Number of threads can't be greater than number of nodes" << std::endl;
        return -1;
    }
    pthread_mutex_init(&lock, NULL);
    int params[numberOfThreads];
    for(int i = 0; i < numberOfThreads; ++i) {
        params[i] = i;
        pthread_create(&threads[i], NULL, threadPool, (void*)(params + i));
    }
    multithreadedBFS(s); 
    if(found) {
        std::cout << "Cycle found" << std::endl;
    } else {
        std::cout << "Cycle not found" << std::endl;
    }
    alive = false;
    sendSignal();
    for(int i = 0; i < numberOfThreads; ++i) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&lock);
    return 0;
}
