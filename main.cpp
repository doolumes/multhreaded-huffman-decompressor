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


struct HuffmanNodeComparison{
   bool operator()( const HuffmanNode* a, const HuffmanNode* b ) const{
        if( a->freq != b->freq)
            return ( a->freq > b->freq);
        return ( a->c > b->c);
   }
};

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

void print(HuffmanNode* root){
    if(root == nullptr)
        return;
    if(root->c)
        std::cout << "Symbol: " << root->c << ", Frequency: " << root->freq << ", Code: " << root->code << std::endl;
    print(root->left);
    print(root->right);
}

void build_huffman_tree(std::vector<HuffmanNode*> &huffmanNodeVector){
    std::ifstream input;
    std::string line;

    char c;
    int freq;
    std::string inputFile;
    std::cin >> inputFile;
    input.open(inputFile);

    while(getline(input, line)){
        
        c = line.at(0);
        freq = stoi(line.substr(2));
        huffmanNodeVector.push_back(new HuffmanNode(freq ,c));
    }
    
    sort(huffmanNodeVector.begin(), huffmanNodeVector.end(), HuffmanNodeComparison());

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


void *code_to_string(void *decompress_info_void_ptr){
    decompress_info *decompress_info_ptr = (decompress_info*) decompress_info_void_ptr;
    char c = decompress_info_ptr->root->decode(decompress_info_ptr->str);
    
    for(int i = 0; i < decompress_info_ptr->positions.size(); i++)
        decompress_info_ptr->output[decompress_info_ptr->positions[i]] = c;
}

std::string convertToString(char* a, int size){
    int i;
    std::string s = "";
    
    for (i = 0; i < size; i++)
        s = s + a[i];
    return s;
}

void decompress(HuffmanNode* huffmanNode){
    std::ifstream compressed;
    std::string line;

    std::string compressedFile;
    std::cin >> compressedFile;
    compressed.open(compressedFile);

    char output[huffmanNode->freq];
    static std::vector<decompress_info*> array_of_decompress_info_structs;
    static std::vector<pthread_t> tid;

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
        pthread_t thread;
        decompress_info* temp = new decompress_info(huffmanNode, code, positions, output);
        tid.push_back(thread);
        array_of_decompress_info_structs.push_back(temp);
    }

    for(int i = 0; i < array_of_decompress_info_structs.size(); i++){
        if(pthread_create(&tid[i], NULL, code_to_string, (void*)array_of_decompress_info_structs[i])){
            fprintf(stderr, "Error creating thread\n");
            return;
        }
    }

    for(int i = 0; i < tid.size(); i++)
        pthread_join(tid[i], NULL);
    std::cout << "Original message: " << convertToString(output, sizeof(output) / sizeof(char)) << std::endl;
}

int main(){
    std::vector<HuffmanNode*> huffmanNodeVector;
    build_huffman_tree(huffmanNodeVector);
    create_huffman_code(huffmanNodeVector[0], "");
    print(huffmanNodeVector[0]);
    decompress(huffmanNodeVector[0]);
    return 0;
}
