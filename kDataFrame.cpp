#include "kDataFrame.hpp"
#include "KmerCounter/kmer.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <limits>
using namespace std;

kDataFrameMQFIterator::kDataFrameMQFIterator(QF* mqf)
:_kDataFrameIterator()
{
  qfi= new QFi();
  qf_iterator(mqf,qfi,0);
}
void kDataFrameMQFIterator::operator ++ (int){
  qfi_next(qfi);
  end=qfi_end(qfi)==1;
}
kmerRow kDataFrameMQFIterator::operator * (){
  kmerRow res;
  qfi_get(qfi,&res.kmerHash,&res.tag,&res.count);
  return res;
}

kDataFrame::kDataFrame(){
  kSize=31;
  autoResize=false;
  hashFunctions.push_back(new IntegerHasher(BITMASK(2*kSize)));
}
kDataFrame::kDataFrame(double falsePositiveRate,uint8_t k_size){
  kSize=k_size;
  autoResize=false;
  Hasher* hasher;
  if(falsePositiveRate==0){
    hasher=new IntegerHasher(BITMASK(2*kSize));
  }
  else if(falsePositiveRate<1){
  hasher=new MumurHasher(2038074761);
  }
  hashFunctions.push_back(hasher);
}

uint64_t kDataFrame::hashKmer(string kmer){
  return hashFunctions[0]->hash(kmer::str_to_int(kmer));
}
kDataFrame* kDataFrame::load(string filePath){
  return kDataFrameMQF::load(filePath);
}

kDataFrameMQF::kDataFrameMQF():kDataFrame(){
  mqf=new QF();
  qf_init(mqf, (1ULL<<16), 2*kSize, 0,2, true, "", 2038074761);
}
kDataFrameMQF::kDataFrameMQF(uint64_t ksize,uint8_t q,uint8_t fixedCounterSize,uint8_t tagSize,double falsePositiveRate):
kDataFrame(falsePositiveRate,ksize){
  mqf=new QF();
  qf_init(mqf, (1ULL<<q), 2*ksize,tagSize,fixedCounterSize, true, "", 2038074761);
}
kDataFrameMQF::kDataFrameMQF(QF* mqf,uint64_t ksize,double falsePositiveRate):
kDataFrame(falsePositiveRate,ksize)
{
  this->mqf=mqf;
}

kDataFrameMQF::kDataFrameMQF(uint64_t ksize,vector<uint64_t> countHistogram,uint8_t tagSize
  ,double falsePositiveRate):
  kDataFrame(falsePositiveRate,ksize)
  {
    uint64_t nSlots;
    uint64_t fixedCounterSize;
    uint64_t memory;
    kDataFrameMQF::estimateParameters(countHistogram,2*ksize,tagSize,
    &nSlots,&fixedCounterSize,&memory);
    qf_init(mqf, nSlots, 2*ksize,tagSize,fixedCounterSize, true, "", 2038074761);
  }

uint64_t kDataFrameMQF::estimateMemory(uint64_t nslots,uint64_t slotSize, uint64_t fcounter, uint64_t tagSize)
     {
       uint64_t SLOTS_PER_BLOCK=64;
       uint64_t xnslots = nslots + 10*sqrt((double)nslots);
     	uint64_t nblocks = (xnslots + SLOTS_PER_BLOCK - 1) / SLOTS_PER_BLOCK;
       uint64_t blocksize=17;

       return ((nblocks)*(blocksize+8*(slotSize+fcounter+tagSize)))/1024;

     }

void kDataFrameMQF::estimateParameters(vector<uint64_t> countHistogram,
  uint64_t numHashBits,uint64_t tagSize,
uint64_t *res_noSlots,uint64_t *res_fixedSizeCounter, uint64_t *res_memory){

  uint64_t noDistinctKmers=countHistogram[0];
  *res_memory=numeric_limits<uint64_t>::max();
  for(int i=8;i<64;i++)
  {
    uint64_t noSlots=(1ULL)<<i;
    if(noSlots<noDistinctKmers)
    continue;
    bool moreWork=false;
    uint64_t slotSize=numHashBits-log2((double)noSlots);
    for(uint64_t fixedSizeCounter=1;fixedSizeCounter<slotSize;fixedSizeCounter++)
    {
      if(isEnough(countHistogram,noSlots,fixedSizeCounter,slotSize))
      {
        uint64_t tmpMem=estimateMemory(noSlots,slotSize,fixedSizeCounter,tagSize);
        if(*res_memory>tmpMem)
        {
          *res_memory=tmpMem;
          *res_fixedSizeCounter=fixedSizeCounter;
          *res_noSlots=noSlots;
          moreWork=true;
        }
        else{
          break;
        }
      }

    }
    if(!moreWork && *res_memory!=numeric_limits<uint64_t>::max())
    break;
  }
  if(*res_memory==numeric_limits<uint64_t>::max())
  {
    throw std::overflow_error("Data limits exceeds MQF capabilities(> uint64). Check if ntcard file is corrupted");
  }

}

