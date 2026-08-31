# MINI - REDIS - ENGINE

A high-performance, multi-threaded, in-memory key-value database engine written from scratch in modern C++17. This engine implements a custom TCP/IP network server, durable crash recovery, and advanced probabilistic data structures designed for high-throughput concurrency.

##  Architecture & Core Features

### 1. Multi-Threaded TCP/IP Engine
* **Raw Network Sockets:** Decoupled client-server architecture utilizing raw Windows sockets (`ws2_32`) for zero-dependency networking.
* **Concurrency:** Implements a thread-per-client connection model. Core memory structures are protected by `std::shared_mutex`, allowing massive simultaneous read operations while safely isolating state-modifying writes.

### 2. Probabilistic Data Structures (Skip List)
* Designed a custom C++ **Skip List** from the ground up to power the `ZSET` (Sorted Set) implementation.
* Achieves mathematical $O(\log N)$ time complexity for insertions, deletions, and continuous range queries.
* Engineered to handle high-volume, dynamic scoring environments like live competitive programming contest leaderboards or international Elo rating brackets.

### 3. Bitwise Memory Optimization (FEN Compression)
* Developed a custom bit-packing algorithm to compress standard chess FEN strings (~80 bytes) down to dense 8-bit binary buffers (`uint8_t`).
* Maps pieces and board states (including castling rights, active color, and en passant targets) to 4-bit and bit-flag representations, drastically reducing RAM overhead for mass game-state storage. 

### 4. Durability & Crash Recovery
* **Write-Ahead Log (WAL):** Prevents data loss during system failures by synchronously appending modifying commands to a local `.wal` file. 
* OS-level buffer flushing guarantees absolute disk persistence before returning success tokens to the client. On boot, the engine instantly replays the log to rebuild the exact in-memory state.

##  Benchmarks & Performance
Stress testing was conducted locally over the TCP/IP loopback interface (`127.0.0.1:6379`) to measure pure algorithmic throughput and disk I/O synchronization.

| Metric | Result | Notes |
| :--- | :--- | :--- |
| **Test Payload** | 100,000 continuous `SET` commands | Sequential network requests |
| **Total Time** | 10,538.84 ms | Includes full TCP round-trip |
| **Throughput** | **~9,488 OPS** (Operations Per Second) | Bound by WAL disk I/O flushes |

##  Getting Started (Build Instructions)

### Prerequisites
* CMake (v3.14+)
* MinGW-w64 (GCC C++17 Compiler)
* Windows Environment (for `ws2_32` socket support)

### 1. Build the Project
Configure and compile the engine using CMake:
```cmd
1.1 cmake -G "MinGW Makefiles" -B build
1.2 cmake --build build
2. Start the ServerRun the database instance. It will automatically bind to port 6379 and wait for connections:DOS.\build\mini_redis_cli.exe
3. Connect as a ClientOpen a secondary terminal and connect via standard Telnet to interact with the database:DOStelnet 127.0.0.1 6379
```
### 2. Supported CommandsBasic 
#### Core Key-Value Operations

| Command | Description |
| :--- | :--- |
| `SET <key> <value>` | Stores a string key-value pair in RAM and appends to the Write-Ahead Log (WAL). |
| `GET <key>` | Retrieves a string value by its key. |
| `DEL <key>` | Removes a key from memory and appends the deletion event to the WAL. |

#### Memory-Optimized Game States

| Command | Description |
| :--- | :--- |
| `SET_FEN <key> <fen_string>`| Compresses and stores a chess board state. |
| `GET_FEN <key>` | Retrieves and decompresses a stored board state. |

#### Leaderboards (Skip List)

| Command | Description |
| :--- | :--- |
| `ZADD <board> <score> <key>` | Adds or updates a user's score in $O(\log N)$ time. |
| `ZSCORE <board> <key>` | Retrieves a specific user's score from the given board. |
| `ZRANGE <board> <min> <max>` | Scans and returns all keys falling within the designated score bracket. |
