
#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>
#include <gmpxx.h>
#include <chrono> // For std::chrono::system_clock
#include <ctime>
#include <cstdlib>
#include <algorithm> // For std::shuffle
#include <random>
#include <utility>   // For std::pair
#include <unordered_map>
using namespace std;

// Phase 1: Setup--- it generates a vector of random values (for each invocation)
vector<vector<mpz_class>> Setup(int number_of_OT, int n, unsigned int bit_size) {
  // Declare the outer vector to hold 'number_of_OT' vectors of random numbers
  vector<vector<mpz_class>> random_numbers_collection;
  random_numbers_collection.reserve(number_of_OT); // Reserve space for efficiency
  // Initialize GMP random state
  gmp_randclass rand_gen(gmp_randinit_default);
  rand_gen.seed(time(nullptr)); // Seed with current time
  // Generate 'number_of_OT' vectors of 'n' random numbers
  for (int j = 0; j < number_of_OT; ++j) {
    std::vector<mpz_class> v(n); // Create a vector with space for 'n' random numbers
    for (int i = 0; i < n; ++i) {
      v[i] = rand_gen.get_z_bits(bit_size); // Generate random number of bit_size bits
    }
    random_numbers_collection.push_back(std::move(v)); // Add the vector to the collection
  }
  return random_numbers_collection;
}


// // Fisher-Yates shuffle for random permutation
// template<typename T>
// void FisherYatesShuffle(vector<T>& vec) {
//
//     random_device rd;
//     mt19937 g(rd());
//
//     for (int i = vec.size() - 1; i > 0; --i) {
//         uniform_int_distribution<> dis(0, i);
//         int j = dis(g);
//         swap(vec[i], vec[j]);
//     }
// }

// Function to generate a hash table. It will be used bu Find to fetch elements efficiently
unordered_map<int, int> createIndexMap(const vector<int>& v) {
  unordered_map<int, int> indexMap;
  for (int i = 0; i < v.size(); ++i) {
    indexMap[v[i]] = i;
  }
  return indexMap;
}

// Phase 2: genQuery---Query Generation-- It generates two vectors of queries: result[i], y[i] (for each invocation)
vector<vector<int>> genQuery(int number_of_OT, int p_size, vector<vector<int>> p, int n, vector<vector<int>>& y) {
  // Pre-allocate memory for the result and y vectors
  vector<vector<int>> result(number_of_OT);
  y.resize(number_of_OT);
  int t = p_size;
  // Generate the vector v with elements from 0 to n-1
  vector<int> v(n);
  for (int i = 0; i < n; ++i) {
    v[i] = i;
  }
  // Initialize random engine for shuffling
  random_device rd;
  default_random_engine rng(rd());
  for (int i = 0; i < number_of_OT; ++i) {
    // Shuffle v in-place
    shuffle(v.begin(), v.end(), rng);
    result[i] = v;
    // Directly use the shuffle result to assign y[i]
    y[i].resize(t);
    if (n == 2) {
      // Direct mapping for n = 2
      for (int j = 0; j < t; ++j) {
        y[i][j] = (p[i][j] == 0) ? v[0] : v[1];
      }
    } else {
      // General case for larger n
      unordered_map<int, int> map;
      map.reserve(n); // Reserve space to avoid rehashing
      for (int j = 0; j < n; ++j) {
        map[v[j]] = j;
      }
      for (int j = 0; j < t; ++j) {
        y[i][j] = map[p[i][j]];
      }
    }
  }
  return result;
}

// Phase 3: GenRes---Generates a response to the receiver's query (for each invocation)
vector<vector<mpz_class>> GenRes(const vector<mpz_class>& m, int number_of_OT, const vector<vector<mpz_class>>& r, const vector<vector<int>>& w) {
  size_t m_size = m.size();  // Store m.size() in a variable to avoid recomputing
  vector<vector<mpz_class>> x(number_of_OT, vector<mpz_class>(m_size));
  vector<vector<mpz_class>> z(number_of_OT);
  // Preallocate memory for each vector in z using the stored m_size
  for (int j = 0; j < number_of_OT; ++j) {
    z[j].reserve(m_size);
  }
  // Process each OT separately
  for (int j = 0; j < number_of_OT; ++j) {
    unordered_map<int, int> indexMap = createIndexMap(w[j]);
    for (size_t i = 0; i < m_size; ++i) {
      // Perform XOR operation
      mpz_class xor_result = m[i] ^ r[j][i];
      z[j].push_back(xor_result);
      // Find the corresponding index in x
      auto it = indexMap.find(i);
      if (it != indexMap.end()) {
        int y_i = it->second;
        x[j][y_i] = xor_result;
      } else {
        cerr << "\n ** Error: invalid index during computing GenRes" << endl;
        return {}; // Return an empty vector to indicate an error
      }
    }
  }
  return x;
}

