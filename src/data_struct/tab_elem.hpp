#ifndef KAMRAT_DATASTRUCT_TABELEM_HPP
#define KAMRAT_DATASTRUCT_TABELEM_HPP

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "tab_header.hpp"

class TabElem
{
public:
    const void ParseTabElem(std::istringstream &line_conv, std::ofstream &idx_file,
                            float &rep_val,
                            const TabHeader &tab_header); // Index and get the score value ---> for KaMRaT merge
    const float ParseTabElem(std::istringstream &line_conv, std::ofstream &idx_file,
                             std::vector<float> &count_vect,
                             const TabHeader &tab_header); // Index and get the count vector ---> for KaMRaT rank

    const std::vector<float> &GetCountVect(std::vector<float> &count_vect, std::ifstream &idx_file, size_t nb_count) const;
    const float GetValueAt(std::ifstream &idx_file, size_t value_serial, size_t nb_count) const;
    const void GetVectsAndClear(std::vector<float> &count_vect, std::vector<float> &value_vect,
                                std::ifstream &idx_file, size_t nb_count, size_t nb_value);
    const size_t GetIndexPos() const;

private:
    std::vector<float> value_vect_; // value vector ------ used both in inMem or onDsk
    std::vector<float> count_vect_; // count vector ------ only used in inMem
    size_t index_pos_;              // indexed position for k-mer count ------ only used in onDsk
};

using code2serial_t = std::unordered_map<uint64_t, size_t>; // external dictionary to link k-mer with row serial number
using countTab_t = std::vector<TabElem>;                    // feature table

#endif //KAMRAT_DATASTRUCT_TABELEM_HPP