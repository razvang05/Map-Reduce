#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <queue>
#include <set>
#include <math.h>
using namespace std;

// struct for passing arguments to threads
struct ThreadArgs {
    int thread_id;
    int nr_mappers;
    int nr_reducers;
    const vector<string>* input_files;
    // mapper results shared between all threads
    vector<unordered_map<string, vector<int>>>* mapper_results;
    pthread_barrier_t* barrier;
    pthread_mutex_t* que_mutex;
    pthread_mutex_t* map_mutex;
    queue<int>* files_queue;
};

char* process_word(char* word) {
    int i = 0, j = 0;
    while (word[i]) {
        if (isalpha(word[i])) {
            word[j++] = tolower(word[i]);
        }
        i++;
    }
    word[j] = '\0';
    return word;

}

void mapper_function(ThreadArgs *args) {

    auto* file_queue = args->files_queue;
    auto* que_mutex = args->que_mutex;
    auto* map_mutex = args->map_mutex;
    const auto& input_files = *(args->input_files);
    auto& mapper_results = *(args->mapper_results);
    // unordered_map for the current thread
    unordered_map<std::string, std::vector<int>> local_map;
    int file_index=-1;

    while(true) {

        string file;
        // lock the queue to get the next file
        // and unlock it after getting the file
        pthread_mutex_lock(que_mutex);
        if (file_queue->empty()) {
            pthread_mutex_unlock(que_mutex);
            break;
        }
        file_index = file_queue->front();
        file = input_files[file_index];
        file_queue->pop();
        pthread_mutex_unlock(que_mutex);

        ifstream file_stream(file);
        if (!file_stream.is_open()) {
            cerr << "Failed to open file " << file << "\n";
            return;
        }

        string word;
        set <string> words_uniq;
        // read the words from the file
        // and insert them into the set
        while (file_stream >> word) {
            word = process_word(word.data());

           if (!word.empty()) {
                words_uniq.insert(word);
            }
        }
        file_stream.close();

        // insert the words into the local map
        for (const auto& word : words_uniq) {
            local_map[word].push_back(file_index + 1);
        }
    }
    
    // lock the map to insert the local map
    // and unlock it after inserting
    pthread_mutex_lock(map_mutex);
    mapper_results.push_back(move(local_map));
    pthread_mutex_unlock(map_mutex);
    // wait for all threads to finish
    pthread_barrier_wait(args->barrier);
   
}

void write_to_file(char current_letter, 
                    const vector<pair<string, vector<int>>>& sorted_results) {

    string filename = string(1, current_letter) + ".txt";
    ofstream output(filename);
    if (!output.is_open()) {
        cerr << "Failed to open file " << filename << "\n";
        return;
    }
    
    for (const auto& [word, files] : sorted_results) {
        if (word[0] == current_letter) {
            output << word << ":[";
            for (size_t j = 0; j < files.size(); ++j) {
                output << files[j];
                if (j != files.size() - 1) {
                    output << " ";
                }
            }
            output << "]\n";
        }
    }
    output.close();
}

void reducer_function(ThreadArgs* args) {

    // wait for all threads to finish
    pthread_barrier_wait(args->barrier);

    int reducer_id = args->thread_id - args->nr_mappers;
    int num_reducers = args->nr_reducers;

    auto& mapper_results = *(args->mapper_results);

    // calculate the range of words that the reducer is responsible for
    // every reducer is responsible for a range of 26/num_reducers letters
    int start = reducer_id * 26 / num_reducers;
    int end = fmin((reducer_id + 1) * 26 / num_reducers, 26);

    unordered_map<string, set<int>> local_aggregated_map;
    // aggregate the results from the mappers
    for (const auto& local_map : mapper_results) {
        for (const auto& [word, files] : local_map) {
            if (word[0] >= ('a' + start) && word[0] < ('a' + end)) {
                local_aggregated_map[word].insert(files.begin(), files.end());
            }
        }
    }
    // put the results in a vector to sort them
    vector<pair<string, vector<int>>> sorted_results;
    for (auto& [word, file_set] : local_aggregated_map) {
        sorted_results.emplace_back(word, vector<int>(file_set.begin(), file_set.end()));
    }
    // sort the results by the number of files and then by the word
    sort(sorted_results.begin(), sorted_results.end(),
              [](const auto& a, const auto& b) {
                  if (a.second.size() != b.second.size()) {
                      return a.second.size() > b.second.size();
                  }
                  return a.first < b.first;
              });
    // write the results to the output files
    for (int i = start; i < end; i++) {
        char current_letter = 'a' + i;
        write_to_file(current_letter, sorted_results);
    }

}

void *thread_function(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int thread_id = args->thread_id;
    // check if the thread is a mapper or a reducer
    if (thread_id < args->nr_mappers) {
        mapper_function(args);
    } else {
        reducer_function(args);
    }
    return 0;

}

int main(int argc, char **argv)
{
    if (argc != 4) {
        cerr << "number of arguments is not correct\n";
        return 1;
    }

    int mappers_nr = stoi(argv[1]);
    int reducers_nr = stoi(argv[2]);

    ifstream file(argv[3]);
    if (!file.is_open()) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    int nr_files;
    file >> nr_files;
    vector<string> files(nr_files);
    queue<int> files_queue;
    for (int i = 0; i < nr_files; i++) {
        file >> files[i];
        files_queue.push(i);

    }
    file.close();

    int nr_threads = mappers_nr + reducers_nr;
    pthread_t threads[nr_threads];
    ThreadArgs args[nr_threads];

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, nr_threads);
    pthread_mutex_t map_mutex;
    pthread_mutex_init(&map_mutex, NULL);

    // vector of mapper results shared between all threads
    vector<unordered_map<string, vector<int>>> mapper_results(mappers_nr);
    // create the threads
    for (int i = 0; i < nr_threads; i++) {
        args[i]= {
            i,
            mappers_nr,
            reducers_nr,
            &files,
            &mapper_results,
            &barrier,
            &mutex,
            &map_mutex,
            &files_queue
        };

        int err = pthread_create(&threads[i], NULL, thread_function, &args[i]);
        if (err) {
            cerr << "Error creating thread " << i << "\n";
            return 1;
        }
    }
    
    // wait for all threads to finish
    for (int i = 0; i < nr_threads; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err) {
            cerr << "Error joining thread " << i << "\n";
            return 1;
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&map_mutex);

    return 0;
}
