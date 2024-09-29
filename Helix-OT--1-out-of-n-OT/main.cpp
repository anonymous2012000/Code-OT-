
#include <iostream>
#include <vector>
#include <bitset>
#include <cmath>
#include <gmpxx.h>
#include <ctime>
#include <cstdlib>
#include <random>
#include <chrono>
using namespace std;

// Function to generate a random big integer with a specified number of bits
mpz_class generate_random_bigint(gmp_randclass& rng, int num_bits) {
  return rng.get_z_bits(num_bits);
}

// Function to perform the TBCS algorithm with big integers
vector<mpz_class> TBCS(int database_size, const vector<mpz_class>& m, const vector<int>& b) {
  int n = database_size;
  int e = log2(n);
  // Check if the length of b matches log2(length(m))
  if (b.size() != e) {
    cerr << "\n Error: The length of bit-string b must be log2(length of m)." << endl;
    return {}; // Return an empty vector to indicate an error
  }
  // Create a vector for the current state of the tree, starting with the leaf nodes (messages)
  vector<mpz_class> tree = m;
  // Apply controlled swaps according to the bit-string b
  for (int h = e - 1; h >= 0; --h) {
    int group_size = 1 << (e - h); // 2^(e-h)
    int half_group_size = group_size / 2;
    for (int i = 0; i < n; i += group_size) {
      if (b[h] == 1) {
        // Swap the two halves of the current group
        for (int j = 0; j < half_group_size; ++j) {
          std::swap(tree[i + j], tree[i + j + half_group_size]);
        }
      }
    }
  }
  // Return the final permutation of messages
  return tree;
}

///-------  Phase 1: Setup
vector<vector<mpz_class>> setup(int number_of_OT, int n, unsigned int bit_size) {
  // Declare the outer vector to hold 'number_of_OT' vectors of random numbers
  vector<vector<mpz_class>> random_numbers_collection;
  random_numbers_collection.reserve(number_of_OT); // Reserve space for efficiency
  // Initialize GMP random state
  gmp_randclass rand_gen(gmp_randinit_default);
  rand_gen.seed(time(nullptr)); // Seed with current time
  // Generate 'number_of_OT' vectors of 'n' random numbers
  for (int j = 0; j < number_of_OT; ++j) {
    vector<mpz_class> v(n); // Create a vector with space for 'n' random numbers
    for (int i = 0; i < n; ++i) {
      v[i] = rand_gen.get_z_bits(bit_size); // Generate random number of bit_size bits
    }
    random_numbers_collection.push_back(std::move(v)); // Add the vector to the collection
  }
  return random_numbers_collection;
}

//geenrates a vector of random integers
vector<int> generateRandomIntegers(int number_of_OT, int n) {
  // Vector to hold the collection of random integers
  vector<int> random_integers;
  random_integers.reserve(number_of_OT); // Reserve space for efficiency
  // Seed the random number generator with a unique seed
  unsigned seed = chrono::system_clock::now().time_since_epoch().count();
  mt19937 rng(seed);
  uniform_int_distribution<int> dist(0, n); // Range [0, n] inclusive
  // Generate 'number_of_OT' random integers
  for (int i = 0; i < number_of_OT; ++i) {
    random_integers.push_back(dist(rng));
  }
  return random_integers;
}

// XOR-based secret sharing
vector<vector<int>> SS(int number_of_OT, const vector<int>& secret_index, vector<vector<int>> &p_shares){
  //initiates the parameters
  mt19937 rng(static_cast<unsigned int>(time(0)));
  uniform_int_distribution<int> dist(0, 1);
  vector<vector<int>> s_shares(number_of_OT);
  for (int i = 0; i < number_of_OT ; i++){
    p_shares[i].resize(1);
    s_shares[i].resize(1);
    p_shares[i][0] = dist(rng);
    s_shares[i][0] = p_shares[i][0] ^ secret_index[i];
  }
  return s_shares;
}


