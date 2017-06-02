#include "preprocess.h"
#include "thulac_base.h"
#include "cb_tagging_decoder.h"
#include "postprocess.h"
#include "timeword.h"
#include "negword.h"
#include "punctuation.h"
#include "filter.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace thulac;
namespace {
    bool checkfile(const char* filename){
        std::fstream infile;
        infile.open(filename, std::ios::in);
        if(!infile){
            return false;
        } else {
            infile.close();
            return true;
        }
    }
}

namespace {
    char* user_specified_dict_name=NULL;
    char* model_path_char=NULL;
    Character separator = '_';
    bool useT2S = false;
    bool seg_only = false;
    bool useFilter = false;
    int max_length = 50000;
    TaggingDecoder* cws_decoder=NULL;
    permm::Model* cws_model = NULL;
    DAT* cws_dat = NULL;
    char** cws_label_info = NULL;
    int** cws_pocs_to_tags = NULL;
    int result_size = 0;
    
    TaggingDecoder* tagging_decoder = NULL;
    permm::Model* tagging_model = NULL;
    DAT* tagging_dat = NULL;
    char** tagging_label_info = NULL;
    int** tagging_pocs_to_tags = NULL;
    Preprocesser* preprocesser = NULL;
    
    Postprocesser* ns_dict = NULL;
    Postprocesser* idiom_dict = NULL;
    Postprocesser* user_dict = NULL;
    
    Punctuation* punctuation = NULL;
    
    NegWord* negword = NULL;
    TimeWord* timeword = NULL;
    
    Filter* filter = NULL;
    char *result = NULL;
}

extern "C" int init(const char * model, const char* dict = NULL, int ret_size = 1024 * 1024 * 16,int t2s = 0, int just_seg=0) {
    if (ret_size < 4) {
        return int(false);
    }
    result_size = ret_size;
    user_specified_dict_name = const_cast<char *>(dict);
    model_path_char = const_cast<char *>(model);
    separator = '_';
    useT2S = bool(t2s);
    seg_only = bool(just_seg);
    std::string prefix;
    if(model_path_char != NULL){
        prefix = model_path_char;
        if(*prefix.rbegin() != '/'){
            prefix += "/";
        }
    }else{
        prefix = "models/";
    }
    
    cws_decoder=new TaggingDecoder();
    if(seg_only){
        cws_decoder->threshold=0;
    }else{
        cws_decoder->threshold=15000;
    }
    cws_model = new permm::Model((prefix+"cws_model.bin").c_str());
    cws_dat = new DAT((prefix+"cws_dat.bin").c_str());
    cws_label_info = new char*[cws_model->l_size];
    cws_pocs_to_tags = new int*[16];
    get_label_info((prefix+"cws_label.txt").c_str(), cws_label_info, cws_pocs_to_tags);
    cws_decoder->init(cws_model, cws_dat, cws_label_info, cws_pocs_to_tags);
    cws_decoder->set_label_trans();
    if(!seg_only){
        tagging_decoder = new TaggingDecoder();
        tagging_decoder->separator = separator;
        tagging_model = new permm::Model((prefix+"model_c_model.bin").c_str());
        tagging_dat = new DAT((prefix+"model_c_dat.bin").c_str());
        tagging_label_info = new char*[tagging_model->l_size];
        tagging_pocs_to_tags = new int*[16];
        
        get_label_info((prefix+"model_c_label.txt").c_str(), tagging_label_info, tagging_pocs_to_tags);
        tagging_decoder->init(tagging_model, tagging_dat, tagging_label_info, tagging_pocs_to_tags);
        tagging_decoder->set_label_trans();
    }
    
    
    preprocesser = new Preprocesser();
    preprocesser->setT2SMap((prefix+"t2s.dat").c_str());
    
    ns_dict = new Postprocesser((prefix+"ns.dat").c_str(), "ns", false);
    idiom_dict = new Postprocesser((prefix+"idiom.dat").c_str(), "i", false);
    
    if(user_specified_dict_name){
        user_dict = new Postprocesser(user_specified_dict_name, "uw", true);
    }
    
    punctuation = new Punctuation((prefix+"singlepun.dat").c_str());
    
    negword = new NegWord((prefix+"neg.dat").c_str());
    timeword = new TimeWord();
    
    filter = NULL;
    if(useFilter){
        filter = new Filter((prefix+"xu.dat").c_str(), (prefix+"time.dat").c_str());
    }
    return int(true);
}

