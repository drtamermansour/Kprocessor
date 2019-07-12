using namespace std;
class Hasher{
public:
	virtual uint64_t hash(string key){return 0;};
	virtual uint64_t hash(uint64_t key){return 0;};
	virtual string Ihash(uint64_t key){
		throw logic_error("Reverese Hash function is not/ cannot be implemented for this hash function.");
	}
	virtual Hasher* clone(){return this;};
};
class MumurHasher: public Hasher{
	using Hasher::hash;
private:
	uint64_t seed;
public:
	MumurHasher(uint64_t Iseed){seed=Iseed;}
	Hasher* clone() { return new MumurHasher(seed);}
	uint64_t hash(string kmer);
};

class IntegerHasher: public Hasher{
private:
	uint64_t mask;
	uint64_t kSize;
public:
	IntegerHasher(uint64_t kSize);
	Hasher* clone() { return new IntegerHasher(kSize);}
	uint64_t hash(string key);
	uint64_t hash(uint64_t key);
	string Ihash(uint64_t key);
};

template<class hashFnType>
class wrapperHasher: public Hasher{
private:
	hashFnType fn;
	uint64_t kSize;
public:
	wrapperHasher(hashFnType fnn,uint64_t kSize)
		:fn(fnn),kSize(kSize){}
	Hasher* clone(){
		return new wrapperHasher(fn,kSize);
	}
	uint64_t hash(string key){
		return fn(key);
	}

	uint64_t hash(uint64_t key){
		string kmerStr=kmer::int_to_str(key,kSize);
		return fn(kmerStr);
	}

};