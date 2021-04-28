#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <ctime>

#include "rank/rank_runinfo.hpp"
#include "data_struct/feature_elem.hpp"
#include "data_struct/scorer.hpp"

#define RESET "\033[0m"
#define BOLDYELLOW "\033[1m\033[33m"

using featureVect_t = std::vector<std::unique_ptr<FeatureElem>>;

void LoadIndexMeta(size_t &nb_smp, size_t &k_len, bool &stranded,
                   std::vector<std::string> &colname_vect, std::vector<double> &smp_sum_vect,
                   const std::string &idx_meta_path); // in utils/index_loading.cpp
void LoadFeaturePosMap(std::unordered_map<std::string, size_t> &ft_pos_map, std::ifstream &idx_mat, const std::string &idx_pos_path,
                       const bool need_skip_code, const size_t nb_smp);                                            // in utils/index_loading.cpp
const std::string &GetTagSeq(std::string &tag_str, std::ifstream &idx_mat, const size_t pos, const size_t nb_smp); // in utils/index_loading.cpp

// const void ScanCountComputeNF(featureVect_t &feature_vect, std::vector<double> &nf_vect, TabHeader &tab_header,
//                               const std::string &raw_counts_path, const std::string &idx_path, const std::string &rep_column)
// {
//     std::ifstream raw_counts_file(raw_counts_path);
//     if (!raw_counts_file.is_open())
//     {
//         throw std::domain_error("count table " + raw_counts_path + " was not found");
//     }
//     boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
//     {
//         size_t pos = raw_counts_path.find_last_of(".");
//         if (pos != std::string::npos && raw_counts_path.substr(pos + 1) == "gz")
//         {
//             inbuf.push(boost::iostreams::gzip_decompressor());
//         }
//     }
//     inbuf.push(raw_counts_file);
//     std::istream kmer_count_instream(&inbuf);

//     std::string line;
//     std::getline(kmer_count_instream, line);
//     std::istringstream conv(line);
//     tab_header.MakeColumns(conv, rep_column);
//     conv.clear();

//     nf_vect.resize(tab_header.GetNbCount(), 0);
//     std::cerr << "\t => Number of samples parsed: " << nf_vect.size() << std::endl;

//     std::ofstream idx_file(idx_path);
//     if (!idx_file.is_open()) // to ensure the file is opened
//     {
//         throw std::domain_error("error open file: " + idx_path);
//     }

//     std::vector<float> count_vect;
//     std::string value_str;
//     double rep_value;
//     while (std::getline(kmer_count_instream, line))
//     {
//         conv.str(line);
//         rep_value = tab_header.ParseRowStr(count_vect, value_str, conv);
//         feature_vect.emplace_back(rep_value, count_vect, value_str, idx_file);
//         for (size_t i(0); i < count_vect.size(); ++i)
//         {
//             nf_vect[i] += count_vect[i];
//         }
//         count_vect.clear();
//         conv.clear();
//     }
//     feature_vect.shrink_to_fit();
//     std::cerr << "\t => Number of features parsed: " << feature_vect.size() << std::endl;
//     double mean_sample_sum = (std::accumulate(nf_vect.cbegin(), nf_vect.cend(), 0.0) / nf_vect.size());
//     for (size_t i(0); i < nf_vect.size(); ++i)
//     {
//         if (nf_vect[i] == 0)
//         {
//             nf_vect[i] = 0;
//         }
//         else
//         {
//             nf_vect[i] = mean_sample_sum / nf_vect[i];
//         }
//     }
//     raw_counts_file.close();
//     idx_file.close();
// }

// void PrintNF(const std::string &smp_sum_outpath, const std::vector<double> &nf_vect, const TabHeader &tab_header)
// {
//     std::ofstream sum_out(smp_sum_outpath);
//     for (size_t i(1), j(0); i < tab_header.GetNbCol(); ++i)
//     {
//         if (tab_header.IsColCount(i))
//         {
//             if (nf_vect[j] > 100 || nf_vect[j] < 0.01)
//             {
//                 std::cerr << BOLDYELLOW << "[warning]"
//                           << RESET << " sample " << tab_header.GetColNameAt(i) << " has nomralization vector above 100 or below 0.01" << std::endl;
//             }
//             sum_out << tab_header.GetColNameAt(i) << "\t" << nf_vect[j++] << std::endl;
//         }
//     }
//     sum_out.close();
// }

