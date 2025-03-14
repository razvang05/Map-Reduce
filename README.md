
# Parallel Calculation of an Inverted Index Using the Map-Reduce Paradigm

## Author
**Gheorghe Marius Razvan**  
**Student ID**: 334CA  
**Email**: rzvrazvan03@gmail.com  
**Date**: 8.12.2024

## Project Description
This project implements the parallel calculation of an inverted index using the Map-Reduce paradigm. The project uses threading to parallelize the processing of documents, enabling fast and efficient creation of the inverted index. The program reads input files, processes each file individually, and constructs an index that can be used for fast searches on a set of documents.

## Implemented Functions

### In the `main` function:
- The program reads command-line arguments to determine the number of mapper and reducer threads, as well as the input file.
- It opens the file that contains the input data and reads each line, saving the filenames into a vector and pushing the indices of each file into a queue.
- The vector of threads is created, synchronization elements such as mutexes and barriers are declared and initialized, and a loop is used to initialize each thread's structure with the necessary data.
- Each thread is created by passing this structure as an argument, along with the most important argument: the function it must execute.
- After this loop, the program waits for all threads to finish execution and deallocates the synchronization elements used.

```cpp
if (argc != 4) {
    cerr << "number of arguments is not correct
";
    return 1;
}

int mappers_nr = stoi(argv[1]);
int reducers_nr = stoi(argv[2]);

ifstream file(argv[3]);
if (!file.is_open()) {
    cerr << "Failed to open input file
";
    return 1;
}
```

### In the `thread_function`:
- This function receives the structure for each thread as an argument. It extracts the thread ID and compares it to the number of mappers. If the thread ID is smaller than the number of mappers, the thread will execute the mapper function; otherwise, it will execute the reducer function.
  
```cpp
void *thread_function(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    if (args->thread_id < args->nr_mappers) {
        mapper_function(args);
    } else {
        reducer_function(args);
    }
    return NULL;
}
```

### In the `mapper_function`:
- The mapper function receives the thread's structure as an argument. It extracts the necessary elements to execute the function. A `while(true)` loop is used, as long as there are elements in the queue. Each mapper thread will extract an index from the queue and process the corresponding file.
- This method optimizes parallelization since some threads may process smaller files and finish faster. This is done through synchronization, using a mutex to lock and unlock the queue to prevent concurrent access by multiple threads.

```cpp
while(true) {
    string file;
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
        cerr << "Failed to open file " << file << "
";
        return;
    }
}
```

- Words are processed using the `process_word` function, which ensures they are formatted as lowercase without non-alphabetic characters. Each word is then inserted into a set to avoid duplicates. Each thread has its local map to avoid conflicts, and the words are added to it along with the index of the file they belong to.

```cpp
set<string> words_uniq;
while (file_stream >> word) {
    word = process_word(word.data());
    if (!word.empty()) {
        words_uniq.insert(word);
    }
}

for (const auto& word : words_uniq) {
    local_map[word].push_back(file_index + 1);
}
```

- After processing, each thread writes its local map to a shared structure using a mutex to avoid race conditions. A barrier ensures all mapper threads finish before the reducer threads start.

### In the `reducer_function`:
- The reducer function waits for all threads to reach the barrier and then begins its execution. Each reducer thread processes a range of words based on assigned letters. It iterates through the `mapper_results` and checks if the word falls within the assigned range.
- Reducers eliminate duplicates by using a set and aggregate the results in an unordered map. The results are then sorted: first by the number of files a word appears in (in descending order) and then alphabetically.

```cpp
for (const auto& local_map : mapper_results) {
    for (const auto& [word, files] : local_map) {
        if (word[0] >= ('a' + start) && word[0] < ('a' + end)) {
            local_aggregated_map[word].insert(files.begin(), files.end());
        }
    }
}
```

- Finally, the sorted results are written to output files, where each file corresponds to a particular starting letter of the words.

```cpp
for (int i = start; i < end; i++) {
    char current_letter = 'a' + i;
    write_to_file(current_letter, sorted_results);
}
```

### Synchronization
- **Barrier**: Ensures all mappers have completed their work before reducers begin.
- **Mutexes**: Used to synchronize access to shared data structures, preventing race conditions when threads modify the `mapper_results` or access the queue.

```cpp
pthread_barrier_wait(args->barrier);
pthread_mutex_lock(map_mutex);
mapper_results.push_back(move(local_map));
pthread_mutex_unlock(map_mutex);
```

## How to Compile and Run

1. **Clone the Repository**:
    ```bash
    git clone <repository-url>
    ```

## Acknowledgments
- Special thanks to the faculty and technical staff for their guidance and feedback during the development of this project.
- Thanks to all contributors for their support in refining and testing the parallelization strategy.
