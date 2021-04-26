#ifndef KAMRAT_RANK_RANKRUNINFO_HPP
#define KAMRAT_RANK_RANKRUNINFO_HPP

#include <iostream>
#include <string>
#include <memory>

const void ParseScorer(std::unique_ptr<Scorer> &scorer, std::string &score_method, const bool to_ln, const bool to_standardize)
{
    std::string score_cmd;
    size_t split_pos = score_method.find(":");
    if (split_pos != std::string::npos)
    {
        score_cmd = score_method.substr(split_pos + 1);
        score_method = score_method.substr(0, split_pos);
    }
    if (score_method == "rsd")
    {
        if (to_standardize)
        {
            throw std::invalid_argument("Standard deviation score should not be applied on standardized counts\n"
                                        "And the forced running is not permitted either (divided by 0)");
        }
        scorer = std::make_unique<Scorer>(ScoreMethodCode::kRelatSD);
    }
    else if (score_method == "ttest")
    {
        if (!to_ln && score_cmd != "F")
        {
            throw std::invalid_argument("Ttest requires ln(x + 1) transformation, put -score-method ttest:F to force to run");
        }
        scorer = std::make_unique<Scorer>(ScoreMethodCode::kTtest);
    }
    else if (score_method == "snr")
    {
        scorer = std::make_unique<Scorer>(ScoreMethodCode::kSNR);
    }
    else if (score_method == "lrc")
    {
        if (!score_cmd.empty())
        {
            scorer = std::make_unique<Scorer>(ScoreMethodCode::kLogitReg, std::stoul(score_cmd)); // C++: typedef unsigned long size_t
        }
        else
        {
            scorer = std::make_unique<Scorer>(ScoreMethodCode::kLogitReg, 1); // by default, evaluate features without cross-validation
        }
    }
    else if (score_method == "nbc")
    {
        if (!score_cmd.empty())
        {
            scorer = std::make_unique<Scorer>(ScoreMethodCode::kNaiveBayes, std::stoul(score_cmd)); // C++: typedef unsigned long size_t
        }
        else
        {
            scorer = std::make_unique<Scorer>(ScoreMethodCode::kNaiveBayes, 1); // by default, evaluate features without cross-validation
        }
    }
    else if (score_method == "svm")
    {
        if (!to_standardize && score_cmd != "F")
        {
            throw std::invalid_argument("Standardization is required for SVM classification, put -score-method svm:F to force to run");
        }
        scorer = std::make_unique<Scorer>(ScoreMethodCode::kSVM);
    }
    else
    {
        if (score_cmd == "dec")
        {
            scorer = std::make_unique<Scorer>(score_method, SortModeCode::kDec);
        }
        else if (score_cmd == "decabs")
        {
            scorer = std::make_unique<Scorer>(score_method, SortModeCode::kDecAbs);
        }
        else if (score_cmd == "inc")
        {
            scorer = std::make_unique<Scorer>(score_method, SortModeCode::kInc);
        }
        else if (score_cmd == "incabs")
        {
            scorer = std::make_unique<Scorer>(score_method, SortModeCode::kIncAbs);
        }
        else if (score_cmd.empty())
        {
            throw std::invalid_argument("user-defined ranking with column (" + score_method + ") but without indicating sorting mode");
        }
        else
        {
            throw std::invalid_argument("unknown sorting mode: " + score_cmd);
        }
    }
}

void RankWelcome()
{
    std::cerr << "KaMRaT rank: univariate feature ranking" << std::endl
              << "-------------------------------------------------------------------------------------------------------------------" << std::endl;
}