// void EvalScore(featureVect_t &feature_vect,
//                std::ifstream &idx_file, const std::vector<double> &nf_vect,
//                std::unique_ptr<Scorer> &scorer, const TabHeader &tab_header,
//                const bool to_ln, const bool to_standardize, const bool no_norm)
// {
//     std::vector<size_t> label_vect;
//     scorer->LoadSampleLabels(tab_header.GetSampleLabelVect(label_vect));
//     for (size_t i_ft(0); i_feature < feature_vect.size(); ++i_feature)
//     {
//         scorer->PrepareCountVect(feature_vect[i_feature], nf_vect, idx_file, to_ln, to_standardize, no_norm);
//         scorer->CalcFeatureStats(feature_vect[i_feature]);
//         feature_vect[i_feature].SetScore(scorer->EvaluateScore(feature_vect[i_feature]));
//     }
// }

// void PrintHeader(std::ostream &out_s, const TabHeader &tab_header, const ScoreMethodCode score_method_code)
// {
//     std::cout << tab_header.GetColNameAt(0);
//     if (score_method_code == ScoreMethodCode::kRelatSD) // relative sd ranking output stats for all samples
//     {
//         std::cout << "\t" << kScoreMethodName[score_method_code] << "\tmean.all\tsd.all";
//     }
//     else if (score_method_code != ScoreMethodCode::kUser) // user ranking do not output score or condition stats
//     {
//         const std::vector<std::string> &condi_name_vect = tab_header.GetCondiNameVect();
//         std::cout << "\t" << kScoreMethodName[score_method_code];
//         for (size_t i(0); i < tab_header.GetNbCondition(); ++i)
//         {
//             std::cout << "\tmean." << condi_name_vect[i];
//         }
//         for (size_t i(0); i < tab_header.GetNbCondition(); ++i)
//         {
//             std::cout << "\tsd." << condi_name_vect[i];
//         }
//     }
//     std::string value_str;
//     for (size_t i(1); i < tab_header.GetNbCol(); ++i)
//     {
//         if (!tab_header.IsColCount(i)) // first output non-sample values
//         {
//             out_s << "\t" << tab_header.GetColNameAt(i);
//         }
//         else
//         {
//             value_str += "\t" + tab_header.GetColNameAt(i);
//         }
//     }
//     out_s << value_str << std::endl; // then output sample counts
// }

// void PrintFeature(std::ostream &out_s, const FeatureElem &feature_elem, std::ifstream &idx_file,
//                   const size_t nb_count, const ScoreMethodCode score_method_code)
// {
//     static std::vector<float> count_vect(nb_count);
//     static std::string value_str;
//     feature_elem.RetrieveCountVectValueStr(count_vect, value_str, idx_file, nb_count);
//     size_t split_pos = value_str.find_first_of(" \t");
//     out_s << value_str.substr(0, split_pos);
//     if (score_method_code != ScoreMethodCode::kUser)
//     {
//         std::cout << "\t" << feature_elem.GetScore();
//         for (const double m : feature_elem.GetCondiMeanVect())
//         {
//             std::cout << "\t" << m;
//         }
//         for (const double s : feature_elem.GetCondiStddevVect())
//         {
//             std::cout << "\t" << s;
//         }
//     }
//     if (split_pos != std::string::npos) // if some other value remains in value string
//     {
//         out_s << value_str.substr(split_pos);
//     }
//     for (size_t i(0); i < nb_count; ++i)
//     {
//         out_s << "\t" << count_vect[i];
//     }
//     out_s << std::endl;
// }

// void ModelPrint(const featureVect_t &feature_vect, std::ifstream &idx_file, const size_t nb_sel,
//                 const TabHeader &tab_header, const std::string &out_path, const ScoreMethodCode score_method_code)
// {
//     std::ofstream out_file;
//     if (!out_path.empty())
//     {
//         out_file.open(out_path);
//         if (!out_file.is_open())
//         {
//             throw std::domain_error("cannot open file: " + out_path);
//         }
//     }
//     auto backup_buf = std::cout.rdbuf();
//     if (!out_path.empty()) // output to file if a path is given, to screen if not
//     {
//         std::cout.rdbuf(out_file.rdbuf());
//     }
//     PrintHeader(std::cout, tab_header, score_method_code);
//     const size_t parsed_nb_sel = (nb_sel == 0 ? feature_vect.size() : nb_sel), nb_count = tab_header.GetNbCount();
//     for (size_t i(0); i < parsed_nb_sel; ++i)
//     {
//         PrintFeature(std::cout, feature_vect[i], idx_file, nb_count, score_method_code);
//     }
//     std::cout.rdbuf(backup_buf);
//     if (out_file.is_open())
//     {
//         out_file.close();
//     }
// }

