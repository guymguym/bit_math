#ifndef BITS__H__
#define BITS__H__

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <deque>

#define DO1(X) do { X; } while(0)
#define DO2(X) DO1(X); DO1(X)
#define DO4(X) DO2(X); DO2(X)
#define DO8(X) DO4(X); DO4(X)
#define DO(__N__, X) \
	do { \
		int __n__ = __N__; \
		while (__n__ >= 8)  { DO8(X); __n__ -= 8; } \
		while (__n__ >= 4)  { DO4(X); __n__ -= 4; } \
		while (__n__ >= 2)  { DO2(X); __n__ -= 2; } \
		while (__n__ >= 0)  { DO1(X); __n__ -= 1; } \
	} while(0)

namespace bit_math {

template <class T>
class SharedData 
{
private:
	int _shares;
public:
	T data;
	SharedData() 
		: _shares(0) {}
	SharedData(SharedData& other) 
		: _shares(0)
		, data(other.data) 
		{ other.remove_share(); }
	bool is_shared() { return _shares > 0; }
	void add_share() { ++_shares; }
	void remove_share() { --_shares; }
};

class Bit
{
private:
	unsigned char _val : 1;
public:
	Bit() : _val(0) {}
	Bit(bool val) : _val(val ? 1 : 0) {}
	Bit(int val)  : _val(val ? 1 : 0) {}
	Bit(const Bit& bit) : _val(bit._val) {}
	operator bool() const { return _val ? true : false; }
	operator int() const { return _val ? 1 : 0; }
	const Bit&  operator=(const Bit&  bit) { _val = bit._val;    return bit; }
	const bool& operator=(const bool& val) { _val = val ? 1 : 0; return val; }
	const int&  operator=(const int&  val) { _val = val ? 1 : 0; return val; }
	bool operator==(const Bit& bit) const { return _val == bit._val; }
	bool operator!=(const Bit& bit) const { return _val != bit._val; }
	bool operator<(const Bit& bit) const { return _val < bit._val; }
	bool operator<=(const Bit& bit) const { return _val <= bit._val; }
	bool operator>(const Bit& bit) const { return _val > bit._val; }
	bool operator>=(const Bit& bit) const { return _val >= bit._val; }
	
	Bit operator!() const { return _val ? Bit(0) : Bit(1); }
	int to_sign() const { return _val ? -1 : 1; }
};

class Int
{
private:
	typedef unsigned char Word;
	typedef uint32_t Index;
	enum { 
		BITS_IN_BYTE = 8u,
		WORD_SIZE = sizeof(Word), 
		WORD_BITS = BITS_IN_BYTE * WORD_SIZE, 
	};
	enum {
		WORD_ONE = Word(1u),
		WORD_MASK = Word(-1u)
	};
	// using deque instead of vector to avoid large memory allocations
	typedef std::deque<Word> Vec;
	typedef SharedData<Vec> SharedVec;
	SharedVec* _data;
	Bit _sign;

private:
	void prepare_for_write(bool copy) {
		if (_data->is_shared()) {
			if (copy) {
				SharedVec* new_data = new SharedVec(*_data);
				_data = new_data;
			} else {
				_data = new SharedVec;
			}
		}
	}
	
	Vec& vec() { return _data->data; }
	const Vec& vec() const { return _data->data; }
	Index nwords() const { return vec().size(); }

	const Word& get_word(Index word) const { 
		static const Word Z = 0;
		return word > nwords() ? Z : vec()[word];
	}

	void set_word(Index word, Word val) { 
		if (val != 0) {
			prepare_for_write(true);
			if (word >= vec().size()) 
				vec().resize(word+1, 0);
			vec()[word] = val;
			return;
		}
		// val == 0
		if (word >= vec().size()) 
			return;
		prepare_for_write(true);
		vec()[word] = val; 
		// try truncating down the vec
		if (word + 1 == vec().size()) {
			while (vec()[word] == 0) 
				--word;
			vec().resize(word+1);
		}
	}

	void clear() { 
		prepare_for_write(false);
		vec().clear(); 
		_sign = 0; 
	}
	
