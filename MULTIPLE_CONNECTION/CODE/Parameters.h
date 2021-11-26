#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <assert.h>

class Parameters{
private:
    std::unordered_map<std::string, std::string> map;

public:
    Parameters() = default;

    // Fills up the "map" param using argc and argv.
    void Parse(int argc, char *argv[]){
        for(int i = 0; i < argc; i++){
            if(argv[i][0] == '-'){
                std::string key = std::string(argv[i]); // Copy char* to string using string constructor.
                std::string value = "\0";
                if(i + 1 < argc && argv[i + 1][0] != '-') // If the next argument isnt of type '-' add it as a pair.
                    value = std::string(argv[i + 1]);

                map.insert({key, value});
            }   
        }
    }

    void Print(){
        std::cout << "Key | Values" << std::endl;
        for(auto pair : map){
            std::cout << "Key: " << pair.first << " | Value: " << pair.second << std::endl << std::endl;
        }   
    }

    std::vector<std::string> Get_Keys(){
        std::vector<std::string> keys;
        for(auto pair : map){
            keys.push_back(pair.first);
        }

        return keys;
    }

    bool HasKey(std::string key){
        for(auto pair : map)
            if(pair.first == key)
                return true;

        return false;
    }

    std::string GetValue(std::string key){
        for(auto pair : map)
            if(pair.first == key)
                return pair.second;

        assert(false);
        return "";
    }
};

#endif