void MakeFeatureVectFromIndex(featureVect_t &ft_vect, std::unordered_map<std::string, size_t> &ft_pos_map)
{
    for (auto it = ft_pos_map.cbegin(); it != ft_pos_map.cend(); it = ft_pos_map.erase(it))
    {
        ft_vect.emplace_back(std::make_unique<FeatureElem>(it->first, it->second));
    }
}

const bool MakeFeatureVectFromFile(featureVect_t &ft_vect, std::unordered_map<std::string, size_t> &ft_pos_map,
                                   const std::string &with_path)
{
    bool after_merge(false);
    std::ifstream with_file(with_path);
    if (!with_file.is_open())
    {
        throw std::domain_error("error open feature list file: " + with_path);
    }
    std::string line, feature;
    size_t nb_mem_pos(1);
    std::vector<size_t> mem_pos_vect;
    std::unordered_map<std::string, size_t>::const_iterator it;
    std::istringstream line_conv;
    while (std::getline(with_file, line))
    {
        line_conv.str(line);
        line_conv >> feature;
        if (line_conv >> nb_mem_pos)
        {
            mem_pos_vect.resize(nb_mem_pos);
            line_conv.read(reinterpret_cast<char *>(&mem_pos_vect[0]), nb_mem_pos * sizeof(size_t));
            ft_vect.emplace_back(std::make_unique<FeatureElem>(feature, mem_pos_vect));
            mem_pos_vect.clear();
            after_merge = true;
        }
        else
        {
            it = ft_pos_map.find(feature);
            if (it == ft_pos_map.cend())
            {
                throw std::invalid_argument("feature " + feature + " in the given list not contained in the index");
            }
            ft_vect.emplace_back(std::make_unique<FeatureElem>(feature, it->second));
        }
        line_conv.clear();
    }
    ft_pos_map.clear();
    with_file.close();
    return after_merge;
}

void ParseDesign(std::vector<size_t> &condi_label_vect, std::vector<size_t> &batch_label_vect,
                 const std::string &dsgn_path, const std::vector<std::string> &colname_vect, const size_t nb_smp)
{
    std::ifstream dsgn_file(dsgn_path);
    if (!dsgn_file.is_open())
    {
        throw std::invalid_argument("error open design file: " + dsgn_path);
    }
    condi_label_vect.resize(nb_smp), batch_label_vect.resize(nb_smp);
    std::string line, smp_name, condi, batch;
    std::istringstream line_conv;
    size_t nb_condi(0), nb_batch(0);
    std::unordered_map<std::string, size_t> condi2label, batch2label;
    std::unordered_map<std::string, std::pair<size_t, size_t>> smp_condi_batch_map;
    while (std::getline(dsgn_file, line))
    {
        line_conv.str(line);
        line_conv >> smp_name >> condi >> batch;
        const auto &ins_condi = condi2label.insert({condi, nb_condi + 1});
        if (ins_condi.second)
        {
            nb_condi++;
        }
        const auto &ins_batch = batch2label.insert({batch, nb_batch + 1});
        if (ins_batch.second)
        {
            nb_batch++;
        }
        smp_condi_batch_map.insert({smp_name, std::make_pair(ins_condi.first->second, ins_batch.first->second)});
        line_conv.clear();
    }
    for (size_t i(1); i <= nb_smp; ++i)
    {
        const auto &it = smp_condi_batch_map.find(colname_vect[i]);
        if (it == smp_condi_batch_map.cend())
        {
            throw std::invalid_argument("column name in index not found in design file: " + colname_vect[i]);
        }
        condi_label_vect[i - 1] = it->second.first;
        batch_label_vect[i - 1] = it->second.second;
    }
    // for (size_t i(0); i < nb_smp; ++i)
    // {
    //     std::cout << colname_vect[i + 1] << "\t" << condi_label_vect[i] << "\t" << batch_label_vect[i] << std::endl;
    // }
    dsgn_file.close();
}