void PrintRankHelper()
{
    std::cerr << "[USAGE]    kamrat rank -idx-dir STR -count-mode STR -rank-by STR [-options] FEATURE_TAB_PATH" << std::endl
              << std::endl;
    std::cerr << "[OPTION]    -h,-help             Print the helper " << std::endl;
    std::cerr << "            -idxdir STR              Indexing folder by KaMRaT index, mandatory" << std::endl;
    std::cerr << "            -rankby STR          Ranking method, mandatory, can be one of: " << std::endl
              << "                                     sd     standard deviation" << std::endl
              << "                                     rsd    relative standard deviation" << std::endl
              << "                                     ttest  adjusted p-value of t-test between conditions (require -ln)" << std::endl
              << "                                     snr    signal-to-noise ratio between conditions" << std::endl
              << "                                     lrc:nfold    accuracy by logistic regression classifier" << std::endl
              << "                                     nbc:nfold    accuracy by naive Bayes classifier" << std::endl
              << "                                     svm:nfold    accuracy on SVM classifier" << std::endl;
    std::cerr << "            -out-nf              If present, output nomalization factor to given file" << std::endl;
    std::cerr << "            -with STR1[:STR2]    File indicating features to rank (STR1) and counting mode (STR2)" << std::endl
              << "                                     if not provided, all indexed features are used for ranking" << std::endl
              << "                                     STR2 can be one of [rep, mean, median]" << std::endl;
    std::cerr << "            -design STR          File indicating sample-condition design, without header line" << std::endl
              << "                                     if not provided, all samples are assigned by the same condition" << std::endl
              << "                                     if provided, each row can be either: " << std::endl
              << "                                         sample name, sample condition" << std::endl
              << "                                         sample name, sample condition, sample batch (only for lrc, nbc, and svm)" << std::endl;
    std::cerr << "            -ln                  Apply ln(x + 1) transformation for score estimation [false]" << std::endl;
    std::cerr << "            -standardize         Standarize count vector for score estimation [false]" << std::endl;
    std::cerr << "            -rankonraw           Estimate scores and rank on raw count, without normalization" << std::endl;
    std::cerr << "            -seltop NUM          If NUM > 1, it indicates top number of features to output (treated as integer)" << std::endl
              << "                                 If NUM <= 1, it indicates the ratio of features to output" << std::endl;
    std::cerr << "            -outpath STR         Path of ranking result" << std::endl
              << "                                     if not provided, output to screen" << std::endl;
    std::cerr << "            -withcounts          Output sample count vectors [false]" << std::endl
              << std::endl;
    std::cerr << "[NOTE]      For ranking methods lrc, nbc, and svm, there can be a second univariant cross-validaton option (nfold)" << std::endl
              << "                if nfold = 0, leave-one-out cross-validation" << std::endl
              << "                if nfold = 1, without cross-validation, training and testing on the whole datset" << std::endl
              << "                if nfold >=2, n-fold cross-validation" << std::endl
              << std::endl;
}

void PrintRunInfo(const std::string &idx_dir,
                  const std::string &rk_mthd, const size_t nfold,
                  const std::string &out_nf,
                  const std::string &with_path, const std::string &count_mode,
                  const std::string &dsgn_path,
                  const bool ln_transf,
                  const bool standardize,
                  const bool no_norm,
                  const float sel_top,
                  const std::string &out_path,
                  const bool with_counts)
{
    std::cerr << std::endl;
    std::cerr << "KaMRaT index:             " << idx_dir << std::endl;
    std::cerr << "Ranking method:           " << rk_mthd << std::endl;
    if (rk_mthd == "nbc" || score_method_code == ScoreMethodCode::kLogitReg)
    {
        size_t nb_fold = scorer->GetNbFold();
        if (nb_fold == 0)
        {
            std::cerr << "    Leave-one-out cross-validation";
        }
        else if (nb_fold == 1)
        {
            std::cerr << "    No cross-validation, train and test on the whole dataset";
        }
        else
        {
            std::cerr << "    Cross-validation with fold number = " << nb_fold;
        }
        std::cerr << std::endl;
    }



    std::cerr << "Select k-mers in file:    " << (sel_path.empty() ? "k-mers in index" : sel_path) << std::endl;
    std::cerr << "Representative mode:      " << rep_mode << std::endl;
    std::cerr << "Intervention method:      " << itv_mthd << std::endl;
    if (itv_mthd != "none")
    {
        std::cerr << "\tthreshold = " << itv_thres << std::endl;
    }
    std::cout << "Minimal output length:    " + std::to_string(min_nb_kmer) << std::endl;
    std::cerr << "Output:                   " << (out_path.empty() ? "to screen" : out_path) << std::endl;
    std::cerr << "\t" << (out_mode.empty() ? "without" : out_mode) + " count vectors" << std::endl
              << std::endl;

    std::cerr << "k-mer count path:                             " << kmer_count_path << std::endl;
    std::cerr << "k-mer count index path:                       " << idx_dir << std::endl;
    std::cerr << "Nomalization factor to path:                  " << out_nf << std::endl;
    if (!dsgn_path.empty())
    {
        std::cerr << "Sample info path:                             " << dsgn_path << std::endl;
    }
    const ScoreMethodCode score_method_code = scorer->GetScoreMethodCode();
    std::cerr << "Evaluation method:                            " << kScoreMethodName[score_method_code] << std::endl;
    if (score_method_code == ScoreMethodCode::kNaiveBayes || score_method_code == ScoreMethodCode::kLogitReg)
    {
        size_t nb_fold = scorer->GetNbFold();
        if (nb_fold == 0)
        {
            std::cerr << "    Leave-one-out cross-validation";
        }
        else if (nb_fold == 1)
        {
            std::cerr << "    No cross-validation, train and test on the whole dataset";
        }
        else
        {
            std::cerr << "    Cross-validation with fold number = " << nb_fold;
        }
        std::cerr << std::endl;
    }
    else if (score_method_code == ScoreMethodCode::kUser)
    {
        std::cerr << "    Score column name = " << scorer->GetRepColname() << std::endl;
    }
    std::cerr << "Sorting mode:                                 " << kSortModeName[scorer->GetSortModeCode()] << std::endl;
    std::cerr << "Number of feature to output (0 for all):      " << sel_top << std::endl;
    std::cerr << "Ln(x + 1) for score estiamtion:               " << (ln_transf ? "On" : "Off") << std::endl;
    std::cerr << "Standardize for score estimation:             " << (standardize ? "On" : "Off") << std::endl;
    std::cerr << "Feature evaluation:                           on " << (no_norm ? "raw" : "normalized") << " counts" << std::endl;
    if (!out_path.empty())
    {
        std::cerr << "Output path:                                  " << out_path << std::endl;
    }
    else
    {
        std::cerr << "Output to screen" << std::endl;
    }
    std::cerr << std::endl;
}