bool kDataFrameMQF::isEnough(vector<uint64_t> histogram,uint64_t noSlots,uint64_t fixedSizeCounter,uint64_t slotSize)
{
  // cout<<"noSlots= "<<noSlots<<endl
  //     <<"fcounter= "<<fixedSizeCounter<<endl
  //     <<"slot size= "<<numHashBits<<endl;

  noSlots=(uint64_t)((double)noSlots*0.90);
  for(uint64_t i=1;i<1000;i++)
  {
    uint64_t usedSlots=1;

    if(i>((1ULL)<<fixedSizeCounter)-1)
    {
      uint64_t nSlots2=0;
      __uint128_t capacity;
      do{
        nSlots2++;
        capacity=((__uint128_t)(1ULL)<<(nSlots2*slotSize+fixedSizeCounter))-1;
      //  cout<<"slots num "<<nSlots2<<" "<<capacity<<endl;
    }while((__uint128_t)i>capacity);
      usedSlots+=nSlots2;
    }
    //cout<<"i= "<<i<<"->"<<usedSlots<<" * "<<histogram[i]<<endl;
    if(noSlots>=(usedSlots*histogram[i]))
    {
      noSlots-=(usedSlots*histogram[i]);
    }
    else
    {
    //  cout<<"failed"<<endl<<endl;
      return false;
    }

  }
  //cout<<"success"<<endl<<endl;
  return true;
}



bool kDataFrameMQF::setCounter(string kmer,uint64_t count){
  uint64_t hash=hashKmer(kmer)%mqf->metadata->range;
  uint64_t currentCount=qf_count_key(mqf,hash);
  if(currentCount>count){
    qf_remove(mqf,hash,currentCount-count,false,false);
  }
  else{
    qf_insert(mqf,hash,count-currentCount,false,false);
  }
  return true;
}
bool kDataFrameMQF::incrementCounter(string kmer,uint64_t count){
  uint64_t hash=hashKmer(kmer)%mqf->metadata->range;
  qf_insert(mqf,hash,count,true,true);
  return true;
}
uint64_t kDataFrameMQF::getCounter(string kmer){
  uint64_t hash=hashKmer(kmer)%mqf->metadata->range;
  return qf_count_key(mqf,hash);
}

bool kDataFrameMQF::setTag(string kmer,uint64_t tag){
  uint64_t hash=hashKmer(kmer)%mqf->metadata->range;
  return qf_add_tag(mqf,hash,tag,false,false)==1;
}
uint64_t kDataFrameMQF::getTag(string kmer){
  uint64_t hash=hashKmer(kmer)%mqf->metadata->range;
  return qf_get_tag(mqf,hash);
}

bool kDataFrameMQF::removeKmer(string kmer){
  uint64_t hash=hashKmer(kmer)%mqf->metadata->range;
  uint64_t currentCount=qf_count_key(mqf,hash);
  qf_remove(mqf,hash,currentCount,true,true);
  return true;
}

uint64_t kDataFrameMQF::size(){
  return mqf->metadata->xnslots;
}
uint64_t kDataFrameMQF::filled_space(){
  return (uint64_t)qf_space(mqf);
}
bool kDataFrameMQF::isFull(){
  return mqf->metadata->noccupied_slots>=mqf->metadata->maximum_occupied_slots;
}

void kDataFrameMQF::save(string filePath){
  ofstream file(filePath+".extra");
  file<<kSize<<endl;
  // uint64_t legendSize=tagsLegend.size();
  // file<<legendSize<<endl;
  // auto it = tagsLegend.begin();
  // while(it==tagsLegend.end())
  // {
  //   file<<it->first<<" "<<it->second<<endl;
  //   it++;
  // }
  // file.close();
  qf_serialize(mqf,(filePath+".mqf").c_str());
}
kDataFrame* kDataFrameMQF::load(string filePath){
  ifstream file(filePath+".extra");
  uint64_t filekSize;
  file>>filekSize;
  QF* mqf=new QF();
  qf_deserialize(mqf,(filePath+".mqf").c_str());
  return new kDataFrameMQF(mqf,filekSize,0);
}

kDataFrameIterator kDataFrameMQF::begin(){
    return *(new kDataFrameIterator(new kDataFrameMQFIterator(mqf)));
}

kDataFrameMQF* kDataFrameMQF::index(vector<kDataFrameMQF*> kframes){
  uint64_t targetOccupiedSlots=0;
  for(auto kframe:kframes){
    targetOccupiedSlots+=kframe->mqf->metadata->noccupied_slots;
  }
  vector<uint64_t> countHistogram(2);
  countHistogram[0]=targetOccupiedSlots;
  countHistogram[1]=targetOccupiedSlots;

  kDataFrameMQF *res= new kDataFrameMQF(kframes[0]->kSize,24,2,15,0);
  QF* qf_arr[kframes.size()];
  for(int i=0;i<kframes.size();i++){
    qf_arr[i]=kframes[i]->mqf;
  }
  qf_invertable_merge(qf_arr,kframes.size(),res->mqf);
  return res;
}