void SortScore(featureVect_t &ft_vect, const ScorerCode scorer_code)
{
    if (scorer_code == ScorerCode::kSNR) // decabs
    {
        auto comp = [](const std::unique_ptr<FeatureElem> &ft1, const std::unique_ptr<FeatureElem> &ft2)
            -> bool { return fabs(ft1->GetScore()) > fabs(ft2->GetScore()); };
        std::sort(ft_vect.begin(), ft_vect.end(), comp);
    }
    else if (scorer_code == ScorerCode::kNBC || scorer_code == ScorerCode::kLR || scorer_code == ScorerCode::kSVM) // dec
    {
        auto comp = [](const std::unique_ptr<FeatureElem> &ft1, const std::unique_ptr<FeatureElem> &ft2)
            -> bool { return ft1->GetScore() > ft2->GetScore(); };
        std::sort(ft_vect.begin(), ft_vect.end(), comp);
    }
    else if (scorer_code == ScorerCode::kTtest) // inc
    {
        auto comp = [](const std::unique_ptr<FeatureElem> &ft1, const std::unique_ptr<FeatureElem> &ft2)
            -> bool { return ft1->GetScore() < ft2->GetScore(); };
        std::sort(ft_vect.begin(), ft_vect.end(), comp);
    }
    // else if (...) // incabs, useless
    // {
    //     auto comp = [](const std::unique_ptr<FeatureElem> &ft1, const std::unique_ptr<FeatureElem> &ft2)
    //         -> bool { return fabs(ft1->GetScore()) < fabs(ft2->GetScore()); };
    //     std::sort(ft_vect.begin(), ft_vect.end(), comp);
    // }
}

void PrintHeader(const bool after_merge, const std::vector<std::string> &colname_vect, const std::string &scorer_name)
{
    if (after_merge)
    {
        std::cout << "contig\tnb-merged-kmer";
    }
    else
    {
        std::cout << "feature";
    }
    std::cout << "\t" << scorer_name;
    for (const auto &s : colname_vect)
    {
        std::cout << "\t" << s;
    }
    std::cout << std::endl;
}

void PrintWithCounts(const bool after_merge, const featureVect_t &ft_vect, std::ifstream &idx_mat,
                     const std::string &count_mode, const size_t nb_smp, const size_t max_to_sel)
{
    std::vector<float> count_vect;
    std::string rep_seq;
    for (size_t i_ft(0); i_ft < max_to_sel; ++i_ft)
    {
        std::cout << ft_vect[i_ft]->GetFeature();
        if (after_merge)
        {
            std::cout << "\t" << ft_vect[i_ft]->GetNbMemPos();
            std::cout << "\t" << GetTagSeq(rep_seq, idx_mat, ft_vect[i_ft]->GetRepPos(), nb_smp);
        }
        std::cout << "\t" << ft_vect[i_ft]->GetScore();
        ft_vect[i_ft]->EstimateCountVect(count_vect, idx_mat, nb_smp, count_mode);
        for (float x : count_vect)
        {
            std::cout << "\t" << x;
        }
        std::cout << std::endl;
        count_vect.clear();
    }
}

void PrintAsIntermediate(const featureVect_t &ft_vect, const size_t max_to_sel)
{
    for (size_t i_ft(0); i_ft < max_to_sel; ++i_ft)
    {
        std::cout << ft_vect[i_ft]->GetFeature() << "\t" << ft_vect[i_ft]->GetScore() << std::endl;
    }
}