//------- Phase 2: GenQuery
vector<vector<int>> gen_query(int n, int number_of_OT, const vector<int>& indices, int bit_size, vector<vector<int>>& share2) {
    // Initialize vectors with reserved space for efficiency
    vector<vector<int>> share1(number_of_OT);
    share2.resize(number_of_OT);
    if(n == 2){
      share1 = SS(number_of_OT, indices, share2);
      return share1;
    }
    // Seed the random number generator
    mt19937 rng(static_cast<unsigned int>(time(0)));
    uniform_int_distribution<int> dist(0, 1);
    for (int j = 0; j < number_of_OT; ++j) {
      // Convert each index in indices to a binary representation
      bitset<64> binary(indices[j]);
      // Share each bit of each binary representation "binary"
      for (int i = bit_size - 1; i >= 0; --i) {
        int original_bit = binary[i];
        // Generate random share1 (either 0 or 1)
        int random_share = dist(rng);
        // Calculate share2 using XOR: original_bit = share1 ^ share2 => share2 = original_bit ^ random_share
        int share_2 = original_bit ^ random_share;
        // Store the shares
        share1[j].push_back(random_share);
        share2[j].push_back(share_2);
        }
    }
    return share1; // Return share1 if needed
  }

// Phase 3: Gen response
vector<vector<mpz_class>> gen_res_(int number_of_OT, int n, vector<mpz_class>& vec1,  vector<vector<mpz_class>>& vec2,  vector<vector<int>>& share1) {
  vector<vector<mpz_class>> result(number_of_OT);
  for (size_t j = 0; j < number_of_OT; ++j) {
    result[j].resize(n);
    for (size_t i = 0; i < n; ++i) {
      // Perform component-wise XOR and store directly in result[j][i]
      result[j][i] = vec1[i] ^ vec2[j][i];
    }
    // Apply the TBCS transformation after the XOR operation
    result[j] = TBCS(n, result[j], share1[j]);
  }
  return result;
}

// Phase 4: Oblivious Filter
vector<mpz_class> obli_filter(int number_of_OT, int n, vector<vector<mpz_class>>& vec, vector<vector<int>>& share2) {
  vector<mpz_class> result(number_of_OT);
  //cout << "m.size(): " << m.size() << ", b.size(): " << b.size() << ", e: " << e << endl;
  // Perform TBCS
  for(int i = 0; i < number_of_OT; i++){
    vector<mpz_class> permuted_messages = TBCS(n, vec[i], share2[i]);
    // Select the first element
    result[i] = permuted_messages[0];
  }
  return result;
}



//--------- Phase 5: Retreive
mpz_class retrive(const mpz_class& enc_m, mpz_class r) {
  // decrypt enc_m
  mpz_class result = enc_m ^ r;
  return result;
}

//check if an integer is a power of two
void checkPowerOfTwo(int n) {
  if (n <= 0 || (n & (n - 1)) != 0) {
    throw invalid_argument("\n\n ** Eror: The value *n* must be a power of two.**");
  }
}


