#ifndef _defaultColumn_H_
#define _defaultColumn_H_

#include <vector>
#include <stdint.h>
#include <iostream>
#include <parallel_hashmap/phmap.h>
#include "sdsl/vectors.hpp"
#include <stack>

using phmap::flat_hash_map;
using namespace std;

//to add new column create a column class and add the name in getContainer Bynamefn
class Column{
public:
    Column(){}
    virtual ~Column(){}

    static Column* getContainerByName(size_t name);

    virtual void serialize(string filename)=0;
    virtual void deserialize(string filename)=0;


};





template<typename  T>
class vectorColumn: public Column{
public:
    vector<T> dataV;
    vectorColumn(){

    }
    vectorColumn(uint32_t size){
        dataV=vector<T>(size);
    }
    ~vectorColumn(){

    }
    uint32_t  insertAndGetIndex(T item);
    T getWithIndex(uint32_t index);

    void insert(T item,uint32_t index);
    T get(uint32_t index);

    void serialize(string filename);
    void deserialize(string filename);


};

class colorNode{
public:
    flat_hash_map<uint32_t, colorNode*> edges;
    uint32_t currColor;
    colorNode() {
        currColor = 0;
    }
    ~colorNode()
    {
//        for(auto e:edges)
//            delete e.second;
    }
};
class colorIndex{
public:
    colorNode* root;
    uint32_t lastColor;
    uint32_t noSamples;
    colorIndex()
    {
        root=new colorNode();
        lastColor=0;
        noSamples=0;
    }
    ~colorIndex();

    bool hasColorID(vector<uint32_t>& v);
    uint32_t getColorID(vector<uint32_t>& v);
    void serialize(string fileName);
    void deserialize(string filename);

    void populateColors(vector<vector<uint32_t > >& colors);

    void optimize();
    void stats();

};

class stringColorIndex{
public:
    flat_hash_map<string,uint32_t> colors;
    uint32_t lastColor;
    stringColorIndex()
    {

        lastColor=0;
    }
    ~stringColorIndex(){

    }
    inline string toString(vector<uint32_t>& v)
    {
        string res="";
        for(auto c:v)
        {
            res+=to_string(c);
            res+=";";
        }
        return res;

    }
    bool hasColorID(vector<uint32_t>& v);
    uint32_t getColorID(vector<uint32_t>& v);
    void serialize(string fileName);
    void deserialize(string filename);

    void populateColors(vector<vector<uint32_t > >& colors);

};



class colorColumn: public Column{
public:
    vector<vector<uint32_t > > colors;
    colorIndex colorInv;
    uint64_t  noSamples;
    colorColumn(){
        colors.push_back(vector<uint32_t >());
    }
    colorColumn(uint64_t noSamples){
        this->noSamples=noSamples;
        colors.push_back(vector<uint32_t >());
    }

    ~colorColumn(){

    }
    uint32_t  insertAndGetIndex(vector<uint32_t > item);
    vector<uint32_t > getWithIndex(uint32_t index);

    void insert(vector<uint32_t >& item,uint32_t index);
    vector<uint32_t > get(uint32_t index);

    void serialize(string filename);
    void deserialize(string filename);

    void populateColors();

    uint32_t getNumColors(){
        return colors.size();
    }



};
class vectorBase{
public:
    uint32_t beginID;
    vectorBase(){
        beginID=0;
    }
    vectorBase(uint32_t b){
        beginID=b;
    }
    virtual uint32_t size()=0;
    void save(string filename);
    void load(string filename);

    virtual void serialize()=0;
    virtual void deseriazlize()=0;

    virtual uint64_t sizeInBytes()=0;

    virtual vector<uint32_t> get(uint32_t index)=0;
    virtual void set(uint32_t index,vector<uint32_t>& v)=0;

    friend inline bool operator< (const vectorBase& lhs, const vectorBase& rhs){
        return lhs.beginID < rhs.beginID ;
    }

};

class vectorOfVectors: public vectorBase{
public:
    vector<sdsl::int_vector<> > vecs;

    vectorOfVectors()
    {

    }
    vectorOfVectors(uint32_t beginId,uint32_t noColors)
            :vectorBase(beginId)
    {
        vecs.resize(noColors+1);
    }