// Phase 4: oblFilter---Oblivious filtering--returns a single message (for each invocation)
vector<vector<mpz_class>> oblFilter(int number_of_OT, int p_size, const vector<vector<mpz_class>>& res_s, const vector<vector<int>>& y) {
  // Preallocate the outer vector with the correct size
  vector<vector<mpz_class>> res(number_of_OT, vector<mpz_class>(p_size));
  // Iterate over the number of OTs
  for (int j = 0; j < number_of_OT; ++j) {
    // Iterate over each element in the current OT
    for (int i = 0; i < p_size; ++i) {
      // Assign the value from res_s based on the index from y
      res[j][i] = res_s[j][y[j][i]];
    }
  }
  return res;
}


// Phase 4: retreive---messgae retreival (for each invocation)
mpz_class retreive(const mpz_class& res_h, int j, const vector<mpz_class>& r, const vector<int>& p) {
  // Check if j is within the valid range
  if (j >= p.size()) {
    cerr << "\n *** Error: priority must be smaller than the size of priority vector p" << endl;
    return {};
  }
  // Return the result of the XOR operation directly
  return res_h ^ r[p[j]];
}

// Function to generate a random big integer with a specified number of bits-- used for test
mpz_class generate_random_bigint(gmp_randclass& rng, int num_bits) {
  return rng.get_z_bits(num_bits);
}

// generates vectors of random values
vector<vector<int>> generateRandomVectors(int p_size, int number_of_OT, int n) {
  // Vector to hold the collection of vectors
  vector<vector<int>> random_vectors;
  random_vectors.reserve(number_of_OT); // Reserve space for efficiency
  // Generate 'number_of_OT' vectors
  for (int i = 0; i < number_of_OT; ++i) {
    // Create a vector with all possible values from 0 to n-1
    vector<int> values(n);
    for (int j = 0; j < n; ++j) {
      values[j] = j;
    }
    // Seed the random number generator with a unique seed
    unsigned seed = chrono::system_clock::now().time_since_epoch().count() + i;
    shuffle(values.begin(), values.end(), mt19937(seed));
    // Select the first 'p_size' elements to ensure they are distinct
    vector<int> vec(values.begin(), values.begin() + p_size);
    // Add the generated vector to the collection
    random_vectors.push_back(move(vec));
  }
  return random_vectors;
}