	template <class T> 
	static Word cut_word(T& val) {
		Word w = val & WORD_MASK;
		val >>= WORD_BITS;
		return w;
	}

	template <class T> 
	void init(T val) {
		_sign = (val < 0);
		_data = new SharedVec;
		int i = 0;
		DO((sizeof(val)+WORD_SIZE-1)/WORD_SIZE, set_word(i++,cut_word(val)));
	}

	void init(const Int& num) {
		_sign = num._sign;
		_data = const_cast<SharedVec*>(num._data);
		_data->add_share();
	}

	void release() {
		if (_data->is_shared()) 
			_data->remove_share(); 
		else 
			delete _data;
		_data = 0;
	}

public:
	// constructors
	Int() : _data(new SharedVec) {}
	Int(uint32_t val) { init(val); }
	Int(uint64_t val) { init(val); }
	Int(int32_t  val) { init(val); }
	Int(int64_t  val) { init(val); }
	Int(const Int& num) { init(num); }
	~Int() { release(); }

	const Int& operator=(const Int& num) { release(); init(num); return num; }

	bool parse(std::string str) {
		clear();
		// TODO
		int t = atoi(str.c_str());
		set_word(0, t);
		return true;
	}
	friend std::ostream& operator<<(std::ostream& os, const Int& num) {
		return os << int(num.get_word(0)); // TODO
	}

	Bit get_sign() const	{ return _sign; }
	void set_sign(Bit sign) { _sign = sign; }
	void toggle_sign()		{ _sign = !_sign; }
	
	Index bit_count() const { 
		return nwords()*WORD_BITS; // TODO count exact bits in last word?
	}
	
	Bit get_bit(Index bit) const {
		const Index word = bit / WORD_BITS;
		const Word pos = WORD_ONE << (bit % WORD_BITS);
		const Word& w = get_word(word);
		return w & pos;
	}
	void set_bit(Index bit, Bit val) {
		const Index word = bit / WORD_BITS;
		const Word pos = WORD_ONE << (bit % WORD_BITS);
		const Word& w = get_word(word);
		set_word(word, val ? (w|pos) : (w&~pos));
	}

	void plus(const Int& num) {
		int keep = 0;
		const Index len = num.nwords();
		for (Index i = 0; i < len || keep != 0; ++i) {
			Word x = get_word(i);
			Word y = num.get_word(i);
			int next_keep = 0;
			if (_sign && x > 0) {
				x = (WORD_MASK - x) + 1;
				--next_keep; // lend from next word
			}
			if (num._sign && y > 0) {
				y = (WORD_MASK - y) + 1;
				next_keep -= _sign.to_sign(); // lend from next word
			}
			Word z;
			if (x > WORD_MASK - y) {
				z = (x - (WORD_MASK - y)) - 1;
				++next_keep; // overflow to next word
			} else {
				z = x + y;
			}
			if (keep > 0) {
				if (z > WORD_MASK - keep) {
					z = z - WORD_MASK + keep;
					++next_keep;
				} else {
					z += keep;
				}
			} else if (keep < 0) {
				if (z < -keep) {
					z = z - WORD_MASK + keep;
					++next_keep;
				} else {
					z += keep;
				}
			}
			keep = next_keep;
		}
	}
	void mult(const Int& num) {}
	void div(const Int& num) {}
	void mod(const Int& num) {}
	void rshift(Index n) {}
	void lshift(Index n) { lshift(n,0); }
	void lshift(Index n, Bit insert) {}

	Int operator-() const { Int i(*this); i.toggle_sign(); return i; }
	Int operator+(const Int& num) const { Int i(*this); i.plus(num); return i; }
	Int operator-(const Int& num) const { Int i(*this); i.plus(-num); return i; }
	Int operator*(const Int& num) const { Int i(*this); i.mult(num); return i; }
	Int operator/(const Int& num) const { Int i(*this); i.div(num); return i; }
	Int operator%(const Int& num) const { Int i(*this); i.mod(num); return i; }
	Int operator>>(int n) const { Int i(*this); i.rshift(n); return i; }
	Int operator<<(int n) const { Int i(*this); i.lshift(n); return i; }
};


}; // namespace BitMath

#endif // BITS__H__

