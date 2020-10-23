#include <fstream>
#include <sstream>

#include "feature_tab_header.hpp"

FeatureTabHeader::FeatureTabHeader(const std::string &sample_info_path, const std::unordered_set<std::string> &preserved_cond_tags)
    : nb_col_(0), nb_condi_(0), nb_value_(0), nb_count_(0), nb_str_(0)
{
    if (!sample_info_path.empty())
    {
        std::ifstream sample_info_file(sample_info_path);
        if (!sample_info_file.is_open())
        {
            throw std::domain_error("could not open sample-info file: " + sample_info_path);
        }
        std::string sample_info_line, sample, condition;
        std::istringstream conv;
        while (std::getline(sample_info_file, sample_info_line))
        {
            conv.str(sample_info_line);
            conv >> sample >> condition;
            if (conv.fail())
            {
                condition = ""; // empty string indicating the unique condition
            }
            if (preserved_cond_tags.find(condition) != preserved_cond_tags.cend())
            {
                throw std::domain_error("condition name (" + condition + ") is preserved by KaMRaT, please change another name for this condition");
            }
            auto ins = condi2lab_.insert({condition, 'A' + nb_condi_}); // try to add the condition into the condition dictionary
            if (ins.second)                                             // if a new condition is added to the dictionary
            {
                ++nb_condi_;
            }
            if (!smp2lab_.insert({sample, ins.first->second}).second) // associate the sample with its condition label
            {
                throw std::domain_error("sample-info file has duplicated sample name: " + sample);
            }
            conv.clear();
        }
        sample_info_file.close();
        if (nb_condi_ != condi2lab_.size()) // should not happen, for debug
        {
            throw std::domain_error("number of condition not consistent");
        }
    }
}

const size_t FeatureTabHeader::MakeColumnInfo(const std::string &header_line, const std::string &rep_colname)
{
    if (header_line.empty()) // quit if header line is empty
    {
        throw std::domain_error("cannot parse column information with an empty header line");
    }
    size_t repval_pos(0);
    std::istringstream conv(header_line);
    std::string term;
    // the first column is supposed to be feature string //
    conv >> term;
    colname_vect_.emplace_back(term);
    colnature_vect_.emplace_back('s');
    colserial_vect_.emplace_back(0); // place hoder: first column always as string
    ++nb_col_;
    // following columns //
    if (condi2lab_.empty()) // if sample info file NOT provided, then all next columns are samples
    {
        while (conv >> term)
        {
            colname_vect_.emplace_back(term);
            colnature_vect_.emplace_back('A');
            colserial_vect_.emplace_back(nb_count_++);
            ++nb_col_;
        }
    }
    else // sample info file provided => next columns may contain non-sample values
    {
        while (conv >> term)
        {
            colname_vect_.emplace_back(term);
            auto iter = smp2lab_.find(term);
            if (iter != smp2lab_.cend())
            {
                colnature_vect_.emplace_back(iter->second);
                colserial_vect_.emplace_back(nb_count_++);
            }
            else if (term == "tag" || term == "feature" || term == "contig")
            {
                colnature_vect_.emplace_back('s');
                colserial_vect_.emplace_back(nb_str_++);
            }
            else
            {
                if (term == rep_colname)
                {
                    repval_pos = nb_col_;
                }
                colnature_vect_.emplace_back('v');
                colserial_vect_.emplace_back(nb_value_++);
            }
            ++nb_col_;
        }
    }
    if (nb_count_ == 0)
    {
        throw std::domain_error("no sample column found");
    }
    if (nb_col_ != colname_vect_.size() || nb_col_ != colserial_vect_.size() || nb_col_ != colnature_vect_.size()) // should not happen, for debug
    {
        throw std::domain_error("colname, colserial, colnature sizes not coherent");
    }
    return repval_pos;
}

const size_t FeatureTabHeader::GetNbValue() const
{
    return nb_value_;
}

const size_t FeatureTabHeader::GetNbCount() const
{
    return nb_count_;
}

const size_t FeatureTabHeader::GetNbStr() const
{
    return nb_str_;
}

const size_t FeatureTabHeader::GetNbCol() const
{
    return nb_col_;
}

const size_t FeatureTabHeader::GetNbCondition() const
{
    return nb_condi_;
}

const std::vector<std::string> &FeatureTabHeader::GetColNameVect() const
{
    return colname_vect_;
}

const std::vector<char> &FeatureTabHeader::GetColNatureVect() const
{
    return colnature_vect_;
}

const std::vector<size_t> &FeatureTabHeader::GetColSerialVect() const
{
    return colserial_vect_;
}

const bool FeatureTabHeader::IsSample(size_t i_col) const
{
    if (i_col > nb_col_)
    {
        throw std::domain_error("column number in search exceeds table's column number");
    }
    return (colnature_vect_[i_col] >= 'A' && colnature_vect_[i_col] <= 'Z');
}

const void FeatureTabHeader::ParseSmpLabels(std::vector<size_t> &smp_labels)
{
    if (!smp_labels.empty())
    {
        throw std::domain_error("sample label vector not empty before adding sample labels");
    }
    for (size_t i(0); i < colnature_vect_.size(); ++i)
    {
        if (colnature_vect_[i] >= 'A' && colnature_vect_[i] <= 'Z')
        {
            smp_labels.emplace_back(colnature_vect_[i] - 'A');
        }
    }
}
