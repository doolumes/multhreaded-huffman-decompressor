#include <iostream>
#include <fstream>

#include <vector>
#include <algorithm>
#include<iterator>

#include <string>
#include <sstream>

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

//information of each line of input file will be stored in a Huffman node
struct HuffmanNode {
    char c;
    int freq;
    std::string code;
    HuffmanNode* left;
    HuffmanNode* right ;

    HuffmanNode(int freq, char c = '\0'){

        this->c = c;
        this->freq = freq;
        this->left = nullptr;
        this->right = nullptr;
    }

    //returns code based on position of node on Huffman Tree
    char decode(std::string str){
        if (str == "")
            return c;
        else{
            char temp  = str.at(0);
            if (temp == '0')
                return this->left->decode(str.substr(1));
            return this->right->decode(str.substr(1));
        }
    }
};

//args for pthread_create will be a pointer to this struct
struct decompress_info{
    HuffmanNode* root;
    std::string str;
    std::vector<int> positions;
    char* output;

    decompress_info(HuffmanNode* root, std::string str, std::vector<int> positions, char* output){
        this->root = root;
        this->str = str;
        copy(positions.begin(), positions.end(), back_inserter(this->positions));
        this->output = output;
    }
};

//comparator for strong ordering of nodes contained in vector
struct HuffmanNodeComparison{
   bool operator()( const HuffmanNode* a, const HuffmanNode* b ) const{
        if( a->freq != b->freq)
            return ( a->freq > b->freq);
        return ( a->c > b->c);
   }
};

//args function for pthread_create; adds decoded characters to final output array based on their positions
void *code_to_string(void *decompress_info_void_ptr){
    decompress_info *decompress_info_ptr = (decompress_info*) decompress_info_void_ptr;
    /*
    Root node of the Huffman Tree is used to determine the character it is
    based on the code string passed to struct in the parameter of this function that was
    passed in by our pthread_create function in our decompress function
    */
    char c = decompress_info_ptr->root->decode(decompress_info_ptr->str);

    for(int i = 0; i < decompress_info_ptr->positions.size(); i++)
        /*stores the decompressed character (c) a total of (size of positions vector) times
        in our output array which was created in and therefore accessible by the main thread*/
        decompress_info_ptr->output[decompress_info_ptr->positions[i]] = c;
}

//converts the output array to a string that can be printed
std::string convertToString(char* a, int size){
    int i;
    std::string s = "";

    for (i = 0; i < size; i++)
        s = s + a[i];
    return s;
}

//uses algorithm to build Huffman tree based on the nodes in the vector passed in parameter list
void build_huffman_tree(std::vector<HuffmanNode*> &huffmanNodeVector){
    std::ifstream input;
    std::string line;

    char c;
    int freq;
    std::string inputFile;
    //name of input file is read from STDIN
    std::cin >> inputFile;
    input.open(inputFile);

    //alphabet information is read of the input file
    while(getline(input, line)){

        c = line.at(0);
        freq = stoi(line.substr(2));
        huffmanNodeVector.push_back(new HuffmanNode(freq ,c));
    }

    sort(huffmanNodeVector.begin(), huffmanNodeVector.end(), HuffmanNodeComparison());

    //Huffman tree is generated based on the vector of nodes
    while(huffmanNodeVector.size() > 1){

        HuffmanNode* left = huffmanNodeVector[huffmanNodeVector.size()-1];
        HuffmanNode* right = huffmanNodeVector[huffmanNodeVector.size()-2];

        huffmanNodeVector.pop_back();
        huffmanNodeVector.pop_back();

        HuffmanNode* root = new HuffmanNode(left->freq + right->freq);

        root->left = left;
        root->right = right;

        huffmanNodeVector.push_back(root);

        sort(huffmanNodeVector.begin(), huffmanNodeVector.end(), HuffmanNodeComparison());
    }
    input.close();
}

//creates the codes for each node in Huffman tree
void create_huffman_code(HuffmanNode* root, std::string code){
    if(root == nullptr)
        return;
    if(root->left == nullptr && root->right == nullptr)
        root->code = code;
    if(root->left)
        create_huffman_code(root->left, code + '0');
    if(root->right)
        create_huffman_code(root->right, code + '1');
}

//prints nodes of Huffman tree in preorder traversal
void print_huffman_tree(HuffmanNode* root){
    if(root == nullptr)
        return;
    if(root->c)
        std::cout << "Symbol: " << root->c << ", Frequency: " << root->freq << ", Code: " << root->code << std::endl;
    print_huffman_tree(root->left);
    print_huffman_tree(root->right);
}

//compressed file is decoded using multi-threaded approach
void decompress_huffman_code(HuffmanNode* huffmanNode){
    std::ifstream compressed;
    std::string line;

    std::string compressedFile;
    //name of compressed file is read from STDIN
    std::cin >> compressedFile;
    compressed.open(compressedFile);

    char output[huffmanNode->freq];
    //list of args for our pthread_create function
    static std::vector<decompress_info*> array_of_decompress_info_structs;
    //list of threads since there will be N threads
    static std::vector<pthread_t> tid;

    //information is read from the compressed file
    while(getline(compressed, line)){
        std::istringstream ss(line);
        std::string code;
        std::string buffer;
        std::vector<int> positions;
        int position;
        bool getCode = true;

        while(ss >> buffer){
            if(getCode){
                code = buffer;
                getCode = false;
            }
            else{
                positions.push_back(stoi(buffer));
            }
        }
        //n POSIX threads are created (n is the number of lines in the compressed file)
        pthread_t thread;
        decompress_info* temp = new decompress_info(huffmanNode, code, positions, output);
        tid.push_back(thread);
        array_of_decompress_info_structs.push_back(temp);
    }

    for(int i = 0; i < array_of_decompress_info_structs.size(); i++){
        /*
        each thread receives a pointer to a struct that contains the following:
        struct decompress_info{
            HuffmanNode* root;
            std::string str;
            std::vector<int> positions;
            char* output;
        }

        (1) a pointer to the root node of the Huffman tree
        (2) the binary code
        (3) a vector of positions in which the code should be placed in the output array
        (4) a pointer to the output array that will be mutated by each thread
        */
        if(pthread_create(&tid[i], NULL, code_to_string, (void*)array_of_decompress_info_structs[i])){
            fprintf(stderr, "Error creating thread\n");
            return;
        }
    }

    //joining threads together after their processes are finished
    for(int i = 0; i < tid.size(); i++)
        pthread_join(tid[i], NULL);
    /*After threads mutate the output array,
    and are joined, the original message is printed after*/
    std::cout << "Original message: " << convertToString(output, sizeof(output) / sizeof(char)) << std::endl;
}

int main(){
    std::vector<HuffmanNode*> huffmanNodeVector;
    //1. Generate Huffman tree using a sorted vector as a priority queue
    build_huffman_tree(huffmanNodeVector);
    //2. Huffman codes are created from the generated tree
    create_huffman_code(huffmanNodeVector[0], "");
    //3. Huffman codes are printed from the generated tree
    print_huffman_tree(huffmanNodeVector[0]);
    //4. compressed file is decompressed using the codes of each character in the tree; implemented with POSIX threads
    decompress_huffman_code(huffmanNodeVector[0]);
    return 0;
}
