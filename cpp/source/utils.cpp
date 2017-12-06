#include "utils.h"
#include <cassert>

long unsigned StartClock() {
    timeval time;
    gettimeofday(&time, NULL);
    long unsigned initTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    return initTime;
}

long unsigned StopClock(long unsigned initTime) {
    timeval time;
    gettimeofday(&time, NULL);
    long unsigned endTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    return endTime - initTime;
}

int LoadGraph(char * graphFileName,
              std::vector< Edge >& edgeList,
              Node& maxNodeId,
              uint32_t nbLinesToSkip) {
    // Opening file
    std::ifstream inFile;
    inFile.open((const char *)graphFileName);
    if(!inFile) {
        printf( "Graph: Error Openning Graph File\n" );
        return 1;
    }
    for (uint32_t i = 0; i < nbLinesToSkip; i++) {
        std::string s;
        std::getline(inFile, s);
    }
    // Loading edges
    Node node1, node2;
    maxNodeId = 0;
    while( inFile >> node1 ) {
        inFile >> node2;
        edgeList.push_back(std::make_pair(node1, node2));
        if (node1 > maxNodeId) {
            maxNodeId = node1;
        }
        if (node2 > maxNodeId) {
            maxNodeId = node2;
        }
    }
    inFile.close();
    return 0;
}

template <typename stream_t>
uint64_t GetVarint(stream_t &is) {
  auto get_byte = [&is]() -> uint8_t {
    uint8_t result;
    is.read(reinterpret_cast<char*>(&result), 1);
    return result;
  };
  uint64_t u, v = get_byte();
  if (!(v & 0x80)) return v;
  v &= 0x7F;
  u = get_byte(), v |= (u & 0x7F) << 7;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 14;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 21;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 28;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 35;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 42;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 49;
  if (!(u & 0x80)) return v;
  u = get_byte(), v |= (u & 0x7F) << 56;
  if (!(u & 0x80)) return v;
  u = get_byte();
  if (u & 0xFE)
    throw std::overflow_error("Overflow during varint64 decoding.");
  v |= (u & 0x7F) << 63;
  return v;
}

int LoadBinaryGraph(const std::vector<std::string>& paths, std::vector<Edge> &edgeList, Node& maxNodeId) {
    maxNodeId = 0;
    if (!paths.empty()) {
        std::ifstream is;
        size_t _file_index = 0;

        auto next_input = [&]() {
            is.close();
            if (_file_index < paths.size()) {
                is.open(paths[_file_index++]);
                if (!is) {
                    printf("Graph: Error Opening Graph file\n");
                }
            }
        };

        next_input();

        if (!is) return 1;

        for (uint32_t u = 0; is.good() && is.is_open(); ++u) {
            if (u > maxNodeId) maxNodeId = u;
            for (size_t deg = GetVarint(is); deg > 0 && is.good(); --deg) {
                uint32_t v;
                if (!is.read(reinterpret_cast<char*>(&v), 4)) {
                    throw std::runtime_error("I/O error while reading next neighbor");
                }

                assert(u != v);
                
                if (v > maxNodeId) maxNodeId = v;

                edgeList.emplace_back(u, v);
            }

            if (is.is_open() && (is.peek() == std::char_traits<char>::eof() || !is.good())) {
                next_input();
            }
        }
    }
    
    return 0;
}

int BuildNeighborhoods(std::vector< Edge >& edgeList, std::vector< NodeSet >& nodeNeighbors) {
    for (std::vector< Edge >::iterator it = edgeList.begin(); it != edgeList.end(); ++it) {
        Edge edge(*it);
        nodeNeighbors[edge.first].insert(edge.second);
        nodeNeighbors[edge.second].insert(edge.first);
    }
    return 0;
}

int PrintBinaryPartition(const std::string& fileName,
                         const std::vector<uint32_t>& nodeCommunity) {
    std::ofstream outFile(fileName, std::ios::binary);

    if (!outFile) return 1;

    for (Node u = 0; u < nodeCommunity.size(); ++u) {
        outFile.write(reinterpret_cast<const char*>(&u), 4);
        outFile.write(reinterpret_cast<const char*>(&nodeCommunity[u]), 4);
    }

    return 0;
}

int PrintPartition(const char* fileName,
                   std::map< uint32_t, std::set< Node > >& communities,
                   bool removeSingleton) {
    std::ofstream outFile;
    outFile.open(fileName);
    uint32_t nbCommunities = 0;
    for (auto kv : communities) {
        if (!removeSingleton || kv.second.size() > 1) {
            std::set<Node>::iterator it2 = kv.second.begin();
            Node nodeId;
            while ( true ) {
                nodeId = *it2;
                ++it2;
                if (it2 != kv.second.end()) {
                    outFile << nodeId << " ";
                } else {
                    break;
                }
            }
            outFile << nodeId << std::endl;
            nbCommunities++;
        }
    }
    printf("%-32s %i\n", "Number of Communities:", nbCommunities);
    outFile.close();
    return 0;
}

int GetCommunities(const std::vector< uint32_t >& nodeCommunity,
                   Node maxNodeId,
                   std::map< uint32_t, std::set< Node > >& communities) {
    for (Node i = 0; i <= maxNodeId; i++) {
        if (nodeCommunity[i] > 0) {
            communities[nodeCommunity[i]].insert(i);
        }
    }
    return 0;
}
