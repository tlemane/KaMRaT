#include <algorithm>

#include "contig_elem.hpp"

inline void ToComplement(std::string &seq)
{
    auto lambda_trans = [](const char c) {
        switch (c)
        {
        case 'A':
            return 'T';
        case 'G':
            return 'C';
        case 'C':
            return 'G';
        case 'T':
            return 'A';
        default:
            return 'N';
        }
    };
    std::transform(seq.cbegin(), seq.cend(), seq.begin(), lambda_trans);
}

static inline const void ReverseComplementSeq(std::string &seq)
{
    reverse(seq.begin(), seq.end());
    ToComplement(seq);
}

ContigElem::ContigElem(const std::string &seq, const size_t init_serial)
    : is_used_(false), seq_(seq),
      head_kmer_serial_(init_serial), rep_kmer_serial_(init_serial), rear_kmer_serial_(init_serial)
{
    mem_kmer_serial_vect_.push_back(init_serial);
}

const bool ContigElem::IsUsed() const
{
    return is_used_;
}

const void ContigElem::SetUsed()
{
    is_used_ = true;
}

const std::string &ContigElem::GetSeq() const
{
    return seq_;
}

const unsigned int ContigElem::GetNbKMer() const
{
    return mem_kmer_serial_vect_.size();
}

const size_t ContigElem::GetHeadKMerSerial(const bool if_need_reverse) const
{
    return (if_need_reverse ? rear_kmer_serial_ : head_kmer_serial_);
}

const uint64_t ContigElem::GetRepKMerSerial() const
{
    return rep_kmer_serial_;
}

const size_t ContigElem::GetRearKMerSerial(const bool if_need_reverse) const
{
    return (if_need_reverse ? head_kmer_serial_ : rear_kmer_serial_);
}

const std::vector<size_t> &ContigElem::GetMemKMerSerialVect() const
{
    return mem_kmer_serial_vect_;
}


const void ContigElem::LeftExtend(const ContigElem &left_contig_elem, const bool need_left_rc, unsigned int n_overlap)
{
    static std::string left_seq;
    left_seq = left_contig_elem.GetSeq();
    if (need_left_rc)
    {
        ReverseComplementSeq(left_seq);
    }
    seq_ = left_seq + seq_.substr(n_overlap);
    head_kmer_serial_ = left_contig_elem.GetHeadKMerSerial(need_left_rc);
    mem_kmer_serial_vect_.insert(mem_kmer_serial_vect_.end(),
                                 std::make_move_iterator(left_contig_elem.GetMemKMerSerialVect().begin()),
                                 std::make_move_iterator(left_contig_elem.GetMemKMerSerialVect().end()));
    left_seq.clear();
}

const void ContigElem::RightExtend(const ContigElem &right_contig_elem, const bool need_right_rc, unsigned int n_overlap)
{
    static std::string right_seq;
    right_seq = right_contig_elem.GetSeq();
    if (need_right_rc)
    {
        ReverseComplementSeq(right_seq);
    }
    seq_ = seq_ + std::move(right_seq).substr(n_overlap);
    rear_kmer_serial_ = right_contig_elem.GetRearKMerSerial(need_right_rc);
    mem_kmer_serial_vect_.insert(mem_kmer_serial_vect_.end(),
                                 std::make_move_iterator(right_contig_elem.GetMemKMerSerialVect().begin()),
                                 std::make_move_iterator(right_contig_elem.GetMemKMerSerialVect().end()));
    right_seq.clear();
}