int RankMain(int argc, char *argv[])
{
    RankWelcome();

    std::clock_t begin_time = clock(), inter_time;
    std::string idx_dir, rk_mthd, with_path, count_mode, dsgn_path, out_path;
    float sel_top(-1); // negative value means without selection, print all features
    size_t nfold, nb_smp, k_len, max_to_sel;
    bool ln_transf(false), standardize(false), no_norm(false), with_counts(false), after_merge(false), _stranded; // _stranded not needed in KaMRaT-rank

    ParseOptions(argc, argv, idx_dir, rk_mthd, nfold, with_path, count_mode, dsgn_path, ln_transf, standardize, no_norm, sel_top, out_path, with_counts);
    PrintRunInfo(idx_dir, rk_mthd, nfold, with_path, count_mode, dsgn_path, ln_transf, standardize, no_norm, sel_top, out_path, with_counts);

    std::vector<std::string> colname_vect;
    std::vector<double> smp_sum_vect;
    LoadIndexMeta(nb_smp, k_len, _stranded, colname_vect, smp_sum_vect, idx_dir + "/idx-meta.bin");

    std::ifstream idx_mat(idx_dir + "/idx-mat.bin");
    if (!idx_mat.is_open())
    {
        throw std::invalid_argument("loading index-mat failed, KaMRaT index folder not found or may be corrupted");
    }

    featureVect_t ft_vect;
    std::unordered_map<std::string, size_t> ft_pos_map;
    LoadFeaturePosMap(ft_pos_map, idx_mat, idx_dir + "/idx-pos.bin", k_len != 0, nb_smp);
    if (with_path.empty())
    {
        MakeFeatureVectFromIndex(ft_vect, ft_pos_map);
    }
    else
    {
        after_merge = MakeFeatureVectFromFile(ft_vect, ft_pos_map, with_path);
    }
    std::cerr << "Option parsing and index loading finished, execution time: " << (float)(clock() - begin_time) / CLOCKS_PER_SEC << "s." << std::endl;
    inter_time = clock();

    if (sel_top <= 0)
    {
        max_to_sel = ft_vect.size();
    }
    else if (sel_top < 1)
    {
        max_to_sel = static_cast<size_t>(ft_vect.size() * sel_top + 0.5);
    }
    else if (sel_top <= ft_vect.size())
    {
        max_to_sel = static_cast<size_t>(sel_top + 0.00005);
    }
    else
    {
        throw std::invalid_argument("number of top feature selection exceeds total feature number: " +
                                    std::to_string(static_cast<size_t>(sel_top + 0.00005)) + ">" + std::to_string(ft_vect.size()));
    }
    std::vector<size_t> condi_label_vect, batch_label_vect;
    ParseDesign(condi_label_vect, batch_label_vect, dsgn_path, colname_vect, nb_smp);
    Scorer scorer(rk_mthd, nfold, smp_sum_vect, condi_label_vect, batch_label_vect);
    std::vector<float> count_vect;
    for (const auto &ft : ft_vect)
    {
        ft->EstimateCountVect(count_vect, idx_mat, nb_smp, count_mode);
        ft->SetScore(scorer.EstimateScore(count_vect, no_norm, ln_transf, standardize));
    }
    SortScore(ft_vect, scorer.GetScorerCode());
    std::cerr << "Score evalution finished, execution time: " << (float)(clock() - inter_time) / CLOCKS_PER_SEC << "s." << std::endl;
    inter_time = clock();
    if (scorer.GetScorerCode() == ScorerCode::kTtest) // BH procedure
    {
        std::cerr << "\tadjusting p-values using BH procedure..." << std::endl
                  << std::endl;
        for (size_t tot_num(ft_vect.size()), i_ft(tot_num - 2); i_ft >= 0 && i_ft < tot_num; --i_ft)
        {
            ft_vect[i_ft]->AdjustScore(static_cast<double>(tot_num) / (i_ft + 1), 0, ft_vect[i_ft + 1]->GetScore());
        }
        std::cerr << "P-value adjusting finished, execution time: " << (float)(clock() - inter_time) / CLOCKS_PER_SEC << "s." << std::endl;
        inter_time = clock();
    }

    if (with_counts)
    {
        PrintHeader(after_merge, colname_vect, scorer.GetScorerName());
        PrintWithCounts(after_merge, ft_vect, idx_mat, count_mode, nb_smp, max_to_sel);
    }
    else
    {
        PrintAsIntermediate(ft_vect, max_to_sel);
    }
    std::cerr << "Output finished, execution time: " << (float)(clock() - inter_time) / CLOCKS_PER_SEC << "s." << std::endl;
    std::cerr << "Executing time: " << (float)(clock() - begin_time) / CLOCKS_PER_SEC << std::endl;

    idx_mat.close();
    return EXIT_SUCCESS;
}
