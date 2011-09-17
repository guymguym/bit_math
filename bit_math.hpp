#ifndef BIT_MATH__H__
#define BIT_MATH__H__

#include <stdint.h>
#include <cstdlib>
#include <cstring>

#include <deque>

#include <iostream>
#include <iomanip>
#include <ios>

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

template <class WORD = unsigned char>
class Int
{
private:
	typedef WORD Word;
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
	void prepare_for_write(bool cow) {
		if (!_data) {
			_data = new SharedVec;
		} else if (_data->is_shared()) {
			if (cow) {
				SharedVec* new_data = new SharedVec(*_data);
				_data = new_data;
			} else {
				_data = new SharedVec;
			}
		}
	}
	
	Vec& wvec(bool cow = true) { 
		prepare_for_write(cow); 
		return _data->data; 
	}
	const Vec& vec() const { 
		static const Vec V;
		return _data ? _data->data : V;
	}
	Index nwords() const { return vec().size(); }

	const Word& get_word(Index word) const { 
		static const Word Z = 0;
		return word < nwords() ? vec()[word] : Z;
	}

	void set_word(Index word, Word val) { 
		if (val != 0) {
			if (word >= nwords()) 
				wvec().resize(word+1, 0);
			wvec()[word] = val;
			return;
		}
		// val == 0
		if (word >= nwords()) 
			return;
		wvec()[word] = 0; 
		// try truncating down the vec
		if (word + 1 == nwords()) {
			while (word > 0 && vec()[word-1] == 0) --word;
			wvec().resize(word);
			if (word == 0) _sign = 0; // clear sign for zero
		}
	}

	void push_front_word(Word val) {
		wvec().push_front(val);
	}

	void clear() { 
		wvec(false).clear(); // don't copy-on-write for clear
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
		int i = 0;
		DO((sizeof(val)+WORD_SIZE-1)/WORD_SIZE, set_word(i++,cut_word(val)));
	}

	void init(const Int& num) {
		_sign = num._sign;
		_data = const_cast<SharedVec*>(num._data);
		if (_data) _data->add_share();
	}

	void release() {
		if (!_data) return;
		if (_data->is_shared()) 
			_data->remove_share(); 
		else 
			delete _data;
		_data = 0;
	}

public:
	// constructors
	Int() : _data(0) {}
	Int(uint32_t val) { init(val); }
	Int(uint64_t val) { init(val); }
	Int(int32_t  val) { init(val); }
	Int(int64_t  val) { init(val); }
	Int(const Int& num) { init(num); }
	~Int() { release(); }

	const Int& operator=(const Int& num) { if (this!=&num) { release(); init(num); } return num; }

	bool is_zero() const	{ return vec().empty(); }
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

	void lshift(Index n) { lshift(n,0); }
	void lshift(Index n, Bit insert) {
		while (n > WORD_BITS) {
			push_front_word(insert ? WORD_MASK : 0);
			n-= WORD_BITS;
		}
		Word keep = insert ? (WORD_MASK >> (WORD_BITS-n)) : 0;
		const Index len = nwords();
		for (Index i = 0; i < len || keep != 0; ++i) {
			Word x = get_word(i);
			Word next_keep = x >> (WORD_BITS-n);
			x = (x << n) | keep;
			set_word(i, x);
			keep = next_keep;
		}
	}
	
	void rshift(Index n) {}
	
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

	Int operator-() const { Int i(*this); i.toggle_sign(); return i; }
	Int operator+(const Int& num) const { Int i(*this); i.plus(num); return i; }
	Int operator-(const Int& num) const { Int i(*this); i.plus(-num); return i; }
	Int operator*(const Int& num) const { Int i(*this); i.mult(num); return i; }
	Int operator/(const Int& num) const { Int i(*this); i.div(num); return i; }
	Int operator%(const Int& num) const { Int i(*this); i.mod(num); return i; }
	Int operator>>(int n) const { Int i(*this); i.rshift(n); return i; }
	Int operator<<(int n) const { Int i(*this); i.lshift(n); return i; }
	
	
	bool parse(std::string str) {
		clear();
		int base = 10;
		size_t i = 0;
		if (i < str.size() && str[i] == '-') {
			_sign = 1;
			++i;
		}
		if (i+1 < str.size() && str[i] == '0' && str[i+1] == 'b') {
			base = 2;
			i += 2;
		} else if (i+1 < str.size() && str[i] == '0' && str[i+1] == 'x') {
			base = 16;
			i += 2;
		} else if (i < str.size() && str[i] == '0') {
			base = 8;
			i += 1;
		}

		switch (base) {
		case 2:
			for (; i < str.size(); ++i) {
				switch (str[i]) {
				case '0': lshift(1, 0); break;
				case '1': lshift(1, 1); break;
				default: return false;
				}
			}
			break;
		case 8:
			for (; i < str.size(); ++i) {
				if (str[i] < '0' || str[i] > '7')
					return false;
				int oct = str[i] - '0';
				for (int k=2; k>=0; --k)
					lshift(1, (oct >> k) & 1);
			}
			break;
		case 16:
			for (; i < str.size(); ++i) {
				int hex;
				if (str[i] >= '0' && str[i] <= '9')
					hex = str[i] - '0';
				else if (str[i] >= 'a' && str[i] <= 'f')
					hex = 10 + str[i] - 'a';
				else if (str[i] >= 'A' && str[i] <= 'F')
					hex = 10 + str[i] - 'A';
				else
					return false;
				for (int k=3; k>=0; --k)
					lshift(1, (hex >> k) & 1);
			}
			break;
		case 10: // TODO parse base 10
		default:
			return false;
		}
		return true;
	}

	friend std::ostream& operator<<(std::ostream& os, const Int& num) {
		if (num.is_zero()) 
			return os << '0';
		std::ios_base::fmtflags base = os.flags() & std::ios_base::basefield;
		//std::cout << "base = " << base << std::endl;
		const Index len = num.nwords();
		switch (base) {
		case std::ios_base::dec: // TODO decimal base
		case std::ios_base::hex:
			for (Index i = 0; i<len; ++i) {
				const Word& x = num.get_word(len-1-i);
				//std::cout << "i=" << i << " x=" << int(x) << std::endl;
				for (int k=WORD_BITS-BITS_IN_BYTE; k>=0; k-=BITS_IN_BYTE) {
					Word vh = (x >> (k+4)) & 0xF;
					Word vl = (x >> k) & 0xF;
					//std::cout << "k=" << k << " vh=" << int(vh) << " vl=" << int(vl) << std::endl;
					os << char(vh<10 ? '0'+vh : 'a'+vh-10);
					os << char(vl<10 ? '0'+vl : 'a'+vl-10);
				}
			}
			break;
		case std::ios_base::oct: {
			//int bits = 3 - (len*WORD_BITS) % 3;
			//int oct = 0;
			for (Index i = 0; i<len; ++i) {
				const Word& x = num.get_word(len-1-i);
				for (int k=WORD_BITS-BITS_IN_BYTE; k>=0; k-=BITS_IN_BYTE) {
					Word vh = (x >> (k+6)) & 0x3;
					Word vm = (x >> (k+3)) & 0x7;
					Word vl = (x >> k) & 0x7;
					os << char('0'+vh) << char('0'+vm) << char('0'+vl);
				}
			}
			break;
		}
		default:
			break;
		}
		return os;
	}

};


}; // namespace bit_math

#endif // BIT_MATH__H__

