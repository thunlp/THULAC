#pragma once
#include <vector>
#include <iostream>
#include <map>
#include <list>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "thulac_raw.h"

namespace thulac{


typedef Raw Word;
typedef Raw RawSentence;
typedef std::vector<Word> SegmentedSentence;

enum POC{
    kPOC_B='0',
    kPOC_M='1',
    kPOC_E='2',
    kPOC_S='3'
};


typedef std::vector<POC> POCSequence;
typedef std::vector<int> POCGraph;

struct WordWithTag{
    Word word;
    std::string tag;
	char separator;
	WordWithTag(char separator){
		this->separator = separator;
	}
    friend std::ostream& operator<< (std::ostream& os,WordWithTag& wwt){
        os<<wwt.word<<wwt.separator<<wwt.tag;
        return os;
    }
};

//typedef std::vector<WordWithTag> TaggedSentence;
class TaggedSentence: public std::vector<WordWithTag>{
    friend std::ostream& operator<< (std::ostream& os,TaggedSentence& sentence){
        for(size_t i=0;i<sentence.size();i++){
            if(i!=0)os<<" ";
            os<<sentence[i];
        }
        return os;    
    };

};


struct LatticeEdge{
    int begin;//begin
    Word word;//string
    std::string tag;//pos tag of the word
    int margin;//margin
    
    friend std::ostream& operator<< (std::ostream& os,LatticeEdge& edge){
        os<<edge.begin<<"_"<<edge.word<<"_"<<edge.tag<<"_"<<edge.margin;
        return os;    
    };
    friend std::istream& operator>> (std::istream& is,LatticeEdge& edge){
        return is;
    };
};

class Lattice: public std::vector<LatticeEdge>{
public:
    static int split_item(std::string& item,int offset,std::string& str){
        int del_ind=item.find_first_of('_',offset);
        str=item.substr(offset,del_ind-offset);
        return del_ind+1;
    };
    friend std::ostream& operator<< (std::ostream& os,Lattice& lattice){
        for(int i=0;i<lattice.size();i++){
            if(i>0)os<<" ";
            os<<lattice[i];
        }
        return os;
    }
    friend std::istream& operator>> (std::istream& is,Lattice& lattice){
        lattice.clear();
        std::string str;
        std::string item;
        std::getline(is,str);

        std::istringstream iss(str);
        while(iss){
            item.clear();
            iss>>item;
            if(item.length()==0)continue;
            lattice.push_back(LatticeEdge());
            LatticeEdge& edge=lattice.back();
            int offset=0;
            std::string begin_str;
            offset=split_item(item,offset,begin_str);
            edge.begin=atoi(begin_str.c_str());
            std::string word_str;
            offset=split_item(item,offset,word_str);
            string_to_raw(word_str,edge.word);
            //edge.word+=word_str;
            offset=split_item(item,offset,edge.tag);
            std::string margin_str;
            offset=split_item(item,offset,margin_str);
            edge.margin=atoi(margin_str.c_str());
            

        }
        return is;
    };

};

void load_restrict(const char* filename, int* char_map, std::vector<std::vector<int> >& list){
    FILE* infile=fopen(filename,"rb");
    int buf[1];
    int rtn=0;
    while (1){
        rtn=fread(buf,sizeof(int),1,infile);
        if(!rtn)break;
        list.push_back(std::vector<int>());
        char_map[buf[0]]=list.size()-1;
        while(1){
            rtn=fread(buf,sizeof(int),1,infile);
            list.back().push_back(buf[0]);
            if(buf[0]==-1) break;
        }
    }
};

void join_list(int* l1,std::vector<int>& l2, int* result);
void join_list(int* l1,int* result);

void join_list(int* l1,std::vector<int>& l2, int* result){
    int*p=l1;
    int j=0;
    int i=0;
    do{
        while((l2[i]!=-1)&&(*p>l2[i])){
            i++;
        };
        if(l2[i]==-1)break;
        if(*p==l2[i]){
            result[j]=*p;
            j++;
        }
        p++;

    }while(*p!=-1);

    if(j==0){
        join_list(l1,result);
    }else{
        result[j]=-1;
    }
    
};

void join_list(int* l1,int* result){
    int*p=l1;
    int j=0;
    do{
        result[j]=*p;
        j++;p++;
    }while(*p!=-1);
    result[j]=-1;
};

void get_label_info(const char* filename, char** label_info, int** pocs_to_tags){
        std::list<int> poc_tags[16];
    char* str=new char[16];
    FILE *fp;
    fp = fopen(filename, "r");
    int ind=0;
    while( fscanf(fp, "%s", str)==1){
        label_info[ind]=str;
        int seg_ind=str[0]-'0';
        for(int j=0;j<16;j++){
            if((1<<seg_ind)&(j)){
                poc_tags[j].push_back(ind);
            }
        }
        str=new char[16];
        ind++;
    }
    delete[]str;
    fclose(fp);

    /*pocs_to_tags*/
    for(int j=1;j<16;j++){
        pocs_to_tags[j]=new int[(int)poc_tags[j].size()+1];
        int k=0;
        for(std::list<int>::iterator plist = poc_tags[j].begin();
                plist != poc_tags[j].end(); plist++){
            pocs_to_tags[j][k++]=*plist;
        };
        pocs_to_tags[j][k]=-1;
    }

}


template<class KEY>
class Indexer{
public:
    std::map<KEY,int> dict;
    std::vector<KEY> list;
    Indexer(){dict.clear();};
    void save_as(const char* filename);
    int get_index(const KEY& key){
        typename std::map<KEY,int>::iterator it;
        it=dict.find(key);
        if(it==dict.end()){
            int id=(int)dict.size();
            dict[key]=id;
            list.push_back(key);
            return id;
        }else{
            return it->second;
        }
    };
    KEY* get_object(int ind){
        if(ind<0||ind>=dict.size())return NULL;
        return &list[ind];
    }
};

template<class KEY>
class Counter:public std::map<KEY,int>{
public:
    void update(const KEY& key){
        typename std::map<KEY,int>::iterator it;
        it=this->find(key);
        if(it==this->end()){
            (*this)[key]=0;
        }
        (*this)[key]++;
    }
};

}//end of thulac
