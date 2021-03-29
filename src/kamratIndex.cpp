#include <iostream>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "index/index_runinfo.hpp"

/* -------------------------------------------------- *\ 
 * idx-info:                                          *
 *   - header line indicating column names            *
 *   - {position, feature} pairs ordered by position  *
 *   - normalization factor (binarized double vector) *
 * idx-mat:                                           *
 *   - feature counts (binarized float vector)        *
\* -------------------------------------------------- */

void ScanIndexCountTab(std::ofstream &idx_info, std::ofstream &idx_mat, std::istream &kmer_count_instream)
{
    size_t nb_smp(0);
    std::string line, term;
    std::getline(kmer_count_instream, line);
    idx_info << line << std::endl; // in idx_info: write the header line
    std::istringstream conv(line);
    for (conv >> term; conv >> term; ++nb_smp) // count sample number, skipping the first column of feature names
    {
    }
    conv.clear();

    std::map<std::string, size_t> feature_pos_map;
    std::vector<float> count_vect;
    std::vector<double> nf_vect(nb_smp, 0);
    double all_sum(0);
    while (std::getline(kmer_count_instream, line))
    {
        conv.str(line);
        conv >> term;
        feature_pos_map.insert({term, static_cast<size_t>(idx_mat.tellp())});
        for (; conv >> term; count_vect.push_back(std::stof(term))) // parse sample count skipping the first column of feature names
        {
        }
        idx_mat.write(reinterpret_cast<char *>(&count_vect[0]), count_vect.size() * sizeof(float)); // in idx_mat: write feature count vector
        conv.clear();
        if (count_vect.size() != nb_smp)
        {
            throw std::length_error("sample numbers are not consistent: " + std::to_string(nb_smp) + " vs " + std::to_string(count_vect.size()));
        }
        for (size_t i_smp(0); i_smp < nb_smp; ++i_smp) // add the counts for normalization factor evaluation
        {
            nf_vect[i_smp] += count_vect[i_smp];
            all_sum += count_vect[i_smp];
        }
        count_vect.clear();
    }
    idx_mat << std::endl;
    for (size_t i_smp(0); i_smp < nb_smp; ++i_smp)
    {
        nf_vect[i_smp] = all_sum / nb_smp / nf_vect[i_smp]; // norm_count = raw_count * nf
    }
    idx_info.write(reinterpret_cast<char *>(&nf_vect[0]), nf_vect.size() * sizeof(double)); // in idx_info: write normalization factors
    idx_info << std::endl;
    for (auto &elem : feature_pos_map) // in idx_info: write {position, feature} pairs
    {
        idx_info.write(reinterpret_cast<char *>(&(elem.second)), sizeof(size_t));
        idx_info << elem.first << std::endl;
    }
}

void ScanIndex(const std::string &out_dir, const std::string &count_tab_path)
{
    std::ofstream idx_info(out_dir + "/kamrat-idx-info.bin"), idx_mat(out_dir + "/kamrat-idx-mat.bin");
    if (!idx_info.is_open() || !idx_mat.is_open())
    {
        throw std::invalid_argument("output folder for index does not exist: " + out_dir);
    }
    std::ifstream count_tab(count_tab_path);
    if (!count_tab.is_open())
    {
        throw std::invalid_argument("cannot open count table file: " + count_tab_path);
    }
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    {
        size_t pos = count_tab_path.find_last_of(".");
        if (pos != std::string::npos && count_tab_path.substr(pos + 1) == "gz")
        {
            inbuf.push(boost::iostreams::gzip_decompressor());
        }
    }
    inbuf.push(count_tab);
    std::istream kmer_count_instream(&inbuf);

    ScanIndexCountTab(idx_info, idx_mat, kmer_count_instream);

    count_tab.close();
    idx_mat.close();
    idx_info.close();
}

int IndexMain(int argc, char **argv)
{
    IndexWelcome();

    std::clock_t begin_time = clock();
    std::string out_dir, count_tab_path;

    ParseOptions(argc, argv, out_dir, count_tab_path);
    PrintRunInfo(out_dir, count_tab_path);

    std::vector<double> nf_vect;
    ScanIndex(out_dir, count_tab_path);

    std::cerr << "Count table indexing finished, execution time: " << (float)(clock() - begin_time) / CLOCKS_PER_SEC << "s." << std::endl;
    std::cerr << "Total executing time: " << (float)(clock() - begin_time) / CLOCKS_PER_SEC << "s." << std::endl;

    return EXIT_SUCCESS;
}