inline void ParseOptions(int argc,
                         char *argv[],
                         std::string &idx_dir,
                         std::string &out_nf,
                         std::string &dsgn_path,
                         std::unique_ptr<Scorer> &scorer,
                         size_t &sel_top,
                         bool &to_ln,
                         bool &to_standardize,
                         bool &no_norm,
                         std::string &out_path,
                         std::string &kmer_count_path)
{
    std::string score_method;
    int i_opt(1);
    while (i_opt < argc && argv[i_opt][0] == '-')
    {
        std::string arg(argv[i_opt]);
        if (arg == "-h" || arg == "-help")
        {
            PrintRankHelper();
            exit(EXIT_SUCCESS);
        }
        else if (arg == "-idx-path" && i_opt + 1 < argc)
        {
            idx_dir = argv[++i_opt];
        }
        else if (arg == "-nf-path" && i_opt + 1 < argc)
        {
            out_nf = argv[++i_opt];
        }
        else if (arg == "-smp-info" && i_opt + 1 < argc)
        {
            dsgn_path = argv[++i_opt];
        }
        else if (arg == "-score-method" && i_opt + 1 < argc)
        {
            score_method = argv[++i_opt];
        }
        else if (arg == "-top-num" && i_opt + 1 < argc)
        {
            sel_top = atoi(argv[++i_opt]);
        }
        else if (arg == "-ln")
        {
            to_ln = true;
        }
        else if (arg == "-standardize")
        {
            to_standardize = true;
        }
        else if (arg == "-no-norm")
        {
            no_norm = true;
        }
        else if (arg == "-out-path" && i_opt + 1 < argc)
        {
            out_path = argv[++i_opt];
        }
        else
        {
            PrintRankHelper();
            throw std::invalid_argument("unknown option: " + arg);
        }
        ++i_opt;
    }
    if (i_opt == argc)
    {
        PrintRankHelper();
        throw std::invalid_argument("k-mer count table path is mandatory");
    }
    kmer_count_path = argv[i_opt++];

    if (idx_dir.empty())
    {
        PrintRankHelper();
        throw std::invalid_argument("temporary index file path is mandatory");
    }
    if (out_nf.empty() && !no_norm)
    {
        PrintRankHelper();
        throw std::invalid_argument("path for normalization factor is mandatory unless with -no-norm");
    }
    else if (!out_nf.empty() && no_norm)
    {
        PrintRankHelper();
        throw std::invalid_argument("no need for giving normalization factor path with -no-norm");
    }

    ParseScorer(scorer, score_method, to_ln, to_standardize);
}

#endif //KAMRAT_RANK_RANKRUNINFO_HPP