    vector<uint32_t> get(uint32_t index){
        vector<uint32_t> res;
        copy(vecs[index].begin(), vecs[index].end(), back_inserter(res));
        return res;
    };
    void set(uint32_t index,vector<uint32_t>& v)
    {
        vecs[index]=sdsl::int_vector<>(v.size());
        for(unsigned int i=0;i<v.size();i++)
            vecs[index][i]=v[i];
        sdsl::util::bit_compress(vecs[index]);
    };

    uint32_t size()override {
        return vecs.size();
    }
    void serialize(){}
    void deseriazlize(){}
    uint64_t sizeInBytes(){
        uint64_t res=0;
        for(auto vec:vecs)
        {
            res+=sdsl::size_in_bytes(vec);
        }
        return res;
    }
};


class constantVector: public vectorBase{
public:
    uint32_t  noColors;
    constantVector()
    {
        noColors=0;
    }
    constantVector(uint32_t noColors)
    {
        this->noColors=noColors;
    }

    vector<uint32_t> get(uint32_t index){
        //there is no color id =0 but there is a sample equal zero
        vector<uint32_t> res={index-1};
        return res;
    }
    void set(uint32_t index,vector<uint32_t>& v)
    {
    }

    uint32_t size()override {
        return noColors;
    }
    void serialize(){}
    void deseriazlize(){}
    uint64_t sizeInBytes(){

        return 4;
    }
};

class fixedSizeVector: public vectorBase{
public:
    typedef  sdsl::int_vector<> vectype;
    vectype vec;
    uint32_t colorsize;
    fixedSizeVector()
    {

    }
    fixedSizeVector(uint32_t noColors,uint32_t size)
    {
        this->colorsize=size;
        vec.resize(noColors*size);
    }

    vector<uint32_t> get(uint32_t index){
        vector<uint32_t> res(colorsize);
        for(unsigned int i=0;i<colorsize;i++)
        {
            res[i]=vec[(index*colorsize)+i];
        }
        return res;
    }
    void set(uint32_t index,vector<uint32_t>& v)
    {
        if(v.size()!=colorsize)
            cout<<"error "<<index<<" "<<colorsize<<" "<<v.size()<<endl;
        for(unsigned int i=0;i<colorsize;i++)
        {
            vec[(index*colorsize)+i]=v[i];
        }
    }

    uint32_t size()override {
        return vec.size()/colorsize;
    }
    void serialize(){}
    void deseriazlize(){}
    uint64_t sizeInBytes(){

        return sdsl::size_in_bytes(vec)+4;
    }
};
class compressedColorColumn: public Column{
public:
    deque<vectorBase*> colors;
    uint64_t  noSamples;
    sdsl::int_vector<> idsMap;
    uint64_t numColors;
    compressedColorColumn(){
        colors.push_back(new vectorOfVectors());
    }
    compressedColorColumn(uint64_t noSamples){
        this->noSamples=noSamples;
        colors.push_back(new vectorOfVectors());
    }
    compressedColorColumn(colorColumn* col);
    ~compressedColorColumn(){

    }
    uint32_t  insertAndGetIndex(vector<uint32_t > item);
    vector<uint32_t > getWithIndex(uint32_t index);

    void insert(vector<uint32_t >& item,uint32_t index);
    vector<uint32_t > get(uint32_t index);

    void serialize(string filename);
    void deserialize(string filename);
    void optimize(colorColumn* col);
    void optimize2();
    void optimize3(colorColumn* col);

    uint32_t getNumColors(){
        return colors.size();
    }
    uint64_t sizeInBytes();



};

class StringColorColumn: public Column{
public:
    vector<vector<uint32_t > > colors;
    flat_hash_map<uint32_t,string> namesMap;

    StringColorColumn(){
        colors.push_back(vector<uint32_t > ());
    }

    ~StringColorColumn(){

    }

    vector<string > getWithIndex(uint32_t index);
    //uint32_t  insertAndGetIndex(vector<uint32_t > item);
//    void insert(vector<uint32_t > item,uint32_t index);
//    vector<uint32_t > get(uint32_t index);

    void serialize(string filename);
    void deserialize(string filename);

    void populateColors();



};


#endif