extern "C" void deinit() {
    delete preprocesser;
    delete ns_dict;
    delete idiom_dict;
    if(user_dict != NULL){
        delete user_dict;
    }
    
    delete negword;
    delete timeword;
    delete punctuation;
    if(useFilter){
        delete filter;
    }
    
    
    delete cws_decoder;
    if(cws_model != NULL){
        for(int i = 0; i < cws_model->l_size; i ++){
            if(cws_label_info) delete[](cws_label_info[i]);
        }
    }
    delete[] cws_label_info;
    
    if(cws_pocs_to_tags){
        for(int i = 1; i < 16; i ++){
            delete[] cws_pocs_to_tags[i];
        }
    }
    delete[] cws_pocs_to_tags;
    
    delete cws_dat;
    
    if(cws_model!=NULL) delete cws_model;
    
    delete tagging_decoder;
    if(tagging_model != NULL){
        for(int i = 0; i < tagging_model->l_size; i ++){
            if(tagging_label_info) delete[](tagging_label_info[i]);
        }
    }
    delete[] tagging_label_info;
    
    if(tagging_pocs_to_tags){
        for(int i = 1; i < 16; i ++){
            delete[] tagging_pocs_to_tags[i];
        }
    }
    delete[] tagging_pocs_to_tags;
    
    delete tagging_dat;
    if(tagging_model!=NULL) delete tagging_model;
}

extern "C" char *getResult() {
    return result;
}

extern "C" void freeResult() {
    if (result != NULL) {
        std::free(result);
        result = NULL;
    }
}

extern "C" int seg(const char *in) {
    
    POCGraph poc_cands;
    int rtn=1;
    thulac::RawSentence raw;
    thulac::RawSentence oiraw;
    thulac::RawSentence traw;
    thulac::SegmentedSentence segged;
    thulac::TaggedSentence tagged;
    
    const int BYTES_LEN=strlen(in);
    char* s=new char[BYTES_LEN+100];
    
    clock_t start = clock();
    std::string str(in);
    std::istringstream ins(str);
    std::ostringstream ous;
    std::string ori;
    std::vector<thulac::RawSentence> vec;
    while(std::getline(ins,ori)){
        
        strcpy(s,ori.c_str());
        size_t in_left=ori.length();
        thulac::get_raw(oiraw, s, in_left);
        
        if(oiraw.size() > max_length) {
            thulac::cut_raw(oiraw, vec, max_length);
        }
        else {
            vec.clear();
            vec.push_back(oiraw);
        }
        for(int vec_num = 0; vec_num < vec.size(); vec_num++) {
            if(useT2S) {
                preprocesser->clean(vec[vec_num],traw,poc_cands);
                preprocesser->T2S(traw, raw);
            }
            else {
                preprocesser -> clean(vec[vec_num],raw,poc_cands);
            }
            
            if(raw.size()){
                
                if(seg_only){
                    cws_decoder->segment(raw, poc_cands, tagged);
                    cws_decoder->get_seg_result(segged);
                    ns_dict->adjust(segged);
                    idiom_dict->adjust(segged);
                    punctuation->adjust(segged);
                    timeword->adjust(segged);
                    negword->adjust(segged);
                    if(user_dict){
                        user_dict->adjust(segged);
                    }
                    if(useFilter){
                        filter->adjust(segged);
                    }
                    
                    for(int j = 0; j < segged.size(); j++){
                        if(j!=0) ous<<" ";
                        ous<<segged[j];
                    }
                }else{
                    tagging_decoder->segment(raw,poc_cands,tagged);
                    
                    //后处理
                    ns_dict->adjust(tagged);
                    idiom_dict->adjust(tagged);
                    punctuation->adjust(tagged);
                    timeword->adjustDouble(tagged);
                    negword->adjust(tagged);
                    if(user_dict){
                        user_dict->adjust(tagged);
                    }
                    if(useFilter){
                        filter->adjust(tagged);
                    }
                    
                    ous<<tagged;
                }
            }
        }
    }
    
    clock_t end = clock();
    double duration = (double)(end - start) / CLOCKS_PER_SEC;
    delete [] s;
    std::string ostr = ous.str();
    size_t len = ostr.length();
    if (len > result_size) {
        len = result_size;
    }
    char * p = (char *)calloc(len+1, 1);
    if (p == NULL) {
        return int(false);
    }
    std::memcpy(p, ostr.c_str(), ostr.length());
    result = p;
    return 1;
}