///////////////////////////////
int main() {
  int n = 16;// 16, 256, 4096, 65536, 1048576
  int number_of_OT = 1;
  unsigned int bit_size = 128;
  int p_size = 10;// it is t in t-out-of-n OT
  vector<vector<int>>y;
  int number_of_tests = 20;
  float phase1_, phase2_, phase3_, phase4_, phase5_;
  float phase1_total, phase2_total, phase3_total, phase4_total, phase5_total;
  float counter = 0;
  //cout<<"\n======= Message ========"<< endl;
  for(int i = 0; i < number_of_tests; i++){
    vector<vector<int>> collection_of_p = generateRandomVectors(p_size, number_of_OT, n);
    // ****----Start_1: uncomment the below lines to get the values of p (indices)
    // cout<<"\n========= P ======"<<endl;
    // for(int j=0;j<number_of_OT;j++){
    //   cout<<"\n"<<j<<"th-invocation of OT"<<endl;
    //   for (int i = 0; i < p_size; ++i) {
    //     cout<<"\n p["<<j<<"]["<<i<<"]: "<< collection_of_p[j][i]<<endl;
    //   }
    //   cout<<"\n...."<<endl;
    // }
    // ----End_1
    gmp_randclass rng(gmp_randinit_default);
    rng.seed(time(nullptr));
    vector<mpz_class> m;
    //generate n random messages
    for (int i = 0; i < n; ++i) {
      m.push_back(generate_random_bigint(rng, bit_size));
    }
    // ****----Start_2: uncomment below lines to get the values of messages (held by the sender)
    // for (int i = 0; i < n; ++i) {
    // cout<<"\n m["<<i<<"]: "<< m[i]<<endl;
    // }
    // ----End_2
    //cout<<"\n======SetuP========="<<endl;
    double phase1 = 0;//time related variable
    double start_phase1 = clock();//time related variable
    vector<vector<mpz_class>> r = Setup(number_of_OT, n, bit_size);
    double end_phase1 = clock();//time related variable
    phase1 = end_phase1 - start_phase1;//time related variable
    phase1_ = phase1 / (double) CLOCKS_PER_SEC;//time related variable
    //cout<<"\n======genQuery========="<<endl;
    double phase2 = 0;//time related variable
    double start_phase2 = clock();//time related variable
    //GenQuery
    auto w = genQuery(number_of_OT, p_size, collection_of_p, n, y);
    double end_phase2 = clock();//time related variable
    phase2 = end_phase2 - start_phase2;//time related variable
    phase2_ = phase2 / (double) CLOCKS_PER_SEC;//time related variable
    // ----Start_3: uncomment below lines to get the values of queries
    // for(int k = 0;k<number_of_OT; k++){
    //   for (int i = 0; i < n; ++i) {
    //     cout<<"\n w: "<<w[k][i]<<endl;
    //   }
    //   cout<<"\n........."<<endl;
    //   for (int i = 0; i < p_size; ++i) {
    //     cout<<"\n y: "<<y[k][i]<<endl;
    //   }
    //   cout<<"\n---------"<<endl;
    // }
    // ----End_3
    //cout<<"\n======GenRes========="<<endl;
    double phase3 = 0;//time related variable
    double start_phase3 = clock();//time related variable
    // GenRes
    vector<vector<mpz_class>> res_s = GenRes(m, number_of_OT, r, w);
    double end_phase3 = clock();//time related variable
    phase3 = end_phase3 - start_phase3;//time related variable
    phase3_ = phase3 / (double) CLOCKS_PER_SEC;//time related variable
    //cout<<"\n======oblFilter========="<<endl;
    double phase4 = 0;//time related variable
    double start_phase4 = clock();//time related variable
    // oblFilter
    vector<vector<mpz_class>> res_h = oblFilter(number_of_OT, p_size, res_s, y);
    double end_phase4 = clock();//time related variable
    phase4 = end_phase4 - start_phase4;//time related variable
    phase4_ = phase4 / (double) CLOCKS_PER_SEC;//time related variable
    // retreive (the final result(s))
    //cout<<"\n======retreive========="<<endl;
    double phase5 = 0;//time related variable
    double start_phase5 = clock();//time related variable
    // retreive (the final result(s))
    for(int k = 0; k<number_of_OT; k++){
      //cout<<"\n"<<k<<"-th OT invocation"<<endl;
      for(int j=0; j< p_size; j++){
        mpz_class retreived_message = retreive(res_h[k][j], j, r[k], collection_of_p[k]);
      }
    }
    double end_phase5 = clock();//time related variable
    phase5 = end_phase5 - start_phase5;//time related variable
    phase5_ = phase5 / (double) CLOCKS_PER_SEC;//time related variable
    phase1_total += phase1_;
    phase2_total += phase2_;
    phase3_total += phase3_;
    phase4_total += phase4_;
    phase5_total += phase5_;
    counter += phase5_ + phase4_ + phase3_ + phase2_ + phase1_;
  }
  cout<<"\n\n number_of_OT: "<<number_of_OT<<endl;
  cout<<"\n\n================== Runtime Breakdown ========================"<<endl;
  cout<<"\n Phase 1 runtime: "<<phase1_total/number_of_tests<<endl;
  cout<<"\n Phase 2 runtime: "<<phase2_total/number_of_tests<<endl;
  cout<<"\n Phase 3 runtime: "<<phase3_total/number_of_tests<<endl;
  cout<<"\n Phase 4 runtime: "<<phase4_total/number_of_tests<<endl;
  cout<<"\n Phase 5 runtime: "<<phase5_total/number_of_tests<<endl;
  cout<<"\n\n================== Total Runtime for Priority OT========================"<<endl;
  cout<<"\n Setting:  Priority OT, t= " <<p_size<<", n= "<<n<<endl;
  cout<<"\n Total cost for: " <<number_of_OT<<" OT executions: "<<counter/number_of_tests<<endl;
  cout<<"\n\n=======================================================\n"<<endl;
  cout<<"\n n: "<<n<<endl;
  return 0;
}


//// g++ -std=c++11 -lgmpxx -lgmp main.cpp -o main