int main() {

  int n =4; // Size of vector m (must be a power of two), 2, 16, 256, 4096, 65536, 1048576
  checkPowerOfTwo(n);
  int number_of_OT = 1;
  const int e = log2(n);
  int bit_size = 128; // Number of bits for each big integer in vector m
  //int index = 0;    // Example index
  int number_of_tests = 30;
  float phase1_=0;
  float phase2_=0;
  float phase3_=0;
  float phase4_=0;
  float phase5_=0;
  float phase1_total=0;
  float phase2_total=0;
  float phase3_total=0;
  float phase4_total=0;
  float phase5_total=0;
  float counter = 0;
  for(int i = 0; i < number_of_tests; i++){
    // Initialize the random number generator
    vector<vector<int>> share2;
    gmp_randclass rng(gmp_randinit_default);
    rng.seed(time(nullptr));
    // geenrate a vectors of random indices
    vector<int> indices = generateRandomIntegers(number_of_OT, n-1);
    //*** Uncomment to print the indices
    // for (int i = 0; i < n; ++i) {
    //   cout<<"\n index: "<<indices[i]<<endl;
    // }
    // Generate random big integers for vector m
    std::vector<mpz_class> m;
    for (int i = 0; i < n; ++i) {
      m.push_back(generate_random_bigint(rng, bit_size));
    }
    //*** Uncomment to print the messages
     // for (int i = 0; i < n; ++i) {
     // cout<<"\n m["<<i<<"]: "<< m[i]<<endl;
     // }
    vector<mpz_class> res_m(number_of_OT);
    auto phase1 = 0;//time related variable
    auto start_phase1 = clock();//time related variable
    vector<vector<mpz_class>> r = setup(number_of_OT, n, bit_size);
    auto end_phase1 = clock();//time related variable
    phase1 = end_phase1 - start_phase1;//time related variable
    phase1_ = phase1/(double) CLOCKS_PER_SEC;//time related variable
    double phase2 = 0;//time related variable
    double start_phase2 = clock();//time related variable
    // Query Generation
    vector<vector<int>>  share1 = gen_query(n, number_of_OT, indices, e, share2);
    double end_phase2 = clock();//time related variable
    phase2 = end_phase2 - start_phase2;//time related variable
    phase2_ = phase2 / (double) CLOCKS_PER_SEC;//time related variable
    double phase3 = 0;//time related variable
    double start_phase3 = clock();//time related variable
    //** Gen response
    vector<vector<mpz_class>> s_res = gen_res_(number_of_OT, n, m, r, share1);
    double end_phase3 = clock();//time related variable
    phase3 = end_phase3 - start_phase3;//time related variable
    phase3_ = phase3 / (double) CLOCKS_PER_SEC;//time related variable
    double phase4 = 0;//time related variable
    double start_phase4 = clock();//time related variable
    //** OblFilter
    vector<mpz_class> p_res = obli_filter(number_of_OT, n, s_res, share2);
    double end_phase4 = clock();//time related variable
    phase4 = end_phase4 - start_phase4;//time related variable
    phase4_ = phase4 / (double) CLOCKS_PER_SEC;//time related variable
    double phase5 = 0;//time related variable
    double start_phase5 = clock();//time related variable
    //** Retreive
    for(int j=0;j<number_of_OT; j++){
      res_m[j] = retrive(p_res[j], r[j][indices[j]]);
    }
    double end_phase5 = clock();//time related variable
    phase5 = end_phase5 - start_phase5;//time related variable
    phase5_ = phase5 / (double) CLOCKS_PER_SEC;//time related variable
    counter += phase5_ + phase4_ + phase3_ + phase2_ + phase1_;
    phase1_total += phase1_;
    phase2_total += phase2_;
    phase3_total += phase3_;
    phase4_total += phase4_;
    phase5_total += phase5_;
  }
  cout<<"\n\n================== Runtime Breakdown ========================"<<endl;
  cout<<"\n Phase 1 runtime: "<<phase1_total/number_of_tests<<endl;
  cout<<"\n Phase 2 runtime: "<<phase2_total/number_of_tests<<endl;
  cout<<"\n Phase 3 runtime: "<<phase3_total/number_of_tests<<endl;
  cout<<"\n Phase 4 runtime: "<<phase4_total/number_of_tests<<endl;
  cout<<"\n Phase 5 runtime: "<<phase5_total/number_of_tests<<endl;
  cout<<"\n\n================== Total Runtime for Helix OT========================"<<endl;
  cout<<"\n Total cost for " <<number_of_OT<<" OT executions: "<<counter/number_of_tests<<endl;
  cout<<"\n\n=======================================================\n"<<endl;
  cout<<"\n n: "<<n<<endl;
  cout<<"\n\n.....\n"<<endl;
  return 0;
}
