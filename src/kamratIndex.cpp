#include <iostream>
#include <ctime>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "index/index_runinfo.hpp"

#define RESET "\033[0m"
#define BOLDYELLOW "\033[1m\033[33m"

/* ----------------------------------------------------------------- *\ 
 * idx-meta:                                                         *
 *   - sample number, k length (0 if general feature), strandedness  * 
 *   - header row indicating column names                            *
 *   - sample sum vector (binarized double vector)                   *
 * idx-ftpos:                                                        *
 *   - {feature string, position} ordered by feature                 *
 * idx-mat:                                                          *
 *   - feature counts (binarized float vector)                       *
\* ----------------------------------------------------------------- */

const uint64_t Seq2Int(const std::string &seq, const size_t k_len, const bool stranded); // seq_coding.cpp

const size_t CountColumn(std::ofstream &idx_meta, const std::string &line_str)
{
    size_t nb_smp(0);
    std::istringstream conv(line_str);
    std::string term;
    for (conv >> term; conv >> term; ++nb_smp) // count sample number, skipping the first column
    {
    }
    if (nb_smp == 0)
    {
        throw std::domain_error("input table parsing failed: sample number equals to 0");
    }
    return nb_smp;
}

const void IndexCount(std::ofstream &idx_pos, std::ofstream &idx_mat, std::vector<double> &sum_vect,
                      const std::string &line_str, const size_t k_len, const bool stranded, const size_t nb_smp)
{
    static std::istringstream conv(line_str);
    static std::vector<float> count_vect;
    static std::string term;
    static std::set<uint64_t> kmer_set;

    conv.str(line_str);
    conv >> term; // feature name string
    if (k_len != 0)
    {
        if (k_len != term.size()) // check k-mer length
        {
            throw std::length_error("feature length checking failed: length of " + term + " not equal to " + std::to_string(k_len));
        }
        if (!kmer_set.insert(Seq2Int(term, k_len, stranded)).second)
        {
            throw std::domain_error("insertion failed, an equivalent key already existed for feature: " + term);
        }
    }

    idx_pos << term << "\t" << static_cast<size_t>(idx_mat.tellp()) << std::endl; // [idx_pos] feature position in indexed matrix
    for (; conv >> term; count_vect.push_back(std::stof(term))) // parse the following count vector
    {
    }
    idx_mat.write(reinterpret_cast<char *>(&count_vect[0]), count_vect.size() * sizeof(float)); // [idx_mat] feature count vector

    if (count_vect.size() != nb_smp) // check if all rows have same number of columns as the header row
    {
        throw std::length_error("sample numbers are not consistent: " + std::to_string(nb_smp) + " vs " + std::to_string(count_vect.size()));
    }
    for (size_t i_smp(0); i_smp < nb_smp; ++i_smp) // add count vectors together for eventual normalization
    {
        sum_vect[i_smp] += count_vect[i_smp];
    }

    term.clear();
    count_vect.clear();
    conv.clear();
}

void ScanIndex(std::ofstream &idx_meta, std::ofstream &idx_pos, std::ofstream &idx_mat,
               std::istream &kmer_count_instream, const size_t k_len, const bool stranded)
{
    std::string line;
    std::getline(kmer_count_instream, line); // read header row in table
    size_t nb_smp = CountColumn(idx_meta, line);

    idx_meta << nb_smp << "\t" << k_len; // [idx_meta 1] sample number, k-mer length, and strandedness if applicable
    if (k_len != 0)
    {
        idx_meta << "\t" << (stranded ? 'T' : 'F');
    }
    idx_meta << std::endl;

    idx_meta << line << std::endl; // [idx_meta 2] the header row

    std::vector<double> sum_vect(nb_smp, 0);
    while (std::getline(kmer_count_instream, line))
    {
        IndexCount(idx_pos, idx_mat, sum_vect, line, k_len, stranded, nb_smp); // [idx_pos, idx_mat] (inside)
    }
    idx_meta.write(reinterpret_cast<char *>(&sum_vect[0]), sum_vect.size() * sizeof(double)); // [idx_meta 3] sample sum vector
}

int IndexMain(int argc, char **argv)
{
    IndexWelcome();

    std::clock_t begin_time = clock();
    std::string out_dir, count_tab_path;
    size_t k_len(0);
    bool kmer_mode(false), stranded(true);

    ParseOptions(argc, argv, count_tab_path, out_dir, kmer_mode, k_len, stranded);
    PrintRunInfo(count_tab_path, out_dir, kmer_mode, k_len, stranded);

    if (!kmer_mode)
    {
        std::cerr << BOLDYELLOW << "[warning]" << RESET << " indexing in general: features are not considered as k-mers" << std::endl
                  << std::endl;
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

    std::ofstream idx_meta(out_dir + "/idx-info.bin"), idx_pos(out_dir + "/idx-pos.bin"), idx_mat(out_dir + "/idx-mat.bin");
    if (!idx_meta.is_open() || !idx_pos.is_open() || !idx_mat.is_open())
    {
        throw std::invalid_argument("output folder for index does not exist: " + out_dir);
    }
    ScanIndex(idx_meta, idx_pos, idx_mat, kmer_count_instream, k_len, stranded);

    idx_mat.close();
    idx_pos.close();
    idx_meta.close();
    count_tab.close();

    std::cerr << "Count table indexing finished, execution time: " << (float)(clock() - begin_time) / CLOCKS_PER_SEC << "s." << std::endl;
    std::cerr << "Total executing time: " << (float)(clock() - begin_time) / CLOCKS_PER_SEC << "s." << std::endl;

    return EXIT_SUCCESS;
}
